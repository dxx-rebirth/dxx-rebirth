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

#include <stdio.h>

#include "fix.h"

// routines which read basic data types
extern byte file_read_byte(FILE *fp);
extern short file_read_short(FILE *fp);
extern int file_read_int(FILE *fp);
extern fix file_read_fix(FILE *fp);
extern void file_read_string(char *s, FILE *f);

// routines which write basic data types
extern int file_write_byte(byte b, FILE *fp);
extern int file_write_short(short s, FILE *fp);
extern int file_write_int(int i, FILE *fp);
extern int file_write_fix(fix f, FILE *fp);
extern void file_write_string(char *s, FILE *f);

#endif
