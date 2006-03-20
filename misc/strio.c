/**strio.c: string/file manipulation functions by Victor Rachels **/
#include <stdio.h>
#include <stdlib.h>
#include "strio.h"
//added on 9/16/98 by adb to add memory tracking for this module
#include "u_mem.h"
//end additions - adb

char* fsplitword(FILE *f, char splitchar)
{
 int x,y,mem,memx;
 char *word,*buf;
  memx=1;
  mem=memx*256;
  word=(char *) malloc(sizeof(char) * mem);
  x=0;
  word[x]=fgetc(f);
   while(word[x]!=splitchar && !feof(f))
    {
     x++;
      if(x==mem)
       {
	buf=word;
	memx*=2;
	mem=memx*256;
	word=(char *) malloc(sizeof(char) * mem);
	 for(y=0;y<x;y++)
	  word[y]=buf[y];
	free(buf);
       }
     word[x]=fgetc(f);
    }
  word[x]=0;
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
  word = (char *) malloc(sizeof(char) * (l2+1));
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
