#include "hook.h"

PMB	MB_MEM[MODULE_LEN] = {NULL};

static int createBuffer(LPCWSTR pszModule){
	//在已经创建的module中寻找 
	int index = 0;
	for(;index < MODULE_LEN;index++){
		if(MB_MEM[index] == NULL){
			break;
		}else{
			if(wcscmp(MB_MEM[index]->lpModule, pszModule) == 0){
				return index;
			}
		}
	}
	
	//空间已占满，不能再创建了 
	if(index == MODULE_LEN){
		return -1;
	}
	
	//创建module 
	LPVOID lpModuleBuffer = NULL;
	
    HMODULE hModule = GetModuleHandleW(pszModule);
    if(hModule == NULL){
        return -1;
    }
    MEMORY_BASIC_INFORMATION MemoryBasicInfomation;
    SIZE_T size = 0;
    VirtualQueryEx(GetCurrentProcess(), hModule, &MemoryBasicInfomation, sizeof(MEMORY_BASIC_INFORMATION));
    PVOID BaseAddress = MemoryBasicInfomation.AllocationBase;

    while (MemoryBasicInfomation.AllocationBase == BaseAddress)
    {
        size += MemoryBasicInfomation.RegionSize;
        VirtualQueryEx(GetCurrentProcess(), hModule + size, &MemoryBasicInfomation, sizeof(MEMORY_BASIC_INFORMATION));
    }

    lpModuleBuffer = VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    RtlMoveMemory(lpModuleBuffer, hModule, size);
    
    //插入空间
    PMB pmb = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MB));
    pmb->lpModule = pszModule;
    pmb->lpBuffer = lpModuleBuffer;
	MB_MEM[index] = pmb;
	
    return index;
}

BOOL createHook(LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour, LPVOID *ppOriginal){
	//创建module 
	int module_index = createBuffer(pszModule);
	if(module_index == -1){
		return FALSE;
	}
	LPVOID lpModuleBuffer = MB_MEM[module_index]->lpBuffer;
	
	//创建hook 
    HMODULE hModule = NULL;
    LPVOID pTarget = NULL;
    LPBYTE pPatchTarget = NULL;
    DWORD oldProtect;

    hModule = GetModuleHandleW(pszModule);
    if(hModule == NULL){
        return FALSE;
    }

    pTarget = (LPVOID)GetProcAddress(hModule, pszProcName);
    if(pTarget == NULL){
        return FALSE;
    }

    pPatchTarget = (LPBYTE)pTarget;
    if(!VirtualProtect(pPatchTarget, 5, PAGE_EXECUTE_READWRITE, &oldProtect)){
        return FALSE;
    }

    *pPatchTarget = 0xE9;
    *(PUINT32)(pPatchTarget + 1) = (UINT32)((LPBYTE)pDetour - (pPatchTarget + 5));

    VirtualProtect(pPatchTarget, 5, oldProtect, &oldProtect);

    FlushInstructionCache(GetCurrentProcess(), pPatchTarget, 5);

    UINT32 offset = (UINT32)pTarget - (UINT32)hModule;
    *ppOriginal = (LPVOID)((UINT32)lpModuleBuffer + offset);
}

