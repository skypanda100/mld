#include "detector.h"

extern HANDLE G_PROCESS;
extern HANDLE G_THREAD;

static map_t context_hashmap = NULL;
static LPTOP_LEVEL_EXCEPTION_FILTER g_prev = NULL;

static int leak_count = 0;
static ULONG leak_total = 0;

static volatile LONG malloc_lock = FALSE;
static volatile LONG calloc_lock = FALSE;
static volatile LONG HeapAlloc_lock = FALSE;
static volatile LONG realloc_lock = FALSE;
static volatile LONG HeapReAlloc_lock = FALSE;
static volatile LONG free_lock = FALSE;
static volatile LONG HeapFree_lock = FALSE;
static volatile LONG libA_lock = FALSE;
static volatile LONG libW_lock = FALSE;
static volatile LONG libExA_lock = FALSE;
static volatile LONG libExW_lock = FALSE;

static char *EXCEPT_HOOK_DLL[] = {
	"mld.dll",
	"kernel32.dll",
	"msvcrt.dll",
};

static char *EXCEPT_SYM_DLL[] = {
//	"libstdc++-6.dll",
};

static char *LOADED_DLL[LOADED_DLL_LEN] = {NULL};

/**
* malloc
*/
typedef _CRTIMP __cdecl void *(*MALLOC)(size_t);
MALLOC fpMalloc = NULL;
_CRTIMP __cdecl __MINGW_NOTHROW void *DetourMalloc(size_t size){
	enter_malloc_lock(&malloc_lock);

	void *retPtr = fpMalloc(size);

	if(retPtr != NULL){
		add_context((DWORD)retPtr, size);
	}

	leave_malloc_lock(&malloc_lock);

	return retPtr;
}

/**
* calloc
*/
typedef __cdecl void *(*CALLOC)(size_t _NumOfElements,size_t _SizeOfElements);
CALLOC fpCalloc = NULL;
__cdecl void *DetourCalloc(size_t _NumOfElements,size_t _SizeOfElements){
	enter_calloc_lock(&calloc_lock);

	void *retPtr = fpCalloc(_NumOfElements, _SizeOfElements);

	if(retPtr != NULL){
		add_context((DWORD)retPtr, _NumOfElements * _SizeOfElements);
	}

	leave_calloc_lock(&calloc_lock);

	return retPtr;
}

/**
* HeapAlloc
*/
typedef LPVOID WINAPI(*HEAPALLOC)(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
HEAPALLOC fpHeapAlloc = NULL;
WINBASEAPI LPVOID WINAPI DetourHeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes){
	enter_HeapAlloc_lock(&HeapAlloc_lock);

	LPVOID retPtr = fpHeapAlloc(hHeap, dwFlags, dwBytes);

	if(retPtr != NULL){
		add_context((DWORD)retPtr, dwBytes);
	}

	leave_HeapAlloc_lock(&HeapAlloc_lock);		

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

	if(retPtr != NULL){
		del_context((DWORD)ptr);
		add_context((DWORD)retPtr, size);
	}

	leave_realloc_lock(&realloc_lock);

	return retPtr;
}

/**
* HeapReAlloc
*/
typedef LPVOID WINAPI(*HEAPREALLOC)(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes);
HEAPREALLOC fpHeapReAlloc = NULL;
WINBASEAPI LPVOID WINAPI DetourHeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes){
	enter_HeapReAlloc_lock(&HeapReAlloc_lock);

	LPVOID retPtr = fpHeapReAlloc(hHeap, dwFlags, lpMem, dwBytes);

	if(retPtr != NULL){
		del_context((DWORD)lpMem);
		add_context((DWORD)retPtr, dwBytes);
	}

	leave_HeapReAlloc_lock(&HeapReAlloc_lock);

	return retPtr;
}

/**
* free
*/
typedef _CRTIMP __cdecl void (*FREE)(void *);
FREE fpFree = NULL;
_CRTIMP __cdecl __MINGW_NOTHROW void DetourFree(void *ptr){
	enter_free_lock(&free_lock);

	del_context((DWORD)ptr);

	fpFree(ptr);
	
	leave_free_lock(&free_lock);
}

/**
* HeapFree
*/
typedef WINBOOL WINAPI(*HEAPFREE)(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem);
HEAPFREE fpHeapFree = NULL;
WINBASEAPI WINBOOL WINAPI DetourHeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem){
	enter_HeapFree_lock(&HeapFree_lock);

	del_context((DWORD)lpMem);

	WINBOOL retF = fpHeapFree(hHeap, dwFlags, lpMem);
	
	leave_HeapFree_lock(&HeapFree_lock);
	
	return retF;
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
	if(retInstance != NULL){
		load_symbol(retInstance);
		create_hooks_a(lpFileName);
	}

	leave_libA_lock(&libA_lock);

    return retInstance; 
}

