/* Replace "dll.h" with the name of your header */
#include "dll.h"
#include <windows.h>

DLLIMPORT void HelloWorld()
{
	MessageBox(0,"","",MB_ICONINFORMATION);
	char *leak_1 = (char *)malloc(77);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	switch(fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			break;
		}
		case DLL_THREAD_ATTACH:
		{
			break;
		}
		case DLL_THREAD_DETACH:
		{
			break;
		}
	}
	
	/* Return TRUE on success, FALSE on failure */
	return TRUE;
}
