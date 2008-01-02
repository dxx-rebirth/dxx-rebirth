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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/cfile/cfile.c,v $
 * $Revision: 1.2 $
 * $Author: michaelstather $
 * $Date: 2006/03/18 23:07:34 $
 * 
 * Functions for accessing compressed files.
 * 
 * $Log: cfile.c,v $
 * Revision 1.2  2006/03/18 23:07:34  michaelstather
 * New build system by KyroMaster
 *
 * Revision 1.1.1.1  2006/03/17 19:45:07  zicodxx
 * initial import
 *
 * Revision 1.3  1999/09/05 11:22:31  sekmu
 * windows mission and hudlog altdirs
 *
 * Revision 1.2  1999/06/14 23:44:11  donut
 * Orulz' svgalib/ggi/noerror patches.
 *
 * Revision 1.1.1.1  1999/06/14 22:02:45  donut
 * Import of d1x 1.37 source.
 *
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


#ifdef RCS
static char rcsid[] = "$Id: cfile.c,v 1.2 2006/03/18 23:07:34 michaelstather Exp $";
#endif

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>

#include "pstypes.h"

#include "cfile.h"

#include "d_io.h"
//added 05/17/99 Matt Mueller
#include "u_mem.h"
//end addition -MM
//added on 6/15/99 by Owen Evans
#include "strutil.h"
//end added - OE
//added on 9/4/99 by Victor Rachels
#include "d_slash.h"
//end addition -VR
typedef struct hogfile {
	char	name[13];
	int	offset;
	int 	length;
} hogfile;

#define MAX_HOGFILES 250

hogfile HogFiles[MAX_HOGFILES];
char Hogfile_initialized = 0;
int Num_hogfiles = 0;

hogfile AltHogFiles[MAX_HOGFILES];
char AltHogfile_initialized = 0;
int AltNum_hogfiles = 0;
char AltHogFilename[64];

char AltHogDir[64];
char AltHogdir_initialized = 0;

// case-insensitive search for file in
// directory. We only need this on non-Windows systems.
#ifndef __WINDOWS__
char *cfile_find_file(const char *dirname, const char *filename)
{
	DIR *d = opendir(dirname);
	struct dirent *entry_p;
	struct dirent entry;
	int dn_len = strlen(dirname);

	if (!d)
		return NULL;

	while ( (entry_p = readdir(d)) )
	{
		entry = *entry_p;
		if (strcasecmp(entry.d_name, filename) == 0)
		{
			int fn_len;
			char *path;
			// There _might_ be more of course, but we'll
			// just take the first one found and use it...
			closedir(d);
			fn_len = strlen(entry.d_name);
			path = malloc(dn_len + fn_len + 2);
			strcpy(path, dirname);
			path[dn_len] = USEDSLASH;
			path[dn_len+1] = '\0';
			strcat(path, entry.d_name);

			return path;
		}
	}
	closedir(d);
	return NULL;
}

// ifopen() is like fopen() but case-insensitive; if the
// file is not found it will look for variations of the
// filename with only case differences.
FILE *ignorecase_fopen(const char *path, const char *mode)
{
	// First, try a regular fopen(), see if it works
	FILE *f = fopen(path, mode);
	// Keep searching only if we open for reading, of course
	if (f || mode[0] != 'r')
		return f;

	// No? Then let's hope it's just a case difference
	{
		char *tmp_path = strdup(path);
		char *dirname;
		char *filename;
		char *replacement_path;

		dirname = strrchr(tmp_path, USEDSLASH);
		if (dirname)
		{
			filename = &dirname[1];
			*dirname = '\0';
			dirname = tmp_path;
		}
		else
		{
			dirname = ".";
			filename = tmp_path;
		}

		replacement_path = cfile_find_file(dirname, filename);
		free(tmp_path);
		if (!replacement_path)
			// Nothing found
			return NULL;

		f = fopen(replacement_path, mode);
		free(replacement_path);
		/* f _might_ still be NULL if something goes wrong
		 * of course, but we leave the error handling for
		 * the caller. */
	}
	return f;
}
#endif

