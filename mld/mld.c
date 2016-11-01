#include <windows.h>
#include "hook.h"
#include "print.h"
#include "callstack.h"

static LPTOP_LEVEL_EXCEPTION_FILTER g_prev = NULL;

static void init_all()
{
	g_prev = SetUnhandledExceptionFilter(exception_filter);

	output_init();
	 
	if (!init_hook())
	{
		output_print("init_hook is not ok\n");
	}
	
	if (!create_hook())
	{
		output_print("create_hook is not ok\n");
	}
	
	if (!enable_hook(MH_ALL_HOOKS))
	{
		output_print("enable_hook is not ok\n");
	}

	if (SymInitialize(GetCurrentProcess(), 0, FALSE) == TRUE)
	{
		load_symbol(NULL);		
	}
}

static void uninit_all()
{
	SetUnhandledExceptionFilter(g_prev);
	
	disable_hook(MH_ALL_HOOKS);
	
	release_hook();
	
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
