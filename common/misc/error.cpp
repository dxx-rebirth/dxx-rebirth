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
 * Error handling/printing/exiting code
 *
 */

#include <cstdlib>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "messagebox.h"
#include "pstypes.h"
#include "console.h"
#include "dxxerror.h"

namespace dcx {

#define MAX_MSG_LEN 2048

#if !DXX_USE_EDITOR || (!(defined(WIN32) || defined(__APPLE__) || defined(__MACH__)))
static void warn_printf(const std::span<const char> s)
{
	con_puts(CON_URGENT, s);
}
#endif

#if DXX_USE_EDITOR
static void (*warn_func)(std::span<const char> s) =
#if defined(WIN32) || defined(__APPLE__) || defined(__MACH__)
	msgbox_warning
#else
	warn_printf
#endif
	;

//provides a function to call with warning messages
void set_warn_func(void (*f)(std::span<const char> s))
{
	warn_func = f;
}

#if !(defined(WIN32) || defined(__APPLE__) || defined(__MACH__))
//uninstall warning function - install default printf
void clear_warn_func()
{
	warn_func = warn_printf;
}
#endif
#else
constexpr auto warn_func = &warn_printf;
#endif

namespace {

static void print_exit_message(const std::span<const char> exit_message)
{
	con_puts(CON_CRITICAL, exit_message);
	msgbox_error(exit_message.data());
}

[[noreturn]]
static void abort_print_exit_message(const std::span<const char> exit_message)
{
	print_exit_message(exit_message);
	d_debugbreak();
	std::abort();
}

[[noreturn]]
static void graceful_print_exit_message(const std::span<const char> exit_message)
{
	print_exit_message(exit_message);
	exit(1);
}

}

void (Error_puts)(const char *filename, const unsigned line, const char *func, const char *str)
{
	char exit_message[MAX_MSG_LEN]; // don't put the new line in for dialog output
	const auto len = snprintf(exit_message, sizeof(exit_message), "%s:%u: %s: error: %s", filename, line, func, str);
	abort_print_exit_message({exit_message, len > 0 ? static_cast<std::size_t>(len) : 0});
}

void (UserError_puts)(const char *str, std::size_t len)
{
	graceful_print_exit_message({str, len});
}

//terminates with error code 1, printing message
void (Error)(const char *filename, const unsigned line, const char *func, const char *fmt,...)
{
	char exit_message[MAX_MSG_LEN]; // don't put the new line in for dialog output
	va_list arglist;

	const std::size_t leader = std::max(snprintf(exit_message, sizeof(exit_message), "%s:%u: %s: error: ", filename, line, func), 0);
	va_start(arglist,fmt);
	const std::size_t len = std::max(vsnprintf(exit_message + leader, sizeof(exit_message) - leader, fmt, arglist), 0);
	va_end(arglist);
	abort_print_exit_message({exit_message, leader + len});
}

void (UserError)(const char *fmt,...)
{
	char exit_message[MAX_MSG_LEN]; // don't put the new line in for dialog output
	va_list arglist;
	va_start(arglist,fmt);
	const std::size_t len = std::max(vsnprintf(exit_message, sizeof(exit_message), fmt, arglist), 0);
	va_end(arglist);
	graceful_print_exit_message({exit_message, len});
}

void Warning_puts(const char *str)
{
	const auto warn{warn_func};
#if DXX_USE_EDITOR
	if (warn == NULL)
		return;
#endif
	std::array<char, MAX_MSG_LEN> warn_message;
	const auto written = snprintf(std::data(warn_message), std::size(warn_message), "Warning: %s", str);
	(*warn)(std::span<const char>(warn_message.data(), written));
}

//print out warning message to user
void (Warning)(const char *fmt,...)
{
	va_list arglist;

	const auto warn{warn_func};
#if DXX_USE_EDITOR
	if (warn == NULL)
		return;
#endif

	std::array<char, MAX_MSG_LEN> warn_message;
	strcpy(warn_message.data(), "Warning: ");

	va_start(arglist,fmt);
	const auto written = vsnprintf(std::next(warn_message.begin(), 9), std::size(warn_message) - 9, fmt, arglist);
	va_end(arglist);

	(*warn)(std::span<const char>(warn_message.data(), written + 9));
}

}
