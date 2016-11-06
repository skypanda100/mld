#include <windows.h>
#include <stdio.h>
#include "mld.h"


__attribute((constructor)) void before_main()  
{
	LoadLibraryA("mld.dll");
    mld_begin();
}  
  
__attribute((destructor)) void after_main()  
{  
    mld_end();
}

int
main()
{
	char *leak = (char *)malloc(555);
//	free(leak);
	HINSTANCE sampledll = LoadLibraryA("sampledll.dll");
	if(sampledll != NULL){
		typedef void (*MEMORY_LEAK_TEST)();
		MEMORY_LEAK_TEST memory_leak_func = (MEMORY_LEAK_TEST)GetProcAddress(sampledll,"HelloWorld");
		if(memory_leak_func != NULL){
			memory_leak_func();
		}
		FreeLibrary(sampledll);
	}

	MessageBox(0,"Hello World from DLL!\n","Hi",MB_ICONINFORMATION);
	
	return 0;
}

