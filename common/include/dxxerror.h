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

#include <cstddef>
#include <stdio.h>
#include "dxxsconf.h"
#include "dsx-ns.h"
#include <assert.h>

#ifdef __cplusplus

#ifdef __GNUC__
#define __noreturn __attribute__ ((noreturn))
#else
#define __noreturn
#endif

namespace dcx {

void Warning_puts(const char *str) __attribute_nonnull();
void Warning(const char *fmt,...) __attribute_format_printf(1, 2);				//print out warning message to user
#define Warning(F,...)	dxx_call_printf_checked(Warning,Warning_puts,(),(F),##__VA_ARGS__)
#if DXX_USE_EDITOR
void set_warn_func(void (*f)(const char *s));//specifies the function to call with warning messages
void clear_warn_func();//say this function no longer valid
#endif
__noreturn
__attribute_nonnull()
void Error_puts(const char *file, unsigned line, const char *func, const char *str);
#define Error_puts(F)	Error_puts(__FILE__, __LINE__, __func__, F)
__noreturn
__attribute_format_printf(4, 5)
__attribute_nonnull()
void Error(const char *file, unsigned line, const char *func, const char *fmt,...);				//exit with error code=1, print message
#define Error(F,...)	dxx_call_printf_checked(Error,(Error_puts),(__FILE__, __LINE__, __func__),(F),##__VA_ARGS__)

__noreturn
void UserError_puts(const char *str, std::size_t);

template <std::size_t len>
__noreturn
static inline void UserError_puts(const char (&str)[len])
{
	UserError_puts(str, len - 1);
}
void UserError(const char *fmt, ...) __noreturn __attribute_format_printf(1, 2);

}
#define DXX_STRINGIZE_FL2(F,L,S)	F ":" #L ": " S
#define DXX_STRINGIZE_FL(F,L,S)	DXX_STRINGIZE_FL2(F, L, S)
#define UserError(F,...)	dxx_call_printf_checked(UserError,(UserError_puts),(),DXX_STRINGIZE_FL(__FILE__, __LINE__, F),##__VA_ARGS__)
#define Assert assert

#define LevelErrorV(V,F,...)	con_printf(V, DXX_STRINGIZE_FL(__FILE__, __LINE__, F "  Please report this to the level author, not to the Rebirth maintainers."), ##__VA_ARGS__)
#define LevelError(F,...)	LevelErrorV(CON_URGENT, F, ##__VA_ARGS__)

/* Compatibility with x86-specific name */
#define Int3() d_debugbreak()

#ifndef NDEBUG		//macros for debugging
#	define DXX_ENABLE_DEBUGBREAK_TRAP
#endif

/* Allow macro override */

#if defined(DXX_ENABLE_DEBUGBREAK_TRAP) && !defined(DXX_DEBUGBREAK_TRAP)

#if defined __clang__
	/* Must be first, since clang also defines __GNUC__ */
#	define DXX_DEBUGBREAK_TRAP()	__builtin_debugtrap()
#elif defined __GNUC__
#	if defined(__i386__) || defined(__amd64__)
#		define DXX_DEBUGBREAK_TRAP()	__asm__ __volatile__("int3" ::: "cc", "memory")
#	endif
#elif defined _MSC_VER
#	define DXX_DEBUGBREAK_TRAP()	__debugbreak()
#endif

#ifndef DXX_DEBUGBREAK_TRAP
#	if defined __unix__
/* for raise */
#		include <signal.h>
#		define DXX_DEBUGBREAK_TRAP()	raise(SIGTRAP)
#	elif defined __GNUC__
	/* May terminate execution */
#		define DXX_DEBUGBREAK_TRAP()	__builtin_trap()
#	endif
#endif

#endif

namespace dcx {

// Encourage optimizer to treat d_debugbreak paths as unlikely
__attribute_cold
// Requested by btb to force Xcode to stay in the calling function
__attribute_always_inline()
static inline void d_debugbreak()
{
	/* Allow explicit activation in NDEBUG builds */
#ifdef DXX_DEBUGBREAK_TRAP
	DXX_DEBUGBREAK_TRAP();
#endif

}

}

#endif
