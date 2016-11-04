#ifndef HOOK
#define HOOK

#include <windows.h>
#include "MinHook.h"
#include "callstack.h"
#include "hashmap.h"

#define KEYLEN	8 + 1
#define BACKTRACELEN 1024*4

struct Context_Element
{
	char call_str[BACKTRACELEN];
	DWORD addr;
	DWORD size;
};

void init_hook_targets();

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

void enter_malloc_lock(volatile LONG *);

void leave_malloc_lock(volatile LONG *);

void enter_realloc_lock(volatile LONG *);

void leave_realloc_lock(volatile LONG *);

void enter_free_lock(volatile LONG *);

void leave_free_lock(volatile LONG *);

void enter_libA_lock(volatile LONG *);

void leave_libA_lock(volatile LONG *);

void enter_libW_lock(volatile LONG *);

void leave_libW_lock(volatile LONG *);
#endif
