#include <windows.h>
#include "detector.h"

static void mld_begin(){
	init_detector();
}

static void mld_end(){
	uninit_detector();
}

void MingwLeakDetector(){
	mld_begin();
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_PROCESS_DETACH:
		mld_end();
		break;
	}
	return TRUE;
}
