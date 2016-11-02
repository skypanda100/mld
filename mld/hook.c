#include "hook.h"

map_t context_hashmap = NULL;

/**
* malloc
*/
typedef _CRTIMP __cdecl void *(*MALLOC)(size_t);
MALLOC fpMalloc = NULL;
_CRTIMP __cdecl __MINGW_NOTHROW void *DetourMalloc(size_t size)
{
	void *retPtr = fpMalloc(size);
	disable_hook(MH_ALL_HOOKS);
	PCONTEXT pcontext = current_context();
	add_context((DWORD)retPtr, size, pcontext);
	enable_hook(MH_ALL_HOOKS);
	
	return retPtr;
}

/**
* realloc
*/
typedef _CRTIMP __cdecl void *(*REALLOC)(void *, size_t);
REALLOC fpRealloc = NULL;
_CRTIMP __cdecl __MINGW_NOTHROW void *DetourRealloc(void *ptr, size_t size)
{
	void *retPtr = fpRealloc(ptr, size);
	disable_hook(MH_ALL_HOOKS);
	PCONTEXT pcontext = current_context();
	del_context((DWORD)ptr);
	add_context((DWORD)retPtr, size, pcontext);
	enable_hook(MH_ALL_HOOKS);
	
	return retPtr;
}

/**
* free
*/
typedef _CRTIMP __cdecl void (*FREE)(void *);
FREE fpFree = NULL;
_CRTIMP __cdecl __MINGW_NOTHROW void DetourFree(void *ptr)
{
	disable_hook(MH_ALL_HOOKS);
	del_context((DWORD)ptr);
	enable_hook(MH_ALL_HOOKS);
	
	fpFree(ptr);
}

/**
* LoadLibraryA
*/
typedef HINSTANCE (WINAPI *LOADLIBRARYA)(LPCSTR); 
LOADLIBRARYA fpLoadLibraryA = NULL;
HINSTANCE WINAPI DetourLoadLibraryA(LPCSTR lpFileName)
{
	//loadlibrary 
	HINSTANCE retInstance = fpLoadLibraryA(lpFileName);

	disable_hook(MH_ALL_HOOKS);
	//loadsymbol
	if (retInstance != NULL)
	{
		load_symbol(retInstance);		
	}
	enable_hook(MH_ALL_HOOKS);

    return retInstance; 
}

/**
* LoadLibraryW
*/
typedef HINSTANCE (WINAPI *LOADLIBRARYW)(LPCWSTR); 
LOADLIBRARYW fpLoadLibraryW = NULL;
HINSTANCE WINAPI DetourLoadLibraryW(LPCWSTR lpFileName)
{
	//loadlibrary 
	HINSTANCE retInstance = fpLoadLibraryW(lpFileName);

	disable_hook(MH_ALL_HOOKS);
	//loadsymbol
	if (retInstance != NULL)
	{
		load_symbol(retInstance);		
	}
	enable_hook(MH_ALL_HOOKS);

    return retInstance; 
}

/**
* 初始化hook 
*/
BOOL init_hook()
{
	init_context();
	
	if (MH_Initialize() != MH_OK)
	{
		return FALSE;
	}
	
	return TRUE;
}

/**
* 解除hook 
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
* 创建hook 
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

	if (MH_CreateHookApi(L"kernel32", "LoadLibraryA", &DetourLoadLibraryA, (LPVOID)&fpLoadLibraryA) != MH_OK)
    {
        return FALSE;
    }
    
	if (MH_CreateHookApi(L"kernel32", "LoadLibraryW", &DetourLoadLibraryW, (LPVOID)&fpLoadLibraryW) != MH_OK)
    {
        return FALSE;
    }
    
    return TRUE;
}

/**
* 活性hook 
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
* 非活性hook 
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
* 初始化Context_Element
*/
void init_context()
{
	if(context_hashmap == NULL)
	{
		context_hashmap = hashmap_new();
	}
}

/**
* 释放hook 
*/
static int leak_count = 0;
static ULONG leak_total = 0;

void release_hook()
{	
	hashmap_iterate(context_hashmap, &loop_context, NULL);
	uninit_context();
	output_print("------------------------------ memory leak (count %d): total size = %ld ------------------------------\n"
		, leak_count
		, leak_total);
	leak_count = 0;
	leak_total = 0;
}

/**
* 添加Context_Element
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
	memset(context_element->call_str, '\0', BACKTRACELEN);
	call_stack(pcontext, context_element->call_str);
	
	hashmap_put(context_hashmap, key_str, context_element);
}

/**
* 删除Context_Element
*/
void del_context(DWORD addr)
{
	//key
	char *key_str = (char *)malloc(KEYLEN);
	sprintf(key_str, "%08X", addr);
	
	hashmap_remove(context_hashmap, key_str);
}

/**
* 遍历Context_Element
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
* 销毁Context_Element
*/
void uninit_context()
{
	hashmap_free(context_hashmap);
}

