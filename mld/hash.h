#ifndef HASH
#define HASH

#include <windows.h>

#define HASHSIZE 1024

struct _Context
{
	DWORD addr;
	PCONTEXT pcontext;
	struct _Context *next;
};

int search_hash(DWORD addr);

void insert_hash(DWORD addr, PCONTEXT pcontext);

void delete_hash(DWORD addr);

#endif
