/* $Id: mem.c,v 1.13 2004-08-01 13:01:39 schaffner Exp $ */
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

#ifdef HAVE_CONFIG_H
#include <conf.h>
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
#include <string.h>
#if defined(__APPLE__) && defined(__MACH__)
#include <sys/malloc.h>
#elif defined(macintosh)
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include "pstypes.h"
#include "mono.h"
#include "error.h"

#ifdef MACINTOSH

	#include <Gestalt.h>
	#include <Memory.h>
	#include <Processes.h>
	ubyte virtual_memory_on = 0;

	#if defined(RELEASE) || defined(NDEBUG)
		#define MEMSTATS	0
	#else
		#define MEMSTATS	1
	#endif

	
	#if MEMSTATS
		static 	int		sMemStatsFileInitialized	= false;
		static 	FILE* 	sMemStatsFile 				= NULL;
		static	char  	sMemStatsFileName[32]		= "memstats.txt";
	#endif	// end of if MEMSTATS
	
#else	// no memstats on pc
	#define MEMSTATS 0
#endif

#define FULL_MEM_CHECKING 1

#if defined(FULL_MEM_CHECKING) && !defined(NDEBUG)

#define CHECKSIZE 16
#define CHECKBYTE 0xFC

#define MAX_INDEX 10000

static void *MallocBase[MAX_INDEX];
static unsigned int MallocSize[MAX_INDEX];
static unsigned char Present[MAX_INDEX];
static char * Filename[MAX_INDEX];
static char * Varname[MAX_INDEX];
static int LineNum[MAX_INDEX];
static int BytesMalloced = 0;

int show_mem_info = 1;

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
	
#ifdef MACINTOSH

// need to check for virtual memory since we will need to do some
// tricks based on whether it is on or not

	{
		int 			mem_type;
		THz				theD2PartionPtr = nil;
		unsigned long	thePartitionSize = 0;
		
		MaxApplZone();
		MoreMasters();			// allocate 240 more master pointers (mainly for snd handles)
		MoreMasters();
		MoreMasters();
		MoreMasters();
		virtual_memory_on = 0;
		if(Gestalt(gestaltVMAttr, &mem_type) == noErr)
			if (mem_type & 0x1)
				virtual_memory_on = 1;
				
		#if MEMSTATS
			sMemStatsFile = fopen(sMemStatsFileName, "wt");
	
			if (sMemStatsFile != NULL)
			{
				sMemStatsFileInitialized = true;
	
				theD2PartionPtr = ApplicationZone();
				thePartitionSize =  ((unsigned long) theD2PartionPtr->bkLim) - ((unsigned long) theD2PartionPtr);
				fprintf(sMemStatsFile, "\nMemory Stats File Initialized.");
				fprintf(sMemStatsFile, "\nDescent 2 launched in partition of %u bytes.\n",
						thePartitionSize);
			}
		#endif	// end of ifdef MEMSTATS
	}

#endif	// end of ifdef macintosh

}

void PrintInfo( int id )
{
	fprintf( stderr, "\tBlock '%s' created in %s, line %d.\n", Varname[id], Filename[id], LineNum[id] );
}


void * mem_malloc( unsigned int size, char * var, char * filename, int line, int fill_zero )
{
	int i, id;
	void *ptr;
	char * pc;

	if (Initialized==0)
		mem_init();

//	printf("malloc: %d %s %d\n", size, filename, line);

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
		fprintf( stderr,"\nMEM_OUT_OF_SLOTS: Not enough space in mem.c to hold all the mallocs.\n" );		
		fprintf( stderr, "\tBlock '%s' created in %s, line %d.\n", var, filename, line );
		Error( "MEM_OUT_OF_SLOTS" );
	}

	id = free_list[ num_blocks++ ];

	if (id > LargestIndex ) LargestIndex = id;

	if (id==-1)
	{
		fprintf( stderr,"\nMEM_OUT_OF_SLOTS: Not enough space in mem.c to hold all the mallocs.\n" );		
		fprintf( stderr, "\tBlock '%s' created in %s, line %d.\n", Varname[id], Filename[id], LineNum[id] );
		Error( "MEM_OUT_OF_SLOTS" );
	}

#ifndef MACINTOSH
	ptr = malloc( size+CHECKSIZE );
#else
	ptr = (void *)NewPtrClear( size+CHECKSIZE );
#endif

	/*
	for (j=0; j<=LargestIndex; j++ )
	{
		if (Present[j] && MallocBase[j] == (unsigned int)ptr )
		{
			fprintf( stderr,"\nMEM_SPACE_USED: Malloc returned a block that is already marked as preset.\n" );
			fprintf( stderr, "\tBlock '%s' created in %s, line %d.\n", Varname[id], Filename[id], Line[id] );
			Warning( "MEM_SPACE_USED" );
			Int3();
			}
	}
	*/

	if (ptr==NULL)
	{
		out_of_memory = 1;
		fprintf( stderr, "\nMEM_OUT_OF_MEMORY: Malloc returned NULL\n" );
		fprintf( stderr, "\tBlock '%s' created in %s, line %d.\n", Varname[id], Filename[id], LineNum[id] );
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

	if (fill_zero)
		memset( ptr, 0, size );

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

	CheckData = (char *)((char *)MallocBase[block_number] + MallocSize[block_number]);

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

#ifndef MACINTOSH
	free( buffer );
#else
	DisposePtr( (Ptr)buffer );
#endif
	
	
	Present[id] = 0;
	MallocBase[id] = 0;
	MallocSize[id] = 0;

	free_list[ --num_blocks ] = id;
}

