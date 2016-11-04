#include "hook.h"

map_t context_hashmap = NULL;
LPVOID lpMalloc = NULL;
LPVOID lpRealloc = NULL;
LPVOID lpFree = NULL;
LPVOID lpLoadLibraryA = NULL;
LPVOID lpLoadLibraryW = NULL;
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
_CRTIMP __cdecl __MINGW_NOTHROW void *DetourMalloc(size_t size)
{
	enter_malloc_lock(&malloc_lock);

	void *retPtr = fpMalloc(size);

	disable_hook(MH_ALL_HOOKS);
	PCONTEXT pcontext = current_context();
	add_context((DWORD)retPtr, size, pcontext);
	enable_hook(MH_ALL_HOOKS);

	leave_malloc_lock(&malloc_lock);
	
	return retPtr;
}

/**
* realloc
*/
typedef _CRTIMP __cdecl void *(*REALLOC)(void *, size_t);
REALLOC fpRealloc = NULL;
_CRTIMP __cdecl __MINGW_NOTHROW void *DetourRealloc(void *ptr, size_t size)
{
	enter_realloc_lock(&realloc_lock);

	void *retPtr = fpRealloc(ptr, size);

	disable_hook(MH_ALL_HOOKS);
	PCONTEXT pcontext = current_context();
	del_context((DWORD)ptr);
	add_context((DWORD)retPtr, size, pcontext);
	enable_hook(MH_ALL_HOOKS);

	leave_realloc_lock(&realloc_lock);

	return retPtr;
}

/**
* free
*/
typedef _CRTIMP __cdecl void (*FREE)(void *);
FREE fpFree = NULL;
_CRTIMP __cdecl __MINGW_NOTHROW void DetourFree(void *ptr)
{
	enter_free_lock(&free_lock);

	fpFree(ptr);
	
	disable_hook(MH_ALL_HOOKS);
	del_context((DWORD)ptr);
	enable_hook(MH_ALL_HOOKS);

	leave_free_lock(&free_lock);
}

/**
* LoadLibraryA
*/
typedef HINSTANCE (WINAPI *LOADLIBRARYA)(LPCSTR); 
LOADLIBRARYA fpLoadLibraryA = NULL;
HINSTANCE WINAPI DetourLoadLibraryA(LPCSTR lpFileName)
{
	enter_libA_lock(&libA_lock);

	//loadlibrary 
	HINSTANCE retInstance = fpLoadLibraryA(lpFileName);

	//loadsymbol
	if (retInstance != NULL)
	{
		disable_hook(MH_ALL_HOOKS);
		load_symbol(retInstance);
		enable_hook(MH_ALL_HOOKS);
	}

	leave_libA_lock(&libA_lock);

    return retInstance; 
}

/**
* LoadLibraryW
*/
typedef HINSTANCE (WINAPI *LOADLIBRARYW)(LPCWSTR); 
LOADLIBRARYW fpLoadLibraryW = NULL;
HINSTANCE WINAPI DetourLoadLibraryW(LPCWSTR lpFileName)
{
	enter_libW_lock(&libW_lock);

	//loadlibrary 
	HINSTANCE retInstance = fpLoadLibraryW(lpFileName);
	
	//loadsymbol
	if (retInstance != NULL)
	{
		disable_hook(MH_ALL_HOOKS);
		load_symbol(retInstance);
		enable_hook(MH_ALL_HOOKS);
	}

	leave_libW_lock(&libW_lock);

    return retInstance; 
}

/**
* 获取各个hook目标 
*/
void init_hook_targets()
{
	HMODULE hModule;
    LPVOID  pTarget;

    hModule = GetModuleHandleW(L"msvcrt");
    if(hModule != NULL){
    	//malloc
    	pTarget = (LPVOID)GetProcAddress(hModule, "malloc");
    	if(pTarget != NULL){
	 	   	lpMalloc = pTarget;    		
		}
		//realloc
    	pTarget = (LPVOID)GetProcAddress(hModule, "realloc");
    	if(pTarget != NULL){
	 	   	lpRealloc = pTarget;    		
		}
		//free
    	pTarget = (LPVOID)GetProcAddress(hModule, "free");
    	if(pTarget != NULL){
	 	   	lpFree = pTarget;    		
		}
	}
	
	hModule = GetModuleHandleW(L"kernel32");
	if(hModule != NULL){
    	//LoadLibraryA
    	pTarget = (LPVOID)GetProcAddress(hModule, "LoadLibraryA");
    	if(pTarget != NULL){
	 	   	lpLoadLibraryA = pTarget;    		
		}
		//LoadLibraryW
    	pTarget = (LPVOID)GetProcAddress(hModule, "LoadLibraryW");
    	if(pTarget != NULL){
	 	   	lpLoadLibraryW = pTarget;    		
		}
	}
}
/**
* 初始化hook 
*/
BOOL init_hook()
{
	init_context();

	if (MH_Initialize() != MH_OK)
	{
		return FALSE;
	}
	
	return TRUE;
}

