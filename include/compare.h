//added 03/05/99 Matt Mueller - comparison functions.. used in checking 
//versions without need to duplicate a function or have kludgy code

#ifndef _COMPARE_H
#define _COMPARE_H

#include "types.h"

typedef int32_t compare_int32_func(int32_t a,int32_t b);

compare_int32_func int32_greaterorequal;
compare_int32_func int32_lessthan;

#endif
