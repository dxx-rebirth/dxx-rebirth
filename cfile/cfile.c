/* $Id: cfile.c,v 1.18 2003-10-04 02:58:23 btb Exp $ */
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
 * Functions for accessing compressed files.
 *
 * Old Log:
 * Revision 1.7  1995/10/27  15:18:20  allender
 * get back to descent directory before trying to read a hog file
 *
 * Revision 1.6  1995/10/21  23:48:24  allender
 * hogfile(s) are now in :Data: folder
 *
 * Revision 1.5  1995/08/14  09:27:31  allender
 * added byteswap header
 *
 * Revision 1.4  1995/05/12  11:54:33  allender
 * changed memory stuff again
 *
 * Revision 1.3  1995/05/04  20:03:38  allender
 * added code that was missing...use NewPtr instead of malloc
 *
 * Revision 1.2  1995/04/03  09:59:49  allender
 * *** empty log message ***
 *
 * Revision 1.1  1995/03/30  10:25:02  allender
 * Initial revision
 *
 *
 * --- PC RCS Information ---
 * Revision 1.24  1995/03/15  14:20:27  john
 * Added critical error checker.
 *
 * Revision 1.23  1995/03/13  15:16:53  john
 * Added alternate directory stuff.
 *
 * Revision 1.22  1995/02/09  23:08:47  matt
 * Increased the max number of files in hogfile to 250
 *
 * Revision 1.21  1995/02/01  20:56:47  john
 * Added cfexist function
 *
 * Revision 1.20  1995/01/21  17:53:48  john
 * Added alternate pig file thing.
 *
 * Revision 1.19  1994/12/29  15:10:02  john
 * Increased hogfile max files to 200.
 *
 * Revision 1.18  1994/12/12  13:20:57  john
 * Made cfile work with fiellentth.
 *
 * Revision 1.17  1994/12/12  13:14:25  john
 * Made cfiles prefer non-hog files.
 *
 * Revision 1.16  1994/12/09  18:53:26  john
 * *** empty log message ***
 *
 * Revision 1.15  1994/12/09  18:52:56  john
 * Took out mem, error checking.
 *
 * Revision 1.14  1994/12/09  18:10:31  john
 * Speed up cfgets, which was slowing down the reading of
 * bitmaps.tbl, which was making POF loading look slow.
 *
 * Revision 1.13  1994/12/09  17:53:51  john
 * Added error checking to number of hogfiles..
 *
 * Revision 1.12  1994/12/08  19:02:55  john
 * Added cfgets.
 *
 * Revision 1.11  1994/12/07  21:57:48  john
 * Took out data dir.
 *
 * Revision 1.10  1994/12/07  21:38:02  john
 * Made cfile not return error..
 *
 * Revision 1.9  1994/12/07  21:35:34  john
 * Made it read from data directory.
 *
 * Revision 1.8  1994/12/07  21:33:55  john
 * Stripped out compression stuff...
 *
 * Revision 1.7  1994/04/13  23:44:59  matt
 * When file cannot be opened, free up the buffer for that file.
 *
 * Revision 1.6  1994/02/18  12:38:20  john
 * Optimized a bit
 *
 * Revision 1.5  1994/02/15  18:13:20  john
 * Fixed more bugs.
 *
 * Revision 1.4  1994/02/15  13:27:58  john
 * Works ok...
 *
 * Revision 1.3  1994/02/15  12:51:57  john
 * Crappy inbetween version
 *
 * Revision 1.2  1994/02/14  20:12:29  john
 * First version working with new cfile stuff.
 *
 * Revision 1.1  1994/02/14  15:51:33  john
 * Initial revision
 *
 * Revision 1.1  1994/02/10  15:45:12  john
 * Initial revision
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "pstypes.h"
#include "u_mem.h"
#include "strutil.h"
#include "d_io.h"
#include "error.h"
#include "cfile.h"
#include "byteswap.h"

struct CFILE {
	FILE    *file;
	int     size;
	int     lib_offset;
	int     raw_position;
};

typedef struct hogfile {
	char    name[13];
	int     offset;
	int     length;
} hogfile;

#define MAX_HOGFILES 300

hogfile HogFiles[MAX_HOGFILES];
char Hogfile_initialized = 0;
int Num_hogfiles = 0;
char HogFilename[64];

