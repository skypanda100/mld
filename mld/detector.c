#include "detector.h"

map_t context_hashmap = NULL;

static int leak_count = 0;
static ULONG leak_total = 0;

volatile LONG malloc_lock = FALSE;
volatile LONG realloc_lock = FALSE;
volatile LONG free_lock = FALSE;
volatile LONG libA_lock = FALSE;
volatile LONG libW_lock = FALSE;

/**
* malloc
*/
typedef _CRTIMP __cdecl void *(*MALLOC)(size_t);
MALLOC fpMalloc = NULL;
_CRTIMP __cdecl __MINGW_NOTHROW void *DetourMalloc(size_t size){
	enter_malloc_lock(&malloc_lock);

	void *retPtr = fpMalloc(size);

	disableHook();
	PCONTEXT pcontext = current_context();
	add_context((DWORD)retPtr, size, pcontext);
	enableHook();

	leave_malloc_lock(&malloc_lock);

	return retPtr;
}

/**
* realloc
*/
typedef _CRTIMP __cdecl void *(*REALLOC)(void *, size_t);
REALLOC fpRealloc = NULL;
_CRTIMP __cdecl __MINGW_NOTHROW void *DetourRealloc(void *ptr, size_t size){
	enter_realloc_lock(&realloc_lock);

	void *retPtr = fpRealloc(ptr, size);

	disableHook();
	PCONTEXT pcontext = current_context();
	del_context((DWORD)ptr);
	add_context((DWORD)retPtr, size, pcontext);
	enableHook();

	leave_realloc_lock(&realloc_lock);

	return retPtr;
}

/**
* free
*/
typedef _CRTIMP __cdecl void (*FREE)(void *);
FREE fpFree = NULL;
_CRTIMP __cdecl __MINGW_NOTHROW void DetourFree(void *ptr){
	enter_free_lock(&free_lock);

	disableHook();
	del_context((DWORD)ptr);
	enableHook();

	fpFree(ptr);
	
	leave_free_lock(&free_lock);
}

/**
* LoadLibraryA
*/
typedef HINSTANCE (WINAPI *LOADLIBRARYA)(LPCSTR); 
LOADLIBRARYA fpLoadLibraryA = NULL;
HINSTANCE WINAPI DetourLoadLibraryA(LPCSTR lpFileName){
	enter_libA_lock(&libA_lock);

	//loadlibrary 
	HINSTANCE retInstance = fpLoadLibraryA(lpFileName);
	disableHook();
	if(retInstance != NULL){
		load_symbol(retInstance);		
	}
	enableHook();

	leave_libA_lock(&libA_lock);

    return retInstance; 
}

/**
* LoadLibraryW
*/
typedef HINSTANCE (WINAPI *LOADLIBRARYW)(LPCWSTR); 
LOADLIBRARYW fpLoadLibraryW = NULL;
HINSTANCE WINAPI DetourLoadLibraryW(LPCWSTR lpFileName){
	enter_libW_lock(&libW_lock);

	//loadlibrary 
	HINSTANCE retInstance = ((LOADLIBRARYW)fpLoadLibraryW)(lpFileName);
	disableHook();
	if(retInstance != NULL){
		load_symbol(retInstance);
	}
	enableHook();

	leave_libW_lock(&libW_lock);

    return retInstance; 
}

/**
* 初始化内存泄漏检查 
*/
BOOL init_detector(){
	//1
	init_context();
	
	//2
	init_report();
	
	//3
	init_symbol();

	//4
	if (createHook(L"msvcrt", "malloc", &DetourMalloc, (LPVOID)&fpMalloc) != TRUE){
		return FALSE;
	}

	if (createHook(L"msvcrt", "realloc", &DetourRealloc, (LPVOID)&fpRealloc) != TRUE){
		return FALSE;
	}

	if (createHook(L"msvcrt", "free", &DetourFree, (LPVOID)&fpFree) != TRUE){
		return FALSE;
	}
	
    if (createHook(L"kernel32", "LoadLibraryA", &DetourLoadLibraryA, (LPVOID)&fpLoadLibraryA) != TRUE){
        return FALSE;
    }
    
	if (createHook(L"kernel32", "LoadLibraryW", &DetourLoadLibraryW, (LPVOID)&fpLoadLibraryW) != TRUE){
        return FALSE;
    }
    
    //5
    enableHook();
    
    return TRUE;
}

