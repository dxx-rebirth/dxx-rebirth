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
 * $Source: /cvs/cvsroot/d2x/arch/dos/dpmi.c,v $
 * $Revision: 1.3 $
 * $Author: bradleyb $
 * $Date: 2001-10-19 09:01:56 $
 *
 * Dos DPMI
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2001/01/29 13:35:08  bradleyb
 * Fixed build system, minor fixes
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef __DJGPP__
#define _BORLAND_DOS_REGS 1
#define far
#include <sys/segments.h>
#include <sys/nearptr.h>
#include <crt0.h>
#define FP_SEG(p) _my_ds()
#define FP_OFF(p) (int)p
 int _crt0_startup_flags=_CRT0_FLAG_NONMOVE_SBRK+_CRT0_FLAG_FILL_SBRK_MEMORY+_CRT0_FLAG_FILL_DEADBEEF+_CRT0_FLAG_NEARPTR+_CRT0_FLAG_NO_LFN;
#endif

#include <dos.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#include "pstypes.h"
#include "mono.h"
#include "error.h"
#include "u_dpmi.h"

int dpmi_find_dos_memory()
{
	union REGS r;

	memset(&r,0,sizeof(r));
	r.x.eax = 0x0100;				// DPMI allocate DOS memory 
	r.x.ebx = 0xffff;	// Number of paragraphs requested
	int386 (0x31, &r, &r);
	//if ( (r.x.eax & 0xffff) == 0x08 )
	//if ( (r.x.eax & 0xffff) == 0x08 )
	if ( r.x.cflag )
		return ((r.x.ebx & 0xffff)*16);
	else
		return 640*1024;
}

void *dpmi_real_malloc( int size, ushort *selector )
{
	union REGS r;

	memset(&r,0,sizeof(r));
	r.x.eax = 0x0100;				// DPMI allocate DOS memory 
	r.x.ebx = (size + 15) >> 4;	// Number of paragraphs requested
	int386 (0x31, &r, &r);

	if (r.x.cflag)  // Failed
		return ((uint) 0);

	if(selector!=NULL)
		*selector = r.x.edx & 0xFFFF;

#ifdef __DJGPP__
	return (void *) ((r.x.eax & 0xFFFF) << 4)+__djgpp_conventional_base;
#else
	return (void *) ((r.x.eax & 0xFFFF) << 4);
#endif
}

void dpmi_real_free( ushort selector )
{
	union REGS r;

	memset(&r,0,sizeof(r));
	r.x.eax = 0x0101;				// DPMI free DOS memory 
	r.x.ebx = selector;			// Selector to free
	int386 (0x31, &r, &r);
}

int dos_stack_initialized = 0;
ubyte * dos_stack = NULL;
ubyte * dos_stack_top = NULL;
#define DOS_STACK_SIZE (4*1024)                 // A big ol' 4K stack!!!

static void dpmi_setup_stack(dpmi_real_regs *rregs)  {
	ushort temp_selector;

        if ( !dos_stack_initialized )   {
                dos_stack_initialized = 1;
                dos_stack = dpmi_real_malloc( DOS_STACK_SIZE, &temp_selector );
                if ( dos_stack == NULL )        {
                        printf( "Error allocating real mode stack!\n" );
                        dos_stack_top = NULL;
                } else {
                        dos_stack_top = &dos_stack[DOS_STACK_SIZE];
                }
        }
        
        // Give this puppy a stack!!!
        if ( dos_stack_top )    {
                rregs->ss = DPMI_real_segment(dos_stack_top);
                rregs->sp = DPMI_real_offset(dos_stack_top);
        }
}


void dpmi_real_int386x( ubyte intno, dpmi_real_regs * rregs )
{
	union REGS regs;
	struct SREGS sregs;

    /* Use DMPI call 300h to issue the DOS interrupt */

   dpmi_setup_stack(rregs);
	memset(&regs,0,sizeof(regs));
	memset(&sregs,0,sizeof(sregs));
   regs.w.ax = 0x0300;
   regs.h.bl = intno;
   regs.h.bh = 0;
   regs.w.cx = 0;
   sregs.es = FP_SEG(rregs);
   regs.x.edi = FP_OFF(rregs);
   int386x( 0x31, &regs, &regs, &sregs );
}