hogfile D1HogFiles[MAX_HOGFILES];
char D1Hogfile_initialized = 0;
int D1Num_hogfiles = 0;
char D1HogFilename[64];

hogfile AltHogFiles[MAX_HOGFILES];
char AltHogfile_initialized = 0;
int AltNum_hogfiles = 0;
char AltHogFilename[64];

char AltHogDir[64];
char AltHogdir_initialized = 0;

// routine to take a DOS path and turn it into a macintosh
// pathname.  This routine is based on the fact that we should
// see a \ character in the dos path.  The sequence .\ a tthe
// beginning of a path is turned into a :

#ifdef MACINTOSH
void macify_dospath(char *dos_path, char *mac_path)
{
	char *p;

	if (!strncmp(dos_path, ".\\", 2)) {
		strcpy(mac_path, ":");
		strcat(mac_path, &(dos_path[2]) );
	} else
		strcpy(mac_path, dos_path);

	while ( (p = strchr(mac_path, '\\')) != NULL)
		*p = ':';

}
#endif

void cfile_use_alternate_hogdir( char * path )
{
	if ( path )	{
		strcpy( AltHogDir, path );
		AltHogdir_initialized = 1;
	} else {
		AltHogdir_initialized = 0;
	}
}

//in case no one installs one
int default_error_counter=0;

//ptr to counter of how many critical errors
int *critical_error_counter_ptr=&default_error_counter;

//tell cfile about your critical error counter
void cfile_set_critical_error_counter_ptr(int *ptr)
{
	critical_error_counter_ptr = ptr;

}


FILE * cfile_get_filehandle( char * filename, char * mode )
{
	FILE * fp;
	char temp[128];

	*critical_error_counter_ptr = 0;
	fp = fopen( filename, mode );
	if ( fp && *critical_error_counter_ptr )	{
		fclose(fp);
		fp = NULL;
	}
	if ( (fp==NULL) && (AltHogdir_initialized) )	{
		strcpy( temp, AltHogDir );
		strcat( temp, "/");
		strcat( temp, filename );
		*critical_error_counter_ptr = 0;
		fp = fopen( temp, mode );
		if ( fp && *critical_error_counter_ptr )	{
			fclose(fp);
			fp = NULL;
		}
	}
	return fp;
}

//returns 1 if file loaded with no errors
int cfile_init_hogfile(char *fname, hogfile * hog_files, int * nfiles )
{
	char id[4];
	FILE * fp;
	int i, len;

	*nfiles = 0;

	fp = cfile_get_filehandle( fname, "rb" );
	if ( fp == NULL ) return 0;

	fread( id, 3, 1, fp );
	if ( strncmp( id, "DHF", 3 ) )	{
		fclose(fp);
		return 0;
	}

	while( 1 )
	{
		if ( *nfiles >= MAX_HOGFILES ) {
			fclose(fp);
			Error( "HOGFILE is limited to %d files.\n",  MAX_HOGFILES );
		}
		i = fread( hog_files[*nfiles].name, 13, 1, fp );
		if ( i != 1 )	{		//eof here is ok
			fclose(fp);
			return 1;
		}
		i = fread( &len, 4, 1, fp );
		if ( i != 1 )	{
			fclose(fp);
			return 0;
		}
		hog_files[*nfiles].length = INTEL_INT(len);
		hog_files[*nfiles].offset = ftell( fp );
		*nfiles = (*nfiles) + 1;
		// Skip over
		i = fseek( fp, INTEL_INT(len), SEEK_CUR );
	}
}

