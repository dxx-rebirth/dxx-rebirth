/* $Id: cfile.c,v 1.31 2004-10-09 21:47:49 schaffner Exp $ */
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
 * (Actually, the files are not compressed, but concatenated within hogfiles)
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#ifdef _WIN32_WCE
# include <windows.h>
#elif defined(macintosh)
# include <Files.h>
# include <CFURL.h>
#else
# include <sys/stat.h>
#endif

#include "pstypes.h"
#include "u_mem.h"
#include "strutil.h"
#include "d_io.h"
#include "error.h"
#include "cfile.h"
#include "byteswap.h"

/* a file (if offset == 0) or a part of a hog file */
struct CFILE {
	FILE	*file;
	int	size;
	int	offset;
	int	raw_position;
};

struct file_in_hog {
	char	name[13];
	int	offset;
	int	length;
};

#define MAX_FILES_IN_HOG 300

/* a hog file is an archive, like a tar file */
typedef struct {
	char filename[64];
	int num_files;
	struct file_in_hog files[MAX_FILES_IN_HOG];
} hog;

hog *builtin_hog = NULL;
hog *alt_hog = NULL;
hog *d1_hog = NULL;

void free_builtin_hog() { if (builtin_hog) d_free (builtin_hog); }
void free_alt_hog() { if (alt_hog) d_free (alt_hog); }
void free_d1_hog() { if (d1_hog) d_free (d1_hog); }

char AltHogDir[64];
char AltHogdir_initialized = 0;

// routine to take a DOS path and turn it into a macintosh
// pathname.  This routine is based on the fact that we should
// see a \ character in the dos path.  The sequence .\ a tthe
// beginning of a path is turned into a :

// routine to take a POSIX style path and turn it into a pre OS X
// pathname.  This routine uses CFURL's. This function is necessary
// because even though fopen exists in StdCLib
// it must take a path in the OS native format.

#ifdef macintosh
void macify_posix_path(char *posix_path, char *mac_path)
{
    CFURLRef	url;

    url = CFURLCreateWithBytes (kCFAllocatorDefault, (ubyte *) posix_path, strlen(posix_path), GetApplicationTextEncoding(), NULL);
    CFURLGetFileSystemRepresentation (url, 0, (ubyte *) mac_path, 255);
    CFRelease(url);
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
#ifdef macintosh
    char mac_path[256];
#endif

    *critical_error_counter_ptr = 0;
#ifdef macintosh
    macify_posix_path(filename, mac_path);
    fp = fopen( mac_path, mode );
#else
    fp = fopen( filename, mode );
#endif
    if ( fp && *critical_error_counter_ptr )	{
        fclose(fp);
        fp = NULL;
    }
    if ( (fp==NULL) && (AltHogdir_initialized) )	{
        strcpy( temp, AltHogDir );
        strcat( temp, "/");
        strcat( temp, filename );
        *critical_error_counter_ptr = 0;
#ifdef macintosh
        macify_posix_path(temp, mac_path);
        fp = fopen( mac_path, mode );
#else
        fp = fopen( temp, mode );
#endif
        if ( fp && *critical_error_counter_ptr )	{
            fclose(fp);
            fp = NULL;
        }
    }
    return fp;
}

hog * cfile_init_hogfile (char *fname)
{
	char id[4];
	FILE * fp;
	int i, len;

	fp = cfile_get_filehandle (fname, "rb");
	if (fp == NULL)
		return 0;

	// verify that it is really a Descent Hog File
	fread( id, 3, 1, fp );
	if ( strncmp( id, "DHF", 3 ) )	{
		fclose(fp);
		return NULL;
	}

	hog *new_hog = (hog *) d_malloc (sizeof (hog));

	new_hog->num_files = 0;
	strcpy (new_hog->filename, fname);

	// read file name or reach EOF
	while (! feof (fp)) {
		if (new_hog->num_files >= MAX_FILES_IN_HOG) {
			fclose (fp);
			d_free (new_hog);
			Error ("Exceeded max. number of files in hog (%d).\n",
			       MAX_FILES_IN_HOG);
		}
		i = fread (new_hog->files[new_hog->num_files].name, 13, 1, fp);
		if (i != 1)
			break; // we assume it is EOF, so it is OK
		
		i = fread( &len, 4, 1, fp );
		if (i != 1)
			break;
		new_hog->files[new_hog->num_files].length = INTEL_INT (len);
		new_hog->files[new_hog->num_files].offset = ftell (fp);
		// skip over embedded file:
		i = fseek (fp, INTEL_INT (len), SEEK_CUR);
		new_hog->num_files++;
	}
	fclose (fp);
	return new_hog;
}

// Opens builtin hog given its filename. Returns 1 on success.
int cfile_init(char *hogname)
{
	Assert (builtin_hog == NULL);

	builtin_hog = cfile_init_hogfile (hogname);
	atexit (free_builtin_hog);
	return builtin_hog != NULL ? 1 : 0;
}


int cfile_size(char *hogname)
{
	CFILE *fp;
	int size;

	fp = cfopen(hogname, "rb");
	if (fp == NULL)
		return -1;
	size = ffilelength(fp->file);
	cfclose(fp);
	return size;
}

