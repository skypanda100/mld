#include "iat_hook.h"

static volatile LONG hook_lock = FALSE;

static PJP JP_MEM[PROC_LEN] = {NULL};


static void enter_hook_lock(volatile LONG *lock)
{
	size_t c = 0;
	while(InterlockedCompareExchange(lock, TRUE, FALSE) != FALSE)
	{
		if(c < 20){
			Sleep(0);
		}else{
			Sleep(1);
		}
		c++;
	}
}

static void leave_hook_lock(volatile LONG *lock)
{
	InterlockedExchange(lock, FALSE);
}

BOOL create_iat_hook(LPCSTR pszTarget, LPCSTR pszModule, LPCSTR pszProcName, FARPROC detourProc, LPVOID *ppOriginal){
	HMODULE lpBase = GetModuleHandleA(pszTarget);
	if(lpBase == NULL){
		return FALSE;
	}
	
	HMODULE hModule = GetModuleHandleA(pszModule);
	if(hModule == NULL){
		return FALSE;
	}
	
	FARPROC hookProc = GetProcAddress(hModule, pszProcName);
	if(hookProc == NULL){
		return FALSE;
	}
	
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)lpBase;
	PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS32)((PBYTE)lpBase + pDosHeader->e_lfanew);; 
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)((PBYTE)lpBase + pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
	PROC* procAddr = NULL;

	while(pImportDesc->FirstThunk){
		PSTR pszModName = (PSTR)((PBYTE)lpBase + pImportDesc->Name);
		if(stricmp(pszModName, pszModule) == 0){
			PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA)((PBYTE)lpBase + pImportDesc->FirstThunk);
			
			while(pThunk->u1.Function){
				PROC* ppfn = (PROC*)&(pThunk->u1.Function);
				if(*ppfn == hookProc){
					procAddr = ppfn;
				    break;
				}
				pThunk++;
			}
		}
		pImportDesc++;
	}
	
    //±£´æÐÂ¾Éº¯Êý 
	PJP pjp = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(JP));
	pjp->oldProc = hookProc;
	pjp->newProc = detourProc;
	pjp->procAddr = procAddr;

    int i = 0;
	for(;i < PROC_LEN;i++){
		if(JP_MEM[i] == NULL){
			JP_MEM[i] = pjp;
			break;
		}else{
			if(JP_MEM[i]->oldProc == hookProc){
				JP_MEM[i]->newProc = detourProc;
				break;
			}
		}
	}
	if(i == PROC_LEN){
		return FALSE;
	}

	*ppOriginal = hookProc;

    return TRUE;
}

BOOL enable_iat_hook(){
	enter_hook_lock(&hook_lock);

	for(int i = 0;i < PROC_LEN;i++){
		if(JP_MEM[i] != NULL){
			FARPROC detourProc = JP_MEM[i]->newProc;
			PROC* procAddr = JP_MEM[i]->procAddr;
			if(procAddr == NULL){
				continue;
			}
			
			DWORD oldProtect;
			if(!VirtualProtect(procAddr, sizeof(PROC), PAGE_READWRITE, &oldProtect)){
				continue;
			}
	        *procAddr = detourProc;
	
	        VirtualProtect(procAddr, sizeof(PROC), oldProtect, &oldProtect);
    		FlushInstructionCache(GetCurrentProcess(), procAddr, sizeof(PROC));
		}
	}

	leave_hook_lock(&hook_lock);

	return TRUE;
}

BOOL disable_iat_hook(){
	enter_hook_lock(&hook_lock);

	for(int i = 0;i < PROC_LEN;i++){
		if(JP_MEM[i] != NULL){
			FARPROC hookProc = JP_MEM[i]->oldProc;
			PROC* procAddr = JP_MEM[i]->procAddr;
			if(procAddr == NULL){
				continue;
			}
			
			DWORD oldProtect;
			if(!VirtualProtect(procAddr, sizeof(PROC), PAGE_READWRITE, &oldProtect)){
				continue;
			}
	        *procAddr = hookProc;
	
	        VirtualProtect(procAddr, sizeof(PROC), oldProtect, &oldProtect);
    		FlushInstructionCache(GetCurrentProcess(), procAddr, sizeof(PROC));
		}
	}

	leave_hook_lock(&hook_lock);

	return TRUE;
}
