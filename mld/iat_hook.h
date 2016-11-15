#ifndef IAT_HOOK
#define IAT_HOOK

#include <windows.h>
#include <imagehlp.h>

BOOL create_iat_hook(HMODULE lpBase, LPCSTR pszModule, LPCSTR pszProcName, FARPROC detourProc);


#endif
