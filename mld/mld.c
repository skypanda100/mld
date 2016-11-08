#include <windows.h>
#include "detector.h"

static void init_all()
{
	init_detector();
}

static void uninit_all()
{
	
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
