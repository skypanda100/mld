#include "mld.h"
#include <iostream>
/* run this program using the console pauser or add your own getch, system("pause") or input loop */

DWORD addr;
void test(){
	int *new_leak = new int[100];
	addr = (DWORD)new_leak;
	BYTE *p_size = (BYTE *)new_leak - 8;
	for(int i = 0;i < 8;i++){
		printf("%d\t:\t%d\n", i, *(p_size + i));
	}
	delete new_leak;
	printf("########################\n");
	for(int i = 0;i < 8;i++){
		printf("%d\t:\t%d\n", i, *(p_size + i));
	}
}

int main(int argc, char** argv) {
	MingwLeakDetector();
	
	test();
	printf("%d\n", IsBadHugeWritePtr((LPVOID)addr, 400));
	
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

	void *heapAlloc_leak = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 500);

	return 0;
}
