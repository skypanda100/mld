#include "detector.h"
#include <stdio.h>

volatile LONG malloc_lock = FALSE;
volatile LONG realloc_lock = FALSE;
volatile LONG free_lock = FALSE;
volatile LONG libA_lock = FALSE;
volatile LONG libW_lock = FALSE;

/**
* malloc
*/
typedef _CRTIMP __cdecl void *(*MALLOC)(size_t);
MALLOC fpMalloc = NULL;
_CRTIMP __cdecl __MINGW_NOTHROW void *DetourMalloc(size_t size){
	void *retPtr = fpMalloc(size);
	return retPtr;
}

/**
* realloc
*/
typedef _CRTIMP __cdecl void *(*REALLOC)(void *, size_t);
REALLOC fpRealloc = NULL;
_CRTIMP __cdecl __MINGW_NOTHROW void *DetourRealloc(void *ptr, size_t size){
	void *retPtr = fpRealloc(ptr, size);

	return retPtr;
}

/**
* free
*/
typedef _CRTIMP __cdecl void (*FREE)(void *);
FREE fpFree = NULL;
_CRTIMP __cdecl __MINGW_NOTHROW void DetourFree(void *ptr){
	fpFree(ptr);
}

/**
* LoadLibraryA
*/
typedef HINSTANCE (WINAPI *LOADLIBRARYA)(LPCSTR); 
LOADLIBRARYA fpLoadLibraryA = NULL;
HINSTANCE WINAPI DetourLoadLibraryA(LPCSTR lpFileName){
	//loadlibrary 
	HINSTANCE retInstance = fpLoadLibraryA(lpFileName);
	
    return retInstance; 
}

/**
* LoadLibraryW
*/
typedef HINSTANCE (WINAPI *LOADLIBRARYW)(LPCWSTR); 
LOADLIBRARYW fpLoadLibraryW = NULL;
HINSTANCE WINAPI DetourLoadLibraryW(LPCWSTR lpFileName){
	//loadlibrary 
	HINSTANCE retInstance = ((LOADLIBRARYW)fpLoadLibraryW)(lpFileName);

    return retInstance; 
}

BOOL init_detector(){
	if (createHook(L"msvcrt", "malloc", &DetourMalloc, (LPVOID)&fpMalloc) != TRUE){
		return FALSE;
	}

	if (createHook(L"msvcrt", "realloc", &DetourRealloc, (LPVOID)&fpRealloc) != TRUE){
		return FALSE;
	}

	if (createHook(L"msvcrt", "free", &DetourFree, (LPVOID)&fpFree) != TRUE){
		return FALSE;
	}
	
    if (createHook(L"kernel32", "LoadLibraryA", &DetourLoadLibraryA, (LPVOID)&fpLoadLibraryA) != TRUE){
        return FALSE;
    }
    
	if (createHook(L"kernel32", "LoadLibraryW", &DetourLoadLibraryW, (LPVOID)&fpLoadLibraryW) != TRUE){
        return FALSE;
    }
}
