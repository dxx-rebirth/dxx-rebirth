/* $ Id: $ */
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

int file_write_byte(FILE *fp, byte b)
{
	return (fwrite(&b, 1, 1, fp));
}

int file_write_short(FILE *fp, short s)
{
	s = INTEL_SHORT(s);
	return (fwrite(&s, 2, 1, fp));
}

int file_write_int(FILE *fp, int i)
{
	i = INTEL_INT(i);
	return (fwrite(&i, 4, 1, fp));
}

int write_fix_swap(FILE *fp, fix f)
{
	f = (fix)INTEL_INT((int)f);
	return (fwrite(&f, 4, 1, fp));
}
