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
 * $Source: /cvs/cvsroot/d2x/arch/dos/mono.h,v $
 * $Revision: 1.2 $
 * $Author: schaffner $
 * $Date: 2004-08-28 23:17:45 $
 *
 * Header for monochrome/mprintf functions
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2001/01/19 03:30:15  bradleyb
 * Import of d2x-0.0.8
 *
 * Revision 1.1.1.1  1999/06/14 21:58:40  donut
 * Import of d1x 1.37 source.
 *
 */

#ifndef _MONO_H
#define _MONO_H

#if !(defined(NDEBUG) || defined(NMONO))		//normal, functioning versions

//==========================================================================
// Open and close the mono screen.  close(0) clears it.
extern int minit();	//returns true if mono card, else false

// Use n = 0 to clear the entire screen, any other number just closes the
// specific window.
extern void mclose(int n);

//==========================================================================
// Opens a scrollable window on the monochrome screen.
extern void mopen( int n, int row, int col, int width, int height, char * title );

//==========================================================================
// Displays a integer variable and what it is equal to on window n.
// ie.. if john=5,  then mDumpInt(1,john); would print "john=5" to window 1.
#define mDumpD(window, int_var) mprintf( window, #int_var"=%d\n", int_var)
// Does the same thing only prints out in 8 hexidecimal places
#define mDumpX(window, int_var) mprintf( window, #int_var"=%08X\n", int_var)

//==========================================================================
// Clears a window
extern void mclear( int n );

//==========================================================================
// Prints a formatted string on window n
extern void _mprintf( int n, char * format, ... );

#define mprintf(args) _mprintf args

//==========================================================================
// Prints a formatted string on window n at row, col.
extern void _mprintf_at( int n, int row, int col, char * format, ... );

#define mprintf_at(args) _mprintf_at args

//==========================================================================
// Puts a char in window n at current cursor position
extern void mputc( int n, char c );

//==========================================================================
// Puts a char in window n at specified location
extern void mputc_at( int n, int row, int col, char c );

//==========================================================================
// Moves the cursor... doesn't work.
extern void msetcursor( int row, int col );

//==========================================================================
// Refreshes a window
void mrefresh(short n);

#else			//null versions for when debugging turned off

#define minit()
#define mclose(n)
#define mopen( n, row, col, width, height, title )
#define mDumpD(window, int_var) 
#define mDumpX(window, int_var) 
#define mclear( n )
#define mprintf(args) 
#define mprintf_at(args)
#define mputc( n, c )
#define mputc_at( n, row, col, c )
#define msetcursor( row, col )
#define mrefresh(n)

#endif
#endif


