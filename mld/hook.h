#ifndef HOOK
#define HOOK

#include <windows.h>
#include "MinHook.h"
#include "callstack.h"
#include "hashmap.h"

#define KEYLEN	8 + 1
#define BACKTRACELEN 1024*4 + 1

struct Context_Element
{
	DWORD addr;
	DWORD size;
	char* call_str;
};

BOOL init_hook();

BOOL uninit_hook();

BOOL create_hook();

BOOL enable_hook(LPVOID pTarget);

BOOL disable_hook(LPVOID pTarget);

void release_hook();

void init_context();

void add_context(DWORD addr, size_t size, PCONTEXT pcontext);

void del_context(DWORD addr);

int loop_context(any_t item, any_t data);

void uninit_context();

#endif
