#ifndef _ALLOCA_H_
#define _ALLOCA_H_
 
#include <stddef.h>
 
//--------------------------------------------------------------------------------
#undef alloca
#define alloca(SIZE) __builtin_alloca(SIZE)
extern void * alloca( size_t );
 
#endif // _ALLOCA_H_
