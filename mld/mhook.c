#include "mhook.h"

map_t context_hashmap = NULL;
CRITICAL_SECTION mem_cs;

/**
* malloc
*/
typedef _CRTIMP __cdecl void *(*MALLOC)(size_t);
MALLOC fpMalloc = NULL;
_CRTIMP __cdecl __MINGW_NOTHROW void *DetourMalloc(size_t size)
{
	EnterCriticalSection(&mem_cs);

	void *retPtr = fpMalloc(size);

	disable_hook(MH_ALL_HOOKS);

	PCONTEXT pcontext = current_context();
	add_context((DWORD)retPtr, size, pcontext);
	
	enable_hook(MH_ALL_HOOKS);

	LeaveCriticalSection(&mem_cs);
	
	return retPtr;
}

/**
* realloc
*/
typedef _CRTIMP __cdecl void *(*REALLOC)(void *, size_t);
REALLOC fpRealloc = NULL;
_CRTIMP __cdecl __MINGW_NOTHROW void *DetourRealloc(void *ptr, size_t size)
{
	EnterCriticalSection(&mem_cs);

	void *retPtr = fpRealloc(ptr, size);

	disable_hook(MH_ALL_HOOKS);

	PCONTEXT pcontext = current_context();
	del_context((DWORD)ptr);
	add_context((DWORD)retPtr, size, pcontext);
	
	enable_hook(MH_ALL_HOOKS);

	LeaveCriticalSection(&mem_cs);
	
	return retPtr;
}

/**
* free
*/
typedef _CRTIMP __cdecl void (*FREE)(void *);
FREE fpFree = NULL;
_CRTIMP __cdecl __MINGW_NOTHROW void DetourFree(void *ptr)
{
	EnterCriticalSection(&mem_cs);

	disable_hook(MH_ALL_HOOKS);
	
	del_context((DWORD)ptr);
	
	enable_hook(MH_ALL_HOOKS);

	LeaveCriticalSection(&mem_cs);
	
	fpFree(ptr);
}

/**
* ��ʼ��hook 
*/
BOOL init_hook()
{
	init_context();
	
	InitializeCriticalSection(&mem_cs);
	
	if (MH_Initialize() != MH_OK)
	{
		return FALSE;
	}
	
	return TRUE;
}

/**
* ���hook 
*/
BOOL uninit_hook()
{	
	if (MH_Uninitialize() != MH_OK)
    {
        return FALSE;
    }
    
    return TRUE;
}

/**
* ����hook 
*/
BOOL create_hook()
{
	if (MH_CreateHookApi(L"msvcrt", "malloc", &DetourMalloc, (LPVOID)&fpMalloc) != MH_OK)
	{
		return FALSE;
	}

	if (MH_CreateHookApi(L"msvcrt", "realloc", &DetourRealloc, (LPVOID)&fpRealloc) != MH_OK)
	{
		return FALSE;
	}

	if (MH_CreateHookApi(L"msvcrt", "free", &DetourFree, (LPVOID)&fpFree) != MH_OK)
	{
		return FALSE;
	}
    
    return TRUE;
}

/**
* ����hook 
*/
BOOL enable_hook(LPVOID pTarget)
{
    if (MH_EnableHook(pTarget) != MH_OK)
    {
        return FALSE;
    }

    return TRUE;
}

/**
* �ǻ���hook 
*/ 
BOOL disable_hook(LPVOID pTarget)
{
    if (MH_DisableHook(pTarget) != MH_OK)
    {
        return FALSE;
    }

    return TRUE;
}

/**
* ��ʼ��Context_Element
*/
void init_context()
{
	if(context_hashmap == NULL)
	{
		context_hashmap = hashmap_new();
	}
}

/**
* �ͷ�hook 
*/
static int leak_count = 0;
static ULONG leak_total = 0;

void release_hook()
{	
	hashmap_iterate(context_hashmap, &loop_context, NULL);
	uninit_context();
	output_print("\n------------------------------ memory leak (count %d): total size = %ld ------------------------------\n"
		, leak_count
		, leak_total);
	leak_count = 0;
	leak_total = 0;
}

/**
* ����Context_Element
*/
void add_context(DWORD addr, size_t size, PCONTEXT pcontext)
{
	//key
	char *key_str = (char *)malloc(KEYLEN);
	sprintf(key_str, "%08X", addr);

	//value
	struct Context_Element *context_element = (struct Context_Element *)malloc(sizeof(struct Context_Element));
	context_element->addr = addr;
	context_element->size = size;
	context_element->call_str = (char *)malloc(BACKTRACELEN);
	memset(context_element->call_str, '\0', BACKTRACELEN);
	call_stack(pcontext, context_element->call_str);
	context_element->call_str[BACKTRACELEN - 1] = '\0';

	hashmap_put(context_hashmap, key_str, context_element);
}

/**
* ɾ��Context_Element
*/
void del_context(DWORD addr)
{
	//key
	char *key_str = (char *)malloc(KEYLEN);
	sprintf(key_str, "%08X", addr);

	hashmap_remove(context_hashmap, key_str);
}

/**
* ����Context_Element
*/
int loop_context(any_t item, any_t data)
{
	struct Context_Element *context_element = (struct Context_Element *)data;
	if(context_element != NULL)
	{
		leak_count++;
		leak_total += context_element->size;
		output_print("------------------------------ memory leak (block %d): address = 0x%08X size = %ld ------------------------------\n[callstack]\n%s\n"
		, leak_count
		, context_element->addr
		, context_element->size
		, context_element->call_str);
	}
	return MAP_OK;
}

/**
* ����Context_Element
*/
void uninit_context()
{
	hashmap_free(context_hashmap);
}
