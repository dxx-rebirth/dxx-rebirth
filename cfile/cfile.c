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
#include <string.h>

#include "pstypes.h"
#include "u_mem.h"
#include "strutil.h"
#include "d_io.h"
#include "error.h"
#include "cfile.h"
#include "byteswap.h"

typedef struct hogfile {
	char	name[13];
	int	offset;
	int 	length;
} hogfile;

#define MAX_HOGFILES 300

hogfile HogFiles[MAX_HOGFILES];
char Hogfile_initialized = 0;
int Num_hogfiles = 0;

hogfile AltHogFiles[MAX_HOGFILES];
char AltHogfile_initialized = 0;
int AltNum_hogfiles = 0;
char HogFilename[64];
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


CFILE * cfopen(char * filename, char * mode ) 
{
	int length;
	FILE * fp;
	CFILE *cfile;
	
	if (stricmp( mode, "rb"))	{
		Error( "cfiles can only be opened with mode==rb\n" );
	}

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

	for (i=0; i<n-1; i++ )	{
		do {
			if (fp->raw_position >= fp->size ) {
				*buf = 0;
				return NULL;
			}
			c = fgetc( fp->file );
			fp->raw_position++;
#ifdef MACINTOSH
			if (c == 13) {
				int c1;
				
				c1 = fgetc( fp->file );
				fseek( fp->file, -1, SEEK_CUR);
				if ( c1 == 10 )
					continue;
				else
					break;
			}
#endif
		} while ( c == 13 );
#ifdef MACINTOSH			// because cr-lf is a bad thing on the mac
 		if ( c == 13 )		// and anyway -- 0xod is CR on mac, not 0x0a
 			c = '\n';
#endif
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

void cfclose( CFILE * fp ) 
{	
	fclose(fp->file);
	d_free(fp);
	return;
}

// routines to read basic data types from CFILE's.  Put here to
// simplify mac/pc reading from cfiles.

int cfile_read_int(CFILE *file)
{
	int i;

	if (cfread( &i, sizeof(i), 1, file) != 1)
		Error( "Error reading short in cfile_read_int()" );

	i = INTEL_INT(i);
	return i;
}

short cfile_read_short(CFILE *file)
{
	short s;

	if (cfread( &s, sizeof(s), 1, file) != 1)
		Error( "Error reading short in cfile_read_short()" );

	s = INTEL_SHORT(s);
	return s;
}

byte cfile_read_byte(CFILE *file)
{
	byte b;

	if (cfread( &b, sizeof(b), 1, file) != 1)
		Error( "Error reading byte in cfile_read_byte()" );

	return b;
}

#if 0
fix read_fix(CFILE *file)
{
	fix f;

	if (cfread( &f, sizeof(f), 1, file) != 1)
		Error( "Error reading fix in gamesave.c" );

	f = (fix)INTEL_INT((int)f);
	return f;
}

static void read_vector(vms_vector *v,CFILE *file)
{
	v->x = read_fix(file);
	v->y = read_fix(file);
	v->z = read_fix(file);
}
#endif

