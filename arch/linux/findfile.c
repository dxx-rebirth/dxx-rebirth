#include <conf.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include "findfile.h"
#include "u_mem.h"
#include "error.h"

/* KLUDGE ALERT: evil globals */
static glob_t glob_a;
static int glob_whichfile;

int FileFindFirst(char *search_str, FILEFINDSTRUCT *ffstruct)
{
  int r;
  char *t;

  Assert(search_str != NULL);
  Assert(ffstruct != NULL);

  r = glob(search_str, 0, NULL, &glob_a);
  if (r == GLOB_NOMATCH) return -1;

  glob_whichfile = 0;
  
  t = strrchr(glob_a.gl_pathv[glob_whichfile], '/');
  if (t == NULL) t = glob_a.gl_pathv[glob_whichfile]; else t++;
  strncpy(ffstruct->name, t, 255);
  ffstruct->size = strlen(ffstruct->name); 
  
  return 0;
}

int FileFindNext(FILEFINDSTRUCT *ffstruct)
{
  char *t;
  
  glob_whichfile++;

  if (glob_whichfile >= glob_a.gl_pathc) return -1;

  t = strrchr(glob_a.gl_pathv[glob_whichfile], '/');
  if (t == NULL) t = glob_a.gl_pathv[glob_whichfile]; else t++;
  strncpy(ffstruct->name, t, 255);
  ffstruct->size = strlen(ffstruct->name); 
  return 0;
}

int FileFindClose(void)
{
 globfree(&glob_a);
 return 0;
}
