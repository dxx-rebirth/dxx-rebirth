/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * Routines for displaying HUD messages...
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hudmsg.h"
#include "pstypes.h"
#include "u_mem.h"
#include "strutil.h"
#include "console.h"
#include "object.h"
#include "inferno.h"
#include "game.h"
#include "screens.h"
#include "gauges.h"
#include "physics.h"
#include "dxxerror.h"
#include "menu.h"           // For the font.
#include "collide.h"
#include "newdemo.h"
#include "player.h"
#include "gamefont.h"
#include "screens.h"
#include "text.h"
#include "laser.h"
#include "args.h"
#include "playsave.h"
#include "countarray.h"

namespace {
constexpr std::integral_constant<unsigned, 150> HUD_MESSAGE_LENGTH{};

struct hudmsg
{
	fix time;
	ntstring<HUD_MESSAGE_LENGTH> message;
	template <typename M>
		hudmsg(const fix& t, M &&m) :
		time(t)
	{
		message.copy_if(m);
	}
};

struct hudmsg_array_t : public count_array_t<hudmsg, HUD_MAX_NUM_STOR> {};
}

static hudmsg_array_t HUD_messages;


int HUD_toolong = 0;
static int HUD_color = -1;
static int HUD_init_message_literal_worth_showing(int class_flag, const char *message);

void HUD_clear_messages()
{
	HUD_messages.clear();
	HUD_toolong = 0;
	HUD_color = -1;
}

namespace dsx {
// ----------------------------------------------------------------------------
//	Writes a message on the HUD and checks its timer.
void HUD_render_message_frame(grs_canvas &canvas)
{
	int y;

	HUD_toolong = 0;

	if (HUD_messages.empty())
		return;

	auto expired = [](hudmsg &h) -> int {
		if (h.time <= FrameTime)
			return 1;
		h.time -= FrameTime;
		return 0;
	};
	HUD_messages.erase_if(expired);

	// display last $HUD_MAX_NUM_DISP messages on the list
	if (!HUD_messages.empty())
	{
		if (HUD_color == -1)
			HUD_color = BM_XRGB(0,28,0);
		gr_set_fontcolor(canvas, HUD_color, -1);
		y = FSPACY(1);

		auto &game_font = *GAME_FONT;
		const auto &&line_spacing = LINE_SPACING(game_font, game_font);
#if defined(DXX_BUILD_DESCENT_II)
		if (PlayerCfg.GuidedInBigWindow &&
			Guided_missile[Player_num] &&
			Guided_missile[Player_num]->type == OBJ_WEAPON &&
			get_weapon_id(*Guided_missile[Player_num]) == weapon_id_type::GUIDEDMISS_ID &&
			Guided_missile[Player_num]->signature == Guided_missile_sig[Player_num])
			y += line_spacing;
#endif

		hudmsg_array_t::iterator i, e = HUD_messages.end();
		if (HUD_messages.size() < HUD_MAX_NUM_DISP)
			i = HUD_messages.begin();
		else
			i = e - HUD_MAX_NUM_DISP;
		if (strlen(i->message) > 38)
			HUD_toolong = 1;
		for (; i != e; ++i )	{
			gr_string(canvas, game_font, 0x8000, y, &i->message[0]);
			y += line_spacing;
		}
	}
}
}

static int is_worth_showing(int class_flag)
{
	if (PlayerCfg.NoRedundancy && (class_flag & HM_REDUNDANT))
		return 0;

	if (PlayerCfg.MultiMessages && (Game_mode & GM_MULTI) && !(class_flag & HM_MULTI))
		return 0;
	return 1;
}

// Call to flash a message on the HUD.  Returns true if message drawn.
// (message might not be drawn if previous message was same)
int HUD_init_message_va(int class_flag, const char * format, va_list args)
{
	if (!is_worth_showing(class_flag))
		return 0;

#ifndef macintosh
	char message[HUD_MESSAGE_LENGTH+1] = "";
#else
	char message[1024] = "";
#endif

#ifndef macintosh
	vsnprintf(message, sizeof(char)*HUD_MESSAGE_LENGTH, format, args);
#else
	vsprintf(message, format, args);
#endif
	int r = HUD_init_message_literal_worth_showing(class_flag, message);
	if (r)
		con_puts(CON_HUD, message);
	return r;
}


