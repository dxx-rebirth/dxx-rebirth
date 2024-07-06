/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

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
#include "pstypes.h"
#include "dxxerror.h"
#include "args.h"
#include "console.h"
#include "u_mem.h"
#include <array>

namespace dcx {

#define MEMSTATS 0
#define FULL_MEM_CHECKING 1

#if DXX_USE_DEBUG_MEMORY_ALLOCATOR
#if defined(FULL_MEM_CHECKING)

#define CHECKSIZE 16
#define CHECKBYTE 0xFC

#define MAX_INDEX 10000

static std::array<void *, MAX_INDEX> MallocBase;
static std::array<unsigned int, MAX_INDEX> MallocSize;
static std::array<unsigned char, MAX_INDEX> Present;
static std::array<const char *, MAX_INDEX> Filename;
static std::array<const char *, MAX_INDEX> Varname;
static std::array<int, MAX_INDEX> LineNum;
static int BytesMalloced = 0;

static int free_list[MAX_INDEX];

static int num_blocks = 0;

static int Initialized = 0;
static int LargestIndex = 0;
static int out_of_memory = 0;

void mem_init()
{
	Initialized = 1;

	MallocBase = {};
	MallocSize = {};
	Present = {};
	Filename = {};
	Varname = {};
	LineNum = {};
	for (int i=0; i<MAX_INDEX; i++ )
	{
		free_list[i] = i;
	}

	num_blocks = 0;
	LargestIndex = 0;

	atexit(mem_display_blocks);
}

static void PrintInfo( int id )
{
	con_printf(CON_CRITICAL, "\tBlock '%s' created in %s, line %d.", Varname[id], Filename[id], LineNum[id] );
}


void *mem_malloc(size_t size, const char * var, const char * filename, unsigned line)
{
	int id;
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
		con_printf(CON_CRITICAL,"\nMEM_OUT_OF_SLOTS: Not enough space in mem.c to hold all the mallocs." );		
		con_printf(CON_CRITICAL, "\tBlock '%s' created in %s, line %d.", var, filename, line );
		Error( "MEM_OUT_OF_SLOTS" );
	}

	id = free_list[ num_blocks++ ];

	if (id > LargestIndex ) LargestIndex = id;

	if (id==-1)
	{
		con_printf(CON_CRITICAL,"\nMEM_OUT_OF_SLOTS: Not enough space in mem.c to hold all the mallocs." );		
		//con_printf(CON_CRITICAL, "\tBlock '%s' created in %s, line %d.", Varname[id], Filename[id], LineNum[id] );
		Error( "MEM_OUT_OF_SLOTS" );
	}
	ptr = malloc(size + DXX_DEBUG_BIAS_MEMORY_ALLOCATION + CHECKSIZE);

	if (ptr==NULL)
	{
		out_of_memory = 1;
		con_printf(CON_CRITICAL, "\nMEM_OUT_OF_MEMORY: Malloc returned NULL" );
		con_printf(CON_CRITICAL, "\tBlock '%s' created in %s, line %d.", Varname[id], Filename[id], LineNum[id] );
		Error( "MEM_OUT_OF_MEMORY" );
	}
	ptr = reinterpret_cast<char *>(ptr) + DXX_DEBUG_BIAS_MEMORY_ALLOCATION;
	MallocBase[id] = ptr;
	MallocSize[id] = size;
	Varname[id] = var;
	Filename[id] = filename;
	LineNum[id] = line;
	Present[id]    = 1;

	pc = reinterpret_cast<char *>(ptr);

	BytesMalloced += size;

	for (int i=0; i<CHECKSIZE; i++ )
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
			request = ~std::size_t{0};
	}
	void *ptr = mem_malloc(request, var, filename, line);
	if (ptr)
		memset( ptr, 0, request );
	return ptr;
}

