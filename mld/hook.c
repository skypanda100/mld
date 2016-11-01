#include "hook.h"

/**
* LoadLibraryA
*/
typedef HINSTANCE (WINAPI *LOADLIBRARYA)(LPCSTR); 
LOADLIBRARYA fpLoadLibraryA = NULL;
HINSTANCE WINAPI DetourLoadLibraryA(LPCSTR lpFileName)
{
	if (trace_file != NULL)
	{
		fprintf(trace_file, "LoadLibraryA = %s\n", lpFileName);
		fflush(trace_file);
	}
	call_stack();
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
	if (trace_file != NULL)
	{
		fwprintf(trace_file, L"LoadLibraryW = %s\n", lpFileName);
		fflush(trace_file);		
	}

	//loadlibrary 
	HINSTANCE retInstance = fpLoadLibraryW(lpFileName);

	//loadsymbol
	if (retInstance != NULL)
	{
		load_symbol(retInstance);		
	}
	
    return retInstance; 
}

BOOL init_hook()
{
	if (MH_Initialize() != MH_OK)
	{
		return false;
	}
	return true;
}

BOOL uninit_hook()
{
	if (MH_Uninitialize() != MH_OK)
    {
        return false;
    }
    return true;
}

BOOL create_hook()
{
	if (MH_CreateHookApi(L"kernel32", "LoadLibraryA", &DetourLoadLibraryA, (LPVOID)&fpLoadLibraryA) != MH_OK)
    {
        return false;
    }
    
	if (MH_CreateHookApi(L"kernel32", "LoadLibraryW", &DetourLoadLibraryW, (LPVOID)&fpLoadLibraryW) != MH_OK)
    {
        return false;
    }
    
    return true;
}

BOOL enable_hook(LPVOID pTarget)
{
    if (MH_EnableHook(pTarget) != MH_OK)
    {
        return false;
    }
    return true;
}

BOOL disable_hook(LPVOID pTarget)
{
    if (MH_DisableHook(pTarget) != MH_OK)
    {
        return false;
    }
    return true;
}
