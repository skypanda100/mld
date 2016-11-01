#include "hook.h"

/**
* LoadLibraryA
*/
typedef HINSTANCE (WINAPI *LOADLIBRARYA)(LPCSTR); 
LOADLIBRARYA fpLoadLibraryA = NULL;
HINSTANCE WINAPI DetourLoadLibraryA(LPCSTR lpFileName)
{
	//loadlibrary 
	HINSTANCE retInstance = fpLoadLibraryA(lpFileName);
	
	//loadsymbol
	if (retInstance != NULL)
	{
		load_symbol(retInstance);		
	}
	
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

	//loadsymbol
	if (retInstance != NULL)
	{
		load_symbol(retInstance);		
	}
	
    return retInstance; 
}

/**
* 初始化hook 
*/
BOOL init_hook()
{
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