FILE * cfile_find_in_hog (char * name, int * length, hog *my_hog) {
	FILE * fp;
	int i;
	for (i = 0; i < my_hog->num_files; i++ )
		if (! stricmp (my_hog->files[i].name, name)) {
			fp = cfile_get_filehandle (my_hog->filename, "rb");
			if (fp == NULL)
				return NULL;
			fseek (fp,  my_hog->files[i].offset, SEEK_SET);
			*length = my_hog->files[i].length;
			return fp;
		}
	return NULL;
}

/*
 * return handle for file called "name", embedded in one of the hogfiles
 */
FILE * cfile_find_embedded_file (char * name, int * length)
{
	FILE * fp;
	if (alt_hog) {
		fp = cfile_find_in_hog (name, length, alt_hog);
		if (fp)
			return fp;
	}

	if (builtin_hog) {
		fp = cfile_find_in_hog (name, length, builtin_hog);
		if (fp)
			return fp;
	}

	if (d1_hog) {
		fp = cfile_find_in_hog (name, length, d1_hog);
		if (fp)
			return fp;
	}

	return NULL;
}

int cfile_use_alternate_hogfile (char * name)
{
	if (alt_hog) {
		d_free (alt_hog);
		alt_hog = NULL;
	}
	if (name) {
		alt_hog = cfile_init_hogfile (name);
		atexit (free_alt_hog);
		if (alt_hog)
			return 1; // success
	}
	return 0;
}

/* we use the d1 hog for d1 textures... */
int cfile_use_descent1_hogfile( char * name )
{
	if (d1_hog) {
		d_free (d1_hog);
		d1_hog = NULL;
	}
	if (name) {
		d1_hog = cfile_init_hogfile (name);
		atexit (free_d1_hog);
		if (d1_hog)
			return 1; // success
	}
	return 0;
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

	fp = cfile_find_embedded_file (filename, &length);
	if ( fp )	{
		fclose(fp);
		return 2;		// file found in hog
	}

	return 0;		// Couldn't find it.
}


// Deletes a file.
int cfile_delete(char *filename)
{
#ifndef _WIN32_WCE
	return remove(filename);
#else
	return !DeleteFile(filename);
#endif
}


// Rename a file.
int cfile_rename(char *oldname, char *newname)
{
#ifndef _WIN32_WCE
	return rename(oldname, newname);
#else
	return !MoveFile(oldname, newname);
#endif
}


// Make a directory.
int cfile_mkdir(char *pathname)
{
#ifdef _WIN32
# ifdef _WIN32_WCE
	return !CreateDirectory(pathname, NULL);
# else
	return _mkdir(pathname);
# endif
#elif defined(macintosh)
    char 	mac_path[256];
    Str255	pascal_path;
    long	dirID;	// Insists on returning this

    macify_posix_path(pathname, mac_path);
    CopyCStringToPascal(mac_path, pascal_path);
    return DirCreate(0, 0, pascal_path, &dirID);
#else
	return mkdir(pathname, 0755);
#endif
}


CFILE * cfopen(char * filename, char * mode )
{
	int length;
	FILE * fp;
	CFILE *cfile;

	if (filename[0] != '\x01')
		fp = cfile_get_filehandle( filename, mode );		// Check for non-hog file first...
	else {
		fp = NULL;		//don't look in dir, only in hogfile
		filename++;
	}

	if ( !fp ) {
		fp = cfile_find_embedded_file (filename, &length);
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
		cfile->offset = ftell( fp );
		cfile->raw_position = 0;
		return cfile;
	} else {
		cfile = d_malloc ( sizeof(CFILE) );
		if ( cfile == NULL ) {
			fclose(fp);
			return NULL;
		}
		cfile->file = fp;
		cfile->size = ffilelength(fp);
		cfile->offset = 0;
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
	Assert(cfile->offset == 0);

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
	Assert(cfile->offset == 0);

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
		fp->raw_position = ftell(fp->file)-fp->offset;

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
	int i;
	int c;

#if 0 // don't use the standard fgets, because it will only handle the native line-ending style
	if (fp->offset == 0) // This is not an archived file
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
			c = cfgetc(fp);
			if (c == 0 || c == 10)        // Unix line ending
				break;
			if (c == 13) {      // Mac or DOS line ending
				int c1;

				c1 = cfgetc(fp);
				if (c1 != EOF)	// The file could end with a Mac line ending
					cfseek(fp, -1, SEEK_CUR);
				if ( c1 == 10 ) // DOS line ending
					continue;
				else            // Mac line ending
					break;
			}
		} while ( c == 13 );
 		if ( c == 13 )  // because cr-lf is a bad thing on the mac
 			c = '\n';   // and anyway -- 0xod is CR on mac, not 0x0a
		if ( c=='\n' ) break;
		*buf++ = c;
	}
	*buf++ = 0;
	return  buf;
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
	c = fseek( fp->file, fp->offset + goal_position, SEEK_SET );
	fp->raw_position = ftell(fp->file)-fp->offset;
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
