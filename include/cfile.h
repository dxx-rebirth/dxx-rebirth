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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/include/cfile.h,v $
 * $Revision: 1.2 $
 * $Author: michaelstather $
 * $Date: 2006/03/18 23:07:36 $
 * 
 * Prototypes for compressed file functions...
 * 
 * $Log: cfile.h,v $
 * Revision 1.2  2006/03/18 23:07:36  michaelstather
 * New build system by KyroMaster
 *
 * Revision 1.1.1.1  2006/03/17 19:46:39  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:02:08  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.10  1995/03/13  15:16:47  john
 * Added alternate directory stuff.
 * 
 * Revision 1.9  1995/02/01  20:56:40  john
 * Added cfexist function
 * 
 * Revision 1.8  1995/01/21  17:53:41  john
 * Added alternate pig file thing.
 * 
 * Revision 1.7  1994/12/12  13:19:47  john
 * Made cfile work with fiellentth.
 * 
 * Revision 1.6  1994/12/08  19:02:52  john
 * Added cfgets.
 * 
 * Revision 1.5  1994/12/07  21:34:07  john
 * Stripped out compression stuff...
 * 
 * Revision 1.4  1994/07/13  00:16:53  matt
 * Added include
 * 
 * Revision 1.3  1994/02/17  17:36:19  john
 * Added CF_READ_MODE and CF_WRITE_MODE constants.
 * 
 * Revision 1.2  1994/02/15  12:52:08  john
 * Crappy inbetween version
 * 
 * Revision 1.1  1994/02/15  10:54:23  john
 * Initial revision
 * 
 * Revision 1.1  1994/02/10  15:50:54  john
 * Initial revision
 * 
 * 
 */



#ifndef _CFILE_H
#define _CFILE_H

#include <stdio.h>

typedef struct CFILE {
	FILE 				*file;
	int				size;
	int				lib_offset;
	int				raw_position;
} CFILE;

CFILE * cfopen(char * filename, char * mode);
int cfilelength( CFILE *fp );							// Returns actual size of file...
size_t cfread( void * buf, size_t elsize, size_t nelem, CFILE * fp );
void cfclose( CFILE * cfile );
int cfgetc( CFILE * fp );
int cfseek( CFILE *fp, long int offset, int where );
int cftell( CFILE * fp );
char * cfgets( char * buf, size_t n, CFILE * fp );

int cfexist( char * filename );	// Returns true if file exists on disk (1) or in hog (2).

// Allows files to be gotten from an alternate hog file.
// Passing NULL disables this.
void cfile_use_alternate_hogfile( char * name );

// All cfile functions will check this directory if no file exists
// in the current directory.
void cfile_use_alternate_hogdir( char * path );

extern char AltHogDir[];
extern char AltHogdir_initialized;

#endif
