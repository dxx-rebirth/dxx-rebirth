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

#pragma off (unreferenced)
static char rcsid[] = "$Id: mono.c,v 1.1.1.1 2001-01-19 03:30:14 bradleyb Exp $";
#pragma on (unreferenced)

// Library functions for printing to mono card.

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <dos.h>
#include <conio.h>

#include "key.h"

void mono_int_3();
#pragma aux mono_int_3 = "int 3";

#define MAX_NUM_WINDOWS 2

struct mono_element {
	unsigned char character;
	unsigned char attribute;
};

typedef struct  {
	short   first_row;
	short   height;
	short   first_col;
	short   width;
	short   cursor_row;
	short   cursor_col;
	short   open;
	struct  mono_element save_buf[25][80];
	struct  mono_element text[25][80];
} WINDOW;


void scroll( short n );
void drawbox( short n );

#define ROW             Window[n].first_row
#define HEIGHT          Window[n].height
#define COL             Window[n].first_col
#define WIDTH           Window[n].width
#define CROW            Window[n].cursor_row
#define CCOL            Window[n].cursor_col
#define OPEN            Window[n].open
#define CHAR(r,c)       (*monoscreen)[ROW+(r)][COL+(c)].character
#define ATTR(r,c)       (*monoscreen)[ROW+(r)][COL+(c)].attribute
#define XCHAR(r,c)      Window[n].text[ROW+(r)][COL+(c)].character
#define XATTR(r,c)      Window[n].text[ROW+(r)][COL+(c)].attribute

static WINDOW Window[MAX_NUM_WINDOWS];

struct mono_element (*monoscreen)[25][80];

void mputc( short n, char c )
{
	if (!OPEN) return;

//	if (keyd_pressed[KEY_BACKSP]) 
//		mono_int_3();

	switch (c)
	{
	case 8:
		if (CCOL > 0) CCOL--;
		break;
	case 9:
		CHAR( CROW, CCOL ) = ' ';
		ATTR( CROW, CCOL ) = XATTR( CROW, CCOL );
		XCHAR( CROW, CCOL ) = ' ';
		CCOL++;
		while (CCOL % 4) {
			CHAR( CROW, CCOL ) = ' ';
			ATTR( CROW, CCOL ) = XATTR( CROW, CCOL );
			XCHAR( CROW, CCOL ) = ' ';
			CCOL++;
		}
		break;
	case 10:
	case 13:
		CCOL = 0;
		CROW++;
		break;
	default:
		CHAR( CROW, CCOL ) = c;
		ATTR( CROW, CCOL ) = XATTR( CROW, CCOL );
		XCHAR( CROW, CCOL ) = c;
		CCOL++;
	}

	if ( CCOL >= WIDTH )    {
		CCOL = 0;
		CROW++;
	}
	if ( CROW >= HEIGHT )   {
		CROW--;
		scroll(n);
	}

	msetcursor( ROW+CROW, COL+CCOL );

}

void mputc_at( short n, short row, short col, char c )
{
	CROW = row;
	CCOL = col;

	if (!OPEN) return;

	mputc( n, c );

}


void copy_row(int nwords,short *src, short *dest1, short *dest2 );
#pragma aux copy_row parm [ecx] [esi] [ebx] [edx] modify exact [eax ebx ecx edx esi] = \
"				shr		ecx, 1"	 			\
"				jnc		even_num"			\
"				mov		ax, [esi]"			\
"				add		esi, 2"				\
"				mov		[ebx], ax"			\
"				add		ebx, 2"				\
"				mov		[edx], ax"			\
"				add		edx, 2"				\
"even_num:	cmp		ecx, 0"				\
"				je			done"					\
"rowloop:	mov		eax, [esi]"			\
"				add		esi, 4"				\
"				mov		[edx], eax"			\
"				add		edx, 4"				\
"				mov		[ebx], eax"			\
"				add		ebx, 4"				\
"				loop		rowloop"				\
"done:	"


void scroll( short n )
{
	register row, col;

	if (!OPEN) return;

	col = 0;
	for ( row = 0; row < (HEIGHT-1); row++ )
		copy_row( WIDTH, (short *)&XCHAR(row+1,col), (short *)&CHAR(row,col), (short *)&XCHAR(row,col) );

//		for ( col = 0; col < WIDTH; col++ )
//		{
//			CHAR( row, col ) = XCHAR( row+1, col );
//			ATTR( row, col ) = XATTR( row+1, col );
//			XCHAR( row, col ) = XCHAR( row+1, col );
//			XATTR( row, col ) = XATTR( row+1, col );
//		}

	for ( col = 0; col < WIDTH; col++ )
	{
		CHAR( HEIGHT-1, col ) = ' ';
		ATTR( HEIGHT-1, col ) = XATTR( HEIGHT-1, col );
		XCHAR( HEIGHT-1, col ) = ' ';
	}

}

void msetcursor(short row, short col)
{
	int pos = row*80+col;

	outp( 0x3b4, 15 );
	outp( 0x3b5, pos & 0xFF );
	outp( 0x3b4, 14 );
	outp( 0x3b5, (pos >> 8) & 0xff );
}

