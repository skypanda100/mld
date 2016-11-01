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
		for(int i = 0;i < HASHSIZE;i++)
		{
			struct _Context *pContext = contexts[i];
			if(pContext == NULL)
			{
				pContext = (struct _Context*)malloc(sizeof(struct _Context));
				pContext.addr = addr;
				pContext.pcontext = pcontext;
				pContext.next = NULL;
				contexts[i] = pContext;
				
				if(i > 0 )
				{
					contexts[i - 1].next = pContext;
				}
				
				break;
			}
		}
	}else{
		contexts[index].pcontext = pcontext;
	}
}

void delete_hash(DWORD addr)
{
	
}
