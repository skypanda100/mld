#ifndef DETECTOR
#define DETECTOR

#include <windows.h>
#include "iat_hook.h"
#include "callstack.h"
#include "hashmap.h"
#include "report.h"

#define KEYLEN	(8 + 1) 
#define BACKTRACELEN (1024*10)
#define LOADED_DLL_LEN 1024

typedef struct Context_Element
{
	DWORD		addr;
	DWORD		size;
	char		backtrace[BACKTRACELEN];
}CE, *PCE;

BOOL init_detector();

BOOL uninit_detector();

static void detect();

static void init_symbol();

static void uninit_symbol();

static void init_context();

static void add_context(DWORD addr, size_t size, PCONTEXT pcontext);

static void del_context(DWORD addr);

static int loop_context(any_t item, any_t data);

static void uninit_context();

static BOOL create_hooks_a(LPCSTR);

static BOOL create_hooks_w(LPCWSTR);

static void load_dll();

static void enter_malloc_lock(volatile LONG *);

static void leave_malloc_lock(volatile LONG *);

static void enter_calloc_lock(volatile LONG *);

static void leave_calloc_lock(volatile LONG *);

static void enter_HeapAlloc_lock(volatile LONG *);

static void leave_HeapAlloc_lock(volatile LONG *);

static void enter_realloc_lock(volatile LONG *);

static void leave_realloc_lock(volatile LONG *);

static void enter_HeapReAlloc_lock(volatile LONG *);

static void leave_HeapReAlloc_lock(volatile LONG *);

static void enter_free_lock(volatile LONG *);

static void leave_free_lock(volatile LONG *);

static void enter_HeapFree_lock(volatile LONG *);

static void leave_HeapFree_lock(volatile LONG *);

static void enter_libA_lock(volatile LONG *);

static void leave_libA_lock(volatile LONG *);

static void enter_libExA_lock(volatile LONG *);

static void leave_libExA_lock(volatile LONG *);

static void enter_libW_lock(volatile LONG *);

static void leave_libW_lock(volatile LONG *);

static void enter_libExW_lock(volatile LONG *);

static void leave_libExW_lock(volatile LONG *);

#endif