static char temp_m_buffer[1000];
void mprintf( short n, char * format, ... )
{
	char *ptr=temp_m_buffer;
	va_list args;

	if (!OPEN) return;

	va_start(args, format );
	vsprintf(temp_m_buffer,format,args);
	while( *ptr )
		mputc( n, *ptr++ );

}

void mprintf_at( short n, short row, short col, char * format, ... )
{
	int r,c;
	char buffer[1000], *ptr=buffer;
	va_list args;

	if (!OPEN) return;

	r = CROW; c = CCOL;

	CROW = row;
	CCOL = col;

	va_start(args, format );
	vsprintf(buffer,format,args);
	while( *ptr )
		mputc( n, *ptr++ );


	CROW = r; CCOL = c;

	msetcursor( ROW+CROW, COL+CCOL );

}


void drawbox(short n)
{
	short row, col;

	if (!OPEN) return;

	for (row=0; row <HEIGHT; row++ )    {
		CHAR( row, -1 ) = 179;
		CHAR( row, WIDTH ) = 179;
		XCHAR( row, -1 ) = 179;
		XCHAR( row, WIDTH ) = 179;
	}

	for (col=0; col < WIDTH; col++ )  {
		CHAR( -1, col ) = 196;
		CHAR( HEIGHT, col ) = 196;
		XCHAR( -1, col ) = 196;
		XCHAR( HEIGHT, col ) = 196;
	}

	CHAR( -1,-1 ) = 218;
	CHAR( -1, WIDTH ) = 191;
	CHAR( HEIGHT, -1 ) = 192;
	CHAR( HEIGHT, WIDTH ) = 217;
	XCHAR( -1,-1 ) = 218;
	XCHAR( -1, WIDTH ) = 191;
	XCHAR( HEIGHT, -1 ) = 192;
	XCHAR( HEIGHT, WIDTH ) = 217;

}

void mclear( short n )
{
	short row, col;

	if (!OPEN) return;

	for (row=0; row<HEIGHT; row++ )
		for (col=0; col<WIDTH; col++ )  {
			CHAR(row,col) = 32;
			ATTR(row,col) = 7;
			XCHAR(row,col) = 32;
			XATTR(row,col) = 7;
		}
	CCOL = 0;
	CROW = 0;
}

void mclose(short n)
{
	short row, col;

	if (!OPEN) return;

	for (row=-1; row<HEIGHT+1; row++ )
		for (col=-1; col<WIDTH+1; col++ )  {
			CHAR(row,col) = 32;
			ATTR(row,col) = 7;
		}
	OPEN = 0;
	CCOL = 0;
	CROW = 0;

	msetcursor(0,0);

}

void mrefresh(short n)
{
	short row, col;

	if (!OPEN) return;

	for (row=-1; row<HEIGHT+1; row++ )
		for (col=-1; col<WIDTH+1; col++ )  {
			CHAR(row,col) = XCHAR(row,col);
			ATTR(row,col) = XATTR(row,col);
		}

	msetcursor( ROW+CROW, COL+CCOL );

}


int mono_present();		//return true if mono monitor in system


void mopen( short n, short row, short col, short width, short height, char * title )
{
//	if (n==0) return;

	if (! mono_present()) return;	//error! no mono card

	if (OPEN) mclose(n);

	OPEN = 1;
	ROW = row;
	COL = col;
	WIDTH = width;
	HEIGHT = height;

	for (row=-1; row<HEIGHT+1; row++ )
		for (col=-1; col<WIDTH+1; col++ )  {
			CHAR(row,col) = 32;
			ATTR(row,col) = 7;
			XCHAR(row,col) = 32;
			XATTR(row,col) = 7;
		}

	drawbox(n);
	CROW=-1; CCOL=0;
	mprintf( n, title );
	CROW=0; CCOL=0;
	msetcursor( ROW+CROW, COL+CCOL );

}

#pragma aux mono_present value [eax] modify [bx] = \
	"mov	ax,1a00h"	\
	"int	10h"			\	
	"mov	eax,-1"		\	
	"cmp	bl,1"			\
	"je	got_it"		\
	"cmp	bh,1"			\
	"je	got_it"		\
	"xor	eax,eax"		\	
"got_it:";


int minit()
{
	short n;
	static initialized=0;
	//short col, row;

	if (! mono_present()) return 0;	//error! no mono card

	if (initialized)
		return 1;

	initialized=1;

	monoscreen = (struct mono_element (*)[25][80])0xB0000;

	n=0;
	OPEN=1;
	ROW=2;
	COL=0;
	WIDTH=80;
	HEIGHT=24;
	CCOL=0;
	CROW=0;

	mclear(0);

	for (n=1; n<MAX_NUM_WINDOWS; n++ )  {
		OPEN = 0;
		ROW = 2;
		COL = 1;
		WIDTH=78;
		HEIGHT=23;
		CROW=0;
		CCOL=0;
	}

	return -1;	//everything ok
}