//Specify the name of the hogfile.  Returns 1 if hogfile found & had files
int cfile_init(char *hogname)
{
	#ifdef MACINTOSH
	char mac_path[255];

	macify_dospath(hogname, mac_path);
	#endif

	Assert(Hogfile_initialized == 0);

	#ifndef MACINTOSH
	if (cfile_init_hogfile(hogname, HogFiles, &Num_hogfiles )) {
		strcpy( HogFilename, hogname );
	#else
	if (cfile_init_hogfile(mac_path, HogFiles, &Num_hogfiles )) {
		strcpy( HogFilename, mac_path );
	#endif
		Hogfile_initialized = 1;
		return 1;
	}
	else
		return 0;	//not loaded!
}


int cfile_size(char *hogname)
{
	CFILE *fp;
	struct stat statbuf;

	fp = cfopen(hogname, "rb");
	if (fp == NULL)
		return -1;
	fstat(fileno(fp->file), &statbuf);
	cfclose(fp);
	return statbuf.st_size;
}

/*
 * return handle for file called "name", embedded in one of the hogfiles
 */
FILE * cfile_find_libfile(char * name, int * length)
{
	FILE * fp;
	int i;

	if ( AltHogfile_initialized )	{
		for (i=0; i<AltNum_hogfiles; i++ )	{
			if ( !stricmp( AltHogFiles[i].name, name ))	{
				fp = cfile_get_filehandle( AltHogFilename, "rb" );
				if ( fp == NULL ) return NULL;
				fseek( fp,  AltHogFiles[i].offset, SEEK_SET );
				*length = AltHogFiles[i].length;
				return fp;
			}
		}
	}

	if ( !Hogfile_initialized ) 	{
		//@@cfile_init_hogfile( "DESCENT2.HOG", HogFiles, &Num_hogfiles );
		//@@Hogfile_initialized = 1;

		//Int3();	//hogfile ought to be initialized
	}

	for (i=0; i<Num_hogfiles; i++ )	{
		if ( !stricmp( HogFiles[i].name, name ))	{
			fp = cfile_get_filehandle( HogFilename, "rb" );
			if ( fp == NULL ) return NULL;
			fseek( fp,  HogFiles[i].offset, SEEK_SET );
			*length = HogFiles[i].length;
			return fp;
		}
	}

	if (D1Hogfile_initialized)	{
		for (i = 0; i < D1Num_hogfiles; i++) {
			if (!stricmp(D1HogFiles[i].name, name)) {
				fp = cfile_get_filehandle(D1HogFilename, "rb");
				if (fp == NULL) return NULL;
				fseek(fp,  D1HogFiles[i].offset, SEEK_SET);
				*length = D1HogFiles[i].length;
				return fp;
			}
		}
	}

	return NULL;
}

int cfile_use_alternate_hogfile( char * name )
{
	if ( name )	{
		#ifdef MACINTOSH
		char mac_path[255];

		macify_dospath(name, mac_path);
		strcpy( AltHogFilename, mac_path);
		#else
		strcpy( AltHogFilename, name );
		#endif
		cfile_init_hogfile( AltHogFilename, AltHogFiles, &AltNum_hogfiles );
		AltHogfile_initialized = 1;
		return (AltNum_hogfiles > 0);
	} else {
		AltHogfile_initialized = 0;
		return 1;
	}
}

int cfile_use_descent1_hogfile( char * name )
{
	if (name)	{
#ifdef MACINTOSH
		char mac_path[255];

		macify_dospath(name, mac_path);
		strcpy(D1HogFilename, mac_path);
#else
		strcpy(D1HogFilename, name);
#endif
		cfile_init_hogfile(D1HogFilename, D1HogFiles, &D1Num_hogfiles);
		D1Hogfile_initialized = 1;
		return (D1Num_hogfiles > 0);
	} else {
		D1Hogfile_initialized = 0;
		return 1;
	}
}


// cfeof() Tests for end-of-file on a stream
//
// returns a nonzero value after the first read operation that attempts to read
// past the end of the file. It returns 0 if the current position is not end of file.
// There is no error return.

int cfeof(CFILE *cfile)
{
	Assert(cfile != NULL);

	Assert(cfile->file != NULL);

    return (cfile->raw_position >= cfile->size);
}


int cferror(CFILE *cfile)
{
	return ferror(cfile->file);
}


int cfexist( char * filename )
{
	int length;
	FILE *fp;


	if (filename[0] != '\x01')
		fp = cfile_get_filehandle( filename, "rb" );		// Check for non-hog file first...
	else {
		fp = NULL;		//don't look in dir, only in hogfile
		filename++;
	}

	if ( fp )	{
		fclose(fp);
		return 1;
	}

	fp = cfile_find_libfile(filename, &length );
	if ( fp )	{
		fclose(fp);
		return 2;		// file found in hog
	}

	return 0;		// Couldn't find it.
}


// Deletes a file.
int cfile_delete(char *filename)
{
	return remove(filename);
}


// Rename a file.
int cfile_rename(char *oldname, char *newname)
{
	return rename(oldname, newname);
}


// Make a directory.
int cfile_mkdir(char *pathname)
{
#if defined(__WINDOWS__) || defined(__MINGW32__)
	return mkdir(pathname);
#else
	return mkdir(pathname, 0755);
#endif
}


CFILE * cfopen(char * filename, char * mode )
{
	int length;
	FILE * fp;
	CFILE *cfile;

	if (filename[0] != '\x01') {
		#ifdef MACINTOSH
		char mac_path[255];

		macify_dospath(filename, mac_path);
		fp = cfile_get_filehandle( mac_path, mode);
		#else
		fp = cfile_get_filehandle( filename, mode );		// Check for non-hog file first...
		#endif
	} else {
		fp = NULL;		//don't look in dir, only in hogfile
		filename++;
	}

	if ( !fp ) {
		fp = cfile_find_libfile(filename, &length );
		if ( !fp )
			return NULL;		// No file found
		if (stricmp(mode, "rb"))
			Error("mode must be rb for files in hog.\n");
		cfile = d_malloc ( sizeof(CFILE) );
		if ( cfile == NULL ) {
			fclose(fp);
			return NULL;
		}
		cfile->file = fp;
		cfile->size = length;
		cfile->lib_offset = ftell( fp );
		cfile->raw_position = 0;
		return cfile;
	} else {
		cfile = d_malloc ( sizeof(CFILE) );
		if ( cfile == NULL ) {
			fclose(fp);
			return NULL;
		}
		cfile->file = fp;
		cfile->size = filelength( fileno(fp) );
		cfile->lib_offset = 0;
		cfile->raw_position = 0;
		return cfile;
	}
}

int cfilelength( CFILE *fp )
{
	return fp->size;
}


// cfwrite() writes to the file
//
// returns:   number of full elements actually written
//
//
int cfwrite(void *buf, int elsize, int nelem, CFILE *cfile)
{
	int items_written;

	Assert(cfile != NULL);
	Assert(buf != NULL);
	Assert(elsize > 0);

	Assert(cfile->file != NULL);
	Assert(cfile->lib_offset == 0);

	items_written = fwrite(buf, elsize, nelem, cfile->file);
	cfile->raw_position = ftell(cfile->file);

	return items_written;
}


// cfputc() writes a character to a file
//
// returns:   success ==> returns character written
//            error   ==> EOF
//
int cfputc(int c, CFILE *cfile)
{
	int char_written;

	Assert(cfile != NULL);

	Assert(cfile->file != NULL);
	Assert(cfile->lib_offset == 0);

	char_written = fputc(c, cfile->file);
	cfile->raw_position = ftell(cfile->file);

	return char_written;
}


int cfgetc( CFILE * fp )
{
	int c;

	if (fp->raw_position >= fp->size ) return EOF;

	c = getc( fp->file );
	if (c!=EOF)
		fp->raw_position++;

//	Assert( fp->raw_position==(ftell(fp->file)-fp->lib_offset) );

	return c;
}


// cfputs() writes a string to a file
//
// returns:   success ==> non-negative value
//            error   ==> EOF
//
int cfputs(char *str, CFILE *cfile)
{
	int ret;

	Assert(cfile != NULL);
	Assert(str != NULL);

	Assert(cfile->file != NULL);

	ret = fputs(str, cfile->file);
	cfile->raw_position = ftell(cfile->file);

	return ret;
}


char * cfgets( char * buf, size_t n, CFILE * fp )
{
	char * t = buf;
	int i;
	int c;

#if 0 // don't use the standard fgets, because it will only handle the native line-ending style
	if (fp->lib_offset == 0) // This is not an archived file
	{
		t = fgets(buf, n, fp->file);
		fp->raw_position = ftell(fp->file);
		return t;
	}
#endif

	for (i=0; i<n-1; i++ ) {
		do {
			if (fp->raw_position >= fp->size ) {
				*buf = 0;
				return NULL;
			}
			c = fgetc( fp->file );
			fp->raw_position++;
			if (c == 0 || c == 10)        // Unix line ending
				break;
			if (c == 13) {      // Mac or DOS line ending
				int c1;

				c1 = fgetc( fp->file );
				fseek( fp->file, -1, SEEK_CUR);
				if ( c1 == 10 ) // DOS line ending
					continue;
				else            // Mac line ending
					break;
			}
		} while ( c == 13 );
 		if ( c == 13 )  // because cr-lf is a bad thing on the mac
 			c = '\n';   // and anyway -- 0xod is CR on mac, not 0x0a
		*buf++ = c;
		if ( c=='\n' ) break;
	}
	*buf++ = 0;
	return  t;
}

size_t cfread( void * buf, size_t elsize, size_t nelem, CFILE * fp ) 
{
	unsigned int i, size;

	size = elsize * nelem;
	if ( size < 1 ) return 0;

	i = fread ( buf, 1, size, fp->file );
	fp->raw_position += i;
	return i/elsize;
}


int cftell( CFILE *fp )	
{
	return fp->raw_position;
}

int cfseek( CFILE *fp, long int offset, int where )
{
	int c, goal_position;

	switch( where )	{
	case SEEK_SET:
		goal_position = offset;
		break;
	case SEEK_CUR:
		goal_position = fp->raw_position+offset;
		break;
	case SEEK_END:
		goal_position = fp->size+offset;
		break;
	default:
		return 1;
	}	
	c = fseek( fp->file, fp->lib_offset + goal_position, SEEK_SET );
	fp->raw_position = ftell(fp->file)-fp->lib_offset;
	return c;
}

int cfclose(CFILE *fp)
{
	int result;

	result = fclose(fp->file);
	d_free(fp);

	return result;
}

// routines to read basic data types from CFILE's.  Put here to
// simplify mac/pc reading from cfiles.

int cfile_read_int(CFILE *file)
{
	int32_t i;

	if (cfread( &i, sizeof(i), 1, file) != 1)
		Error( "Error reading int in cfile_read_int()" );

	i = INTEL_INT(i);
	return i;
}

short cfile_read_short(CFILE *file)
{
	int16_t s;

	if (cfread( &s, sizeof(s), 1, file) != 1)
		Error( "Error reading short in cfile_read_short()" );

	s = INTEL_SHORT(s);
	return s;
}

sbyte cfile_read_byte(CFILE *file)
{
	sbyte b;

	if (cfread( &b, sizeof(b), 1, file) != 1)
		Error( "Error reading byte in cfile_read_byte()" );

	return b;
}

fix cfile_read_fix(CFILE *file)
{
	fix f;

	if (cfread( &f, sizeof(f), 1, file) != 1)
		Error( "Error reading fix in cfile_read_fix()" );

	f = (fix)INTEL_INT((int)f);
	return f;
}

fixang cfile_read_fixang(CFILE *file)
{
	fixang f;

	if (cfread(&f, 2, 1, file) != 1)
		Error("Error reading fixang in cfile_read_fixang()");

	f = (fixang) INTEL_SHORT((int) f);
	return f;
}

void cfile_read_vector(vms_vector *v, CFILE *file)
{
	v->x = cfile_read_fix(file);
	v->y = cfile_read_fix(file);
	v->z = cfile_read_fix(file);
}

void cfile_read_angvec(vms_angvec *v, CFILE *file)
{
	v->p = cfile_read_fixang(file);
	v->b = cfile_read_fixang(file);
	v->h = cfile_read_fixang(file);
}

void cfile_read_matrix(vms_matrix *m,CFILE *file)
{
	cfile_read_vector(&m->rvec,file);
	cfile_read_vector(&m->uvec,file);
	cfile_read_vector(&m->fvec,file);
}


void cfile_read_string(char *buf, int n, CFILE *file)
{
	char c;

	do {
		c = (char)cfile_read_byte(file);
		if (n > 0)
		{
			*buf++ = c;
			n--;
		}
	} while (c != 0);
}


// equivalent write functions of above read functions follow

int cfile_write_int(int i, CFILE *file)
{
	i = INTEL_INT(i);
	return cfwrite(&i, sizeof(i), 1, file);
}


int cfile_write_short(short s, CFILE *file)
{
	s = INTEL_SHORT(s);
	return cfwrite(&s, sizeof(s), 1, file);
}


int cfile_write_byte(sbyte b, CFILE *file)
{
	return cfwrite(&b, sizeof(b), 1, file);
}


int cfile_write_string(char *buf, CFILE *file)
{
	int len;

	if ((!buf) || (buf && !buf[0]))
		return cfile_write_byte(0, file);

	len = strlen(buf);
	if (!cfwrite(buf, len, 1, file))
		return 0;

	return cfile_write_byte(0, file);   // write out NULL termination
}
