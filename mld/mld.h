#ifndef MLD
#define MLD

#include <windows.h>

#ifdef __cplusplus
extern "C"{
#endif

void mld_begin();

void mld_end();

#ifdef __cplusplus
}
#endif

__attribute__((constructor(101))) void MLD_BEGIN(){
	mld_begin();
}

__attribute__((destructor(101))) void MLD_END(){
	mld_end();
}

#endif
