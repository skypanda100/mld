#ifndef CALLSTACK
#define CALLSTACK

#define PACKAGE "your-program-name"
#define PACKAGE_VERSION "0.1"

#include <windows.h>
#include <excpt.h>
#include <imagehlp.h>
#include <bfd.h>
#include <psapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "print.h"

#define BFD_ERR_OK          (0)
#define BFD_ERR_OPEN_FAIL   (1)
#define BFD_ERR_BAD_FORMAT  (2)
#define BFD_ERR_NO_SYMBOLS  (3)
#define BFD_ERR_READ_SYMBOL (4)

static const char *const bfd_errors[] = {
	"",
	"(Failed to open bfd)",
	"(Bad format)",
	"(No symbols)",
	"(Failed to read symbols)",
};

struct bfd_ctx {
	bfd * handle;
	asymbol ** symbol;
};

struct bfd_set {
	char * name;
	struct bfd_ctx * bc;
	struct bfd_set *next;
};

struct find_info {
	asymbol **symbol;
	bfd_vma counter;
	const char *file;
	const char *func;
	unsigned line;
};

void lookup_section(bfd *abfd, asection *sec, void *opaque_data);

void find(struct bfd_ctx * b, DWORD offset, const char **file, const char **func, unsigned *line);

int init_bfd_ctx(struct bfd_ctx *bc, const char * procname, int *err);

void close_bfd_ctx(struct bfd_ctx *bc);

struct bfd_ctx *get_bc(struct bfd_set *set , const char *procname, int *err);

void release_set(struct bfd_set *set);

void _backtrace(struct bfd_set *set, int depth , LPCONTEXT context);

void module_path(HINSTANCE moduleInstance, LPSTR lpFileName,DWORD size);

void load_symbol(HINSTANCE retInstance);

LONG WINAPI exception_filter(LPEXCEPTION_POINTERS info);

#endif