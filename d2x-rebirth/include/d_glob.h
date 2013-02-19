/* Globbing functions for descent. Calls the relevant system handlers... */

#ifndef _D_GLOB
#define _D_GLOB

#include <stdlib.h>
#if defined(__DJGPP__) || defined(__LINUX__)
#include <glob.h>
typedef glob_t d_glob_t;
#define d_glob(p,g) glob(p,0,NULL,g)
#define d_globfree(g) globfree(g)
#else
typedef struct
  {
    size_t gl_pathc;
    char **gl_pathv;
  }
d_glob_t;

int d_glob (const char *pattern, d_glob_t * g);
void d_globfree (d_glob_t * g);
#endif

#endif
