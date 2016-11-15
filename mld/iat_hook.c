#include "iat_hook.h"

BOOL create_iat_hook(HMODULE lpBase, LPCSTR pszModule, LPCSTR pszProcName, FARPROC detourProc, LPVOID *ppOriginal){
	if(lpBase == NULL){
		lpBase = GetModuleHandleA(NULL);
		if(lpBase == NULL){
			return FALSE;
		}
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

	while(pImportDesc->FirstThunk){
		PSTR pszModName = (PSTR)((PBYTE)lpBase + pImportDesc->Name);
		if(stricmp(pszModName, pszModule) == 0){
			PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA)((PBYTE)lpBase + pImportDesc->FirstThunk);
			
			while(pThunk->u1.Function){
				PROC* ppfn = (PROC*)&(pThunk->u1.Function);
				if(*ppfn == hookProc){
					DWORD oldProtect;
					if(!VirtualProtect(ppfn, sizeof(PROC), PAGE_READWRITE, &oldProtect)){
						return FALSE;
					}
					*ppfn = detourProc;
					
					VirtualProtect(ppfn, sizeof(PROC), oldProtect, &oldProtect);
					FlushInstructionCache(GetCurrentProcess(), ppfn, sizeof(PROC));
				    break;
				}
				pThunk++;
			}
		}
		pImportDesc++;
	}
	
	*ppOriginal = hookProc;

    return TRUE;
}

