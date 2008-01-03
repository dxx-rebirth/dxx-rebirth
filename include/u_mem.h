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
 * 
 * Headers for safe malloc stuff.
 * 
 */
#ifndef _MEM_H
#define _MEM_H

#include <stdlib.h>
#include <string.h>


#ifndef NDEBUG
void * mem_display_blocks();
void   mem_validate_heap(); // Checks to see if any blocks are overwritten
extern void * mem_malloc( unsigned int size, char * var, char * file, int line, int fill_zero );
extern void * mem_realloc( void *ptr, unsigned int size, char * var, char * filename, int line, int fill_zero );
extern void   mem_free( void * buffer );
#else
#ifdef macintosh
extern char *strdup(const char *str);
#endif
#endif

#define free(ptr)                  do{ free(ptr); ptr=NULL; } while(0)
#define MALLOC( var, type, count ) (var=(type *)malloc((count)*sizeof(type)))
#define stackavail()               16384 /* adb: should work... */

#endif // _MEM_H
