/* Globbing functions for descent. Calls the relevant system handlers... */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "d_glob.h"
#include "error.h"
#include <string.h>
//added 05/17/99 Matt Mueller
#include "u_mem.h"
//end addition -MM

#if defined(__DJGPP__) || defined(__linux__)
#include <glob.h>

int d_glob(const char *pattern, d_glob_t *g)
{
 glob_t a;
 int r;

 Assert(g!=NULL);

 a.gl_offs=0;

 r=glob(pattern,0,NULL,&a);
 g->gl_pathc=a.gl_pathc;
 g->gl_pathv=a.gl_pathv;
 return r;
}

void d_globfree(d_glob_t *g)
{
#ifndef __linux__ // Linux doesn't believe in freeing glob structures... :-)
 glob_t a;

 Assert (g!=NULL);

 a.gl_offs=0;
 a.gl_pathc=g->gl_pathc;
 a.gl_pathv=g->gl_pathv;

 globfree(&a);
#endif
}

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
     globfiles[c] = d_strdup (*wfd.cAlternateFileName ?
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
   if (globfiles[i]) d_free(globfiles[i]);
 }
}

#else // Must be good old Watcom C.
#error "FIXME: Globbing isn't yet supported under Watcom C. Look at misc/d_glob.c"
#endif

