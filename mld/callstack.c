#include "callstack.h"
 
static volatile LONG backtrace_lock = FALSE;

HANDLE G_PROCESS = NULL;
HANDLE G_THREAD = NULL;

static DWORD THREAD_ID[THREAD_COUNT] = {0};

static void lookup_section(bfd *abfd, asection *sec, void *opaque_data)
{
	struct find_info *data = opaque_data;

	if (data->func)
		return;

	if (!(bfd_get_section_flags(abfd, sec) & SEC_ALLOC)) 
		return;

	bfd_vma vma = bfd_get_section_vma(abfd, sec);
	if (data->counter < vma || vma + bfd_get_section_size(sec) <= data->counter) 
		return;

	bfd_find_nearest_line(abfd, sec, data->symbol, data->counter - vma, &(data->file), &(data->func), &(data->line));
}

static void find(struct bfd_ctx * b, DWORD offset, const char **file, const char **func, unsigned *line)
{
	struct find_info data;
	data.func = NULL;
	data.symbol = b->symbol;
	data.counter = offset;
	data.file = NULL;
	data.func = NULL;
	data.line = 0;

	bfd_map_over_sections(b->handle, &lookup_section, &data);
	if (file) {
		*file = data.file;
	}
	if (func) {
		*func = data.func;
	}
	if (line) {
		*line = data.line;
	}
}

static int init_bfd_ctx(struct bfd_ctx *bc, const char * procname, int *err)
{
	bc->handle = NULL;
	bc->symbol = NULL;

	bfd *b = bfd_openr(procname, 0);
	if (!b) {
		if(err) { *err = BFD_ERR_OPEN_FAIL; }
		return 1;
	}

	if(!bfd_check_format(b, bfd_object)) {
		bfd_close(b);
		if(err) { *err = BFD_ERR_BAD_FORMAT; }
		return 1;
	}

	if(!(bfd_get_file_flags(b) & HAS_SYMS)) {
		bfd_close(b);
		if(err) { *err = BFD_ERR_NO_SYMBOLS; }
		return 1;
	}

	void *symbol_table;

	unsigned dummy = 0;
	if (bfd_read_minisymbols(b, FALSE, &symbol_table, &dummy) == 0) {
		if (bfd_read_minisymbols(b, TRUE, &symbol_table, &dummy) < 0) {
			free(symbol_table);
			bfd_close(b);
			if(err) { *err = BFD_ERR_READ_SYMBOL; }
			return 1;
		}
	}

	bc->handle = b;
	bc->symbol = symbol_table;

	if(err) { *err = BFD_ERR_OK; }
	return 0;
}

static void close_bfd_ctx(struct bfd_ctx *bc)
{
	if (bc) {
		if (bc->symbol) {
			free(bc->symbol);
		}
		if (bc->handle) {
			bfd_close(bc->handle);
		}
	}
}

static struct bfd_ctx *get_bc(struct bfd_set *set , const char *procname, int *err)
{
	while(set->name) {
		if (strcmp(set->name , procname) == 0) {
			return set->bc;
		}
		set = set->next;
	}
	struct bfd_ctx bc;
	if (init_bfd_ctx(&bc, procname, err)) {
		return NULL;
	}
	set->next = calloc(1, sizeof(*set));
	set->bc = malloc(sizeof(struct bfd_ctx));
	memcpy(set->bc, &bc, sizeof(bc));
	set->name = strdup(procname);

	return set->bc;
}

static void release_set(struct bfd_set *set)
{
	while(set) {
		struct bfd_set * temp = set->next;
		free(set->name);
		close_bfd_ctx(set->bc);
		free(set);
		set = temp;
	}
}

static void module_path(HINSTANCE moduleInstance, LPSTR lpFileName,DWORD size)
{
	GetModuleFileNameA(moduleInstance, lpFileName, MAX_PATH);
}

static void enter_backtrace_lock(volatile LONG *lock){
	size_t c = 0;
	while(InterlockedCompareExchange(lock, TRUE, FALSE) != FALSE)
	{
		if(c < 20){
			Sleep(0);
		}else{
			Sleep(1);
		}
		c++;
	}
}

static void leave_backtrace_lock(volatile LONG *lock){
	InterlockedExchange(lock, FALSE);
}

