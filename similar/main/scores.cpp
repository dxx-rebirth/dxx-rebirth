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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Inferno High Scores and Statistics System
 *
 */

#include "dxxsconf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <ranges>
#include <ctype.h>

#include "scores.h"
#include "dxxerror.h"
#include "pstypes.h"
#include "window.h"
#include "gr.h"
#include "key.h"
#include "mouse.h"
#include "palette.h"
#include "game.h"
#include "gamefont.h"
#include "u_mem.h"
#include "newmenu.h"
#include "menu.h"
#include "player.h"
#include "object.h"
#include "screens.h"
#include "gamefont.h"
#include "mouse.h"
#include "joy.h"
#include "timer.h"
#include "text.h"
#include "strutil.h"
#include "physfsx.h"
#include "compiler-range_for.h"
#include "d_enumerate.h"
#include "d_levelstate.h"
#include "d_range.h"
#include "d_zip.h"

#define VERSION_NUMBER 		1
#define SCORES_FILENAME 	"descent.hi"
#define COOL_MESSAGE_LEN 	50
namespace dcx {
constexpr std::integral_constant<unsigned, 10> MAX_HIGH_SCORES{};

struct score_items_context
{
	const font_x_scaled_float name, score, difficulty, levels, time_played;
	score_items_context(const font_x_scale_float fspacx, const unsigned border_x) :
		name(fspacx(51) + border_x), score(fspacx(134) + border_x), difficulty(fspacx(151) + border_x), levels(fspacx(217) + border_x), time_played(fspacx(261) + border_x)
	{
	}
};

}

namespace dsx {

namespace {

#if defined(DXX_BUILD_DESCENT_I)
#define DXX_SCORE_STRUCT_PACK	__pack__
#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_SCORE_STRUCT_PACK
#endif

struct stats_info
{
	callsign_t name;
	int		score;
	sbyte   starting_level;
	sbyte   ending_level;
	sbyte   diff_level;
	short 	kill_ratio;		// 0-100
	short	hostage_ratio;  // 
	int		seconds;		// How long it took in seconds...
} DXX_SCORE_STRUCT_PACK;

struct all_scores
{
	char			signature[3];			// DHS
	sbyte           version;				// version
	char			cool_saying[COOL_MESSAGE_LEN];
	stats_info	stats[MAX_HIGH_SCORES];
} DXX_SCORE_STRUCT_PACK;
#if defined(DXX_BUILD_DESCENT_I)
static_assert(sizeof(all_scores) == 294, "high score size wrong");
#elif defined(DXX_BUILD_DESCENT_II)
static_assert(sizeof(all_scores) == 336, "high score size wrong");
#endif

void scores_view(grs_canvas &canvas, const stats_info *last_game, int citem);

static void assign_builtin_placeholder_scores(all_scores &scores)
{
	strcpy(scores.cool_saying, TXT_REGISTER_DESCENT);
	scores.stats[0].name = "Parallax";
	scores.stats[1].name = "Matt";
	scores.stats[2].name = "Mike";
	scores.stats[3].name = "Adam";
	scores.stats[4].name = "Mark";
	scores.stats[5].name = "Jasen";
	scores.stats[6].name = "Samir";
	scores.stats[7].name = "Doug";
	scores.stats[8].name = "Dan";
	scores.stats[9].name = "Jason";

	for (auto &&[idx, stat] : enumerate(scores.stats))
		stat.score = (10 - idx) * 1000;
}

static void scores_read(all_scores *scores)
{
	int fsize;

	// clear score array...
	*scores = {};

	RAIIPHYSFS_File fp{PHYSFS_openRead(SCORES_FILENAME)};
	if (!fp)
	{
	 	// No error message needed, code will work without a scores file
		assign_builtin_placeholder_scores(*scores);
		return;
	}

	fsize = PHYSFS_fileLength(fp);

	if ( fsize != sizeof(all_scores) )	{
		return;
	}
	// Read 'em in...
	PHYSFS_read(fp, scores, sizeof(all_scores), 1);
	if ( (scores->version!=VERSION_NUMBER)||(scores->signature[0]!='D')||(scores->signature[1]!='H')||(scores->signature[2]!='S') )	{
		*scores = {};
		return;
	}
}

static void scores_write(all_scores *scores)
{
	RAIIPHYSFS_File fp{PHYSFS_openWrite(SCORES_FILENAME)};
	if (!fp)
	{
		nm_messagebox(menu_title{TXT_WARNING}, {TXT_OK}, "%s\n'%s'", TXT_UNABLE_TO_OPEN, SCORES_FILENAME);
		return;
	}

	scores->signature[0]='D';
	scores->signature[1]='H';
	scores->signature[2]='S';
	scores->version = VERSION_NUMBER;
	PHYSFS_write(fp, scores,sizeof(all_scores), 1);
}

static void scores_fill_struct(stats_info * stats)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	auto &plr = get_local_player();
	stats->name = plr.callsign;
	auto &player_info = get_local_plrobj().ctype.player_info;
	stats->score = player_info.mission.score;
	stats->ending_level = plr.level;
	if (const auto robots_total = GameUniqueState.accumulated_robots)
		stats->kill_ratio = (plr.num_kills_total * 100) / robots_total;
	else
		stats->kill_ratio = 0;

