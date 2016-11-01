#include "hook.h"

void *context_hashmap = NULL;

/**
* HeapAlloc
*/
typedef PVOID (WINAPI *HEAPALLOC)(HANDLE,DWORD,DWORD);
HEAPALLOC fpHeapAlloc = NULL;
PVOID WINAPI DetourHeapAlloc(HANDLE hHeap, DWORD dwFlags, DWORD dwBytes)
{
	disable_hook(MH_ALL_HOOKS);

	PVOID retPtr = fpHeapAlloc(hHeap, dwFlags, dwBytes);
	PCONTEXT pcontext = current_context();
	char *key_str = (char *)malloc(4);
	sprintf(key_str, "%08X", (DWORD)retPtr);
	hashmap_put(context_hashmap, key_str, pcontext);
	
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
	disable_hook(MH_ALL_HOOKS);
	
	PVOID retPtr = fpHeapReAlloc(hHeap, dwFlags, lpMem, dwBytes);
	PCONTEXT pcontext = current_context();
	char *key_str = (char *)malloc(4);
	sprintf(key_str, "%08X", (DWORD)retPtr);
	hashmap_put(context_hashmap, key_str, pcontext);
	
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
	disable_hook(MH_ALL_HOOKS);

	BOOL retFlg = fpHeapFree(hHeap, dwFlags, lpMem);
	PCONTEXT pcontext = current_context();
	char *key_str = (char *)malloc(4);
	sprintf(key_str, "%08X", (DWORD)lpMem);
	hashmap_remove(context_hashmap, key_str, NULL);
	
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
	disable_hook(MH_ALL_HOOKS);

	//loadlibrary 
	HINSTANCE retInstance = fpLoadLibraryA(lpFileName);
	
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
	disable_hook(MH_ALL_HOOKS);

	//loadlibrary 
	HINSTANCE retInstance = fpLoadLibraryW(lpFileName);

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
	if(context_hashmap == NULL)
	{
		context_hashmap = malloc(sizeof(struct _Context) * HASHSIZE);
	}
	
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
* 释放hook 
*/
void release_hook()
{
	output_print("leak count is %d\n", hashmap_size(context_hashmap));
}

