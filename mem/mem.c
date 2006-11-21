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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/mem/mem.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:39:19 $
 * 
 * Files for debugging memory allocator
 * 
 * $Log: mem.c,v $
 * Revision 1.1.1.1  2006/03/17 19:39:19  zicodxx
 * initial import
 *
 * Revision 1.4  2000/02/07 07:31:08  donut
 * added memsize to printinfo (unfree'd blocks report)
 *
 * Revision 1.3  1999/10/15 05:22:15  donut
 * typedef'd ssize_t on windows
 *
 * Revision 1.2  1999/10/14 04:48:21  donut
 * alpha fixes, and gl_font args
 *
 * Revision 1.1.1.1  1999/06/14 22:13:45  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.18  1995/01/24  20:49:18  matt
 * Made some stuff return when get errors
 * 
 * Revision 1.17  1994/11/29  15:40:38  matt
 * Added extra newline after message
 * 
 * Revision 1.16  1994/11/15  18:27:56  john
 * Took away show mem info by default.
 * 
 * Revision 1.15  1994/11/10  10:00:37  john
 * Made it show_mem_info by default.
 * 
 * Revision 1.14  1994/11/10  09:53:06  john
 * Put in more efficient, but less debugging info version.
 * 
 * Revision 1.13  1994/10/27  00:56:45  john
 * Reduced number of blocks; made show mem usage by default.
 * 
 * Revision 1.12  1994/10/06  19:15:17  john
 * Upped the number of blocks.
 * 
 * 
 * Revision 1.11  1994/07/27  20:04:22  john
 * Neatend printed output.
 * 
 * Revision 1.10  1994/07/20  10:22:17  yuan
 * Added overwrite error
 * 
 * Revision 1.9  1994/03/23  12:58:43  john
 * Made message global.
 * 
 * Revision 1.8  1994/03/15  11:12:59  john
 * Made calloc fill block with zeros like it's
 * supposed to.
 * 
 * Revision 1.7  1994/02/18  12:43:21  john
 * Only print mem debugging info if show_mem_info
 * is true. Need to set in debugger, btw.
 * 
 * Revision 1.6  1994/02/17  17:01:34  john
 * Took out MEM_LEAKAGE warning!
 * 
 * Revision 1.5  1994/01/24  16:04:47  john
 * Added mem_print_all function to dump all
 * allocated memory to inferno.mem file.
 * 
 * 
 * Revision 1.4  1994/01/18  11:01:41  john
 * *** empty log message ***
 * 
 * Revision 1.3  1993/12/10  12:20:50  john
 * Speed up by replacing mem_find_unused_id with a table lookup.
 * 
 * Revision 1.2  1993/12/08  12:38:22  mike
 * Change 10000 to MAX_INDEX
 * Use LargestIndex in place of MAX_INDEX as appropriate.
 * 
 * Revision 1.1  1993/11/02  17:45:28  john
 * Initial revision
 * 
 * 
 */


#ifdef RCS
static char rcsid[] = "$Id: mem.c,v 1.1.1.1 2006/03/17 19:39:19 zicodxx Exp $";
#endif

#if defined(__DJGPP__) || defined(__WATCOMC__)
#define CHECK_DWORD_BELOW
#endif

// Warning( "MEM: Too many malloc's!" );
// Warning( "MEM: Malloc returnd an already alloced block!" );
// Warning( "MEM: Malloc Failed!" );
// Warning( "MEM: Freeing the NULL pointer!" );
// Warning( "MEM: Freeing a non-malloced pointer!" );
// Warning( "MEM: %d/%d check bytes were overwritten at the end of %8x", ec, CHECKSIZE, buffer  );
// Warning( "MEM: %d blocks were left allocated!", numleft );

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#if defined(_DEBUG) && defined(_MSC_VER)
#include <crtdbg.h>
#endif

#include "mono.h"
#include "error.h"
#include "types.h"

#if !defined(_DEBUG) && !defined(_MSC_VER)
#define FULL_MEM_CHECKING
#endif

#ifdef FULL_MEM_CHECKING 

#define CHECKSIZE 16
#define CHECKBYTE 0xFC

#define MAX_INDEX 10000

//edited 05/17/99 Matt Mueller - use pointer vars/formats instead of typecasting to int
static char * MallocBase[MAX_INDEX];
//end edit -MM
static unsigned int MallocSize[MAX_INDEX];
static unsigned int MallocRealSize[MAX_INDEX];
static unsigned int Signature[MAX_INDEX];
static unsigned char Present[MAX_INDEX];
static char * Filename[MAX_INDEX];
static char * Varname[MAX_INDEX];
static int Line[MAX_INDEX];
static int BytesMalloced = 0;
static int MaxMalloced = 0;
static unsigned int LastSignature = 0;

static unsigned int SmallestAddress = 0xFFFFFFF;
static unsigned int LargestAddress = 0x0;

int show_mem_info = 1;

static int free_list[MAX_INDEX];

static int num_blocks = 0;

static int Initialized = 0;

static int LargestIndex = 0;

int out_of_memory = 0;

void mem_display_blocks(void);

void mem_init()
{
	int i;
	
	Initialized = 1;

	for (i=0; i<MAX_INDEX; i++ )
	{
		free_list[i] = i;
		MallocBase[i] = 0;
		MallocSize[i] = 0;
		MallocRealSize[i] = 0;
		Present[i] = 0;
		Filename[i] = NULL;
		Varname[i] = NULL;
		Line[i] = 0;
	}

	SmallestAddress = 0xFFFFFFF;
	LargestAddress = 0x0;

	num_blocks = 0;
	LargestIndex = 0;

	atexit(mem_display_blocks);

}

void PrintInfo( int id )
{
	//edited 05/17/99 Matt Mueller - use pointer vars/formats instead of typecasting to int -- edited 2000/02/06 Matt Mueller - added size to end.
	fprintf( stderr, "\tBlock '%s' created in %s, line %d at %p (sig %u), %i bytes.\n", Varname[id], Filename[id], Line[id], MallocBase[id], Signature[id], MallocSize[id] );
	//end edit -MM
}


void * mem_malloc( unsigned int size, char * var, char * filename, int line, int fill_zero )
{
	size_t base;
	int i, id;
	void *ptr;
	char * pc;
	ssize_t * data;

	if (Initialized==0)
		mem_init();

	if ( num_blocks >= MAX_INDEX )	{
		fprintf( stderr,"\nMEM_OUT_OF_SLOTS: Not enough space in mem.c to hold all the mallocs.\n" );		
		fprintf( stderr, "\tBlock '%s' created in %s, line %d.\n", var, filename, line );
		Error( "MEM_OUT_OF_SLOTS" );
	}

	id = free_list[ num_blocks++ ];
	if (id > LargestIndex ) LargestIndex = id;

	if (id==-1)
	{
		fprintf( stderr,"\nMEM_OUT_OF_SLOTS: Not enough space in mem.c to hold all the mallocs.\n" );		
		fprintf( stderr, "\tBlock '%s' created in %s, line %d.\n", Varname[id], Filename[id], Line[id] );
		Error( "MEM_OUT_OF_SLOTS" );
	}

	ptr = malloc( size+CHECKSIZE );
//this is a bit noisy..	printf("malloc'd %i+%i (%i) at %p id %i\n",size,CHECKSIZE,size+CHECKSIZE,ptr,id);
	/*
	for (j=0; j<=LargestIndex; j++ )
	{
		if (Present[j] && MallocBase[j] == (unsigned int)ptr )
		{
			fprintf( stderr,"\nMEM_SPACE_USED: Malloc returned a block that is already marked as preset.\n" );
			fprintf( stderr, "\tBlock '%s' created in %s, line %d.\n", Varname[j], Filename[j], Line[j] );
			Warning( "MEM_SPACE_USED" );
			Int3();
			}
	}
	*/

	if (ptr==NULL)
	{
		out_of_memory = 1;
		fprintf( stderr, "\nMEM_OUT_OF_MEMORY: Malloc returned NULL\n" );
		fprintf( stderr, "\tBlock '%s' created in %s, line %d.\n", Varname[id], Filename[id], Line[id] );
		Error( "MEM_OUT_OF_MEMORY" );
		Int3();
	}

	base = (size_t)ptr;
	if ( base < SmallestAddress ) SmallestAddress = base;
	if ( (base+size) > LargestAddress ) LargestAddress = base+size;

	//edited 05/17/99 Matt Mueller - use pointer vars/formats instead of typecasting to int
	MallocBase[id] = ptr;
	//end edit -MM
	data = (ssize_t *)((ssize_t)MallocBase[id]-4);
	MallocRealSize[id] = *data;
	MallocSize[id] = size;
	Varname[id] = var;
	Filename[id] = filename;
	Line[id] = line;
	Present[id]    = 1;
	Signature[id] = (LastSignature++);

	pc = (char *)ptr;

	BytesMalloced += size;

	if (BytesMalloced > MaxMalloced)
                MaxMalloced = BytesMalloced;

	for (i=0; i<CHECKSIZE; i++ )
		pc[size+i] = (char)CHECKBYTE;

	if (fill_zero)
		memset( ptr, 0, size );
#if 0
// print call stack
//if (!strcmp(filename,"..."))
if(Signature[id] == 378)
{
#include <stdio.h>
#include <dpmi.h>
#include <sys/segments.h>

#define get_frame_pointer() ({ int ret; asm("mov %%ebp,%0" : "=r" (ret)); (unsigned int *)ret; })
    int i;
    unsigned int maxaddr;
    unsigned int *fp = get_frame_pointer();
    maxaddr = __dpmi_get_segment_limit(_my_ds());
    PrintInfo( id );
    for (i=0;i<10;i++) {
        if (!fp || (unsigned int)fp > maxaddr)
            break;
	fprintf(stderr, "%x ", fp[1]);
        fp = (unsigned int *)fp[0];
    }
    fprintf(stderr, "\n");
}
#endif
	return ptr;

}


int mem_find_id( void * buffer )
{
	int i;

	for (i=0; i<=LargestIndex; i++ )
		if (Present[i]==1)
			//edited 05/17/99 Matt Mueller - use pointer vars/formats instead of typecasting to int
			if (MallocBase[i] == buffer )
			//end edit -MM
				return i;

	// Didn't find id.
	return -1;
}

int mem_check_integrity( int block_number )
{
	#ifdef CHECK_DWORD_BELOW
	int * data;
	#endif
	int i, ErrorCount;
	unsigned char * CheckData;

	CheckData = (ubyte *)((char *)MallocBase[block_number] + MallocSize[block_number]);

	#ifdef CHECK_DWORD_BELOW
	data = (int *)((int)MallocBase[block_number]-4);

	if ( *data != MallocRealSize[block_number] )	{
		fprintf( stderr, "\nMEM_BAD_LENGTH: The length field of an allocated block was overwritten.\n" );
		PrintInfo( block_number );
		//Int3();
		*data = MallocRealSize[block_number];
	}
	#endif

	ErrorCount = 0;
			
	for (i=0; i<CHECKSIZE; i++ )
		if (CheckData[i] != CHECKBYTE ) {
			ErrorCount++;
			fprintf( stderr, "OA: %p ", &CheckData[i] );
		}

	if (ErrorCount &&  (!out_of_memory))	{
		fprintf( stderr, "\nMEM_OVERWRITE: Memory after the end of allocated block overwritten.\n" );
		PrintInfo( block_number );
		fprintf( stderr, "\t%d/%d check bytes were overwritten.\n", ErrorCount, CHECKSIZE );
	}

	return ErrorCount;

}

void * mem_realloc( void *ptr, unsigned int size, char * var, char * filename, int line, int fill_zero ) {
    int id, i;
    ssize_t *data;
    if (!ptr)
        return mem_malloc(size,var,filename,line,fill_zero);
    id = mem_find_id(ptr);
    if (id==-1) {
	fprintf(stderr, "MEM_REALLOC_NOMALLOC: realloc'ing a non-malloced pointer");
	exit(1);
    }
    mem_check_integrity( id );
    ptr = realloc(ptr, size+CHECKSIZE);
    BytesMalloced += (size - MallocSize[id]);
    if (BytesMalloced > MaxMalloced)
	MaxMalloced = BytesMalloced;
    MallocSize[id] = size;
	//edited 05/17/99 Matt Mueller - use pointer vars/formats instead of typecasting to int
    MallocBase[id] = ptr;
	//end edit -MM
    data = (ssize_t *)((ssize_t)MallocBase[id]-4);
    MallocRealSize[id] = *data;
    Filename[id] = filename;
    Line[id] = line;

    for (i=0; i<CHECKSIZE; i++ )
	((char *)ptr)[size+i] = (char)CHECKBYTE;

    return ptr;
}


void mem_free( void * buffer )
{
	int id;

	if (Initialized==0)
		mem_init();

	if (buffer==NULL  &&  (!out_of_memory))
	{
		fprintf( stderr, "\nMEM_FREE_NULL: An attempt was made to free the null pointer.\n" );
		Warning( "MEM: Freeing the NULL pointer!" );
		Int3();
		return;
	}

	id = mem_find_id( buffer );

	if (id==-1 &&  (!out_of_memory))
	{
		fprintf( stderr, "\nMEM_FREE_NOMALLOC: An attempt was made to free a ptr that wasn't\nallocated with mem.h included.\n" );
		Warning( "MEM: Freeing a non-malloced pointer!" );
		Int3();
		return;
	}
	
	mem_check_integrity( id );
	
	BytesMalloced -= MallocSize[id];

	free( buffer );
	
	
	Present[id] = 0;
	MallocBase[id] = 0;
	MallocSize[id] = 0;

	free_list[ --num_blocks ] = id;
}

void mem_display_blocks()
{
	int i, numleft;

	if (Initialized==0) return;
	
	numleft = 0;
	for (i=0; i<=LargestIndex; i++ )
	{
		if (Present[i]==1 &&  (!out_of_memory))
		{
			numleft++;
			if (show_mem_info)	{
				fprintf( stderr, "\nMEM_LEAKAGE: Memory block has not been freed.\n" );
				PrintInfo( i );
			}
			mem_free( (void *)MallocBase[i] );
		}
	}

	if (numleft &&  (!out_of_memory))
	{
		Warning( "MEM: %d blocks were left allocated!", numleft );
	}

	if (show_mem_info)	{
#if defined (__WATCOMC__) || defined (WIN32)
		fprintf( stderr, "\n\nMEMORY USAGE:\n" );
		fprintf( stderr, "  %u Kbytes dynamic data\n", (LargestAddress-SmallestAddress+512)/1024 );
		fprintf( stderr, "  %u Kbytes code/static data.\n", (SmallestAddress-(4*1024*1024)+512)/1024 );
		fprintf( stderr, "  ---------------------------\n" );
		fprintf( stderr, "  %u Kbytes required.\n\n", 	(LargestAddress-(4*1024*1024)+512)/1024 );
#else

#if !defined(NO_ASM) && !defined(_MSC_VER)
		extern int end __asm__("end");
#endif
                fprintf( stderr, "  %u Kbytes dynamic data\n", (MaxMalloced + 512) / 1024);
#if !defined(NO_ASM) && !defined(_MSC_VER)
                fprintf( stderr, "  %u Kbytes code/static data\n", ((unsigned int)&end + 512) / 1024);
#endif
#endif
	}
}

void mem_validate_heap()
{
	int i;
	
	for (i=0; i<LargestIndex; i++  )
		if (Present[i]==1 )
			mem_check_integrity( i );
}

void mem_print_all()
{
	FILE * ef;
	int i, size = 0;

	ef = fopen( "DESCENT.MEM", "wt" );
	
	for (i=0; i<LargestIndex; i++  )
		if (Present[i]==1 )	{
			size += MallocSize[i];
			//fprintf( ef, "Var:%s\t File:%s\t Line:%d\t Size:%d Base:%x\n", Varname[i], Filename[i], Line[i], MallocSize[i], MallocBase[i] );
			fprintf( ef, "%12d bytes in %s declared in %s, line %d\n", MallocSize[i], Varname[i], Filename[i], Line[i]  );
		}
	fprintf( ef, "%d bytes (%d Kbytes) allocated.\n", size, size/1024 ); 
	fclose(ef);
}

#else

static int Initialized = 0;
static unsigned int SmallestAddress = 0xFFFFFFF;
static unsigned int LargestAddress = 0x0;
static unsigned int BytesMalloced = 0;

void mem_display_blocks();

#define CHECKSIZE 16
#define CHECKBYTE 0xFC

int show_mem_info = 0;

void mem_init()
{
	Initialized = 1;

	SmallestAddress = 0xFFFFFFF;
	LargestAddress = 0x0;

	atexit(mem_display_blocks);
}

void * mem_malloc( unsigned int size, char * var, char * filename, int line, int fill_zero )
{
	unsigned int base;
	void *ptr;
	int * psize;

	if (Initialized==0)
		mem_init();

	if (size==0)	{
		fprintf( stderr, "\nMEM_MALLOC_ZERO: Attempting to malloc 0 bytes.\n" );
		fprintf( stderr, "\tVar %s, file %s, line %d.\n", var, filename, line );
		Error( "MEM_MALLOC_ZERO" );
		Int3();
	}

#if defined(_DEBUG) && defined(_MSC_VER)
	ptr = _malloc_dbg( size + CHECKSIZE, _CLIENT_BLOCK, filename, line);
#else
	ptr = malloc( size + CHECKSIZE );
#endif


	if (ptr==NULL)	{
		fprintf( stderr, "\nMEM_OUT_OF_MEMORY: Malloc returned NULL\n" );
		fprintf( stderr, "\tVar %s, file %s, line %d.\n", var, filename, line );
		Error( "MEM_OUT_OF_MEMORY" );
		Int3();
	}

	base = (unsigned int)ptr;
	if ( base < SmallestAddress ) SmallestAddress = base;
	if ( (base+size) > LargestAddress ) LargestAddress = base+size;


	psize = (int *)ptr;
	psize--;
	BytesMalloced += *psize;

	if (fill_zero)
		memset( ptr, 0, size );

	return ptr;
}

void mem_free( void * buffer )
{
	int ErrorCount;
	int * psize = (int *)buffer;
	psize--;

	if (Initialized==0)
		mem_init();

	if (buffer==NULL)	{
		fprintf( stderr, "\nMEM_FREE_NULL: An attempt was made to free the null pointer.\n" );
		Warning( "MEM: Freeing the NULL pointer!" );
		Int3();
		return;
	}

	ErrorCount = 0;

	if (ErrorCount)	{
		fprintf( stderr, "\nMEM_OVERWRITE: Memory after the end of allocated block overwritten.\n" );
		fprintf( stderr, "\tBlock at 0x%p, size %d\n", buffer, *psize );
		fprintf( stderr, "\t%d/%d check bytes were overwritten.\n", ErrorCount, CHECKSIZE );
	}

	BytesMalloced -= *psize;

#if defined(_DEBUG) && defined(_MSC_VER)
	_free_dbg( buffer, _CLIENT_BLOCK);
#else
	free( buffer );
#endif
}

void mem_display_blocks()
{
	if (Initialized==0) return;

	if (show_mem_info)	{
		if (BytesMalloced != 0 )	{
			fprintf( stderr, "\nMEM_LEAKAGE: %u bytes of memory have not been freed.\n", BytesMalloced );
		}

//		fprintf( stderr, "  MaxMalloced:%u\n", MaxMalloced);
#ifdef __WATCOMC__
		fprintf( stderr, "\n\nMEMORY USAGE:\n" );
		fprintf( stderr, "  %u Kbytes dynamic data\n", (LargestAddress-SmallestAddress+512)/1024 );
		fprintf( stderr, "  %u Kbytes code/static data.\n", (SmallestAddress-(4*1024*1024)+512)/1024 );
		fprintf( stderr, "  ---------------------------\n" );
                fprintf( stderr, "  %u Kbytes required.\n",     (LargestAddress-(4*1024*1024)+512)/1024 );
#else
		{
#ifndef NO_ASM
			extern int end __asm__("end");
#endif
//			fprintf( stderr, "  %u Kbytes dynamic data\n", (MaxMalloced + 512) / 1024);
//			fprintf( stderr, "  %u Kbytes code/static data\n", ((unsigned int)&end + 512) / 1024);
		}
#endif
	}
}

void mem_validate_heap()
{
}

void mem_print_all()
{
}

#endif