int insert_thread(DWORD threadId){
	int index = -1;
	for(int i = 0;i < THREAD_COUNT;i++){
		if(threadId == THREAD_ID[i]){
			index = i;
			break;
		}else if(THREAD_ID[i] == 0){
			THREAD_ID[i] = threadId;
			index = i;
			break;
		}
	}
	return index;
}

void load_symbol(HINSTANCE retInstance)
{	
	//获取模块路径 
	char lpFileName[MAX_PATH] = {'\0'}; 
	module_path(retInstance, lpFileName, MAX_PATH);

	//加载模块的调试信息
	DWORD moduleAddress = SymLoadModule(
		G_PROCESS,
		NULL, 
		lpFileName,
		NULL,
		(DWORD)GetModuleHandleA(lpFileName),
		0);
	if (moduleAddress == 0) {
//		report("SymLoadModule(%s) failed: %d\n", lpFileName, GetLastError());
	}else{
		//卸载模块的调试信息 
		struct bfd_ctx bc;
		int err;
		if (init_bfd_ctx(&bc, lpFileName, &err)) {
			SymUnloadModule(G_PROCESS, (DWORD)GetModuleHandleA(lpFileName));
		}
	}
}

PCONTEXT current_context()
{
	PCONTEXT pcontext = (PCONTEXT)malloc(sizeof(CONTEXT));
	memset(pcontext, '\0', sizeof(CONTEXT));
	pcontext->ContextFlags = CONTEXT_FULL;
	GetThreadContext(G_THREAD, pcontext);

	return pcontext;
}

void suspend_thread_except(DWORD threadId){
	for(int i = 0;i < THREAD_COUNT;i++){
		DWORD tThreadId = THREAD_ID[i];
		if(tThreadId == 0){
			break;
		}else{
			if(tThreadId != threadId){
				HANDLE tThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, tThreadId);
				if(tThread != NULL){
					SuspendThread(tThread);
				}
				CloseHandle(tThread);
			}
		}
	}
}

void resume_thread_except(DWORD threadId){
	for(int i = 0;i < THREAD_COUNT;i++){
		DWORD tThreadId = THREAD_ID[i];
		if(tThreadId == 0){
			break;
		}else{
			if(tThreadId != threadId){
				HANDLE tThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, tThreadId);
				if(tThread != NULL){
					ResumeThread(tThread);
				}
				CloseHandle(tThread);
			}
		}
	}
}

void call_stack(DWORD *offset, int offset_len)
{
	enter_backtrace_lock(&backtrace_lock);
	
	bfd_init();

	struct bfd_set *set = calloc(1,sizeof(*set));
	
	char procname[MAX_PATH];
	GetModuleFileNameA(NULL, procname, sizeof procname);

	struct bfd_ctx *bc = NULL;
	int err = BFD_ERR_OK;

	char symbol_buffer[sizeof(IMAGEHLP_SYMBOL) + 255];
	char module_name_raw[MAX_PATH];

	for(int i = 0;i < offset_len;i++){
		IMAGEHLP_SYMBOL *symbol = (IMAGEHLP_SYMBOL *)symbol_buffer;
		symbol->SizeOfStruct = (sizeof *symbol) + 255;
		symbol->MaxNameLength = 254;

		DWORD module_base = SymGetModuleBase(G_PROCESS, offset[i]);

		const char * module_name = "[unknown module]";
		if (module_base && 
			GetModuleFileNameA((HINSTANCE)module_base, module_name_raw, MAX_PATH)) {
			module_name = module_name_raw;
			bc = get_bc(set, module_name, &err);
		}

		const char * file = NULL;
		const char * func = NULL;
		unsigned line = 0;

		if (bc) {
			find(bc, offset[i], &file, &func, &line);
		}

		if (file == NULL) {
			DWORD dummy = 0;
			if (SymGetSymFromAddr(G_PROCESS, offset[i], &dummy, symbol)) {
				file = symbol->Name;
			} else {
				file = "[unknown file]";
			}
		}
		
		if (func == NULL) {
			report("0x%08x : %s : %s %s \n",
				offset[i],
				module_name,
				file,
				bfd_errors[err]);
		} else {
			report("0x%08x : %s : %s (%d) : in function (%s) \n",
				offset[i],
				module_name,
				file,
				line,
				func);
		}
	}
	
	release_set(set);
	
	report("\n\n");

	leave_backtrace_lock(&backtrace_lock);
}

