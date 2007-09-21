/* changes [back]slashes to appropriate format */
#include <stdio.h>
#include <string.h>

#include "d_slash.h"
#include "u_mem.h"


void d_slash(char *path)
{
 char *p;

   for(p=path; *p; p++)
    if(*p == CHANGESLASH)
     *p = USEDSLASH;
   if(p == path || p[-1] == ':')
    *(p++) = '.';
   if(p[-1] != USEDSLASH)
    *(p++) = USEDSLASH;
  *p=0;
}
