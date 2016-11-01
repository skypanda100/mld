#ifndef HOOK
#define HOOK

#include <windows.h>
#include "MinHook.h"
#include "callstack.h"
#include "hashmap.h"

#define HASHSIZE 1024*4

struct _Context
{
	DWORD addr;
	PCONTEXT pcontext;
	struct _Context *next;
	struct _Context *prev;
};

BOOL init_hook();

BOOL uninit_hook();

BOOL create_hook();

BOOL enable_hook(LPVOID pTarget);

BOOL disable_hook(LPVOID pTarget);

void release_hook();

#endif
