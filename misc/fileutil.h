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
 
#ifndef _FILEUTIL_
#define _FILEUTIL_

#include "pstypes.h" 
#include "cfile.h"
#include "fix.h"
 
extern int filelength(int fd);

// routines which read basic data types
extern byte read_byte(CFILE *fp);
extern short read_short(CFILE *fp);
extern int read_int(CFILE *fp);
extern fix read_fix(CFILE *fp);

// versions which swap bytes
#define read_byte_swap(fp) read_byte(fp)
extern short read_short_swap(CFILE *fp);

// routines which write basic data types
extern int write_byte(FILE *fp, byte b);
extern int write_short(FILE *fp, short s);
extern int write_int(FILE *fp, int i);
extern int write_fix(FILE *fp, fix f);

// routines which write swapped bytes
#define write_byte_swap(fp, b) write_byte(fp, b)
extern int write_short_swap(FILE *fp, short s);
extern int write_int_swap(FILE *fp, int i);
extern int write_fix_swap(FILE *fp, fix f);

#endif