void *mem_realloc(void * buffer, unsigned int size, char * var, char * filename, int line)
{
	void *newbuffer;
	int id;

	if (Initialized==0)
		mem_init();

	if (size == 0) {
		mem_free(buffer);
		newbuffer = NULL;
	} else if (buffer == NULL) {
		newbuffer = mem_malloc(size, var, filename, line, 0);
	} else {
		newbuffer = mem_malloc(size, var, filename, line, 0);
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

char *mem_strdup(char *str, char *var, char *filename, int line)
{
	char *newstr;

	newstr = mem_malloc(strlen(str) + 1, var, filename, line, 0);
	strcpy(newstr, str);

	return newstr;
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
			if (show_mem_info)	{
				fprintf( stderr, "\nMEM_LEAKAGE: Memory block has not been freed.\n" );
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
	FILE * ef;
	int i, size = 0;

	ef = fopen( "DESCENT.MEM", "wt" );
	
	for (i=0; i<LargestIndex; i++  )
		if (Present[i]==1 )	{
			size += MallocSize[i];
			//fprintf( ef, "Var:%s\t File:%s\t Line:%d\t Size:%d Base:%x\n", Varname[i], Filename[i], Line[i], MallocSize[i], MallocBase[i] );
			fprintf( ef, "%12d bytes in %s declared in %s, line %d\n", MallocSize[i], Varname[i], Filename[i], LineNum[i]  );
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

#ifdef MACINTOSH

	// need to check for virtual memory since we will need to do some
	// tricks based on whether it is on or not
	
	{
		int 			mem_type;
		THz				theD2PartionPtr = nil;
		unsigned long	thePartitionSize = 0;
		
		MaxApplZone();
		MoreMasters();		// allocate 240 more master pointers (mainly for snd handles)
		MoreMasters();
		MoreMasters();
		MoreMasters();
		virtual_memory_on = 0;
		if(Gestalt(gestaltVMAttr, &mem_type) == noErr)
			if (mem_type & 0x1)
				virtual_memory_on = 1;
				
		#if MEMSTATS
			sMemStatsFile = fopen(sMemStatsFileName, "wt");

			if (sMemStatsFile != NULL)
			{
				sMemStatsFileInitialized = true;

				theD2PartionPtr = ApplicationZone();
				thePartitionSize =  ((unsigned long) theD2PartionPtr->bkLim) - ((unsigned long) theD2PartionPtr);
				fprintf(sMemStatsFile, "\nMemory Stats File Initialized.");
				fprintf(sMemStatsFile, "\nDescent 2 launched in partition of %u bytes.\n",
						thePartitionSize);
			}
		#endif	// end of ifdef memstats
	}
	
#endif	// end of ifdef macintosh

}

void * mem_malloc( unsigned int size, char * var, char * filename, int line, int fill_zero )
{
	unsigned int base;
	void *ptr;
	int * psize;

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

	if (size==0)	{
		fprintf( stderr, "\nMEM_MALLOC_ZERO: Attempting to malloc 0 bytes.\n" );
		fprintf( stderr, "\tVar %s, file %s, line %d.\n", var, filename, line );
		Error( "MEM_MALLOC_ZERO" );
		Int3();
	}

#ifndef MACINTOSH
	ptr = malloc( size + CHECKSIZE );
#else
	ptr = (void *)NewPtrClear( size+CHECKSIZE );
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
	int * psize = (int *)buffer;
	psize--;

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

	if (buffer==NULL)	{
		fprintf( stderr, "\nMEM_FREE_NULL: An attempt was made to free the null pointer.\n" );
		Warning( "MEM: Freeing the NULL pointer!" );
		Int3();
		return;
	}

	BytesMalloced -= *psize;

#ifndef MACINTOSH
	free( buffer );
#else
	DisposePtr( (Ptr)buffer );
#endif
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
		fprintf( stderr, "\nMEM_LEAKAGE: %d bytes of memory have not been freed.\n", BytesMalloced );
	}

	if (show_mem_info)	{
		fprintf( stderr, "\n\nMEMORY USAGE:\n" );
		fprintf( stderr, "  %u Kbytes dynamic data\n", (LargestAddress-SmallestAddress+512)/1024 );
		fprintf( stderr, "  %u Kbytes code/static data.\n", (SmallestAddress-(4*1024*1024)+512)/1024 );
		fprintf( stderr, "  ---------------------------\n" );
		fprintf( stderr, "  %u Kbytes required.\n", 	(LargestAddress-(4*1024*1024)+512)/1024 );
	}
}

void mem_validate_heap()
{
}

void mem_print_all()
{
}

#endif


#ifdef MACINTOSH

// routine to try and compact and purge the process manager zone to squeeze
// some temporary memory out of it for QT purposes.

void PurgeTempMem()
{
	OSErr err;
	Handle tempHandle;
	THz appZone, processZone;
	Size heapSize;
	
	// compact the system zone to try and squeeze some temporary memory out of it
	MaxMemSys(&heapSize);
	
	// compact the Process Manager zone to get more temporary memory
	appZone = ApplicationZone();
	tempHandle = TempNewHandle(10, &err);		// temporary allocation may fail
	if (!err && (tempHandle != NULL) ) {
		processZone = HandleZone(tempHandle);
		if ( MemError() || (processZone == NULL) ) {
			DisposeHandle(tempHandle);
			return;
		}
		SetZone(processZone);
		MaxMem(&heapSize);				// purge and compact the Process Manager Zone.
		SetZone(appZone);
		DisposeHandle(tempHandle);
	}
}

#endif
