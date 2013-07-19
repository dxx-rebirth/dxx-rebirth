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
 * Files for debugging memory allocator
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "physfsx.h"
#include "pstypes.h"
#include "dxxerror.h"
#include "args.h"
#include "console.h"

#define MEMSTATS 0
#define FULL_MEM_CHECKING 1

#if defined(FULL_MEM_CHECKING) && !defined(NDEBUG)

#define CHECKSIZE 16
#define CHECKBYTE 0xFC

#define MAX_INDEX 10000

static void *MallocBase[MAX_INDEX];
static unsigned int MallocSize[MAX_INDEX];
static unsigned char Present[MAX_INDEX];
static const char * Filename[MAX_INDEX];
static const char * Varname[MAX_INDEX];
static int LineNum[MAX_INDEX];
static int BytesMalloced = 0;

static int free_list[MAX_INDEX];

static int num_blocks = 0;

static int Initialized = 0;

static int LargestIndex = 0;

int out_of_memory = 0;

void mem_display_blocks();

void mem_init()
{
	int i;

	Initialized = 1;

	for (i=0; i<MAX_INDEX; i++ )
	{
		free_list[i] = i;
		MallocBase[i] = 0;
		MallocSize[i] = 0;
		Present[i] = 0;
		Filename[i] = NULL;
		Varname[i] = NULL;
		LineNum[i] = 0;
	}

	num_blocks = 0;
	LargestIndex = 0;

	atexit(mem_display_blocks);
}

void PrintInfo( int id )
{
	con_printf(CON_CRITICAL, "\tBlock '%s' created in %s, line %d.\n", Varname[id], Filename[id], LineNum[id] );
}


void * mem_malloc( unsigned int size, const char * var, const char * filename, unsigned line)
{
	int i, id;
	void *ptr;
	char * pc;

	if (Initialized==0)
		mem_init();

#if MEMSTATS
	{
		unsigned long	theFreeMem = 0;
	
		if (sMemStatsFileInitialized)
		{
			theFreeMem = FreeMem();
		
			fprintf(sMemStatsFile,
					"\n%9u bytes free before attempting: MALLOC %9u bytes.",
					theFreeMem,
					size);
		}
	}
#endif	// end of ifdef memstats

	if ( num_blocks >= MAX_INDEX )	{
		con_printf(CON_CRITICAL,"\nMEM_OUT_OF_SLOTS: Not enough space in mem.c to hold all the mallocs.\n" );		
		con_printf(CON_CRITICAL, "\tBlock '%s' created in %s, line %d.\n", var, filename, line );
		Error( "MEM_OUT_OF_SLOTS" );
	}

	id = free_list[ num_blocks++ ];

	if (id > LargestIndex ) LargestIndex = id;

	if (id==-1)
	{
		con_printf(CON_CRITICAL,"\nMEM_OUT_OF_SLOTS: Not enough space in mem.c to hold all the mallocs.\n" );		
		//con_printf(CON_CRITICAL, "\tBlock '%s' created in %s, line %d.\n", Varname[id], Filename[id], LineNum[id] );
		Error( "MEM_OUT_OF_SLOTS" );
	}

	ptr = malloc( size+CHECKSIZE );

	if (ptr==NULL)
	{
		out_of_memory = 1;
		con_printf(CON_CRITICAL, "\nMEM_OUT_OF_MEMORY: Malloc returned NULL\n" );
		con_printf(CON_CRITICAL, "\tBlock '%s' created in %s, line %d.\n", Varname[id], Filename[id], LineNum[id] );
		Error( "MEM_OUT_OF_MEMORY" );
	}

	MallocBase[id] = ptr;
	MallocSize[id] = size;
	Varname[id] = var;
	Filename[id] = filename;
	LineNum[id] = line;
	Present[id]    = 1;

	pc = (char *)ptr;

	BytesMalloced += size;

	for (i=0; i<CHECKSIZE; i++ )
		pc[size+i] = CHECKBYTE;

	return ptr;
}

void *(mem_calloc)( size_t nmemb, size_t size, const char * var, const char * filename, unsigned line)
{
	size_t threshold = 1, request = nmemb * size;
	threshold <<= (4 * sizeof(threshold));
	if ((nmemb | size) >= threshold) {
		/* possible overflow condition */
		if (request / size != nmemb)
			request = ~(size_t)0;
	}
	void *ptr = mem_malloc(request, var, filename, line);
	if (ptr)
		memset( ptr, 0, request );
	return ptr;
}

int mem_find_id( void * buffer )
{
	int i;

	for (i=0; i<=LargestIndex; i++ )
	  if (Present[i]==1)
	    if (MallocBase[i] == buffer )
	      return i;

	// Didn't find id.
	return -1;
}

int mem_check_integrity( int block_number )
{
	int i, ErrorCount;
	ubyte * CheckData;

	CheckData = (ubyte *)((char *)MallocBase[block_number] + MallocSize[block_number]);

	ErrorCount = 0;
			
	for (i=0; i<CHECKSIZE; i++ )
		if (CheckData[i] != CHECKBYTE ) {
			ErrorCount++;
			con_printf(CON_CRITICAL, "OA: %p ", &CheckData[i] );
		}

	if (ErrorCount &&  (!out_of_memory))	{
		con_printf(CON_CRITICAL, "\nMEM_OVERWRITE: Memory after the end of allocated block overwritten.\n" );
		PrintInfo( block_number );
		con_printf(CON_CRITICAL, "\t%d/%d check bytes were overwritten.\n", ErrorCount, CHECKSIZE );
		Int3();
	}

	return ErrorCount;

}

