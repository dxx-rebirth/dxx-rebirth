/* $Id: args.c,v 1.10 2003-11-26 12:26:33 btb Exp $ */
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
 *
 * Functions for accessing arguments.
 *
 * Old Log:
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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: args.c,v 1.10 2003-11-26 12:26:33 btb Exp $";
#endif

#include <stdlib.h>
#include <string.h>

#include "cfile.h"
#include "u_mem.h"
#include "strio.h"
#include "strutil.h"

int Num_args=0;
char * Args[100];

int FindArg( char * s )	{
	int i;

	for (i=0; i<Num_args; i++ )
		if (! stricmp( Args[i], s))
			return i;

	return 0;
}

void args_exit(void)
{
	int i;
	for (i=0; i< Num_args; i++ )
		d_free(Args[i]);
}

void InitArgs( int argc,char **argv )
{
	int i;
	CFILE *f;
	char *line,*word;
	
	Num_args=0;
	
	for (i=0; i<argc; i++ )
		Args[Num_args++] = d_strdup( argv[i] );
        
	
	for (i=0; i< Num_args; i++ ) {
		if ( Args[i][0] == '-' )
			strlwr( Args[i]  );  // Convert all args to lowercase
	}
	if((i=FindArg("-ini")))
		f = cfopen(Args[i+1], "rt");
	else
		f = cfopen("d2x.ini", "rt");
	
	if(f) {
		while(!cfeof(f))
		{
			line=fsplitword(f,'\n');
			word=splitword(line,' ');
			
			Args[Num_args++] = d_strdup(word);
			
			if(line)
				Args[Num_args++] = d_strdup(line);
			
			d_free(line); d_free(word);
		}
		cfclose(f);
	}
	
	atexit(args_exit);
}
