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
 * Code for localizable text
 *
 */


#ifdef RCS
static char rcsid[] = "$Id: text.c,v 1.1.1.1 2006/03/17 19:43:44 zicodxx Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "cfile.h"
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
        //added on 9/13/98 by adb to free all text
        free(Text_string[145]);
        //end addition - adb
        free(text);
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

//load all the text strings for Descent
void load_text()
{
	CFILE  *tfile;
	CFILE *ifile;
	int len,i, have_binary = 0;
	char *tptr;
	char *filename="descent.tex";

	if (GameArg.DbgAltTex)
		filename = GameArg.DbgAltTex;

	if ((tfile = cfopen(filename,"rb")) == NULL) {
		filename="descent.txb";
		if ((ifile = cfopen(filename, "rb")) == NULL)
			Error("Cannot open file DESCENT.TEX or DESCENT.TXB");
		have_binary = 1;

		len = cfilelength(ifile);

//edited 05/17/99 Matt Mueller - malloc an extra byte, and null terminate.
		MALLOC(text,char,len+1);
		atexit((void (*)())free_text);

		cfread(text,1,len,ifile);
		text[len]=0;
//end edit -MM
		cfclose(ifile);

	} else {
		int c;
		char * p;

		len = cfilelength(tfile);

//edited 05/17/99 Matt Mueller - malloc an extra byte, and null terminate.
		MALLOC(text,char,len+1);

		atexit((void (*)())free_text);

		//fread(text,1,len,tfile);
		p = text;
		do {
			c = cfgetc( tfile );
			if ( c != 13 )
				*p++ = c;
		} while ( c!=EOF );
		*p=0;
//end edit -MM

		cfclose(tfile);
	}

	for (i=0,tptr=text;i<N_TEXT_STRINGS;i++) {
		char *p;
		char *buf;

		Text_string[i] = tptr;

		tptr = strchr(tptr,'\n');

		if (!tptr)
			Error("Not enough strings in text file - expecting %d, found %d",N_TEXT_STRINGS,i);

		if ( tptr ) *tptr++ = 0;

		if (have_binary) {
			for (p=Text_string[i]; p < tptr - 1; p++) {
				encode_rotate_left(p);
				*p = *p ^ BITMAP_TBL_XOR;
				encode_rotate_left(p);
			}
		}

		//scan for special chars (like \n)
		for (p=Text_string[i];(p=strchr(p,'\\'));) {
			char newchar=0;//get rid of compiler warning

			if (p[1] == 'n') newchar = '\n';
			else if (p[1] == 't') newchar = '\t';
			else if (p[1] == '\\') newchar = '\\';
			else
				Error("Unsupported key sequence <\\%c> on line %d of file <%s>",p[1],i+1,filename); 

			p[0] = newchar;
// 			strcpy(p+1,p+2);
			MALLOC(buf,char,len+1);
			strcpy(buf,p+2);
			strcpy(p+1,buf);
			p++;
			free(buf);
		}

          switch(i) {
          case 145:
                  Text_string[i]=(char *) malloc(sizeof(char) * 48);
                  strcpy(Text_string[i],"Sidewinder &\nThrustmaster FCS &\nWingman Extreme");
          }

	}

//	Assert(tptr==text+len || tptr==text+len-2);
         
}


