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

static void warn_printf(const char *s)
{
	con_puts(CON_URGENT,s);
}

#if DXX_USE_EDITOR
static void (*warn_func)(const char *s)=warn_printf;

//provides a function to call with warning messages
void set_warn_func(void (*f)(const char *s))
{
	warn_func = f;
}

//uninstall warning function - install default printf
void clear_warn_func()
{
	warn_func = warn_printf;
}
#else
constexpr auto warn_func = &warn_printf;
#endif

static void print_exit_message(const char *exit_message, size_t len)
{
	con_puts(CON_CRITICAL, exit_message, len);
	msgbox_error(exit_message);
}

__noreturn
static void abort_print_exit_message(const char *exit_message, size_t len)
{
	print_exit_message(exit_message, len);
	d_debugbreak();
	std::abort();
}

__noreturn
static void graceful_print_exit_message(const char *exit_message, size_t len)
{
	print_exit_message(exit_message, len);
	exit(1);
}

void (Error_puts)(const char *filename, const unsigned line, const char *func, const char *str)
{
	char exit_message[MAX_MSG_LEN]; // don't put the new line in for dialog output
	int len = snprintf(exit_message, sizeof(exit_message), "%s:%u: %s: error: %s", filename, line, func, str);
	abort_print_exit_message(exit_message, len);
}

void (UserError_puts)(const char *str, std::size_t len)
{
	graceful_print_exit_message(str, len);
}

//terminates with error code 1, printing message
void (Error)(const char *filename, const unsigned line, const char *func, const char *fmt,...)
{
	char exit_message[MAX_MSG_LEN]; // don't put the new line in for dialog output
	va_list arglist;

	int leader = snprintf(exit_message, sizeof(exit_message), "%s:%u: %s: error: ", filename, line, func);
	va_start(arglist,fmt);
	int len = vsnprintf(exit_message+leader,sizeof(exit_message)-leader,fmt,arglist);
	va_end(arglist);
	abort_print_exit_message(exit_message, len);
}

void (UserError)(const char *fmt,...)
{
	char exit_message[MAX_MSG_LEN]; // don't put the new line in for dialog output
	va_list arglist;
	va_start(arglist,fmt);
	int len = vsnprintf(exit_message, sizeof(exit_message), fmt, arglist);
	va_end(arglist);
	graceful_print_exit_message(exit_message, len);
}

void Warning_puts(const char *str)
{
	const auto warn = warn_func;
#if DXX_USE_EDITOR
	if (warn == NULL)
		return;
#endif
	char warn_message[MAX_MSG_LEN];
	snprintf(warn_message, sizeof(warn_message), "Warning: %s", str);
	(*warn)(warn_message);
}

//print out warning message to user
void (Warning)(const char *fmt,...)
{
	va_list arglist;

	const auto warn = warn_func;
#if DXX_USE_EDITOR
	if (warn == NULL)
		return;
#endif

	char warn_message[MAX_MSG_LEN];
	strcpy(warn_message,"Warning: ");

	va_start(arglist,fmt);
	vsnprintf(warn_message+9,sizeof(warn_message)-9,fmt,arglist);
	va_end(arglist);

	(*warn)(warn_message);
}

}
