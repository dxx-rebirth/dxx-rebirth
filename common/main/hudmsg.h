/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#ifndef _HUD_MSG_H
#define _HUD_MSG_H

#include <stdarg.h>
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "fmtcheck.h"
#include "fwd-gr.h"

#ifdef __cplusplus

#define HUD_MAX_NUM_DISP	4
#define HUD_MAX_NUM_STOR	20

// classes - expanded whenever needed
#define HM_DEFAULT		1 // just some normal message
#define HM_MULTI		2 // a message related to multiplayer (game and player messages)
#define HM_REDUNDANT		4 // "you already have..."-type messages. stuff a player is able to supress
#define HM_MAYDUPL		8 // messages that might appear once per frame. for these we want to check all messages we have  in queue and supress it if so

extern int HUD_toolong;
extern void HUD_clear_messages();
#ifdef dsx
namespace dsx {
void HUD_render_message_frame(grs_canvas &);
}
#endif
int HUD_init_message(int class_flag, const char * format, ... ) __attribute_format_printf(2, 3);
#define HUD_init_message(A1,F,...)	dxx_call_printf_checked(HUD_init_message,HUD_init_message_literal,(A1),(F),##__VA_ARGS__)
int HUD_init_message_va(int class_flag, const char * format, va_list args) __attribute_format_printf(2, 0);
int HUD_init_message_literal(int class_flag, const char *str);

#endif

#endif
