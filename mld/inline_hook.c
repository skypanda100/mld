#include "inline_hook.h"

static volatile LONG hook_lock = FALSE;

static PMB	MB_MEM[MODULE_LEN] = {NULL};
static PJF	JF_MEM[FUNC_LEN] = {NULL};

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

    hModule = GetModuleHandleW(pszModule);
    if(hModule == NULL){
        return FALSE;
    }

    pTarget = (LPVOID)GetProcAddress(hModule, pszProcName);
    if(pTarget == NULL){
        return FALSE;
    }

    pPatchTarget = (LPBYTE)pTarget;
    
	//原始指令 
	JC oldJC;
	memcpy(&(oldJC.jmp), pPatchTarget, 1);
	memcpy(&(oldJC.addr), pPatchTarget + 1, 4);

	//新指令 
	JC newJC;
	newJC.jmp = 0xE9;
	newJC.addr = (UINT32)((LPBYTE)pDetour - (pPatchTarget + 5));
	
	//保存指令及函数 
	PJF pjf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(JF));
	pjf->oldJC = oldJC;
	pjf->newJC = newJC;
	pjf->pFunc = pTarget;
	
	int i = 0;
	for(;i < FUNC_LEN;i++){
		if(JF_MEM[i] == NULL){
			JF_MEM[i] = pjf;
			break;
		}else{
			if(JF_MEM[i]->pFunc == pTarget){
				JF_MEM[i]->oldJC = oldJC;
				JF_MEM[i]->newJC = newJC;
				break;
			}
		}
	}
	if(i == FUNC_LEN){
		return FALSE;
	}

    UINT32 offset = (UINT32)pTarget - (UINT32)hModule;
    *ppOriginal = (LPVOID)((UINT32)lpModuleBuffer + offset);
    
    return TRUE;
}

BOOL enableHook(){
	enter_hook_lock(&hook_lock);
	
	for(int i = 0;i < FUNC_LEN;i++){
		PJF pjf = JF_MEM[i];
		if(pjf != NULL){
			DWORD oldProtect;
			LPBYTE pPatchTarget = (LPBYTE)(pjf->pFunc);
			if(!VirtualProtect(pPatchTarget, 5, PAGE_EXECUTE_READWRITE, &oldProtect)){
        		return FALSE;
    		}
			*pPatchTarget = pjf->newJC.jmp;
			*(PUINT32)(pPatchTarget + 1) = pjf->newJC.addr;
    		VirtualProtect(pPatchTarget, 5, oldProtect, &oldProtect);
    		FlushInstructionCache(GetCurrentProcess(), pPatchTarget, 5);
		}
	}
	
	leave_hook_lock(&hook_lock);
	
	return TRUE;
}

BOOL disableHook(){
	enter_hook_lock(&hook_lock);

	for(int i = 0;i < FUNC_LEN;i++){
		PJF pjf = JF_MEM[i];
		if(pjf != NULL){
			DWORD oldProtect;
			LPBYTE pPatchTarget = (LPBYTE)(pjf->pFunc);
			if(!VirtualProtect(pPatchTarget, 5, PAGE_EXECUTE_READWRITE, &oldProtect)){
        		return FALSE;
    		}
			*pPatchTarget = pjf->oldJC.jmp;
			*(PUINT32)(pPatchTarget + 1) = pjf->oldJC.addr;
    		VirtualProtect(pPatchTarget, 5, oldProtect, &oldProtect);
    		FlushInstructionCache(GetCurrentProcess(), pPatchTarget, 5);
		}
	}

	leave_hook_lock(&hook_lock);

	return TRUE;
}