void cfile_use_alternate_hogdir( char * path )
{
	if ( path )	{
		strcpy( AltHogDir, path );
		// change backslashes to slashes, and terminate
		// AltHogDir with a slash without changing its meaning
		//added/edited by Victor Rachels on 9/4/99 for \ or / usage in path
                d_slash( AltHogDir );
		AltHogdir_initialized = 1;
	} else {
		AltHogdir_initialized = 0;
	}
}

extern int descent_critical_error;

FILE * cfile_get_filehandle( char * filename, char * mode )
{
	FILE * fp;
	char temp[128];
	
	descent_critical_error = 0;
#ifdef __WINDOWS__
	fp = fopen( filename, mode );
#else
	fp = ignorecase_fopen( filename, mode );
#endif
	if ( fp && descent_critical_error )	{
		fclose(fp);
		fp = NULL;
	}

	if ( (fp==NULL) && (AltHogdir_initialized) )	{
		strcpy( temp, AltHogDir );
		strcat( temp, filename );

		descent_critical_error = 0;
#ifdef __WINDOWS__
		fp = fopen( temp, mode );
#else
		fp = ignorecase_fopen( temp, mode );
#endif
		if ( fp && descent_critical_error )	{
			fclose(fp);
			fp = NULL;
		}

		// we haven't found the file. let's look in DESCENT_DATA_PATH before giving up
		if (fp == NULL)
		{
			strcpy(temp, DESCENT_DATA_PATH);
			strcat(temp, filename);
			descent_critical_error = 0;
	#ifdef __WINDOWS__
			fp = fopen( temp, mode );
	#else
			fp = ignorecase_fopen( temp, mode );
	#endif
			if ( fp && descent_critical_error )     {
				fclose(fp);
				fp = NULL;
			}
		}
	}
	return fp;
}

#if 0
void cfile_init_hogfile(char *fname, hogfile * hog_files, int * nfiles )
{
	char id[4];
	FILE * fp;
	int i, len;

	fp = cfile_get_filehandle( fname, "rb" );
	if ( fp == NULL ) return;

	fread( id, 3, 1, fp );
	if ( strncmp( id, "DHF", 3 ) )	{
		fclose(fp);
		return;
	}

	*nfiles = 0;

	while( 1 )	
	{	
		if ( *nfiles >= MAX_HOGFILES ) {
			printf( "ERROR: HOGFILE IS LIMITED TO %d FILES\n",  MAX_HOGFILES );
			fclose(fp);
			exit(1);
		}
		i = fread( hog_files[*nfiles].name, 13, 1, fp );
		if ( i != 1 )	{
			fclose(fp);
			return;
		}
		i = fread( &len, 4, 1, fp );
		if ( i != 1 )	{
			fclose(fp);
			return;
		}
		hog_files[*nfiles].length = len;
		hog_files[*nfiles].offset = ftell( fp );
		*nfiles = (*nfiles) + 1;
		// Skip over
		i = fseek( fp, len, SEEK_CUR );
	}
}

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
                cfile_init_hogfile(DESCENT_DATA_PATH "descent.hog", HogFiles, &Num_hogfiles );
		Hogfile_initialized = 1;
	}

	for (i=0; i<Num_hogfiles; i++ )	{
		if ( !stricmp( HogFiles[i].name, name ))	{
			fp = cfile_get_filehandle(DESCENT_DATA_PATH "descent.hog", "rb" );
			if ( fp == NULL ) return NULL;
			fseek( fp,  HogFiles[i].offset, SEEK_SET );
			*length = HogFiles[i].length;
			return fp;
		}
	}
	return NULL;
}

