/* Console */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <cstddef>
#include <cstring>
#include "pstypes.h"
#include "dxxsconf.h"
#include "fmtcheck.h"

#ifdef __cplusplus

/* Priority levels */
#define CON_CRITICAL -3
#define CON_URGENT   -2
#define CON_HUD      -1
#define CON_NORMAL    0
#define CON_VERBOSE   1
#define CON_DEBUG     2

#define CON_LINES_ONSCREEN 18
#define CON_SCROLL_OFFSET  (CON_LINES_ONSCREEN - 3)
#define CON_LINES_MAX      128
static const size_t CON_LINE_LENGTH = 2048;

#define CON_STATE_OPEN 2
#define CON_STATE_OPENING 1
#define CON_STATE_CLOSING -1
#define CON_STATE_CLOSED -2

typedef struct console_buffer
{
	char line[CON_LINE_LENGTH];
	int priority;
} __pack__ console_buffer;

void con_init(void);
void con_puts(int level, char *str, size_t len) __attribute_nonnull();
void con_puts(int level, const char *str, size_t len) __attribute_nonnull();
template <size_t len>
static inline void con_puts_literal(int level, const char (&str)[len])
{
	con_puts(level, str, len - 1);
}
#define con_puts(A1,S,...)	(con_puts(A1,S, _dxx_call_puts_parameter2(1, ## __VA_ARGS__, strlen(S))))
void con_printf(int level, const char *fmt, ...) __attribute_format_printf(2, 3);
#define con_printf(A1,F,...)	dxx_call_printf_checked(con_printf,con_puts_literal,(A1),(F),##__VA_ARGS__)
void con_showup(void);

#endif

#endif /* _CONSOLE_H_ */

