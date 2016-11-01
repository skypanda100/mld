#ifndef HOOK
#define HOOK

#include <windows.h>
#include "MinHook.h"
#include "callstack.h"

BOOL init_hook();

BOOL uninit_hook();

BOOL create_hook();

BOOL enable_hook(LPVOID pTarget);

BOOL disable_hook(LPVOID pTarget);

#endif
