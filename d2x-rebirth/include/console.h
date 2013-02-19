/* Console */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "pstypes.h"

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
#define CON_LINE_LENGTH    2048

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
void con_printf(int level, const char *fmt, ...);
void con_showup(void);

#endif /* _CONSOLE_H_ */

