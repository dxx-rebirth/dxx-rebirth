/* Globbing functions for descent. Calls the relevant system handlers... */

#include "d_glob.h"
#include "error.h"
#include <string.h>
//added 05/17/99 Matt Mueller
#include "u_mem.h"
//end addition -MM

#if defined(__DJGPP__) || defined(__LINUX__)

//nothing to do, but we don't want to fall into the #error below.

#elif defined(__WINDOWS__)

#include <windows.h>

#define MAX_GLOB_FILES 500

/* Using a global variable stops this from being reentrant, but in descent,
   who cares? */
static char *globfiles[MAX_GLOB_FILES];

int d_glob(const char *pattern, d_glob_t *g)
{
 WIN32_FIND_DATA wfd;
 HANDLE wfh;
 int c;

 Assert (g!=NULL);
 c=0;
 if ((wfh=FindFirstFile(pattern,&wfd))!=INVALID_HANDLE_VALUE)
 {
   do {
	 LPSTR sz = wfd.cAlternateFileName;
	 if (sz == NULL || sz [0] == '\0')
		sz = wfd.cFileName;
     globfiles[c] = strdup (*wfd.cAlternateFileName ?
       wfd.cAlternateFileName : wfd.cFileName);

     c++; // Ho ho ho... :-)
     if (c>=MAX_GLOB_FILES) return 0;
   } while (FindNextFile(wfh,&wfd));
   FindClose(wfh);
 }

 g->gl_pathc = c;
 g->gl_pathv = globfiles;
 return (!c)?-1:0;
}

void d_globfree(d_glob_t *g)
{
 unsigned int i;
 Assert (g!=NULL);
 for (i=0; i < g->gl_pathc; i++) {
   if (globfiles[i]) free(globfiles[i]);
 }
}

#else // Must be good old Watcom C.
#error "FIXME: Globbing isn't yet supported under Watcom C. Look at misc/d_glob.c"
#endif