static int HUD_init_message_literal_worth_showing(int class_flag, const char *message)
{
	// check if message is already in list and bail out if so
	if (!HUD_messages.empty())
	{
		hudmsg_array_t::iterator i, e = HUD_messages.end();
		// if "normal" message, only check if it's the same at the most recent one, if marked as "may duplicate" check whole list
		if (class_flag & HM_MAYDUPL)
			i = HUD_messages.begin();
		else
			i = e - 1;
		for (; i != e; ++i)
		{
			if (!d_stricmp(message, i->message))
			{
				i->time = F1_0*2; // keep redundant message in list
				if (std::distance(i, e) < HUD_MAX_NUM_DISP) // if redundant message on display, update them all
				{
					if (HUD_messages.size() < HUD_MAX_NUM_DISP)
						i = HUD_messages.begin();
					else
						i = HUD_messages.end() - HUD_MAX_NUM_DISP;
					for (unsigned j = 1; i != e; ++i, j++)
						i->time = F1_0*(j*2);
				}
				return 0;
			}
		}
	}

	if (HUD_messages.size() >= HUD_MAX_NUM_STOR)
	{
		std::move(HUD_messages.begin() + 1, HUD_messages.end(), HUD_messages.begin());
		HUD_messages.pop_back();
	}
	fix t;
	if (HUD_messages.size() + 1 < HUD_MAX_NUM_DISP)
		t = F1_0*3; // one message - display 3 secs
	else
	{
		hudmsg_array_t::iterator e = HUD_messages.end(), i = e - HUD_MAX_NUM_DISP;
		for (unsigned j = 1; ++i != e; j++) // multiple messages - display 2 seconds each
			i->time = F1_0*(j*2);
		t = F1_0 * ((HUD_MAX_NUM_DISP + 1) * 2);
	}
	HUD_messages.emplace_back(t, message);

	if (HUD_color == -1)
		HUD_color = BM_XRGB(0,28,0);

	if (Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_hud_message( message );

	return 1;
}

int (HUD_init_message)(int class_flag, const char * format, ... )
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = HUD_init_message_va(class_flag, format, args);
	va_end(args);

	return ret;
}

int HUD_init_message_literal(int class_flag, const char *str)
{
	if (!is_worth_showing(class_flag))
		return 0;
	int r = HUD_init_message_literal_worth_showing(class_flag, str);
	if (r)
		con_puts(CON_HUD, str);
	return r;
}

void player_dead_message(grs_canvas &canvas)
{
	if (Player_dead_state == player_dead_state::exploded)
	{
		if ( get_local_player().lives < 2 )    {
			int x, y, w, h;
			auto &huge_font = *HUGE_FONT;
			gr_get_string_size(huge_font, TXT_GAME_OVER, &w, &h, nullptr);
			const int gw = w;
			const int gh = h;
			w += 20;
			h += 8;
			x = (canvas.cv_bitmap.bm_w - w ) / 2;
			y = (canvas.cv_bitmap.bm_h - h ) / 2;
		
			gr_settransblend(canvas, 14, GR_BLEND_NORMAL);
			const uint8_t color = BM_XRGB(0, 0, 0);
			gr_rect(canvas, x, y, x + w, y + h, color);
			gr_settransblend(canvas, GR_FADE_OFF, GR_BLEND_NORMAL);
		
			gr_string(canvas, huge_font, 0x8000, (canvas.cv_bitmap.bm_h - h) / 2 + h / 8, TXT_GAME_OVER, gw, gh);
		}
	
		if (HUD_color == -1)
			HUD_color = BM_XRGB(0,28,0);
		gr_set_fontcolor(canvas, HUD_color, -1);
		auto &game_font = *GAME_FONT;
		gr_string(canvas, game_font, 0x8000, canvas.cv_bitmap.bm_h - LINE_SPACING(game_font, game_font), PlayerCfg.RespawnMode == RespawnPress::Any ? TXT_PRESS_ANY_KEY : "Press fire key or button to continue...");
	}
}
