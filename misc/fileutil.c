/* $Id: fileutil.c,v 1.7 2003-04-12 00:11:46 btb Exp $ */
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
 * utilities for file manipulation
 *
 * Old Log:
 * Revision 1.6  1995/10/30  11:09:51  allender
 * use FILE, not CFILE on the write* routines
 *
 * Revision 1.5  1995/05/11  13:00:34  allender
 * added write functions which swap bytes
 *
 * Revision 1.4  1995/05/04  20:10:38  allender
 * remove include for fcntl
 *
 * Revision 1.3  1995/04/26  10:14:39  allender
 * added byteswap header file
 *
 * Revision 1.2  1995/04/26  10:13:21  allender
 *
 * Revision 1.1  1995/03/30  15:02:34  allender
 * Initial revision
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>

#include "fileutil.h"
#include "fix.h"
#include "byteswap.h"
#include "error.h"

byte file_read_byte(FILE *fp)
{
	byte b;

	if (fread(&b, 1, 1, fp) != 1)
		Error("Error reading byte in file_read_byte()");
	return b;
}

short file_read_short(FILE *fp)
{
	short s;

	if (fread(&s, 2, 1, fp) != 1)
		Error("Error reading short in file_read_short()");
	return INTEL_SHORT(s);
}

int file_read_int(FILE *fp)
{
	uint i;

	if (fread(&i, 4, 1, fp) != 1)
		Error("Error reading int in file_read_int()");
	return INTEL_INT(i);
}

fix file_read_fix(FILE *fp)
{
	fix f;

	if (fread(&f, 4, 1, fp) != 1)
		Error("Error reading fix in file_read_fix()");
	return INTEL_INT(f);
}

void file_read_string(char *s, FILE *f)
{
	if (feof(f))
		*s = 0;
	else
		do
			*s = fgetc(f);
		while (!feof(f) && *s++!=0);
}

int file_write_byte(byte b, FILE *fp)
{
	return (fwrite(&b, 1, 1, fp));
}

int file_write_short(short s, FILE *fp)
{
	s = INTEL_SHORT(s);
	return (fwrite(&s, 2, 1, fp));
}

int file_write_int(int i, FILE *fp)
{
	i = INTEL_INT(i);
	return (fwrite(&i, 4, 1, fp));
}

int file_write_fix(fix f, FILE *fp)
{
	f = (fix)INTEL_INT((int)f);
	return (fwrite(&f, 4, 1, fp));
}

void file_write_string(char *s, FILE *f)
{
	do
		fputc(*s,f);
	while (*s++!=0);
}