	if (const auto hostages_total = GameUniqueState.total_hostages)
		stats->hostage_ratio = (player_info.mission.hostages_rescued_total * 100) / hostages_total;
	else
		stats->hostage_ratio = 0;

	stats->seconds = f2i(plr.time_total) + (plr.hours_total * 3600);

	stats->diff_level = underlying_value(GameUniqueState.Difficulty_level);
	stats->starting_level = plr.starting_level;
}

}

}

namespace dcx {

namespace {

static inline const char *get_placement_slot_string(const unsigned position)
{
	switch(position)
	{
		default:
			Int3();
			[[fallthrough]];
		case 0: return TXT_1ST;
		case 1: return TXT_2ND;
		case 2: return TXT_3RD;
		case 3: return TXT_4TH;
		case 4: return TXT_5TH;
		case 5: return TXT_6TH;
		case 6: return TXT_7TH;
		case 7: return TXT_8TH;
		case 8: return TXT_9TH;
		case 9: return TXT_10TH;
	}
}

struct request_user_high_score_comment :
	std::array<char, sizeof(all_scores::cool_saying)>,
	std::array<newmenu_item, 2>,
	newmenu
{
	all_scores &scores;
	request_user_high_score_comment(all_scores &scores, grs_canvas &canvas) :
		std::array<newmenu_item, 2>{{
			newmenu_item::nm_item_text{TXT_COOL_SAYING},
			newmenu_item::nm_item_input(prepare_input_saying(*this)),
		}},
		newmenu(menu_title{TXT_HIGH_SCORE}, menu_subtitle{TXT_YOU_PLACED_1ST}, menu_filename{nullptr}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(*static_cast<std::array<newmenu_item, 2> *>(this), 0), canvas),
		scores(scores)
	{
	}
	virtual window_event_result event_handler(const d_event &) override;
	static std::array<char, sizeof(all_scores::cool_saying)> &prepare_input_saying(std::array<char, sizeof(all_scores::cool_saying)> &buf)
	{
		buf.front() = 0;
		return buf;
	}
};

window_event_result request_user_high_score_comment::event_handler(const d_event &event)
{
	switch (event.type)
	{
		case event_type::window_close:
			{
				std::array<char, sizeof(all_scores::cool_saying)> &text1 = *this;
				strcpy(scores.cool_saying, text1[0] ? text1.data() : "No comment");
			}
			break;
		default:
			break;
	}
	return newmenu::event_handler(event);
}

}

}

namespace dsx {

void scores_maybe_add_player()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	all_scores scores;
	stats_info last_game;

