#ifndef IAT_HOOK
#define IAT_HOOK

#include <windows.h>
#include <imagehlp.h>

<<<<<<< HEAD
#define PROC_LEN	256

typedef struct Jump_Proc{
	FARPROC		oldProc;
	FARPROC		newProc;
	PROC*		procAddr;
}JP, *PJP;

static void enter_hook_lock(volatile LONG *);

static void leave_hook_lock(volatile LONG *);

static BOOL create_iat_hook(HMODULE lpBase, LPCSTR pszModule, LPCSTR pszProcName, FARPROC detourProc, LPVOID *ppOriginal);

BOOL create_iat_hook_a(LPCSTR pszTarget, LPCSTR pszModule, LPCSTR pszProcName, FARPROC detourProc, LPVOID *ppOriginal);

BOOL create_iat_hook_w(LPCWSTR pszTarget, LPCSTR pszModule, LPCSTR pszProcName, FARPROC detourProc, LPVOID *ppOriginal);

BOOL enable_iat_hook();

BOOL disable_iat_hook();
=======
BOOL create_iat_hook(HMODULE lpBase, LPCSTR pszModule, LPCSTR pszProcName, FARPROC detourProc);

>>>>>>> 323e65d098e3c903db3e59858c6a11c60241b604

#endif