static int mem_find_id( void * buffer )
{
	for (int i=0; i<=LargestIndex; i++ )
	  if (Present[i]==1)
	    if (MallocBase[i] == buffer )
	      return i;

	// Didn't find id.
	return -1;
}

static int mem_check_integrity( int block_number )
{
	int ErrorCount;
	uint8_t * CheckData;

	CheckData = reinterpret_cast<uint8_t *>(reinterpret_cast<char *>(MallocBase[block_number]) + MallocSize[block_number]);

	ErrorCount = 0;
			
	for (int i=0; i<CHECKSIZE; i++ )
		if (CheckData[i] != CHECKBYTE ) {
			ErrorCount++;
			con_printf(CON_CRITICAL, "OA: %p ", &CheckData[i] );
		}

	if (ErrorCount &&  (!out_of_memory))	{
		con_printf(CON_CRITICAL, "\nMEM_OVERWRITE: Memory after the end of allocated block overwritten." );
		PrintInfo( block_number );
		con_printf(CON_CRITICAL, "\t%d/%d check bytes were overwritten.", ErrorCount, CHECKSIZE );
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
		con_printf(CON_CRITICAL, "\nMEM_FREE_NULL: An attempt was made to free the null pointer." );
		Warning_puts("MEM: Freeing the NULL pointer!");
		Int3();
		return;
	}
	id = mem_find_id( buffer );

	if (id==-1 &&  (!out_of_memory))
	{
		con_printf(CON_CRITICAL, "\nMEM_FREE_NOMALLOC: An attempt was made to free a ptr that wasn't\nallocated with mem.h included." );
		Warning("MEM: Freeing a non-malloced pointer: %p!", buffer);
		Int3();
		return;
	}
	
	mem_check_integrity( id );
	
	BytesMalloced -= MallocSize[id];

	buffer = reinterpret_cast<char *>(buffer) - DXX_DEBUG_BIAS_MEMORY_ALLOCATION;
	free( buffer );

	Present[id] = 0;
	MallocBase[id] = 0;
	MallocSize[id] = 0;

	free_list[ --num_blocks ] = id;
}

void *mem_realloc(void *buffer, size_t size, const char *var, const char *filename, unsigned line)
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
	int numleft;

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
	for (int i=0; i<=LargestIndex; i++ )
	{
		if (Present[i]==1 &&  (!out_of_memory))
		{
			numleft++;
			if (CGameArg.DbgShowMemInfo)	{
				con_printf(CON_CRITICAL, "\nMEM_LEAKAGE: Memory block has not been freed." );
				PrintInfo( i );
			}
		}
	}

	if (numleft &&  (!out_of_memory))
	{
		Warning( "MEM: %d blocks were left allocated!\n", numleft );
	}

}

void mem_validate_heap()
{
	for (int i=0; i<LargestIndex; i++  )
		if (Present[i]==1 )
			mem_check_integrity( i );
}

#else

static int Initialized = 0;
static unsigned int SmallestAddress = 0xFFFFFFF;
static unsigned int LargestAddress = 0x0;
static unsigned int BytesMalloced = 0;

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
		con_printf(CON_CRITICAL, "\nMEM_LEAKAGE: %d bytes of memory have not been freed.", BytesMalloced );
	}

	if (CGameArg.DbgShowMemInfo)	{
		con_printf(CON_CRITICAL, "\n\nMEMORY USAGE:" );
		con_printf(CON_CRITICAL, "  %u Kbytes dynamic data", (LargestAddress-SmallestAddress+512)/1024 );
		con_printf(CON_CRITICAL, "  %u Kbytes code/static data.", (SmallestAddress-(4*1024*1024)+512)/1024 );
		con_printf(CON_CRITICAL, "  ---------------------------" );
		con_printf(CON_CRITICAL, "  %u Kbytes required.", 	(LargestAddress-(4*1024*1024)+512)/1024 );
	}
}

void mem_validate_heap()
{
}

#endif
#endif

}
