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

#ifndef _U_MEM_H
#define _U_MEM_H

#include <stdlib.h>

#define MEM_K 1.5	// Dynamic array growth factor

void mem_init(void);

#if !defined(NDEBUG)
void mem_display_blocks();
extern void * mem_malloc( unsigned int size, const char * var, const char * file, unsigned line);
void * mem_calloc( size_t nmemb, size_t size, const char * var, const char * filename, unsigned line);
extern void * mem_realloc( void * buffer, unsigned int size, const char * var, const char * file, int line );
extern void mem_free( void * buffer );

/* DPH: Changed malloc, etc. to d_malloc. Overloading system calls is very evil and error prone */

// Checks to see if any blocks are overwritten
void mem_validate_heap();

#define mem_calloc(nmemb,size,var,file,line)	((mem_calloc)((size) ? (nmemb) : 0, (size), (var), (file), (line)))

#else

#define mem_malloc(size,var,file,line)	malloc((size))
#define mem_calloc(nmemb,size,var,file,line)	calloc((nmemb), (size))
#define mem_realloc(ptr,size,var,file,line)	realloc((ptr),(size))
#define mem_free	free

#endif

#define MALLOC( var, type, count )	(var=(type *)mem_malloc((count)*sizeof(type),#var, __FILE__,__LINE__ ))
#define CALLOC( var, type, count )	(var=(type *)mem_calloc((count),sizeof(type),#var, __FILE__,__LINE__ ))

#define d_malloc(size)      mem_malloc((size),"Unknown", __FILE__,__LINE__ )
#define d_calloc(nmemb,size)    mem_calloc((nmemb),(size),"Unknown", __FILE__,__LINE__ )
#define d_realloc(ptr,size) mem_realloc((ptr),(size),"Unknown", __FILE__,__LINE__ )
#define d_free(ptr)         (mem_free(ptr), ptr=NULL)

#endif // _U_MEM_H
