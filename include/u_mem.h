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

#ifndef _U_MEM_H
#define _U_MEM_H

#include <stdlib.h>

#define MEM_K 1.5	// Dynamic array growth factor

#ifdef MACINTOSH
extern ubyte virtual_memory_on;
#endif

void mem_init(void);

#if !defined(NDEBUG)

void mem_display_blocks();
extern void * mem_malloc( unsigned int size, char * var, char * file, int line, int fill_zero );
extern void * mem_realloc( void * buffer, unsigned int size, char * var, char * file, int line );
extern void mem_free( void * buffer );
extern char * mem_strdup(char * str, char * var, char * file, int line );

/* DPH: Changed malloc, etc. to d_malloc. Overloading system calls is very evil and error prone */
#define d_malloc(size)      mem_malloc((size),"Unknown", __FILE__,__LINE__, 0 )
#define d_calloc(n,size)    mem_malloc((n*size),"Unknown", __FILE__,__LINE__, 1 )
#define d_realloc(ptr,size) mem_realloc((ptr),(size),"Unknown", __FILE__,__LINE__ )
#define d_free(ptr)         do{ mem_free(ptr); ptr=NULL; } while(0)
#define d_strdup(str)       mem_strdup((str),"Unknown",__FILE__,__LINE__)

#define MALLOC( var, type, count )   (var=(type *)mem_malloc((count)*sizeof(type),#var, __FILE__,__LINE__,0 ))

// Checks to see if any blocks are overwritten
void mem_validate_heap();

#else

#ifdef macintosh
extern char *strdup(const char *str);
#endif

#define d_malloc(size)      malloc(size)
#define d_calloc(n, size)   calloc(n, size)
#define d_realloc(ptr,size) realloc(ptr,size)
#define d_free(ptr)         do{ free(ptr); ptr=NULL; } while(0)
#define d_strdup(str)       strdup(str)

#define MALLOC( var, type, count )   (var=(type *)malloc((count)*sizeof(type)))

#endif

#endif // _U_MEM_H
