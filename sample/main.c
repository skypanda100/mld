#include <windows.h>
#include <stdio.h>

static void
foo()
{
	int *f=NULL;
	*f = 0;
}

static void
bar()
{
	foo();
}

int
main()
{
	printf("sample created\n");
	char *leak = (char *)malloc(555);
//	free(leak);
//	HINSTANCE sampledll = LoadLibraryA("sampledll.dll");
//	if(sampledll != NULL){
//		typedef void (*MEMORY_LEAK_TEST)();
//		MEMORY_LEAK_TEST memory_leak_func = (MEMORY_LEAK_TEST)GetProcAddress(sampledll,"HelloWorld");
//		if(memory_leak_func != NULL){
//			memory_leak_func();
//		}
//		FreeLibrary(sampledll);
//	}

	MessageBox(0,"Hello World from DLL!\n","Hi",MB_ICONINFORMATION);
	
	return 0;
}

