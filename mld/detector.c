#include "detector.h"

static map_t context_hashmap = NULL;
static LPTOP_LEVEL_EXCEPTION_FILTER g_prev = NULL;

static int leak_count = 0;
static ULONG leak_total = 0;

static volatile LONG malloc_lock = FALSE;
static volatile LONG HeapAlloc_lock = FALSE;
static volatile LONG realloc_lock = FALSE;
static volatile LONG HeapReAlloc_lock = FALSE;
static volatile LONG free_lock = FALSE;
static volatile LONG HeapFree_lock = FALSE;
static volatile LONG libA_lock = FALSE;
static volatile LONG libW_lock = FALSE;

/**
* malloc
*/
typedef _CRTIMP __cdecl void *(*MALLOC)(size_t);
MALLOC fpMalloc = NULL;
_CRTIMP __cdecl __MINGW_NOTHROW void *DetourMalloc(size_t size){
	enter_malloc_lock(&malloc_lock);

	void *retPtr = fpMalloc(size);

	disable_iat_hook();
	PCONTEXT pcontext = current_context();
	add_context((DWORD)retPtr, size, pcontext);
	enable_iat_hook();

	leave_malloc_lock(&malloc_lock);

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

	disable_iat_hook();
	PCONTEXT pcontext = current_context();
	add_context((DWORD)retPtr, dwBytes, pcontext);
	enable_iat_hook();
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

	disable_iat_hook();
	PCONTEXT pcontext = current_context();
	del_context((DWORD)ptr);
	add_context((DWORD)retPtr, size, pcontext);
	enable_iat_hook();

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

	disable_iat_hook();
	PCONTEXT pcontext = current_context();
	del_context((DWORD)lpMem);
	add_context((DWORD)retPtr, dwBytes, pcontext);
	enable_iat_hook();

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

	disable_iat_hook();
	del_context((DWORD)ptr);
	enable_iat_hook();

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

	disable_iat_hook();
	del_context((DWORD)lpMem);
	enable_iat_hook();

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
	disable_iat_hook();
	if(retInstance != NULL){
		load_symbol(retInstance);
		create_hooks_a(lpFileName);
	}
	enable_iat_hook();

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
	disable_iat_hook();
	if(retInstance != NULL){
		load_symbol(retInstance);
		create_hooks_w(lpFileName);
	}
	enable_iat_hook();

	leave_libW_lock(&libW_lock);

    return retInstance; 
}

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
* 创建所有挂钩 
*/
static BOOL create_hooks_a(LPCSTR lpFileName){
	if (create_iat_hook_a(lpFileName, "msvcrt.dll", "malloc", (FARPROC)&DetourMalloc, (LPVOID)&fpMalloc) != TRUE){
		return FALSE;
	}

	if (create_iat_hook_a(lpFileName, "msvcrt.dll", "realloc", (FARPROC)&DetourRealloc, (LPVOID)&fpRealloc) != TRUE){
		return FALSE;
	}

	if (create_iat_hook_a(lpFileName, "msvcrt.dll", "free", (FARPROC)&DetourFree, (LPVOID)&fpFree) != TRUE){
		return FALSE;
	}

    if (create_iat_hook_a(lpFileName, "kernel32.dll", "HeapAlloc", (FARPROC)&DetourHeapAlloc, (LPVOID)&fpHeapAlloc) != TRUE){
        return FALSE;
    }

    if (create_iat_hook_a(lpFileName, "kernel32.dll", "HeapReAlloc", (FARPROC)&DetourHeapReAlloc, (LPVOID)&fpHeapReAlloc) != TRUE){
        return FALSE;
    }

    if (create_iat_hook_a(lpFileName, "kernel32.dll", "HeapFree", (FARPROC)&DetourHeapFree, (LPVOID)&fpHeapFree) != TRUE){
        return FALSE;
    }
	
    if (create_iat_hook_a(lpFileName, "kernel32.dll", "LoadLibraryA", (FARPROC)&DetourLoadLibraryA, (LPVOID)&fpLoadLibraryA) != TRUE){
        return FALSE;
    }
    
	if (create_iat_hook_a(lpFileName, "kernel32.dll", "LoadLibraryW", (FARPROC)&DetourLoadLibraryW, (LPVOID)&fpLoadLibraryW) != TRUE){
        return FALSE;
    }
    
    return TRUE;	
}

static BOOL create_hooks_w(LPCWSTR lpFileName){
	if (create_iat_hook_w(lpFileName, "msvcrt.dll", "malloc", (FARPROC)&DetourMalloc, (LPVOID)&fpMalloc) != TRUE){
		return FALSE;
	}

	if (create_iat_hook_w(lpFileName, "msvcrt.dll", "realloc", (FARPROC)&DetourRealloc, (LPVOID)&fpRealloc) != TRUE){
		return FALSE;
	}

	if (create_iat_hook_w(lpFileName, "msvcrt.dll", "free", (FARPROC)&DetourFree, (LPVOID)&fpFree) != TRUE){
		return FALSE;
	}
	
	if (create_iat_hook_w(lpFileName, "kernel32.dll", "HeapAlloc", (FARPROC)&DetourHeapAlloc, (LPVOID)&fpHeapAlloc) != TRUE){
        return FALSE;
    }

    if (create_iat_hook_w(lpFileName, "kernel32.dll", "HeapReAlloc", (FARPROC)&DetourHeapReAlloc, (LPVOID)&fpHeapReAlloc) != TRUE){
        return FALSE;
    }

    if (create_iat_hook_w(lpFileName, "kernel32.dll", "HeapFree", (FARPROC)&DetourHeapFree, (LPVOID)&fpHeapFree) != TRUE){
        return FALSE;
    }

    if (create_iat_hook_w(lpFileName, "kernel32.dll", "LoadLibraryA", (FARPROC)&DetourLoadLibraryA, (LPVOID)&fpLoadLibraryA) != TRUE){
        return FALSE;
    }
    
	if (create_iat_hook_w(lpFileName, "kernel32.dll", "LoadLibraryW", (FARPROC)&DetourLoadLibraryW, (LPVOID)&fpLoadLibraryW) != TRUE){
        return FALSE;
    }
    
    return TRUE;	
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

static void enter_HeapAlloc_lock(volatile LONG *lock)
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

static void enter_HeapReAlloc_lock(volatile LONG *lock)
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


static void enter_HeapFree_lock(volatile LONG *lock)
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

static void leave_HeapAlloc_lock(volatile LONG *lock)
{
	InterlockedExchange(lock, FALSE);
}

static void leave_realloc_lock(volatile LONG *lock)
{
	InterlockedExchange(lock, FALSE);
}

static void leave_HeapReAlloc_lock(volatile LONG *lock)
{
	InterlockedExchange(lock, FALSE);
}

static void leave_free_lock(volatile LONG *lock)
{
	InterlockedExchange(lock, FALSE);
}

static void leave_HeapFree_lock(volatile LONG *lock)
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
