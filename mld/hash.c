#include "hash.h"

struct _Context *contexts[HASHSIZE] = {NULL};

int search_hash(DWORD addr)
{
	int index = 0;
	struct _Context *pContext = contexts[0];
	while(pContext != NULL)
	{
		if(pContext->addr == addr)
		{
			return index;
		}
		pContext = pContext->pcontext;
		index++;
	}
	return -1;
}

void insert_hash(DWORD addr, PCONTEXT pcontext)
{
	int index = search_hash(addr);
	if(index == -1){
		
	}
}

void delete_hash(DWORD addr)
{
	
}
