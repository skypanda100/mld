#ifndef MLD
#define MLD

#pragma init_seg(compiler)
HMODULE mld_hmodule = NULL;

void MingwLeakDetector(){
	mld_hmodule = LoadLibraryA("mld.dll");
}

#endif