/**
* 解除hook 
*/
BOOL uninit_hook()
{	
	if (MH_Uninitialize() != MH_OK)
    {
        return FALSE;
    }
    
    return TRUE;
}

/**
* 创建hook 
*/
BOOL create_hook()
{
	if (MH_CreateHookApiEx(L"msvcrt", "malloc", &DetourMalloc, (LPVOID)&fpMalloc, (LPVOID)&lpMalloc) != MH_OK)
	{
		return FALSE;
	}

	if (MH_CreateHookApiEx(L"msvcrt", "realloc", &DetourRealloc, (LPVOID)&fpRealloc, (LPVOID)&lpRealloc) != MH_OK)
	{
		return FALSE;
	}

	if (MH_CreateHookApiEx(L"msvcrt", "free", &DetourFree, (LPVOID)&fpFree, (LPVOID)&lpFree) != MH_OK)
	{
		return FALSE;
	}
	
    if (MH_CreateHookApiEx(L"kernel32", "LoadLibraryA", &DetourLoadLibraryA, (LPVOID)&fpLoadLibraryA, (LPVOID)&lpLoadLibraryA) != MH_OK)
    {
        return FALSE;
    }
    
	if (MH_CreateHookApiEx(L"kernel32", "LoadLibraryW", &DetourLoadLibraryW, (LPVOID)&fpLoadLibraryW, (LPVOID)&lpLoadLibraryW) != MH_OK)
    {
        return FALSE;
    }
    
    return TRUE;
}

/**
* 活性hook 
*/
BOOL enable_hook(LPVOID pTarget)
{
    if (MH_EnableHook(pTarget) != MH_OK)
    {
        return FALSE;
    }

    return TRUE;
}

/**
* 非活性hook 
*/ 
BOOL disable_hook(LPVOID pTarget)
{
    if (MH_DisableHook(pTarget) != MH_OK)
    {
        return FALSE;
    }

    return TRUE;
}

/**
* 初始化Context_Element
*/
void init_context()
{
	if(context_hashmap == NULL)
	{
		context_hashmap = hashmap_new();
	}
}

/**
* 释放hook 
*/
static int leak_count = 0;
static ULONG leak_total = 0;

void release_hook()
{	
	hashmap_iterate(context_hashmap, &loop_context, NULL);
	uninit_context();
	output_print("\n------------------------------ memory leak (count %d): total size = %ld ------------------------------\n"
		, leak_count
		, leak_total);
	leak_count = 0;
	leak_total = 0;
}

/**
* 添加Context_Element
*/
void add_context(DWORD addr, size_t size, PCONTEXT pcontext)
{
	//key
	char *key_str = (char *)malloc(KEYLEN);
	sprintf(key_str, "%08X", addr);

	//value
	struct Context_Element *context_element = (struct Context_Element *)malloc(sizeof(struct Context_Element));
	context_element->addr = addr;
	context_element->size = size;
	memset(context_element->call_str, '\0', BACKTRACELEN);
	call_stack(pcontext, context_element->call_str);
	context_element->call_str[BACKTRACELEN - 1] = '\0';

//	output_print("addr = 0x%08X\n", addr);
//	call_stack(pcontext, NULL);

	hashmap_put(context_hashmap, key_str, context_element);
}

/**
* 删除Context_Element
*/
void del_context(DWORD addr)
{
	//key
	char *key_str = (char *)malloc(KEYLEN);
	sprintf(key_str, "%08X", addr);

	hashmap_remove(context_hashmap, key_str);
}

/**
* 遍历Context_Element
*/
int loop_context(any_t item, any_t data)
{
	struct Context_Element *context_element = (struct Context_Element *)data;
	if(context_element != NULL)
	{
		leak_count++;
		leak_total += context_element->size;
		output_print("------------------------------ memory leak (block %d): address = 0x%08X size = %ld ------------------------------\n[callstack]\n%s\n"
		, leak_count
		, context_element->addr
		, context_element->size
		, context_element->call_str);
	}
	return MAP_OK;
}

/**
* 销毁Context_Element
*/
void uninit_context()
{
	hashmap_free(context_hashmap);
}

/**
* 上锁 
*/
void enter_malloc_lock(volatile LONG *lock)
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

void enter_realloc_lock(volatile LONG *lock)
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

void enter_free_lock(volatile LONG *lock)
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

void enter_libA_lock(volatile LONG *lock)
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

void enter_libW_lock(volatile LONG *lock)
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
void leave_malloc_lock(volatile LONG *lock)
{
	InterlockedExchange(lock, FALSE);
}

void leave_realloc_lock(volatile LONG *lock)
{
	InterlockedExchange(lock, FALSE);
}

void leave_free_lock(volatile LONG *lock)
{
	InterlockedExchange(lock, FALSE);
}

void leave_libA_lock(volatile LONG *lock)
{
	InterlockedExchange(lock, FALSE);
}

void leave_libW_lock(volatile LONG *lock)
{
	InterlockedExchange(lock, FALSE);
}
