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
 * Prototypes for DPMI services.
 *
 */

#ifndef _DPMI_H
#define _DPMI_H

#include "pstypes.h"

typedef struct dpmi_real_regs {
    uint edi;
    uint esi;
    uint ebp;
    uint reserved_by_system;
    uint ebx;
    uint edx;
    uint ecx;
    uint eax;
    ushort flags;
    ushort es,ds,fs,gs,ip,cs,sp,ss;
} dpmi_real_regs;

#ifdef __WATCOMC__
#pragma intrinsic( inp );
#pragma intrinsic( outp );
#pragma intrinsic( _enable );
#pragma intrinsic( _disable );
#endif
#ifdef __DJGPP__
/* inp & outp are functions in DJGPP, while inportb & outportb are inlined */
#define inp(port) inportb(port)
#define outp(port,value) outportb(port,value)
#if 0 /* only causes problems */
#undef _enable()
#undef _disable()
#define _enable() __asm__ __volatile__ ("sti")
#define _disable() __asm__ __volatile__ ("cli")
#endif
#endif

#ifdef __DJGPP__
#include "sys/nearptr.h"
#define DPMI_real_segment(P) ((((uint)(P)-(uint)(__djgpp_conventional_base)) >> 4) & 0xffff)
#define DPMI_real_offset(P) (((uint)(P)-(uint)(__djgpp_conventional_base)) & 0xf)
#else
#define DPMI_real_segment(P)	((((uint) (P)) >> 4) & 0xFFFF)
#define DPMI_real_offset(P)     (((uint) (P)) & 0xF)
#endif

// Initializes dpmi. Returns zero if failed.
extern int dpmi_init(int verbose);
// Returns a pointer to a temporary dos memory block. Size must be < 1024 bytes.
extern void *dpmi_get_temp_low_buffer( int size );
extern void *dpmi_real_malloc( int size, ushort *selector );
extern void dpmi_real_free( ushort selector );
extern void dpmi_real_int386x( ubyte intno, dpmi_real_regs * rregs );
extern void dpmi_real_call(dpmi_real_regs * rregs);
extern int dpmi_lock_region(void *address, unsigned length);
extern int dpmi_unlock_region(void *address, unsigned length);
// returns 0 if failed...
extern int dpmi_allocate_selector( void * address, int size, ushort * selector );
extern int dpmi_modify_selector_base( ushort selector, void * address );
extern int dpmi_modify_selector_limit( ushort selector, int size  );


#if defined(__GNUC__) || defined(_MSC_VER) || defined(macintosh)
# define _far
#else
# define _far far
#endif
// Sets the PM handler. Returns 0 if succssful
extern int dpmi_set_pm_handler(unsigned intnum, void _far * isr );

extern unsigned int dpmi_virtual_memory;
extern unsigned int dpmi_available_memory;
extern unsigned int dpmi_physical_memory;
extern unsigned int dpmi_dos_memory;

#endif
 
