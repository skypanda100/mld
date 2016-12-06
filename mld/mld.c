#include <windows.h>
#include "detector.h"

void mld_begin(){
	init_detector();
}

void mld_end(){
	uninit_detector();
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
//		mld_begin();
		break;
	case DLL_PROCESS_DETACH:
//		mld_end();
		break;
	}
	return TRUE;
}
