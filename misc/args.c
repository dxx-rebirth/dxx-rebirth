/* $Id: args.c,v 1.15 2004-12-01 12:48:13 btb Exp $ */
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
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: args.c,v 1.15 2004-12-01 12:48:13 btb Exp $";
#endif

#include <stdlib.h>
#include <string.h>

#include <physfs.h>

#include "args.h"
#include "u_mem.h"
#include "strio.h"
#include "strutil.h"

#define MAX_ARGS 100

int Num_args=0;
char * Args[MAX_ARGS];

int FindArg(char *s)
{
	int i;

	for (i=0; i<Num_args; i++ )
		if (! stricmp( Args[i], s))
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

void args_exit(void)
{
	int i;
	for (i=0; i< Num_args; i++ )
		d_free(Args[i]);
}

void InitArgs( int argc,char **argv )
{
	int i;
	
	Num_args=0;
	
	for (i=0; i<argc; i++ )
		Args[Num_args++] = d_strdup( argv[i] );
        
	
	for (i=0; i< Num_args; i++ ) {
		if ( Args[i][0] == '-' )
			strlwr( Args[i]  );  // Convert all args to lowercase
	}
	AppendArgs();

	atexit(args_exit);
}

void AppendArgs(void)
{
	PHYSFS_file *f;
	char *line,*word;
	int i;
	
	if((i=FindArg("-ini")))
		f = PHYSFS_openRead(Args[i+1]);
	else
		f = PHYSFS_openRead("d2x.ini");
	
	if(f) {
		while(!PHYSFS_eof(f) && Num_args < MAX_ARGS)
		{
			line=fgets_unlimited(f);
			word=splitword(line,' ');
			
			Args[Num_args++] = d_strdup(word);
			
			if(line)
				Args[Num_args++] = d_strdup(line);
			
			d_free(line); d_free(word);
		}
		PHYSFS_close(f);
	}
}
