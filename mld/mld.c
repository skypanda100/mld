#include <windows.h>
#include "mld.h"
#include "detector.h"

static void init_all(){
	init_detector();
}

static void uninit_all(){
	uninit_detector();
}

void mld_begin(){
	init_all();
}

void mld_end(){
	uninit_all();
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
