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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Header for error handling/printing/exiting code
 *
 */

#pragma once

#include <stdio.h>
#include "dxxsconf.h"
#include <assert.h>
#include "fmtcheck.h"

#ifdef __cplusplus

#ifdef __GNUC__
#define __noreturn __attribute__ ((noreturn))
#else
#define __noreturn
#endif

int error_init(void (*func)(const char *));    //init error system, returns 0=ok
void Warning_puts(const char *str) __attribute_nonnull();
void Warning(const char *fmt,...) __attribute_format_printf(1, 2);				//print out warning message to user
#define Warning(F,...)	dxx_call_printf_checked(Warning,Warning_puts,(),(F),##__VA_ARGS__)
void set_warn_func(void (*f)(const char *s));//specifies the function to call with warning messages
void clear_warn_func();//say this function no longer valid
void Error_puts(const char *func, unsigned line, const char *str) __noreturn __attribute_nonnull();
#define Error_puts(F)	Error_puts(__func__, __LINE__,F)
void Error(const char *func, unsigned line, const char *fmt,...) __noreturn __attribute_format_printf(3, 4);				//exit with error code=1, print message
#define Error(F,...)	dxx_call_printf_checked(Error,(Error_puts),(__func__, __LINE__),(F),##__VA_ARGS__)
#define Assert assert

/* Compatibility with x86-specific name */
#define Int3() d_debugbreak()

#ifndef d_debugbreak
#ifndef NDEBUG		//macros for debugging
#	if defined __unix__
/* for raise */
#		include <signal.h>
#	endif
#endif

/* Allow macro override */

#if defined __GNUC__
__attribute__((always_inline))
#endif
static inline void d_debugbreak()
{
	/* If NDEBUG, expand to nothing */
#ifndef NDEBUG

#if defined __clang__
	/* Must be first, since clang also defines __GNUC__ */
	__builtin_debugtrap();
#elif defined __GNUC__
#	if defined(__i386__) || defined(__amd64__)
	__asm__ __volatile__("int3" ::: "cc", "memory");
#	elif defined __unix__
	raise(SIGTRAP);
#	else
	/* May terminate execution */
	__builtin_trap();
#	endif
#elif defined _MSC_VER
	__debugbreak();
#endif

#endif
}
#endif

#endif
