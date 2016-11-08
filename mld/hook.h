#ifndef HOOK
#define HOOK

#include <windows.h>

#define MODULE_LEN	10

typedef struct Module_Buffer
{
	LPCWSTR lpModule;
	LPVOID	lpBuffer;
}MB, *PMB;

static int createBuffer(LPCWSTR pszModule);

BOOL createHook(LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour, LPVOID *ppOriginal);

#endif