void call_frame(PCONTEXT pcontext, DWORD *offset, int offset_len){
	enter_backtrace_lock(&backtrace_lock);

	DWORD currentThreadId = GetCurrentThreadId();
	insert_thread(currentThreadId);
//	suspend_thread_except(currentThreadId);
	
	int depth = offset_len;

	STACKFRAME frame;
	memset(&frame, 0, sizeof(frame));

//	PCONTEXT pcontext = current_context();
	
	frame.AddrPC.Offset = pcontext->Eip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrStack.Offset = pcontext->Esp;
	frame.AddrStack.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = pcontext->Ebp;
	frame.AddrFrame.Mode = AddrModeFlat;

	while(StackWalk(IMAGE_FILE_MACHINE_I386,
		G_PROCESS, 
		G_THREAD, 
		&frame, 
		pcontext, 
		0, 
		SymFunctionTableAccess, 
		SymGetModuleBase, 0)) {
			
		--depth;
		if (depth < 0)
			break;

		offset[offset_len - depth - 1] = frame.AddrPC.Offset;
	}
	
//	resume_thread_except(currentThreadId);

	leave_backtrace_lock(&backtrace_lock);
}

LONG WINAPI exception_filter(LPEXCEPTION_POINTERS info)
{
	enter_backtrace_lock(&backtrace_lock);

	report("------------------------------ exception ------------------------------\n[callstack]\n");

	bfd_init();
	struct bfd_set *set = calloc(1,sizeof(*set));
	PCONTEXT context = info->ContextRecord;
	int depth = 128;

	char procname[MAX_PATH];
	GetModuleFileNameA(NULL, procname, sizeof procname);

	struct bfd_ctx *bc = NULL;
	int err = BFD_ERR_OK;

	STACKFRAME frame;
	memset(&frame,0,sizeof(frame));

	frame.AddrPC.Offset = context->Eip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrStack.Offset = context->Esp;
	frame.AddrStack.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = context->Ebp;
	frame.AddrFrame.Mode = AddrModeFlat;

	char symbol_buffer[sizeof(IMAGEHLP_SYMBOL) + 255];
	char module_name_raw[MAX_PATH];

	while(StackWalk(IMAGE_FILE_MACHINE_I386, 
		G_PROCESS, 
		G_THREAD, 
		&frame, 
		context, 
		0, 
		SymFunctionTableAccess, 
		SymGetModuleBase, 0)) {
        
		--depth;
		if (depth < 0)
			break;

		IMAGEHLP_SYMBOL *symbol = (IMAGEHLP_SYMBOL *)symbol_buffer;
		symbol->SizeOfStruct = (sizeof *symbol) + 255;
		symbol->MaxNameLength = 254;

		DWORD module_base = SymGetModuleBase(G_PROCESS, frame.AddrPC.Offset);

		const char * module_name = "[unknown module]";
		if (module_base && 
			GetModuleFileNameA((HINSTANCE)module_base, module_name_raw, MAX_PATH)) {
			module_name = module_name_raw;
			bc = get_bc(set, module_name, &err);
		}

		const char * file = NULL;
		const char * func = NULL;
		unsigned line = 0;

		if (bc) {
			find(bc,frame.AddrPC.Offset,&file,&func,&line);
		}

		if (file == NULL) {
			DWORD dummy = 0;
			if (SymGetSymFromAddr(G_PROCESS, frame.AddrPC.Offset, &dummy, symbol)) {
				file = symbol->Name;
			} else {
				file = "[unknown file]";
			}
		}
		
		if (func == NULL) {
			report("0x%08x : %s : %s %s \n",
				frame.AddrPC.Offset,
				module_name,
				file,
				bfd_errors[err]);
		} else {
			report("0x%08x : %s : %s (%d) : in function (%s) \n",
				frame.AddrPC.Offset,
				module_name,
				file,
				line,
				func);
		}
	}
	
	release_set(set);

	leave_backtrace_lock(&backtrace_lock);

	return EXCEPTION_CONTINUE_SEARCH;
}

