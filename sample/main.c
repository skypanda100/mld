#include <windows.h>
#include <stdio.h>
#include "mld.h"

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
	LoadLibraryA("mld.dll");
	mld_begin();
	char *leak = malloc(555);

//	if(sampledll != NULL){
//		typedef int(*HelloFunc)();
//		HelloFunc helloFunc = (HelloFunc)GetProcAddress(sampledll,"HelloWorld");
//		helloFunc();
//		FreeLibrary(sampledll);
//	}
//	bar();

	mld_end();
	return 0;
}


