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


#ifndef _SWIFTCFG_H
#define _SWIFTCFG_H

/* SWIFT configuration parameters			*/
/* used to control compilation of SWIFT.C		*/

// Standard configurations, for which we automatically configure.
// WATCOM C 9.0, target: 32-bit code, Rational DOS/4GW extender
// Borland C++ 3.10, target: 16-bit code, DOS real mode
// Microsoft C 6.00A, target: 16-bit code, DOS real mode
// Microsoft C 7.00, target: 16-bit code, DOS real mode

// For any other configuration:
// Define PROTECTED_MODE if that describes your target environment
// (REAL_MODE is the default)
// Define RATIONAL_EXTENDER if that is your target environment
// Define TARGET_32 or TARGET_16 if your code is 32-bit (386/486) or
// 16-bit respectively.  Within these two big categories, the memory
// models pretty much take care of themselves.
// At this time, only DOS real-mode, and protected mode with DOS/4GW
// are supported by the SWIFT.C module.

#ifdef __WATCOMC__
// assume target is 32-bit protected mode with Rational DOS/4GW
#define PROTECTED_MODE
#define TARGET_32
#define RATIONAL_EXTENDER
// and use 32-bit register names
#if defined(__386__) && !defined(__WINDOWS_386__)
#define AX(r) ((r).x.eax)
#define BX(r) ((r).x.ebx)
#define CX(r) ((r).x.ecx)
#define DX(r) ((r).x.edx)
#define SI(r) ((r).x.esi)
#define DI(r) ((r).x.edi)
#endif
#endif

#ifdef __BORLANDC__
#define REAL_MODE
#define TARGET_16
#endif

#ifdef _MSC_VER
#define REAL_MODE
#define TARGET_16
#endif

#if !defined(PROTECTED_MODE) && !defined(REAL_MODE)
#define REAL_MODE
#endif

#if !defined(TARGET_32) && !defined(TARGET_16)
#define TARGET_16
#endif

//#define RATIONAL_EXTENDER
//#define PHARLAP_EXTENDER

#ifndef AX
#define AX(r) ((r).x.ax)
#define BX(r) ((r).x.bx)
#define CX(r) ((r).x.cx)
#define DX(r) ((r).x.dx)
#define SI(r) ((r).x.si)
#define DI(r) ((r).x.di)
#endif


#endif
