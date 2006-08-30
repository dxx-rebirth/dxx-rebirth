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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/args.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:42:49 $
 * 
 * Functions for accessing arguments.
 * 
 * $Log: args.c,v $
 * Revision 1.1.1.1  2006/03/17 19:42:49  zicodxx
 * initial import
 *
 * Revision 1.3  1999/08/05 22:53:41  sekmu
 *
 * D3D patch(es) from ADB
 *
 * Revision 1.2  1999/06/14 23:44:11  donut
 * Orulz' svgalib/ggi/noerror patches.
 *
 * Revision 1.1.1.1  1999/06/14 22:05:15  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:31:22  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.9  1994/11/29  01:07:57  john
 * Took out some unused vars.
 * 
 * Revision 1.8  1994/11/29  01:04:30  john
 * Took out descent.ini stuff.
 * 
 * Revision 1.7  1994/09/20  19:29:15  matt
 * Made args require exact (not substring), though still case insensitive.
 * 
 * Revision 1.6  1994/07/25  12:33:11  john
 * Network "pinging" in.
 * 
 * Revision 1.5  1994/06/17  18:07:50  matt
 * Took out printf
 * 
 * Revision 1.4  1994/05/11  19:45:33  john
 * *** empty log message ***
 * 
 * Revision 1.3  1994/05/11  18:42:11  john
 * Added Descent.ini config file.
 * 
 * Revision 1.2  1994/05/09  17:03:30  john
 * Split command line parameters into arg.c and arg.h.
 * Also added /dma, /port, /irq to digi.c
 * 
 * Revision 1.1  1994/05/09  16:49:11  john
 * Initial revision
 * 
 * 
 */


#ifdef RCS
static char rcsid[] = "$Id: args.c,v 1.1.1.1 2006/03/17 19:42:49 zicodxx Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "u_mem.h"
#include "strio.h"
//added 6/15/99 - Owen Evans
#include "strutil.h"
//end added - OE

int Num_args=0;
char * Args[200];

int FindArg( char * s )	{
	int i;

	for (i=0; i<Num_args; i++ )
		if (! strcasecmp( Args[i], s))
			return i;

	return 0;
}

int FindResArg(char *prefix, int *sw, int *sh)
{
	int i;
	int w, h;
	char *endptr;
	int prefixlen = strlen(prefix);

	for (i = 0; i < Num_args; ++i)
		if (Args[i][0] == '-' && !strnicmp(Args[i] + 1, prefix, prefixlen))
		{
			w = strtol(Args[i] + 1 + prefixlen, &endptr, 10);
			if (w > 0 && endptr && endptr[0] == 'x')
			{
				h = strtol(endptr + 1, &endptr, 10);
				if (h > 0 && endptr[0] == '\0')
				{
					*sw = w;
					*sh = h;
					return i;
				}
			}
		}

	return 0;

}

//added 7/11/99 by adb to free arguments (prevent memleak msg)
void FreeArgs(void)
{
   int i;
   for (i=0; i< Num_args; i++ )
    free(Args[i]);
}
//end additions - adb

void InitArgs( int argc,char **argv )
{
 int i;
 FILE *f;
 char *line,*word;

  Num_args=0;

   for (i=0; i<argc; i++ )
    Args[Num_args++] = strdup( argv[i] );
        

   for (i=0; i< Num_args; i++ )
    {
//killed 02/06/99 Matthew Mueller - interferes with filename args which might start with /
//     if ( Args[i][0] == '/' )  
//      Args[i][0] = '-';
//end kill -MM
     if ( Args[i][0] == '-' )
      strlwr( Args[i]  );             // Convert all args to lowercase
    }
  if((i=FindArg("-ini")))
   f=fopen(Args[i+1],"rt");
  else
   f=fopen("d1x.ini","rt");

     if(f)
      {
       while(!feof(f))
        {
         line=fsplitword(f,'\n');
         word=splitword(line,' ');

         Args[Num_args++] = strdup(word);

          if(line)
           Args[Num_args++] = strdup(line);

         free(line); free(word);
        }
       fclose(f);
      }
   
	atexit(FreeArgs);
}
