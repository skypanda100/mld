#ifndef HOOK
#define HOOK

#include <windows.h>

#define MODULE_LEN	10
#define FUNC_LEN	10

typedef struct Module_Buffer{
	LPCWSTR	lpModule;
	LPVOID	lpBuffer;
}MB, *PMB;

typedef struct Jump_Code{
	UINT8	jmp;
	UINT32	addr;
}JC, *PJC;

typedef struct Jump_Func{
	JC		oldJC;
	JC		newJC;
	LPVOID	pFunc;
}JF, *PJF;

static int createBuffer(LPCWSTR pszModule);

static void enter_hook_lock(volatile LONG *);

static void leave_hook_lock(volatile LONG *);

BOOL createHook(LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour, LPVOID *ppOriginal);

BOOL enableHook();

BOOL disableHook();

#endif
