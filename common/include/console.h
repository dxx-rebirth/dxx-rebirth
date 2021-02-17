/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/* Console */

#pragma once

#include <cstddef>
#include <cstring>
#include <type_traits>
#include "pstypes.h"
#include "dxxsconf.h"
#include "fmtcheck.h"
#include "d_srcloc.h"
#ifdef dsx
#include "kconfig.h"
#endif

namespace dcx {

/* Priority levels */
enum con_priority
{
	CON_CRITICAL = -3,
	CON_URGENT,
	CON_HUD,
	CON_NORMAL,
	CON_VERBOSE,
	CON_DEBUG
};

constexpr std::integral_constant<std::size_t, 2048> CON_LINE_LENGTH{};

struct console_buffer
{
	char line[CON_LINE_LENGTH];
	int priority;
};

/* Define to 1 to capture the __FILE__, __LINE__ of callers to
 * con_printf, con_puts, and show the captured value in `gamelog.txt`.
 */
#ifndef DXX_CONSOLE_SHOW_FILE_LINE
#define DXX_CONSOLE_SHOW_FILE_LINE	0
#endif

using con_priority_wrapper = location_value_wrapper<con_priority, DXX_CONSOLE_SHOW_FILE_LINE>;

#undef DXX_CONSOLE_SHOW_FILE_LINE

void con_init(void);
void con_puts(con_priority_wrapper level, char *str, size_t len) __attribute_nonnull();
void con_puts(con_priority_wrapper level, const char *str, size_t len) __attribute_nonnull();

template <typename T>
static inline void con_puts(const con_priority_wrapper level, T &&str)
{
	using rr = typename std::remove_reference<T>::type;
	constexpr std::size_t len = std::extent<rr>::value;
	con_puts(level, str, len && std::is_const<rr>::value ? len - 1 : strlen(str));
}

void con_printf(con_priority_wrapper level, const char *fmt, ...) __attribute_format_printf(2, 3);
#ifdef DXX_CONSTANT_TRUE
#define DXX_CON_PRINTF_CHECK_TRAILING_NEWLINE(F)	\
	(DXX_CONSTANT_TRUE(sizeof((F)) > 1 && (F)[sizeof((F)) - 2] == '\n') &&	\
		(DXX_ALWAYS_ERROR_FUNCTION(dxx_trap_trailing_newline, "trailing literal newline on con_printf"), 0)),
#else
#define DXX_CON_PRINTF_CHECK_TRAILING_NEWLINE(C)
#endif
#define con_printf(A1,F,...)	\
	DXX_CON_PRINTF_CHECK_TRAILING_NEWLINE(F)	\
	dxx_call_printf_checked(con_printf,con_puts,(A1),(F),##__VA_ARGS__)

}

#ifdef dsx
namespace dsx {

void con_showup(control_info &Controls);

}
#endif