void dpmi_real_call(dpmi_real_regs * rregs)
{
	union REGS regs;
	struct SREGS sregs;

   dpmi_setup_stack(rregs);

    /* Use DMPI call 301h to call real mode procedure */
	memset(&regs,0,sizeof(regs));
	memset(&sregs,0,sizeof(sregs));
   regs.w.ax = 0x0301;
   regs.h.bh = 0;
   regs.w.cx = 0;
   sregs.es = FP_SEG(rregs);
   regs.x.edi = FP_OFF(rregs);
   int386x( 0x31, &regs, &regs, &sregs );
	if ( regs.x.cflag )
		exit(regs.w.ax);
}

int total_bytes = 0;

int dpmi_unlock_region(void *address, unsigned length)
{
	union REGS regs;
	unsigned int linear;

	linear = (unsigned int) address;
#ifdef __DJGPP__
	linear += __djgpp_base_address;
#endif

	total_bytes -= length;
	//mprintf( 1, "DPMI unlocked %d bytes\n", total_bytes );

	memset(&regs,0,sizeof(regs));
	regs.w.ax = 0x601;					// DPMI Unlock Linear Region
	regs.w.bx = (linear >> 16);		// Linear address in BX:CX
	regs.w.cx = (linear & 0xFFFF);

	regs.w.si = (length >> 16);		// Length in SI:DI
	regs.w.di = (length & 0xFFFF);
	int386 (0x31, &regs, &regs);
	return (! regs.w.cflag);			// Return 0 if can't lock
}

int dpmi_lock_region(void *address, unsigned length)
{
	union REGS regs;
	unsigned int linear;

	linear = (unsigned int) address;
#ifdef __DJGPP__
	linear += __djgpp_base_address;
#endif

	total_bytes += length;
	//mprintf( 1, "DPMI Locked down %d bytes\n", total_bytes );

	memset(&regs,0,sizeof(regs));
	regs.w.ax = 0x600;					// DPMI Lock Linear Region
	regs.w.bx = (linear >> 16);		// Linear address in BX:CX
	regs.w.cx = (linear & 0xFFFF);

	regs.w.si = (length >> 16);		// Length in SI:DI
	regs.w.di = (length & 0xFFFF);
	int386 (0x31, &regs, &regs);
	return (! regs.w.cflag);			// Return 0 if can't lock
}


int dpmi_modify_selector_base( ushort selector, void * address )
{
	union REGS regs;
	unsigned int linear;

	linear = (unsigned int)address;
#ifdef __DJGPP__
	linear += __djgpp_base_address;
#endif

	memset(&regs,0,sizeof(regs));
	regs.w.ax = 0x0007;					// DPMI Change Selector Base Addres
	regs.w.bx = selector;				// Selector to change
	regs.w.cx = (linear >> 16);		// Base address
	regs.w.dx = (linear & 0xFFFF);
	int386 (0x31, &regs, &regs);		// call dpmi
	if (regs.w.cflag)
		return 0;							// Return 0 if error

	return 1;
}


int dpmi_modify_selector_limit( ushort selector, int size  )
{
	union REGS regs;
	unsigned int segment_limit;

	segment_limit = (unsigned int) size;

	memset(&regs,0,sizeof(regs));
	regs.w.ax = 0x0008;					// DPMI Change Selector Limit
	regs.w.bx = selector;				// Selector to change
	regs.w.cx = (segment_limit >> 16);		// Size of selector
	regs.w.dx = (segment_limit & 0xFFFF);
	int386 (0x31, &regs, &regs);		// call dpmi
	if (regs.w.cflag)
		return 0;							// Return 0 if error

	return 1;
}


int dpmi_allocate_selector( void * address, int size, ushort * selector )
{
	union REGS regs;


	memset(&regs,0,sizeof(regs));
	regs.w.ax = 0;							// DPMI Allocate Selector
	regs.w.cx = 1;							// Allocate 1 selector
	int386 (0x31, &regs, &regs);		// call dpmi
	if (regs.w.cflag)
		return 0;							// Return 0 if error
	*selector = regs.w.ax;

	if ( !dpmi_modify_selector_base( *selector, address ) )
		return 0;

	if ( !dpmi_modify_selector_limit( *selector, size ) )
		return 0;

//	mprintf( 0, "Selector 0x%4x has base of 0x%8x, size %d bytes\n", *selector, linear,segment_limit);

	return 1;
}

