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

#ifndef _MONO_H
#define _MONO_H

#include <stdarg.h>
#include "console.h"

static inline void _do_mprintf(int n, char *fmt, ...)
{
	char buffer[1024];
	va_list arglist;

	va_start (arglist, fmt);
	vsprintf (buffer, fmt, arglist);
	va_end (arglist);
	
	con_printf (CON_DEBUG, buffer);
}

#define mprintf(args) _do_mprintf args

#define minit()
#define mclose(n)
#define mopen( n, row, col, width, height, title )
#define mDumpD(window, int_var) 
#define mDumpX(window, int_var) 
#define mclear( n )
#define mprintf_at(args)
#define mputc( n, c )
#define mputc_at( n, row, col, c )
#define msetcursor( row, col )
#define mrefresh(n)

#endif


