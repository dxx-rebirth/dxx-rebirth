/* $Id: text.c,v 1.9 2003-06-16 06:57:34 btb Exp $ */
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

/*
 *
 * Code for localizable text
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:31:44  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:33:09  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.11  1994/12/14  12:53:23  matt
 * Improved error handling
 *
 * Revision 1.10  1994/12/09  18:36:44  john
 * Added code to make text read from hogfile.
 *
 * Revision 1.9  1994/12/08  20:56:34  john
 * More cfile stuff.
 *
 * Revision 1.8  1994/12/08  17:20:06  yuan
 * Cfiling stuff.
 *
 * Revision 1.7  1994/12/05  15:10:36  allender
 * support encoded descent.tex file (descent.txb)
 *
 * Revision 1.6  1994/12/01  14:18:34  matt
 * Now support backslash chars in descent.tex file
 *
 * Revision 1.5  1994/10/27  00:13:10  john
 * Took out cfile.
 *
 * Revision 1.4  1994/07/11  15:33:49  matt
 * Put in command-line switch to load different text files
 *
 * Revision 1.3  1994/07/10  09:56:25  yuan
 * #include <stdio.h> added for FILE type.
 *
 * Revision 1.2  1994/07/09  22:48:14  matt
 * Added localizable text
 *
 * Revision 1.1  1994/07/09  21:30:46  matt
 * Initial revision
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: text.c,v 1.9 2003-06-16 06:57:34 btb Exp $";
#endif

#include <stdlib.h>
#include <string.h>

#include "pstypes.h"
#include "cfile.h"
#include "u_mem.h"
#include "error.h"

#include "inferno.h"
#include "text.h"
#include "args.h"
#include "compbit.h"

char *text;

char *Text_string[N_TEXT_STRINGS];

void free_text()
{
	d_free(text);
}

// rotates a byte left one bit, preserving the bit falling off the right
void
encode_rotate_left(char *c)
{
	int found;

	found = 0;
	if (*c & 0x80)
		found = 1;
	*c = *c << 1;
	if (found)
		*c |= 0x01;
}

//decode and encoded line of text
void decode_text_line(char *p)
{
	for (;*p;p++) {
		encode_rotate_left(p);
		*p = *p ^ BITMAP_TBL_XOR;
		encode_rotate_left(p);
	}
}

#include <unistd.h>
//load all the text strings for Descent
void load_text()
{
	CFILE  *tfile;
	CFILE *ifile;
	int len,i, have_binary = 0;
	char *tptr;
	char *filename="descent.tex";

	if ((i=FindArg("-text"))!=0)
		filename = Args[i+1];

	if ((tfile = cfopen(filename,"rb")) == NULL) {
		filename="descent.txb";
		if ((ifile = cfopen(filename, "rb")) == NULL) {
			Warning("Cannot open file DESCENT.TEX or DESCENT.TXB");
			return;
		}
		have_binary = 1;

		len = cfilelength(ifile);

		MALLOC(text,char,len);

		atexit(free_text);

		cfread(text,1,len,ifile);

		cfclose(ifile);

	} else {
		int c;
		char * p;

		len = cfilelength(tfile);

		MALLOC(text,char,len);

		atexit(free_text);

		//fread(text,1,len,tfile);
		p = text;
		do {
			c = cfgetc( tfile );
			if ( c != 13 )
				*p++ = c;
		} while ( c!=EOF );

		cfclose(tfile);
	}

	for (i=0,tptr=text;i<N_TEXT_STRINGS;i++) {
		char *p;

		Text_string[i] = tptr;

		tptr = strchr(tptr,'\n');

#ifdef MACINTOSH    // total hack for mac patch since they don't want to patch data.
		if (!tptr && (i == 644) )
			break;
#else
		if (!tptr)
		{
			Warning("Not enough strings in text file - expecting %d, found %d\nThis probably means you have the wrong version of the descent 2 datafiles. You need version 1.2\n",N_TEXT_STRINGS,i);
			if (i == 644) break;
		}
#endif

		if ( tptr ) *tptr++ = 0;

		if (have_binary)
			decode_text_line(Text_string[i]);

		//scan for special chars (like \n)
		for (p=Text_string[i];(p=strchr(p,'\\'));) {
			char newchar;

			if (p[1] == 'n') newchar = '\n';
			else if (p[1] == 't') newchar = '\t';
			else if (p[1] == '\\') newchar = '\\';
			else
				Error("Unsupported key sequence <\\%c> on line %d of file <%s>",p[1],i+1,filename);

			p[0] = newchar;
			strcpy(p+1,p+2);
			p++;
		}

	}

	//Assert(tptr==text+len || tptr==text+len-2);

}


