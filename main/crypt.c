/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef RCS
char crypt_rcsid[] = "$Id: crypt.c,v 1.1.1.1 2001-01-19 03:30:01 bradleyb Exp $";
#endif

#include <conf.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "inferno.h"

char *jcrypt (char *plainstring)
 {
  int i,t,len;
  static char cryptstring[20];
  
  len=strlen (plainstring); 
  if (len>8)
	len=8;
   
  for (i=0;i<len;i++)
	{
    cryptstring[i]=0; 
  
	  for (t=0;t<8;t++)
		{
		 cryptstring[i]^=(plainstring[t] ^ plainstring[i%(t+1)]);
		 cryptstring[i]%=90;
		 cryptstring[i]+=33;
		}
	}
  cryptstring[i]=0;
  return ((char *)cryptstring);
 }
		 
	
  
   
 

 
  