	if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
		return;
	scores_read(&scores);
	auto &player_info = get_local_plrobj().ctype.player_info;
	const auto predicate = [player_mission_score = player_info.mission.score](const stats_info &stats) {
		return player_mission_score > stats.score;
	};
	const auto begin_score_stats = std::begin(scores.stats);
	const auto end_score_stats = std::end(scores.stats);
	/* Find the position at which the player's score should be placed.
	 */
	const auto &&iter_position{std::ranges::find_if(begin_score_stats, end_score_stats, predicate)};
	const auto position = std::distance(begin_score_stats, iter_position);
	/* If iter_position == end_score_stats, then the player's score does
	 * not beat any of the existing high scores.  Include a special case
	 * so that the player's statistics can be shown for the duration of
	 * this menu, despite not being a new record.
	 */
	stats_info *const ptr_last_game = (iter_position == end_score_stats)
		? &last_game
		: nullptr;
	if (ptr_last_game)
	{
		/* Not a new record */
		scores_fill_struct(ptr_last_game);
	} else {
		/* New record - check whether it is the best score.  If so,
		 * allow the player to leave a comment.
		 */
		if (iter_position == begin_score_stats)
		{
			run_blocking_newmenu<request_user_high_score_comment>(scores, grd_curscreen->sc_canvas);
		} else {
			/* New record, but not a new best score.  Tell the player
			 * what slot the new record earned.
			 */
			nm_messagebox(menu_title{TXT_HIGH_SCORE}, {TXT_OK}, "%s %s!", TXT_YOU_PLACED, get_placement_slot_string(position));
		}

		// move everyone down...
		std::move_backward(iter_position, std::prev(end_score_stats), end_score_stats);
		scores_fill_struct(iter_position);
		scores_write(&scores);
	}
	scores_view(grd_curscreen->sc_canvas, ptr_last_game, position);
}

}

namespace dcx {

namespace {

__attribute_nonnull()
static void scores_rputs(grs_canvas &canvas, const grs_font &cv_font, const font_x_scaled_float x, const font_y_scaled_float y, const char *const buffer)
{
	const auto &&[w, h] = gr_get_string_size(cv_font, buffer);
	gr_string(canvas, cv_font, x - w, y, buffer, w, h);
}

static unsigned compute_score_y_coordinate(const unsigned i)
{
	const unsigned y = 59 + i * 9;
	return i ? y : y - 8;
}

}

}