static void * dpmi_dos_buffer = NULL;
static ushort dpmi_dos_selector = 0;

void dpmi_close()
{
	if (dpmi_dos_selector!=0)	{
		dpmi_dos_buffer = NULL;
		dpmi_dos_selector = 0;
	}
}

typedef struct mem_data {
	int	largest_block_bytes;
	int	max_unlocked_page_allocation;
	int	largest_lockable_pages;
	int	total_pages;
	int	unlocked_pages;
	int	unused_physical_pages;
	int	total_physical_pages;
	int 	free_linear_pages;
	int	paging_size_pages;
	int	reserved[3];
} mem_data;

unsigned int dpmi_virtual_memory=0;
unsigned int dpmi_available_memory=0;
unsigned int dpmi_physical_memory=0;
unsigned int dpmi_dos_memory = 0;

#ifdef __WATCOMC__
extern void cdecl _GETDS();
extern void cdecl cstart_();
#endif

int dpmi_init(int verbose)
{
	union REGS regs;
	struct SREGS sregs;
	mem_data mi;

	dpmi_dos_memory = dpmi_find_dos_memory();
	
	dpmi_dos_buffer = dpmi_real_malloc( 1024, &dpmi_dos_selector);
	if (!dpmi_dos_buffer) {
		dpmi_dos_selector = 0;
		printf( "Error allocating 1K of DOS memory\n" );
		exit(1);
	}
	atexit(dpmi_close);

	// Check dpmi
	memset(&regs,0,sizeof(regs));
	regs.x.eax = 0x400;							// DPMI Get Memory Info
   int386( 0x31, &regs, &regs );
	if (!regs.w.cflag)	{
		if (verbose) printf( "V%d.%d, CPU:%d, VMM:", regs.h.ah, regs.h.al, regs.h.cl );
		if (regs.w.bx & 4)	{
			if (verbose) printf( "1" );
			dpmi_virtual_memory = 1;
		} else {
		 	if (verbose) printf( "0" );
		}
	}

	//--------- Find available memory
	memset(&regs,0,sizeof(regs));
	memset(&sregs,0,sizeof(sregs));
	regs.x.eax = 0x500;							// DPMI Get Memory Info
   sregs.es = FP_SEG(&mi);
   regs.x.edi = FP_OFF(&mi);
   int386x( 0x31, &regs, &regs, &sregs );
	if (!regs.w.cflag)	{
		if (verbose) printf( ", P:%dK", mi.largest_lockable_pages*4 );
		if (dpmi_virtual_memory)
			if (verbose) printf( ", A:%dK", mi.largest_block_bytes/1024 );
		//dpmi_physical_memory = mi.largest_lockable_pages*4096;
		//dpmi_available_memory = mi.largest_block_bytes;
		dpmi_physical_memory = mi.total_physical_pages*4096;
		dpmi_available_memory = mi.total_pages * 4096;
	} else {
		if (verbose) printf( "MemInfo failed!" );
		dpmi_physical_memory = 16*1024*1024;		// Assume 16 MB
		dpmi_available_memory = 16*1024*1024;		// Assume 16 MB
	}
	
#ifdef __WATCOMC__
	if (!dpmi_lock_region( _GETDS, 4096 ))	{
		printf( "Error locking _GETDS" );
		exit(1);
	}
	if (!dpmi_lock_region( cstart_, 4096 )) {
		printf( "Error locking cstart" );
		exit(1);
	}
	if (!dpmi_lock_region( _chain_intr, 4096 ))	{
		printf( "Error locking _chain_intr" );
		exit(1);
	}
#endif
	return 1;
}

void *dpmi_get_temp_low_buffer( int size )
{
	if ( dpmi_dos_buffer == NULL ) return NULL;
	if ( size > 1024 ) return NULL;

	return dpmi_dos_buffer;
}

int dpmi_set_pm_handler(unsigned intnum, void far * isr )
{
	union REGS regs;

    /* Use DMPI call 204h to get pm interrrupt */
	memset(&regs,0,sizeof(regs));
   regs.w.ax = 0x0205;
   regs.h.bl = intnum;
	regs.w.cx = FP_SEG(isr);
	regs.x.edx = FP_OFF(isr);
	int386( 0x31, &regs, &regs );
	if (!regs.w.cflag)	
		return 0;
	return 1;
}
