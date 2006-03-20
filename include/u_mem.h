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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/include/u_mem.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:46:29 $
 * 
 * Headers for safe malloc stuff.
 * 
 * $Log: u_mem.h,v $
 * Revision 1.1.1.1  2006/03/17 19:46:29  zicodxx
 * initial import
 *
 * Revision 1.4  2000/02/07 07:34:16  donut
 * added realloc() mem debugging, and fixed strdup debugging
 *
 * Revision 1.3  1999/10/18 00:33:23  donut
 * strdup fix for alphas
 *
 * Revision 1.2  1999/08/05 22:53:41  sekmu
 *
 * D3D patch(es) from ADB
 *
 * Revision 1.1.1.1  1999/06/14 22:02:25  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.6  1995/02/12  18:40:50  matt
 * Made free() work the way it used to when debugging is on
 * 
 * Revision 1.5  1995/02/12  04:07:36  matt
 * Made free() set ptrs to NULL even when no debugging
 * 
 * Revision 1.4  1994/11/27  21:10:58  matt
 * Now supports NDEBUG to turn off all special mem checking
 * 
 * Revision 1.3  1994/03/15  11:12:40  john
 * Made calloc fill block with zeros like it's
 * supposed to.
 * 
 * Revision 1.2  1993/11/04  14:02:39  matt
 * Added calloc() macro
 * 
 * Revision 1.1  1993/11/02  17:45:33  john
 * Initial revision
 * 
 * 
 */
extern int show_mem_info;//moved out of the ifdef by KRB

#ifndef NDEBUG
#include <stdlib.h>
#include <string.h>
//extern int show_mem_info;

void * mem_display_blocks();
extern void * mem_malloc( unsigned int size, char * var, char * file, int line, int fill_zero );
extern void * mem_realloc( void *ptr, unsigned int size, char * var, char * filename, int line, int fill_zero );
extern void mem_free( void * buffer );

#define malloc(size)    mem_malloc((size),"Unknown", __FILE__,__LINE__, 0 )
#define calloc(n,size)  mem_malloc((n*size),"Unknown", __FILE__,__LINE__, 1 )
#define realloc(ptr,size) mem_realloc( ptr, size, "Unknown", __FILE__, __LINE__, 0 )
#define free(ptr)       do{ mem_free(ptr); ptr=NULL; } while(0)

#undef strdup //alpha fix,  strdup is already a #define
#define strdup(ptr)		strcpy(malloc(strlen(ptr)+1),ptr)

#define MALLOC( var, type, count )   (var=(type *)mem_malloc((count)*sizeof(type),#var, __FILE__,__LINE__,0 ))

#define mymalloc(x) malloc(x)
#define mymalloc_align(x,y) malloc(x)
#define mycalloc(x,y) calloc(x,y)
#define myfree(x) free(x)

// Checks to see if any blocks are overwritten
void mem_validate_heap();

#else
#include <stdlib.h>

#define free(ptr)       do{ free(ptr); ptr=NULL; } while(0)

#define MALLOC( var, type, count )   (var=(type *)malloc((count)*sizeof(type)))


#endif


#define stackavail() 16384 /* adb: should work... */
