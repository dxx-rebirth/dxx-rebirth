/* $Id: strio.c,v 1.5 2004-08-06 20:28:57 schaffner Exp $ */
/*
 * strio.c: string/file manipulation functions by Victor Rachels
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <string.h>

#include "cfile.h"
#include "strio.h"
//added on 9/16/98 by adb to add memory tracking for this module
#include "u_mem.h"
//end additions - adb

char *fgets_unlimited(CFILE *f)
{
    int		mem = 256;
    char	*word, *buf, *p;

    MALLOC(word, char, mem);
    p = word;

    while (word && cfgets(p, mem, f) == word + mem) {
        int i;
        
        // Make a bigger buffer, because it read to the end of the buffer.
        buf = word;
        mem *= 2;
        MALLOC(word, char, mem);
        for (i = 0; i < mem/2; i++)
            word[i] = buf[i];
        d_free(buf);
        p = word + mem/2;
    }
    return word;
}

char* splitword(char *s, char splitchar)
{
 int x,l,l2;
 char *word;

   for(l=0;s[l]!=0;l++);
   for(x=0;s[x]!=splitchar&&x<l;x++);
  l2=x;
  s[x]=0;
  word = (char *) d_malloc(sizeof(char) * (l2+1));
   for(x=0;x<=l2;x++)
    word[x]=s[x];

   if(l==l2)
    s[0]=0;
   else
    {
     while(x<=l)
      {
       s[x-l2-1]=s[x];
       x++;
      }
    }
  return word;
}
