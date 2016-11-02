#include "hook.h"

map_t context_hashmap = NULL;

/**
* HeapAlloc
*/
typedef PVOID (WINAPI *HEAPALLOC)(HANDLE,DWORD,DWORD);
HEAPALLOC fpHeapAlloc = NULL;
PVOID WINAPI DetourHeapAlloc(HANDLE hHeap, DWORD dwFlags, DWORD dwBytes)
{	
	PVOID retPtr = fpHeapAlloc(hHeap, dwFlags, dwBytes);

	disable_hook(MH_ALL_HOOKS);
	PCONTEXT pcontext = current_context();
	add_context((DWORD)retPtr, dwBytes, pcontext);
	enable_hook(MH_ALL_HOOKS);
	
	return retPtr;
}

/**
* HeapReAlloc
*/
typedef PVOID (WINAPI *HEAPREALLOC)(HANDLE,DWORD,PVOID,DWORD);
HEAPREALLOC fpHeapReAlloc = NULL;
PVOID WINAPI DetourHeapReAlloc(HANDLE hHeap, DWORD dwFlags, PVOID lpMem, DWORD dwBytes)
{	
	PVOID retPtr = fpHeapReAlloc(hHeap, dwFlags, lpMem, dwBytes);
	
	disable_hook(MH_ALL_HOOKS);
	PCONTEXT pcontext = current_context();
	add_context((DWORD)retPtr, dwBytes, pcontext);
	enable_hook(MH_ALL_HOOKS);
	
	return retPtr;
}

/**
* HeapFree
*/
typedef BOOL (WINAPI *HEAPFREE)(HANDLE,DWORD,PVOID);
HEAPFREE fpHeapFree = NULL;
BOOL WINAPI DetourHeapFree(HANDLE hHeap, DWORD dwFlags, PVOID lpMem)
{
	BOOL retFlg = fpHeapFree(hHeap, dwFlags, lpMem);

	disable_hook(MH_ALL_HOOKS);
	printf("free:0x%08X\n", (DWORD)lpMem);
	del_context((DWORD)lpMem);	
	enable_hook(MH_ALL_HOOKS);
	
	return retFlg;
}

/**
* LoadLibraryA
*/
typedef HINSTANCE (WINAPI *LOADLIBRARYA)(LPCSTR); 
LOADLIBRARYA fpLoadLibraryA = NULL;
HINSTANCE WINAPI DetourLoadLibraryA(LPCSTR lpFileName)
{
	//loadlibrary 
	HINSTANCE retInstance = fpLoadLibraryA(lpFileName);

	disable_hook(MH_ALL_HOOKS);
	//loadsymbol
	if (retInstance != NULL)
	{
		load_symbol(retInstance);		
	}
	enable_hook(MH_ALL_HOOKS);

    return retInstance; 
}

/**
* LoadLibraryW
*/
typedef HINSTANCE (WINAPI *LOADLIBRARYW)(LPCWSTR); 
LOADLIBRARYW fpLoadLibraryW = NULL;
HINSTANCE WINAPI DetourLoadLibraryW(LPCWSTR lpFileName)
{
	//loadlibrary 
	HINSTANCE retInstance = fpLoadLibraryW(lpFileName);

	disable_hook(MH_ALL_HOOKS);
	//loadsymbol
	if (retInstance != NULL)
	{
		load_symbol(retInstance);		
	}
	enable_hook(MH_ALL_HOOKS);

    return retInstance; 
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
	if (MH_CreateHookApi(L"kernel32", "HeapAlloc", &DetourHeapAlloc, (LPVOID)&fpHeapAlloc) != MH_OK)
	{
		return FALSE;
	}

	if (MH_CreateHookApi(L"kernel32", "HeapReAlloc", &DetourHeapReAlloc, (LPVOID)&fpHeapReAlloc) != MH_OK)
	{
		return FALSE;
	}
	
	if (MH_CreateHookApi(L"kernel32", "HeapFree", &DetourHeapFree, (LPVOID)&fpHeapFree) != MH_OK)
	{
		return FALSE;
	}
	
	if (MH_CreateHookApi(L"kernel32", "LoadLibraryA", &DetourLoadLibraryA, (LPVOID)&fpLoadLibraryA) != MH_OK)
    {
        return FALSE;
    }
    
	if (MH_CreateHookApi(L"kernel32", "LoadLibraryW", &DetourLoadLibraryW, (LPVOID)&fpLoadLibraryW) != MH_OK)
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
* 初始化_Context 
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
void release_hook()
{	
	hashmap_iterate(context_hashmap, &loop_context, NULL);
	uninit_context();
}

/**
* 添加_Context 
*/
void add_context(DWORD addr, DWORD size, PCONTEXT pcontext)
{
	//key
	char *key_str = (char *)malloc(KEYLEN);
	sprintf(key_str, "%08X", addr);

	//value
	struct _Context *_context = (struct _Context *)malloc(sizeof(struct _Context));
	_context->addr = addr;
	_context->size = size;
	_context->pcontext = pcontext;
	
	hashmap_put(context_hashmap, key_str, _context);
	
//	printf("alloc:%s %ld\n", key_str, size);
}

/**
* 删除_Context 
*/
void del_context(DWORD addr)
{
	//key
	char *key_str = (char *)malloc(KEYLEN);
	sprintf(key_str, "%08X", addr);
	
	hashmap_remove(context_hashmap, key_str);	
}

/**
* 遍历_Context
*/
int loop_context(any_t item, any_t data)
{
	struct _Context *_context = (struct _Context *)data;
	if(_context != NULL)
	{
		output_print("------------------------------ memory leak : address = 0x%08X size = %ld ------------------------------\n"
		, _context->addr
		, _context->size);
		output_print("[callstack]\n");
		call_stack(_context->pcontext);
	}
	return MAP_OK;
}

/**
* 销毁_Context 
*/
void uninit_context()
{
	hashmap_free(context_hashmap);
}

