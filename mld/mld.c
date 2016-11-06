#include <windows.h>
#include "hook.h"
#include "print.h"
#include "callstack.h"

static LPTOP_LEVEL_EXCEPTION_FILTER g_prev = NULL;

static void init_all()
{
	g_prev = SetUnhandledExceptionFilter(exception_filter);
	
//	if (SymInitialize(GetCurrentProcess(), 0, FALSE) == TRUE)
//	{
//		load_symbol(NULL);		
//	}
	
	output_init();

	init_hook_targets();
	 
	init_hook();
	
	create_hook();
		
	enable_hook(MH_ALL_HOOKS);
}

static void uninit_all()
{
	SetUnhandledExceptionFilter(g_prev);
	
	disable_hook(MH_ALL_HOOKS);
	
	release_hook();
	
	uninit_hook();
	
	output_uninit();
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		init_all(); 
		break;
	case DLL_PROCESS_DETACH:
		uninit_all();
		break;
	}
	return TRUE;
}
