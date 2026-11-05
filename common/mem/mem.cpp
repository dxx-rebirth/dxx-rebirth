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

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pstypes.h"
#include "dxxerror.h"
#include "args.h"
#include "console.h"
#include "u_mem.h"
#include <array>
#include <limits>
#include <optional>

namespace dcx {

#if DXX_USE_DEBUG_MEMORY_ALLOCATOR

constexpr std::size_t CHECKSIZE{16};
constexpr uint8_t CHECKBYTE{0xfc};

constexpr std::size_t MAX_INDEX{10000};

static std::array<void *, MAX_INDEX> MallocBase;
static std::array<std::size_t, MAX_INDEX> MallocSize;
static std::array<unsigned char, MAX_INDEX> Present;
static std::array<const char *, MAX_INDEX> Filename;
static std::array<const char *, MAX_INDEX> Varname;
static std::array<unsigned, MAX_INDEX> LineNum;

static std::array<std::size_t, MAX_INDEX> free_list;

static std::size_t num_blocks = 0;

static int Initialized = 0;
static std::size_t LargestIndex = 0;
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
	for (std::size_t i=0; i<MAX_INDEX; i++ )
	{
		free_list[i] = i;
	}

	num_blocks = 0;
	LargestIndex = 0;

	atexit(mem_display_blocks);
}

static void PrintInfo(const std::size_t id)
{
	con_printf(CON_CRITICAL, "\tBlock '%s' created in %s, line %u.", Varname[id], Filename[id], LineNum[id] );
}

[[noreturn]]
static void mem_out_of_memory(const char *var, const char *filename, const unsigned line)
{
	out_of_memory = 1;
	con_printf(CON_CRITICAL, "\nMEM_OUT_OF_MEMORY: Allocation failed" );
	con_printf(CON_CRITICAL, "\tBlock '%s' created in %s, line %u.", var, filename, line);
	Error( "MEM_OUT_OF_MEMORY" );
}


void *mem_malloc(size_t size, const char * var, const char * filename, unsigned line)
{
	void *ptr;

	if (Initialized==0)
		mem_init();

	constexpr std::size_t overhead = DXX_DEBUG_BIAS_MEMORY_ALLOCATION + CHECKSIZE;
	if (size > std::numeric_limits<std::size_t>::max() - overhead)
		mem_out_of_memory(var, filename, line);

	if ( num_blocks >= MAX_INDEX )	{
		con_printf(CON_CRITICAL,"\nMEM_OUT_OF_SLOTS: Not enough space in mem.c to hold all the mallocs." );		
		con_printf(CON_CRITICAL, "\tBlock '%s' created in %s, line %u.", var, filename, line );
		Error( "MEM_OUT_OF_SLOTS" );
	}

	const auto id = free_list[ num_blocks++ ];

	if (id > LargestIndex ) LargestIndex = id;

	ptr = malloc(size + overhead);

	if (ptr==NULL)
		mem_out_of_memory(var, filename, line);
	ptr = reinterpret_cast<char *>(ptr) + DXX_DEBUG_BIAS_MEMORY_ALLOCATION;
	MallocBase[id] = ptr;
	MallocSize[id] = size;
	Varname[id] = var;
	Filename[id] = filename;
	LineNum[id] = line;
	Present[id]    = 1;

	auto *const pc = reinterpret_cast<uint8_t *>(ptr);

	for (std::size_t i=0; i<CHECKSIZE; i++ )
		pc[size+i] = CHECKBYTE;

	return ptr;
}

void *(mem_calloc)( size_t nmemb, size_t size, const char * var, const char * filename, unsigned line)
{
	if (size && nmemb > std::numeric_limits<std::size_t>::max() / size)
		mem_out_of_memory(var, filename, line);
	const auto request = nmemb * size;
	void *ptr = mem_malloc(request, var, filename, line);
	if (ptr)
		memset( ptr, 0, request );
	return ptr;
}

static std::optional<std::size_t> mem_find_id(const void *buffer)
{
	for (std::size_t i=0; i<=LargestIndex; i++ )
	  if (Present[i]==1)
	    if (MallocBase[i] == buffer )
	      return i;

	// Didn't find id.
	return std::nullopt;
}

static unsigned mem_check_integrity(const std::size_t block_number)
{
	unsigned ErrorCount;
	uint8_t * CheckData;

	CheckData = reinterpret_cast<uint8_t *>(MallocBase[block_number]) + MallocSize[block_number];

	ErrorCount = 0;
			
	for (std::size_t i=0; i<CHECKSIZE; i++ )
		if (CheckData[i] != CHECKBYTE ) {
			ErrorCount++;
			con_printf(CON_CRITICAL, "OA: %p ", &CheckData[i] );
		}

	if (ErrorCount &&  (!out_of_memory))	{
		con_printf(CON_CRITICAL, "\nMEM_OVERWRITE: Memory after the end of allocated block overwritten." );
		PrintInfo( block_number );
		con_printf(CON_CRITICAL, "\t%u/%" DXX_PRI_size_type " check bytes were overwritten.", ErrorCount, CHECKSIZE );
		Int3();
	}

	return ErrorCount;

}

void mem_free( void * buffer )
{
	if (Initialized==0)
		mem_init();

	if (buffer==NULL  &&  (!out_of_memory))
	{
		con_printf(CON_CRITICAL, "\nMEM_FREE_NULL: An attempt was made to free the null pointer." );
		Warning_puts("MEM: Freeing the NULL pointer!");
		Int3();
		return;
	}
	const auto found_id = mem_find_id(buffer);

	if (!found_id)
	{
		if (!out_of_memory)
		{
			con_printf(CON_CRITICAL, "\nMEM_FREE_NOMALLOC: An attempt was made to free a ptr that wasn't\nallocated with mem.h included." );
			Warning("MEM: Freeing a non-malloced pointer: %p!", buffer);
			Int3();
		}
		return;
	}
	const auto id = *found_id;
	
	mem_check_integrity( id );
	
	buffer = reinterpret_cast<char *>(buffer) - DXX_DEBUG_BIAS_MEMORY_ALLOCATION;
	free( buffer );

	Present[id] = 0;
	MallocBase[id] = 0;
	MallocSize[id] = 0;

	free_list[ --num_blocks ] = id;
}

void *mem_realloc(void *buffer, size_t size, const char *var, const char *filename, unsigned line)
{
	if (Initialized==0)
		mem_init();

	if (size == 0) {
		mem_free(buffer);
		return nullptr;
	}
	if (buffer == nullptr)
		return mem_malloc(size, var, filename, line);

	const auto id = mem_find_id(buffer);
	if (!id)
	{
		con_printf(CON_CRITICAL, "\nMEM_REALLOC_NOMALLOC: An attempt was made to resize a ptr that wasn't\nallocated with mem.h included.");
		Warning("MEM: Resizing a non-malloced pointer: %p!", buffer);
		Int3();
		return nullptr;
	}
	auto *const newbuffer = mem_malloc(size, var, filename, line);
	memcpy(newbuffer, buffer, std::min(size, MallocSize[*id]));
	mem_free(buffer);
	return newbuffer;
}

void mem_display_blocks()
{
	std::size_t numleft;

	if (Initialized==0) return;
	
	numleft = 0;
	for (std::size_t i=0; i<=LargestIndex; i++ )
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
		Warning( "MEM: %" DXX_PRI_size_type " blocks were left allocated!\n", numleft );
	}

}

void mem_validate_heap()
{
	for (std::size_t i=0; i<LargestIndex; i++  )
		if (Present[i]==1 )
			mem_check_integrity( i );
}

#endif

}