/**
* 结束内存泄漏检查 
*/
BOOL uninit_detector(){
	//1
	disableHook();
	
	//2
	detect();

	//3
	uninit_context();
	
	//4
	uninit_report();
	
	//5
	uninit_symbol();
}

/**
* 初始化符号处理器 
*/
static void init_symbol(){
	if(SymInitialize(GetCurrentProcess(), 0, false)){
		load_symbol(NULL);
	}
}

/**
* 清空符号 
*/
static void uninit_symbol(){
	SymCleanup(GetCurrentProcess());
}
/**
* 初始化Context_Element
*/
static void init_context(){
	if(context_hashmap == NULL)
	{
		context_hashmap = hashmap_new();
	}
}

/**
* 添加Context_Element
*/
static void add_context(DWORD addr, size_t size, PCONTEXT pcontext){
	//key
	char *key_str = (char *)malloc(KEYLEN);
	sprintf(key_str, "%08X", addr);

	//value
	PCE pce = (PCE)malloc(sizeof(CE));
	pce->addr = addr;
	pce->size = size;
	memset(pce->backtrace, '\0', BACKTRACELEN);
	call_stack(pcontext, pce->backtrace);

	hashmap_put(context_hashmap, key_str, pce);
}

/**
* 删除Context_Element
*/
static void del_context(DWORD addr){
	//key
	char *key_str = (char *)malloc(KEYLEN);
	sprintf(key_str, "%08X", addr);

	//value
	LPVOID pce = NULL;
	hashmap_get(context_hashmap, key_str, &pce);
	if(pce){
		free(pce);
	}
	
	hashmap_remove(context_hashmap, key_str);
}

/**
* 遍历Context_Element
*/
static int loop_context(any_t item, any_t data){
	PCE pce = (PCE)data;
	if(pce != NULL)
	{
		leak_count++;
		leak_total += pce->size;
		report("------------------------------ memory leak (block %d): address = 0x%08X size = %ld ------------------------------\n[callstack]\n%s\n\n"
		, leak_count
		, pce->addr
		, pce->size
		, pce->backtrace);
	}
	return MAP_OK;
}

/**
* 清空Context_Element的hashmap 
*/
static void	uninit_context(){
	hashmap_free(context_hashmap);
}

/**
* 检查内存泄漏状况 
*/
static void detect(){
	hashmap_iterate(context_hashmap, &loop_context, NULL);

	report("\n------------------------------ memory leak (count %d): total size = %ld ------------------------------\n"
		, leak_count
		, leak_total);

	leak_count = 0;
	leak_total = 0;
}

/**
* 上锁 
*/
static void enter_malloc_lock(volatile LONG *lock)
{
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

static void enter_realloc_lock(volatile LONG *lock)
{
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

static void enter_free_lock(volatile LONG *lock)
{
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

static void enter_libA_lock(volatile LONG *lock)
{
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

static void enter_libW_lock(volatile LONG *lock)
{
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

/**
* 解锁 
*/
static void leave_malloc_lock(volatile LONG *lock)
{
	InterlockedExchange(lock, FALSE);
}

static void leave_realloc_lock(volatile LONG *lock)
{
	InterlockedExchange(lock, FALSE);
}

static void leave_free_lock(volatile LONG *lock)
{
	InterlockedExchange(lock, FALSE);
}

static void leave_libA_lock(volatile LONG *lock)
{
	InterlockedExchange(lock, FALSE);
}

static void leave_libW_lock(volatile LONG *lock)
{
	InterlockedExchange(lock, FALSE);
}