/**
* LoadLibraryExA
*/
typedef HMODULE WINAPI(*LOADLIBRARYEXA)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
LOADLIBRARYEXA fpLoadLibraryExA = NULL;
WINBASEAPI HMODULE WINAPI DetourLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags){
	enter_libExA_lock(&libExA_lock);

	//loadlibrary 
	HMODULE retHmodule = fpLoadLibraryExA(lpLibFileName, hFile, dwFlags);
	if(retHmodule != NULL){
		load_symbol(retHmodule);
		create_hooks_a(lpLibFileName);
	}

	leave_libExA_lock(&libExA_lock);

    return retHmodule; 
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
	if(retInstance != NULL){
		load_symbol(retInstance);
		create_hooks_w(lpFileName);
	}

	leave_libW_lock(&libW_lock);

    return retInstance; 
}

/**
* LoadLibraryExW
*/
typedef HMODULE WINAPI(*LOADLIBRARYEXW)(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
LOADLIBRARYEXW fpLoadLibraryExW = NULL;
WINBASEAPI HMODULE WINAPI DetourLoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags){
	enter_libExW_lock(&libExW_lock);

	//loadlibrary 
	HMODULE retHmodule = fpLoadLibraryExW(lpLibFileName, hFile, dwFlags);
	if(retHmodule != NULL){
		load_symbol(retHmodule);
		create_hooks_w(lpLibFileName);
	}

	leave_libExW_lock(&libExW_lock);

    return retHmodule; 
}

//挂钩要素 
static LPVOID HOOK_MODULE[][4] = {
	{"msvcrt.dll",		"malloc",			&DetourMalloc,			&fpMalloc},
	{"msvcrt.dll",		"calloc",			&DetourCalloc,			&fpCalloc},
	{"msvcrt.dll",		"realloc",			&DetourRealloc,			&fpRealloc},
	{"msvcrt.dll",		"free",				&DetourFree,			&fpFree},
	{"kernel32.dll",	"HeapAlloc",		&DetourHeapAlloc,		&fpHeapAlloc},
	{"kernel32.dll",	"HeapReAlloc",		&DetourHeapReAlloc,		&fpHeapReAlloc},
	{"kernel32.dll",	"HeapFree",			&DetourHeapFree,		&fpHeapFree},
	{"kernel32.dll",	"LoadLibraryA",		&DetourLoadLibraryA,	&fpLoadLibraryA},
	{"kernel32.dll",	"LoadLibraryExA",	&DetourLoadLibraryExA,	&fpLoadLibraryExA},
	{"kernel32.dll",	"LoadLibraryW",		&DetourLoadLibraryW,	&fpLoadLibraryW},
	{"kernel32.dll",	"LoadLibraryExW",	&DetourLoadLibraryExW,	&fpLoadLibraryExW},
};

/**
* 初始化内存泄漏检查 
*/
BOOL init_detector(){
    //1
    g_prev = SetUnhandledExceptionFilter(exception_filter);
    
	//2
	init_context();
	
	//3
	init_report();
	
	//4
	init_symbol();

	//5
	if(create_hooks_a(NULL) != TRUE){
		return FALSE;
	}

	load_dll();
	
    //6
    enable_iat_hook();
    
	return TRUE;
}

/**
* 结束内存泄漏检查 
*/
BOOL uninit_detector(){	
	//1
	disable_iat_hook();
	
	//2
	detect();

	//3
	uninit_context();
	
	//4
	uninit_report();
	
	//5
	uninit_symbol();
	
	//6
	SetUnhandledExceptionFilter(g_prev);
}

/**
* 初始化符号处理器 
*/
static void init_symbol(){
	G_PROCESS = GetCurrentProcess();
	G_THREAD = GetCurrentThread();
	insert_thread(GetCurrentThreadId());
//    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
	if(SymInitialize(G_PROCESS, 0, false)){
		load_symbol(NULL);
	}
}

/**
* 清空符号 
*/
static void uninit_symbol(){
	SymCleanup(G_PROCESS);
}
/**
* 初始化Context_Element
*/
static void init_context(){
	if(context_hashmap == NULL){
		context_hashmap = hashmap_new();
	}
}

