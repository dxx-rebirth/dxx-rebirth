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

#ifndef _CFILE_H
#define _CFILE_H

#include <stdio.h>

#include "maths.h"
#include "vecmat.h"

typedef struct CFILE {
	FILE 			*file;
	int				size;
	int				lib_offset;
	int				raw_position;
} CFILE;

//Specify the name of the hogfile.  Returns 1 if hogfile found & had files
int cfile_init(char *hogname);

int cfile_size(char *hogname);

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
// Returns 1 if hogfile found (& contains file), else 0.  
// If NULL passed, returns 1
int cfile_use_alternate_hogfile( char * name );

// Allows files to be gotten from the Descent 1 hog file.
// Passing NULL disables this.
// Returns 1 if hogfile found (& contains file), else 0.
// If NULL passed, returns 1
int cfile_use_descent1_hogfile(char * name);

// All cfile functions will check this directory if no file exists
// in the current directory.
void cfile_use_alternate_hogdir( char * path );

//tell cfile about your critical error counter 
void cfile_set_critical_error_counter_ptr(int *ptr);

// prototypes for reading basic types from cfile
int cfile_read_int(CFILE *file);
short cfile_read_short(CFILE *file);
byte cfile_read_byte(CFILE *file);
fix cfile_read_fix(CFILE *file);
fixang cfile_read_fixang(CFILE *file);
void cfile_read_vector(vms_vector *v, CFILE *file);
void cfile_read_angvec(vms_angvec *v, CFILE *file);
void cfile_read_matrix(vms_matrix *v, CFILE *file);

extern char AltHogDir[];
extern char AltHogdir_initialized;

#endif
