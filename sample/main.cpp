#include "mld.h"
#include <iostream>
#include <windows.h>
#include "dll.h"
/* run this program using the console pauser or add your own getch, system("pause") or input loop */

int *new_leak = (int *)malloc(700);
void test(){
	int *new_leak = new int[100];
//	addr = (DWORD)new_leak;
//	BYTE *p_size = (BYTE *)new_leak - 8;
//	for(int i = 0;i < 8;i++){
//		printf("%d\t:\t%d\n", i, *(p_size + i));
//	}
//	delete new_leak;
//	printf("########################\n");
//	for(int i = 0;i < 8;i++){
//		printf("%d\t:\t%d\n", i, *(p_size + i));
//	}
}

int main(int argc, char** argv) {	
	test();
	
	char *malloc_leak = (char *)malloc(555);

	HelloWorld();

	MessageBox(0,"1\n","Hi",MB_ICONINFORMATION);
	MessageBox(0,"2\n","Hi",MB_ICONINFORMATION);
	MessageBox(0,"3\n","Hi",MB_ICONINFORMATION);

	void *heapAlloc_leak = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 500);
	void *calloc_leak = calloc(10, 10);

	return 0;
}
