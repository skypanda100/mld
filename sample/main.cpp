#include "mld.h"
#include <iostream>
/* run this program using the console pauser or add your own getch, system("pause") or input loop */

int main(int argc, char** argv) {
	MingwLeakDetector();

	char *new_leak = new char[100];	
	char *malloc_leak = (char *)malloc(555);

	HINSTANCE sampledll = LoadLibraryA("sampledll.dll");
	if(sampledll != NULL){
		typedef void (*MEMORY_LEAK_TEST)();
		MEMORY_LEAK_TEST memory_leak_func = (MEMORY_LEAK_TEST)GetProcAddress(sampledll,"HelloWorld");
		if(memory_leak_func != NULL){
			memory_leak_func();
		}
//		FreeLibrary(sampledll);
	}

	MessageBox(0,"1\n","Hi",MB_ICONINFORMATION);
	MessageBox(0,"2\n","Hi",MB_ICONINFORMATION);
	MessageBox(0,"3\n","Hi",MB_ICONINFORMATION);

	return 0;
}
