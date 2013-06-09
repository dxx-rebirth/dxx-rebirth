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
 */


#include <stdlib.h>
#include <string.h>

#include "physfsx.h"
#include "pstypes.h"
#include "u_mem.h"
#include "dxxerror.h"
#include "inferno.h"
#include "text.h"
#include "args.h"

#define SHAREWARE_TEXTSIZE  14677

char *text;

char *Text_string[N_TEXT_STRINGS];

void free_text()
{
	d_free(Text_string[350]);
	d_free(text);
}

// rotates a byte left one bit, preserving the bit falling off the right
static void encode_rotate_left(char *c)
{
	unsigned char v = *c;
	*c = (v >> 7) | (v << 1);
}

#define BITMAP_TBL_XOR 0xD3

//decode an encoded line of text of bitmaps.tbl
void decode_text_line(char *p)
{
	for (;*p;p++) {
		encode_rotate_left(p);
		*p = *p ^ BITMAP_TBL_XOR;
		encode_rotate_left(p);
	}
}

// decode buffer of text, preserves newlines
void decode_text(char *buf, int len)
{
	char *ptr;
	int i;

	for (i = 0, ptr = buf; i < len; i++, ptr++)
	{
		if (*ptr != '\n')
		{
			encode_rotate_left(ptr);
			*ptr = *ptr  ^ BITMAP_TBL_XOR;
			encode_rotate_left(ptr);
		}
	}
}

//load all the text strings for Descent
void load_text()
{
	PHYSFS_file  *tfile;
	PHYSFS_file *ifile;
	int len,i, have_binary = 0;
	char *tptr;
	char *filename="descent.tex";

	if (GameArg.DbgAltTex)
		filename = GameArg.DbgAltTex;

	if ((tfile = PHYSFSX_openReadBuffered(filename)) == NULL) {
		filename="descent.txb";
		if ((ifile = PHYSFSX_openReadBuffered(filename)) == NULL) {
			Error("Cannot open file DESCENT.TEX or DESCENT.TXB");
			return;
		}
		have_binary = 1;

		len = PHYSFS_fileLength(ifile);

//edited 05/17/99 Matt Mueller - malloc an extra byte, and null terminate.
		MALLOC(text,char,len+1);

		PHYSFS_read(ifile,text,1,len);
		text[len]=0;
//end edit -MM
		PHYSFS_close(ifile);

	} else {
		int c;
		char * p;

		len = PHYSFS_fileLength(tfile);

//edited 05/17/99 Matt Mueller - malloc an extra byte, and null terminate.
		MALLOC(text,char,len+1);

		//fread(text,1,len,tfile);
		p = text;
		do {
			c = PHYSFSX_fgetc( tfile );
			if ( c != 13 )
				*p++ = c;
		} while ( c!=EOF );
		*p=0;
//end edit -MM

		PHYSFS_close(tfile);
	}

	for (i=0,tptr=text;i<N_TEXT_STRINGS;i++) {
		char *p;
		char *buf;

		Text_string[i] = tptr;

		tptr = strchr(tptr,'\n');

		if (!tptr)
		{
			if (i == 644) break;    /* older datafiles */

			Error("Not enough strings in text file - expecting %d, found %d\n", N_TEXT_STRINGS, i);
		}

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
// 			strcpy(p+1,p+2);
			MALLOC(buf,char,len+1);
			strcpy(buf,p+2);
			strcpy(p+1,buf);
			d_free(buf);
			p++;
		}

          switch(i) {
				  char *extra;
				  char *str;
				  
			  case 350:
				  extra = "\n<Ctrl-C> converts format\nIntel <-> PowerPC";
				  str = d_malloc(strlen(Text_string[i]) + strlen(extra) + 1);
				  if (!str)
					  break;
				  strcpy(str, Text_string[i]);
				  strcat(str, extra);
				  Text_string[i] = str;
				  break;
          }

	}


	if (i == 644)
	{
		if (len == SHAREWARE_TEXTSIZE)
		{
			Text_string[173] = Text_string[172];
			Text_string[172] = Text_string[171];
			Text_string[171] = Text_string[170];
			Text_string[170] = Text_string[169];
			Text_string[169] = "Windows Joystick";
		}

		Text_string[644] = "Z1";
		Text_string[645] = "UN";
		Text_string[646] = "P1";
		Text_string[647] = "R1";
		Text_string[648] = "Y1";
	}

	//Assert(tptr==text+len || tptr==text+len-2);

}


