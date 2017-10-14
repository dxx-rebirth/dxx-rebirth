/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
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
#include "cli.h"
#include "cmd.h"

#ifdef __cplusplus

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

void con_init(void);
void con_puts(con_priority level, char *str, size_t len) __attribute_nonnull();
void con_puts(con_priority level, const char *str, size_t len) __attribute_nonnull();
template <size_t len>
static inline void con_puts_literal(const con_priority level, const char (&str)[len])
{
	con_puts(level, str, len - 1);
}
#define con_puts(A1,S,...)	(con_puts(A1,S, _dxx_call_puts_parameter2(1, ## __VA_ARGS__, strlen(S))))
void con_printf(con_priority level, const char *fmt, ...) __attribute_format_printf(2, 3);
#ifdef DXX_CONSTANT_TRUE
#define DXX_CON_PRINTF_CHECK_TRAILING_NEWLINE(F)	\
	(DXX_CONSTANT_TRUE(sizeof((F)) > 1 && (F)[sizeof((F)) - 2] == '\n') &&	\
		(DXX_ALWAYS_ERROR_FUNCTION(dxx_trap_trailing_newline, "trailing literal newline on con_printf"), 0)),
#else
#define DXX_CON_PRINTF_CHECK_TRAILING_NEWLINE(C)
#endif
#define con_printf(A1,F,...)	\
	DXX_CON_PRINTF_CHECK_TRAILING_NEWLINE(F)	\
	dxx_call_printf_checked(con_printf,con_puts_literal,(A1),(F),##__VA_ARGS__)
void con_showup(void);

#endif