namespace dsx {

namespace {

struct scores_menu_items
{
	struct row
	{
		callsign_t name;
		uint8_t diff_level;
		std::array<char, 16> score;
		std::array<char, 10> levels;
		std::array<char, 14> time_played;
	};
	struct numbered_row : row
	{
		std::array<char, sizeof("10.")> line_number;
	};
	const int citem;
	fix64 time_last_color_change = timer_query();
	uint8_t looper = 0;
	std::array<numbered_row, MAX_HIGH_SCORES> scores;
	row last_game;
	std::array<char, COOL_MESSAGE_LEN> cool_saying;
	static void prepare_row(row &r, const stats_info &stats, std::ostringstream &oss);
	static void prepare_row(numbered_row &r, const stats_info &stats, std::ostringstream &oss, unsigned idx);
	static std::ostringstream build_locale_stringstream();
	void prepare_scores(const all_scores &all_scores, std::ostringstream &oss);
	scores_menu_items(const int citem, const all_scores &all_scores, const stats_info *last_game_stats);
};

void scores_menu_items::prepare_row(row &r, const stats_info &stats, std::ostringstream &oss)
{
	r.name = stats.name;
	r.diff_level = stats.diff_level;
	{
		/* This std::ostringstream is shared among multiple rows, to
		 * avoid reinitializing the std::locale each time.  Clear the
		 * text before inserting the score for this row.
		 */
		oss.str("");
		oss << stats.score;
		auto &&buffer = oss.str();
		//replace the digit '1' with special wider 1
		const auto bb = buffer.begin();
		const auto be = buffer.end();
		auto ri = r.score.begin();
		if (std::distance(bb, be) < r.score.size() - 1)
			ri = std::replace_copy(bb, be, ri, '1', '\x84');
		*ri = 0;
	}
	{
		r.levels.front() = 0;
		auto starting_level = stats.starting_level;
		auto ending_level = stats.ending_level;
		if (starting_level || ending_level)
		{
			const auto secret_start = (starting_level < 0) ? (starting_level = -starting_level, "S") : "";
			const auto secret_end = (ending_level < 0) ? (ending_level = -ending_level, "S") : "";
			auto levels_length = std::snprintf(r.levels.data(), r.levels.size(), "%s%d-%s%d", secret_start, starting_level, secret_end, ending_level);
			auto lb = r.levels.begin();
			std::replace(lb, std::next(lb, levels_length), '1', '\x84');
		}
	}
	{
		const auto &&d1 = std::div(stats.seconds, 60);
		const auto &&d2 = std::div(d1.rem, 60);
		auto time_length = std::snprintf(r.time_played.data(), r.time_played.size(), "%d:%02d:%02d", d1.quot, d2.quot, d2.rem);
		auto tb = r.time_played.begin();
		std::replace(tb, std::next(tb, time_length), '1', '\x84');
	}
}

void scores_menu_items::prepare_row(numbered_row &r, const stats_info &stats, std::ostringstream &oss, const unsigned idx)
{
	std::snprintf(r.line_number.data(), r.line_number.size(), "%u.", idx);
	auto b = r.line_number.begin();
	std::replace(b, std::next(b, 2), '1', '\x84');
	prepare_row(r, stats, oss);
}

std::ostringstream scores_menu_items::build_locale_stringstream()
{
	const auto user_preferred_locale = []() {
		try {
			/* Use the user's locale if possible. */
			return std::locale("");
		} catch (std::runtime_error &) {
			/* Fall back to the default locale if the user's locale
			 * fails to parse.
			 */
			return std::locale();
		}
	}();
	std::ostringstream oss;
	oss.imbue(user_preferred_locale);
	return oss;
}

void scores_menu_items::prepare_scores(const all_scores &all_scores, std::ostringstream &oss)
{
	for (auto &&[idx, sr, si] : enumerate(zip(scores, all_scores.stats), 1u))
		prepare_row(sr, si, oss, idx);
	std::copy(std::begin(all_scores.cool_saying), std::prev(std::end(all_scores.cool_saying)), cool_saying.begin());
	cool_saying.back() = 0;
}

scores_menu_items::scores_menu_items(const int citem, const all_scores &all_scores, const stats_info *const last_game_stats) :
	citem(citem)
{
	auto oss = build_locale_stringstream();
	prepare_scores(all_scores, oss);
	if (last_game_stats)
		prepare_row(last_game, *last_game_stats, oss);
	else
		last_game = {};
}

struct scores_menu : scores_menu_items, window
{
	scores_menu(grs_canvas &src, int x, int y, int w, int h, int citem, const all_scores &scores, const stats_info *last_game) :
		scores_menu_items(citem, scores, last_game),
		window(src, x, y, w, h)
	{
	}
	virtual window_event_result event_handler(const d_event &) override;
	int get_update_looper();
};

static void scores_draw_item(grs_canvas &canvas, const grs_font &cv_font, const score_items_context &shared_item_context, const unsigned shade, const font_y_scaled_float fspacy_y, const scores_menu_items::row &stats)
{
	gr_set_fontcolor(canvas, BM_XRGB(shade, shade, shade), -1);
	if (!stats.name[0u])
	{
		gr_string(canvas, cv_font, shared_item_context.name, fspacy_y, TXT_EMPTY);
		return;
	}
	gr_string(canvas, cv_font, shared_item_context.name, fspacy_y, stats.name);
	scores_rputs(canvas, cv_font, shared_item_context.score, fspacy_y, stats.score.data());

	if (const auto d = build_difficulty_level_from_untrusted(stats.diff_level))
		gr_string(canvas, cv_font, shared_item_context.difficulty, fspacy_y, MENU_DIFFICULTY_TEXT(*d));

	scores_rputs(canvas, cv_font, shared_item_context.levels, fspacy_y, stats.levels.data());
	scores_rputs(canvas, cv_font, shared_item_context.time_played, fspacy_y, stats.time_played.data());
}

window_event_result scores_menu::event_handler(const d_event &event)
{
	int k;
	const auto &&fspacx = FSPACX();
	const auto &&fspacy = FSPACY();

	switch (event.type)
	{
		case event_type::window_activated:
			game_flush_inputs(Controls);
			break;
			
		case event_type::key_command:
			k = event_key_get(event);
			switch( k )	{
				case KEY_CTRLED+KEY_R:		
					if (citem < 0)
					{
						// Reset scores...
						if (nm_messagebox_str(menu_title{nullptr}, nm_messagebox_tie(TXT_NO, TXT_YES), menu_subtitle{TXT_RESET_HIGH_SCORES}) == 1)
						{
							PHYSFS_delete(SCORES_FILENAME);
							all_scores scores{};
							assign_builtin_placeholder_scores(scores);
							auto oss = build_locale_stringstream();
							prepare_scores(scores, oss);
							return window_event_result::handled;
						}
					}
					return window_event_result::handled;
				case KEY_ENTER:
				case KEY_SPACEBAR:
				case KEY_ESC:
					return window_event_result::close;
			}
			break;

		case event_type::mouse_button_down:
		case event_type::mouse_button_up:
			if (event_mouse_get_button(event) == mbtn::left || event_mouse_get_button(event) == mbtn::right)
			{
				return window_event_result::close;
			}
			break;

#if DXX_MAX_BUTTONS_PER_JOYSTICK
		case event_type::joystick_button_down:
			return window_event_result::close;
#endif

		case event_type::idle:
			timer_delay2(50);
			break;

		case event_type::window_draw:
			
			{
				auto &canvas = w_canv;
			nm_draw_background(w_canv, 0, 0, w_canv.cv_bitmap.bm_w, w_canv.cv_bitmap.bm_h);
			auto &medium3_font = *MEDIUM3_FONT;
			const auto border_x = BORDERX;
			const auto border_y = BORDERY;
			gr_string(canvas, medium3_font, 0x8000, border_y, TXT_HIGH_SCORES);
			auto &game_font = *GAME_FONT;
			gr_set_fontcolor(canvas, BM_XRGB(28, 28, 28), -1);
			gr_printf(canvas, game_font, 0x8000, fspacy(16) + border_y, "\"%s\"  - %s", cool_saying.data(), static_cast<const char *>(scores[0].name));
			const font_x_scaled_float fspacx_line_number(fspacx(42) + border_x);
			const score_items_context shared_item_context(fspacx, border_x);
			gr_set_fontcolor(canvas, BM_XRGB(31, 26, 5), -1);
			const auto x_header = fspacx(56) + border_x;
			const auto fspacy_column_labels = fspacy(35) + border_y;
			gr_string(canvas, game_font, x_header, fspacy_column_labels, TXT_NAME);
			gr_string(canvas, game_font, x_header + fspacx(51), fspacy_column_labels, TXT_SCORE);
			gr_string(canvas, game_font, x_header + fspacx(96), fspacy_column_labels, TXT_SKILL);
			gr_string(canvas, game_font, x_header + fspacx(139), fspacy_column_labels, TXT_LEVELS);
			gr_string(canvas, game_font, x_header + fspacx(182), fspacy_column_labels, TXT_TIME);

			if (citem < 0)
				gr_string(canvas, game_font, 0x8000, fspacy(125) + fspacy_column_labels, TXT_PRESS_CTRL_R);

			for (const auto &&[idx, stat] : enumerate(scores))
			{
				const auto shade = (idx == citem)
					? get_update_looper()
					: 28 - idx * 2;
				const unsigned y = compute_score_y_coordinate(idx);
				const font_y_scaled_float fspacy_y(fspacy(y) + border_y);
				scores_draw_item(canvas, game_font, shared_item_context, shade, fspacy_y, stat);
				scores_rputs(canvas, game_font, fspacx_line_number, fspacy_y, stat.line_number.data());
			}
			
			if (citem == MAX_HIGH_SCORES)
			{
				const auto shade = get_update_looper();
				scores_draw_item(canvas, game_font, shared_item_context, shade, fspacy(compute_score_y_coordinate(citem) + 8), last_game);
			}
			}
			break;
		case event_type::window_close:
			break;
		default:
			break;
	}
	return window_event_result::ignored;
}

int scores_menu::get_update_looper()
{
	if (const auto t2 = timer_query(); t2 >= time_last_color_change + F1_0 / 128)
	{
		time_last_color_change = t2;
		if (++ looper >= fades.size())
			looper = 0;
	}
	const auto shade = 7 + fades[looper];
	return shade;
}

void scores_view(grs_canvas &canvas, const stats_info *const last_game, int citem)
{
	const auto &&fspacx290 = FSPACX(290);
	const auto &&fspacy170 = FSPACY(170);
	all_scores scores;
	scores_read(&scores);
	const auto border_x = get_border_x(canvas);
	const auto border_y = get_border_y(canvas);
	auto menu = window_create<scores_menu>(canvas, ((canvas.cv_bitmap.bm_w - fspacx290) / 2) - border_x, ((canvas.cv_bitmap.bm_h - fspacy170) / 2) - border_y, fspacx290 + (border_x * 2), fspacy170 + (border_y * 2), citem, scores, last_game);
	(void)menu;

	newmenu_free_background();

	set_screen_mode(SCREEN_MENU);
	show_menus();
}

}

void scores_view_menu(grs_canvas &canvas)
{
	scores_view(canvas, nullptr, -1);
}

}