/**
* 添加Context_Element
*/
static void add_context(DWORD addr, size_t size){
	//key
	char *key_str = (char *)malloc(KEYLEN);
	sprintf(key_str, "%08X", addr);

	//value
	PCE pce = (PCE)calloc(1, sizeof(CE));
	pce->addr = addr;
	pce->size = size;
	pce->threadId = GetCurrentThreadId();
	call_frame(pce->offset, sizeof(pce->offset)/sizeof(pce->offset[0]));
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
	if(pce != NULL){
		leak_count++;
		leak_total += pce->size;
		report("------------------------------ memory leak (block %d): address = 0x%08X size = %ld ------------------------------\n[callstack]\n"
		, leak_count
		, pce->addr
		, pce->size);
//		if(OpenThread(SYNCHRONIZE | THREAD_QUERY_INFORMATION, FALSE, pce->threadId)){
			call_stack(pce->offset, sizeof(pce->offset)/sizeof(pce->offset[0]));
//		}
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
* 创建所有挂钩 
*/
static BOOL create_hooks_a(LPCSTR lpFileName){
	int hook_module_len = sizeof(HOOK_MODULE)/sizeof(HOOK_MODULE[0]);
	for(int i = 0;i < hook_module_len;i++){
		if(create_iat_hook_a(lpFileName, (LPCSTR)(HOOK_MODULE[i][0]), (LPCSTR)(HOOK_MODULE[i][1]), (FARPROC)(HOOK_MODULE[i][2]), (LPVOID)(HOOK_MODULE[i][3])) != TRUE){
			return FALSE;
		}
	}
    return TRUE;
}

static BOOL create_hooks_w(LPCWSTR lpFileName){
	int hook_module_len = sizeof(HOOK_MODULE)/sizeof(HOOK_MODULE[0]);
	for(int i = 0;i < hook_module_len;i++){
		if(create_iat_hook_w(lpFileName, (LPCSTR)(HOOK_MODULE[i][0]), (LPCSTR)(HOOK_MODULE[i][1]), (FARPROC)(HOOK_MODULE[i][2]), (LPVOID)(HOOK_MODULE[i][3])) != TRUE){
			return FALSE;
		}
	}
    return TRUE;
}

/**
* 加载exe中的动态库 
*/
static void load_dll(){
	int except_hook_dll_len = sizeof(EXCEPT_HOOK_DLL) / sizeof(char *);
	int except_sym_dll_len = sizeof(EXCEPT_SYM_DLL) / sizeof(char *);
	
	HMODULE lpBase = GetModuleHandleA(NULL);
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)lpBase;
	PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS32)((PBYTE)lpBase + pDosHeader->e_lfanew);; 
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)((PBYTE)lpBase + pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
	PROC* procAddr = NULL;
	while(pImportDesc->FirstThunk){
		PSTR pszModName = (PSTR)((PBYTE)lpBase + pImportDesc->Name);
		int i = 0;
		for(;i < except_hook_dll_len;i++){
			if(stricmp(pszModName, EXCEPT_HOOK_DLL[i]) == 0){
				break;
			}
		}
		if(i == except_hook_dll_len){
			for(int i = 0;i < LOADED_DLL_LEN;i++){
				if(LOADED_DLL[i] == NULL){
					LOADED_DLL[i] = pszModName;
					break;
				}else{
					if(stricmp(LOADED_DLL[i], pszModName) == 0){
						break;
					}
				}
			}
		}
		pImportDesc++;
	}
	for(int i = 0;i < LOADED_DLL_LEN;i++){
		if(LOADED_DLL[i] == NULL){
			break;
		}else{
			create_hooks_a(LOADED_DLL[i]);
			int j = 0;
			for(;j < except_sym_dll_len;j++){
				if(stricmp(EXCEPT_SYM_DLL[j], LOADED_DLL[i]) == 0){
					break;
				}	
			}
			if(j == except_sym_dll_len){
				HINSTANCE instance = LoadLibraryA(LOADED_DLL[i]);
				if(instance != NULL){
					load_symbol(instance);
					FreeLibrary(instance);
				}
			}
		}
	}
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
static void enter_malloc_lock(volatile LONG *lock){
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

static void enter_calloc_lock(volatile LONG *lock){
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

static void enter_HeapAlloc_lock(volatile LONG *lock){
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

static void enter_realloc_lock(volatile LONG *lock){
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

static void enter_HeapReAlloc_lock(volatile LONG *lock){
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

static void enter_free_lock(volatile LONG *lock){
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


static void enter_HeapFree_lock(volatile LONG *lock){
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

static void enter_libA_lock(volatile LONG *lock){
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

static void enter_libExA_lock(volatile LONG *lock){
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

static void enter_libW_lock(volatile LONG *lock){
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

static void enter_libExW_lock(volatile LONG *lock){
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
static void leave_malloc_lock(volatile LONG *lock){
	InterlockedExchange(lock, FALSE);
}

static void leave_calloc_lock(volatile LONG *lock){
	InterlockedExchange(lock, FALSE);
}

static void leave_HeapAlloc_lock(volatile LONG *lock){
	InterlockedExchange(lock, FALSE);
}

static void leave_realloc_lock(volatile LONG *lock){
	InterlockedExchange(lock, FALSE);
}

static void leave_HeapReAlloc_lock(volatile LONG *lock){
	InterlockedExchange(lock, FALSE);
}

static void leave_free_lock(volatile LONG *lock){
	InterlockedExchange(lock, FALSE);
}

static void leave_HeapFree_lock(volatile LONG *lock){
	InterlockedExchange(lock, FALSE);
}

static void leave_libA_lock(volatile LONG *lock){
	InterlockedExchange(lock, FALSE);
}

static void leave_libExA_lock(volatile LONG *lock){
	InterlockedExchange(lock, FALSE);
}

static void leave_libW_lock(volatile LONG *lock){
	InterlockedExchange(lock, FALSE);
}

static void leave_libExW_lock(volatile LONG *lock){
	InterlockedExchange(lock, FALSE);
}
