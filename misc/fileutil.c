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
 
#include <stdio.h>
#include <stdlib.h>

#include "pstypes.h"
#include "fileutil.h"
#include "cfile.h"
#include "fix.h"
#include "byteswap.h"

int filelength(int fd)
{
	int cur_pos, end_pos;
	
	cur_pos = lseek(fd, 0, SEEK_CUR);
	lseek(fd, 0, SEEK_END);
	end_pos = lseek(fd, 0, SEEK_CUR);
	lseek(fd, cur_pos, SEEK_SET);
	return end_pos;
}
#if 0
byte read_byte(CFILE *fp)
{
	byte b;
	
	cfread(&b, sizeof(byte), 1, fp);
	return b;
}

short read_short(CFILE *fp)
{
	short s;
	
	cfread(&s, sizeof(short), 1, fp);
	return (s);
}

short read_short_swap(CFILE *fp)
{
	short s;
	
	cfread(&s, sizeof(short), 1, fp);
	return swapshort(s);
}

int read_int(CFILE *fp)
{
	uint i;
	
	cfread(&i, sizeof(uint), 1, fp);
	return i;
}

fix read_fix(CFILE *fp)
{
	fix f;
	
	cfread(&f, sizeof(fix), 1, fp);
	return f;
}

int write_byte(FILE *fp, byte b)
{
	return (fwrite(&b, sizeof(byte), 1, fp));
}

int write_short(FILE *fp, short s)
{
	return (fwrite(&s, sizeof(short), 1, fp));
}

int write_short_swap(FILE *fp, short s)
{
	s = swapshort(s);
	return (fwrite(&s, sizeof(short), 1, fp));
}

int write_int(FILE *fp, int i)
{
	return (fwrite(&i,sizeof(int), 1, fp));
}

int write_int_swap(FILE *fp, int i)
{
	i = swapint(i);
	return (fwrite(&i,sizeof(int), 1, fp));
}

int write_fix(FILE *fp, fix f)
{
	return (fwrite(&f, sizeof(fix), 1, fp));
}

int write_fix_swap(FILE *fp, fix f)
{
	f = (fix)swapint((int)f);
	return (fwrite(&f, sizeof(fix), 1, fp));
}

#endif
