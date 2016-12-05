#ifndef _DLL_H_
#define _DLL_H_

#if BUILDING_DLL
#define DLLIMPORT __declspec(dllexport)
#else
#define DLLIMPORT __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C"{
#endif

DLLIMPORT void HelloWorld();

#ifdef __cplusplus
}
#endif

#endif