void cfile_use_alternate_hogfile( char * name )
{
	if ( name )	{
		strcpy( AltHogFilename, name );
		cfile_init_hogfile( AltHogFilename, AltHogFiles, &AltNum_hogfiles );
		AltHogfile_initialized = 1;
	} else {
		AltHogfile_initialized = 0;
	}
}
#else
int cfile_init_hogfile(char *fname, hogfile * hog_files )
{
	char id[4];
	FILE * fp;
	int i, len;
	int nfiles=0;

	fp = cfile_get_filehandle( fname, "rb" );
	if ( fp == NULL ) return 0;

	fread( id, 3, 1, fp );
	if ( strncmp( id, "DHF", 3 ) )	{
		fclose(fp);
		return 0;
	}

	while( 1 )	
	{	
		if ( nfiles >= MAX_HOGFILES ) {
			printf( "ERROR: HOGFILE IS LIMITED TO %d FILES\n",  MAX_HOGFILES );
			fclose(fp);
			exit(1);
		}
		i = fread( hog_files[nfiles].name, 13, 1, fp );
		if ( i != 1 )	{
			fclose(fp);
#ifndef NDEBUG
			printf("Got %i files in %s\n",nfiles,fname);
#endif
			return nfiles;
		}
		i = fread( &len, 4, 1, fp );
		if ( i != 1 )	{
			fclose(fp);
#ifndef NDEBUG
			printf("Got %i files in %s\n",nfiles,fname);
#endif
			return nfiles;
		}
		hog_files[nfiles].length = len;
		hog_files[nfiles].offset = ftell( fp );
		nfiles = (nfiles) + 1;
		// Skip over
		i = fseek( fp, len, SEEK_CUR );
	}
}

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
                Num_hogfiles = cfile_init_hogfile(DESCENT_DATA_PATH "descent.hog", HogFiles );
		Hogfile_initialized = 1;
	}

	for (i=0; i<Num_hogfiles; i++ )	{
		if ( !stricmp( HogFiles[i].name, name ))	{
			fp = cfile_get_filehandle(DESCENT_DATA_PATH "descent.hog", "rb" );
			if ( fp == NULL ) return NULL;
			fseek( fp,  HogFiles[i].offset, SEEK_SET );
			*length = HogFiles[i].length;
			return fp;
		}
	}
	return NULL;
}

void cfile_use_alternate_hogfile( char * name )
{
	if ( name )	{
		strcpy( AltHogFilename, name );
		AltNum_hogfiles = cfile_init_hogfile( AltHogFilename, AltHogFiles );
		AltHogfile_initialized = 1;
	} else {
		AltHogfile_initialized = 0;
	}
}
#endif

int cfexist( char * filename )
{
	int length;
	FILE *fp;

	fp = cfile_get_filehandle( filename, "rb" );		// Check for non-hog file first...
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


CFILE * cfopen(char * filename, char * mode ) 
{
	int length;
	FILE * fp;
	CFILE *cfile;
	
	if (stricmp( mode, "rb"))       {
		printf( "CFILES CAN ONLY BE OPENED WITH RB\n" );
		exit(1);
	}

	fp = cfile_get_filehandle( filename, mode );		// Check for non-hog file first...
	if ( !fp ) {
		fp = cfile_find_libfile(filename, &length );
		if ( !fp )
			return NULL;		// No file found
		cfile = malloc ( sizeof(CFILE) );
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
		cfile = malloc ( sizeof(CFILE) );
		if ( cfile == NULL ) {
			fclose(fp);
			return NULL;
		}
		cfile->file = fp;
                cfile->size = ffilelength(fp);
		cfile->lib_offset = 0;
		cfile->raw_position = 0;
		return cfile;
	}
}

int cfilelength( CFILE *fp )
{
	return fp->size;
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

char * cfgets( char * buf, size_t n, CFILE * fp )
{
	char * t = buf;
	int i;
	int c;

	for (i=0; (unsigned int)i<n-1; i++ )	{
		do {
			if (fp->raw_position >= fp->size ) {
				*buf = 0;
				return NULL;
			}
			c = fgetc( fp->file );
			fp->raw_position++;
		} while ( c == 13 );
		*buf++ = c;
		if ( c=='\n' ) break;
	}
	*buf++ = 0;
	return  t;
}

size_t cfread( void * buf, size_t elsize, size_t nelem, CFILE * fp ) 
{
	int i;
	if ((int)(fp->raw_position+(elsize*nelem)) > fp->size ) return EOF;
	i = fread( buf, elsize, nelem, fp->file );
	fp->raw_position += i*elsize;
	return i;
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

void cfclose( CFILE * fp ) 
{	
	fclose(fp->file);
	free(fp);
	return;
}



