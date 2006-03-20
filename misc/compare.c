//added 03/05/99 Matt Mueller - comparison functions.. used in checking
//versions without need to duplicate a function or have kludgy code

#include "compare.h"
int32_t int32_greaterorequal(int32_t a,int32_t b)
{
    return (a>=b);
}

int32_t int32_lessthan(int32_t a,int32_t b)
{
    return (a<b);
}

