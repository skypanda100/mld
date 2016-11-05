#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "hook.h"
#include "print.h"
#include "callstack.h"

extern HANDLE g_process;
extern HANDLE g_thread;

static LPTOP_LEVEL_EXCEPTION_FILTER g_prev = NULL;

void init_all();
void uninit_all();

void OnProcessCreated(const CREATE_PROCESS_DEBUG_INFO*);
void OnThreadCreated(const CREATE_THREAD_DEBUG_INFO*);
void OnException(const EXCEPTION_DEBUG_INFO*);
void OnProcessExited(const EXIT_PROCESS_DEBUG_INFO*);
void OnThreadExited(const EXIT_THREAD_DEBUG_INFO*);
void OnOutputDebugString(const OUTPUT_DEBUG_STRING_INFO*);
void OnRipEvent(const RIP_INFO*);
void OnDllLoaded(const LOAD_DLL_DEBUG_INFO*);
void OnDllUnloaded(const UNLOAD_DLL_DEBUG_INFO*);

int main(){
	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi = { 0 };

	if (CreateProcess(
		"sample.exe",
		NULL,
		NULL,
		NULL,
		FALSE,
		DEBUG_ONLY_THIS_PROCESS | CREATE_NEW_CONSOLE | CREATE_SUSPENDED,
		NULL,
		NULL,
		&si,
		&pi) == FALSE) {
		printf("CreateProcess failed: %d\n", GetLastError());
		return -1;
	}

	BOOL waitEvent = TRUE;

	ResumeThread(pi.hThread);

	DEBUG_EVENT debugEvent;
	while (waitEvent == TRUE && WaitForDebugEvent(&debugEvent, INFINITE)) {
		switch (debugEvent.dwDebugEventCode) {

			case CREATE_PROCESS_DEBUG_EVENT:
				OnProcessCreated(&debugEvent.u.CreateProcessInfo);
				break;

			case CREATE_THREAD_DEBUG_EVENT:
				OnThreadCreated(&debugEvent.u.CreateThread);
				break;

			case EXCEPTION_DEBUG_EVENT:
				OnException(&debugEvent.u.Exception);
				break;

			case EXIT_PROCESS_DEBUG_EVENT:
				OnProcessExited(&debugEvent.u.ExitProcess);
				waitEvent = FALSE;
				break;

			case EXIT_THREAD_DEBUG_EVENT:
				OnThreadExited(&debugEvent.u.ExitThread);
				break;

			case LOAD_DLL_DEBUG_EVENT:
				OnDllLoaded(&debugEvent.u.LoadDll);
				break;

			case UNLOAD_DLL_DEBUG_EVENT:
				OnDllUnloaded(&debugEvent.u.UnloadDll);
				break;

			case OUTPUT_DEBUG_STRING_EVENT:
				OnOutputDebugString(&debugEvent.u.DebugString);
				break;

			case RIP_EVENT:
				OnRipEvent(&debugEvent.u.RipInfo);
				break;

			default:
				break;
		}

		if (waitEvent == TRUE) {
			ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, DBG_CONTINUE);
		}
		else {
			break;
		}
	}

	g_process = pi.hProcess;
	g_thread = pi.hThread;
		
	return 0;
}


/**
* init
*/
void init_all()
{
	g_prev = SetUnhandledExceptionFilter(exception_filter);
	
//	if (SymInitialize(g_hProcess, 0, FALSE) == TRUE)
//	{
//		load_symbol(NULL);		
//	}

	output_init();

	init_hook_targets();

	init_hook();

	create_hook();

	enable_hook(MH_ALL_HOOKS);
}

/**
* uninit
*/ 
void uninit_all()
{
	SetUnhandledExceptionFilter(g_prev);
	
	disable_hook(MH_ALL_HOOKS);
	
	release_hook();
	
	uninit_hook();
	
	output_uninit();
}

void OnProcessCreated(const CREATE_PROCESS_DEBUG_INFO* pInfo) {
	init_all();
	
//	std::wcout << TEXT("Debuggee was created.") << std::endl;
}



void OnThreadCreated(const CREATE_THREAD_DEBUG_INFO* pInfo) {

//	std::wcout << TEXT("A new thread was created.") << std::endl;
}



void OnException(const EXCEPTION_DEBUG_INFO* pInfo) {

//	std::wcout << TEXT("An exception was occured.") << std::endl;
}



void OnProcessExited(const EXIT_PROCESS_DEBUG_INFO* pInfo) {
	uninit_all();
//	std::wcout << TEXT("Debuggee was terminated.") << std::endl;
}



void OnThreadExited(const EXIT_THREAD_DEBUG_INFO* pInfo) {

//	std::wcout << TEXT("A thread was terminated.") << std::endl;
}



void OnOutputDebugString(const OUTPUT_DEBUG_STRING_INFO* pInfo) {

//	std::wcout << TEXT("Debuggee outputed debug string.") << std::endl;
}



void OnRipEvent(const RIP_INFO* pInfo) {

//	std::wcout << TEXT("A RIP_EVENT occured.") << std::endl;
}



void OnDllLoaded(const LOAD_DLL_DEBUG_INFO* pInfo) {

//	std::wcout << TEXT("A dll was loaded.") << std::endl;
}



void OnDllUnloaded(const UNLOAD_DLL_DEBUG_INFO* pInfo) {

//	std::wcout << TEXT("A dll was unloaded.") << std::endl;
}