void mem_free( void * buffer )
{
	int id;

	if (Initialized==0)
		mem_init();

#if MEMSTATS
	{
		unsigned long	theFreeMem = 0;
	
		if (sMemStatsFileInitialized)
		{
			theFreeMem = FreeMem();
		
			fprintf(sMemStatsFile,
					"\n%9u bytes free before attempting: FREE", theFreeMem);
		}
	}
#endif	// end of ifdef memstats

	if (buffer==NULL  &&  (!out_of_memory))
	{
		con_printf(CON_CRITICAL, "\nMEM_FREE_NULL: An attempt was made to free the null pointer.\n" );
		Warning( "MEM: Freeing the NULL pointer!" );
		Int3();
		return;
	}

	id = mem_find_id( buffer );

	if (id==-1 &&  (!out_of_memory))
	{
		con_printf(CON_CRITICAL, "\nMEM_FREE_NOMALLOC: An attempt was made to free a ptr that wasn't\nallocated with mem.h included.\n" );
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

void *mem_realloc(void * buffer, unsigned int size, const char * var, const char * filename, int line)
{
	void *newbuffer;
	int id;

	if (Initialized==0)
		mem_init();

	if (size == 0) {
		mem_free(buffer);
		newbuffer = NULL;
	} else if (buffer == NULL) {
		newbuffer = mem_malloc(size, var, filename, line);
	} else {
		newbuffer = mem_malloc(size, var, filename, line);
		if (newbuffer != NULL) {
			id = mem_find_id(buffer);
			if (MallocSize[id] < size)
				size = MallocSize[id];
			memcpy(newbuffer, buffer, size);
			mem_free(buffer);
		}
	}
	return newbuffer;
}

void mem_display_blocks()
{
	int i, numleft;

	if (Initialized==0) return;
	
#if MEMSTATS
	{	
		if (sMemStatsFileInitialized)
		{
			unsigned long	theFreeMem = 0;

			theFreeMem = FreeMem();
		
			fprintf(sMemStatsFile,
					"\n%9u bytes free before closing MEMSTATS file.", theFreeMem);
			fprintf(sMemStatsFile, "\nMemory Stats File Closed.");
			fclose(sMemStatsFile);
		}
	}
#endif	// end of ifdef memstats

	numleft = 0;
	for (i=0; i<=LargestIndex; i++ )
	{
		if (Present[i]==1 &&  (!out_of_memory))
		{
			numleft++;
			if (GameArg.DbgShowMemInfo)	{
				con_printf(CON_CRITICAL, "\nMEM_LEAKAGE: Memory block has not been freed.\n" );
				PrintInfo( i );
			}
			mem_free( (void *)MallocBase[i] );
		}
	}

	if (numleft &&  (!out_of_memory))
	{
		Warning( "MEM: %d blocks were left allocated!\n", numleft );
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
	PHYSFS_file * ef;
	int i, size = 0;

	ef = PHYSFSX_openWriteBuffered( "DESCENT.MEM" );
	
	for (i=0; i<LargestIndex; i++  )
		if (Present[i]==1 )	{
			size += MallocSize[i];
			PHYSFSX_printf( ef, "%12d bytes in %s declared in %s, line %d\n", MallocSize[i], Varname[i], Filename[i], LineNum[i]  );
		}
	PHYSFSX_printf( ef, "%d bytes (%d Kbytes) allocated.\n", size, size/1024 ); 
	PHYSFS_close(ef);
}

#else

static int Initialized = 0;
static unsigned int SmallestAddress = 0xFFFFFFF;
static unsigned int LargestAddress = 0x0;
static unsigned int BytesMalloced = 0;

void mem_display_blocks();

#define CHECKSIZE 16
#define CHECKBYTE 0xFC

void mem_init()
{
	Initialized = 1;

	SmallestAddress = 0xFFFFFFF;
	LargestAddress = 0x0;

	atexit(mem_display_blocks);
}

void mem_display_blocks()
{
	if (Initialized==0) return;

#if MEMSTATS
	{	
		if (sMemStatsFileInitialized)
		{
			unsigned long	theFreeMem = 0;

			theFreeMem = FreeMem();
		
			fprintf(sMemStatsFile,
					"\n%9u bytes free before closing MEMSTATS file.", theFreeMem);
			fprintf(sMemStatsFile, "\nMemory Stats File Closed.");
			fclose(sMemStatsFile);
		}
	}
#endif	// end of ifdef memstats

	if (BytesMalloced != 0 )	{
		con_printf(CON_CRITICAL, "\nMEM_LEAKAGE: %d bytes of memory have not been freed.\n", BytesMalloced );
	}

	if (GameArg.DbgShowMemInfo)	{
		con_printf(CON_CRITICAL, "\n\nMEMORY USAGE:\n" );
		con_printf(CON_CRITICAL, "  %u Kbytes dynamic data\n", (LargestAddress-SmallestAddress+512)/1024 );
		con_printf(CON_CRITICAL, "  %u Kbytes code/static data.\n", (SmallestAddress-(4*1024*1024)+512)/1024 );
		con_printf(CON_CRITICAL, "  ---------------------------\n" );
		con_printf(CON_CRITICAL, "  %u Kbytes required.\n", 	(LargestAddress-(4*1024*1024)+512)/1024 );
	}
}

void mem_validate_heap()
{
}

void mem_print_all()
{
}

#endif
