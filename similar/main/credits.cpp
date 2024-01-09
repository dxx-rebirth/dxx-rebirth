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
 * Routines to display the credits.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <utility>

#include "dxxerror.h"
#include "pstypes.h"
#include "gr.h"
#include "window.h"
#include "key.h"
#include "mouse.h"
#include "palette.h"
#include "timer.h"
#include "gamefont.h"
#include "pcx.h"
#include "credits.h"
#include "u_mem.h"
#include "screens.h"
#include "text.h"
#include "songs.h"
#include "menu.h"
#include "config.h"
#include "physfsx.h"
#if defined(DXX_BUILD_DESCENT_II)
#include "mission.h"
#include "gamepal.h"
#include "args.h"
#endif
#include "piggy.h"
#include <memory>

#define ROW_SPACING			(SHEIGHT / 17)
#define NUM_LINES			20 //14
#if defined(DXX_BUILD_DESCENT_I)
#define MAKE_CREDITS_PAIR(F)	std::span<const char, sizeof(F) + 1>(F ".tex", sizeof(F) + 1)
#define CREDITS_FILE 			MAKE_CREDITS_PAIR("credits")
#elif defined(DXX_BUILD_DESCENT_II)
#define MAKE_CREDITS_PAIR(F)	std::span<const char>(F ".tex", sizeof(F) + 1)
#define CREDITS_FILE    		(	\
	PHYSFSX_exists("mcredits.tex", 1)	\
		? MAKE_CREDITS_PAIR("mcredits")	\
		: PHYSFSX_exists("ocredits.tex", 1)	\
			? MAKE_CREDITS_PAIR("ocredits") 	\
			: MAKE_CREDITS_PAIR("credits")	\
	)
#define ALLOWED_CHAR			(!Current_mission ? 'R' : (is_SHAREWARE ? 'S' : 'R'))
#endif

namespace dcx {

namespace {

struct credits_window : window
{
	RAIIPHYSFS_File file;
	const uint8_t have_bin_file;
	std::array<PHYSFSX_gets_line_t<80>, NUM_LINES> buffer{};
	int buffer_line = 0;
	int first_line_offset = 0;
	int extra_inc = 0;
	int done = 0;
	int row = 0;
	grs_main_bitmap backdrop{};
	credits_window(grs_canvas &src, int x, int y, int w, int h, RAIIPHYSFS_File f, const uint8_t bin_file) :
		window(src, x, y, w, h), file(std::move(f)), have_bin_file(bin_file)
	{
	}
};

}

}

namespace dsx {

namespace {

struct credits_window : ::dcx::credits_window
{
	using ::dcx::credits_window::credits_window;
	virtual window_event_result event_handler(const d_event &) override;
};

window_event_result credits_window::event_handler(const d_event &event)
{
	int l, y;
	window_event_result result;

	switch (event.type)
	{
		case event_type::key_command:
			if ((result = call_default_handler(event)) == window_event_result::ignored)	// if not print screen, debug etc
			{
				return window_event_result::close;
			}
			return result;

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
			if (done > NUM_LINES)
			{
				return window_event_result::close;
			}
			break;
		case event_type::window_draw:
			{			
			if (row == 0)
			{
				do {
					buffer_line = (buffer_line + 1) % NUM_LINES;
#if defined(DXX_BUILD_DESCENT_II)
				get_line:;
#endif
					if (PHYSFSX_fgets(buffer[buffer_line], file))
					{
						char *p;
						if (have_bin_file) // is this a binary tbl file
							decode_text_line (buffer[buffer_line]);
#if defined(DXX_BUILD_DESCENT_I)
						p = strchr(&buffer[buffer_line][0],'\n');
						if (p) *p = '\0';
#elif defined(DXX_BUILD_DESCENT_II)
						p = buffer[buffer_line];
						if (p[0] == ';')
							goto get_line;
						
						if (p[0] == '%')
						{
							if (p[1] == ALLOWED_CHAR)
							{
								for (int i = 0; p[i]; i++)
									p[i] = p[i+2];
							}
							else
								goto get_line;
						}
#endif	
					} else	{
						//fseek( file, 0, SEEK_SET);
						buffer[buffer_line][0] = 0;
						done++;
					}
				} while (extra_inc--);
				extra_inc = 0;
			}

			// cheap but effective: towards end of credits sequence, fade out the music volume
			if (done >= NUM_LINES-16)
			{
				static int curvol = -10; 
				if (curvol == -10) 
					curvol = CGameCfg.MusicVolume;
				const auto limitvol = (NUM_LINES - done) / 2;
				if (curvol > limitvol)
				{
					curvol = limitvol;
					songs_set_volume(curvol);
				}
			}
			
			y = first_line_offset - row;
			auto &canvas = w_canv;
			show_fullscr(canvas, backdrop);
			for (uint_fast32_t j=0; j != NUM_LINES; ++j, y += ROW_SPACING)
			{
				l = (buffer_line + j + 1 ) %  NUM_LINES;
				const char *s = buffer[l];
				if (!s)
					continue;
				
				const grs_font *cv_font;
				if ( s[0] == '!' ) {
					s++;
					cv_font = canvas.cv_font;
				} else
				{
					const auto c = s[0];
					cv_font = (c == '$'
						? (++s, HUGE_FONT)
						: (c == '*')
							? (++s, MEDIUM3_FONT)
							: MEDIUM2_FONT
						).get();
				}
				
				const auto tempp = strchr( s, '\t' );
				if ( !tempp )	{
					// Wacky Fast Credits thing
					gr_string(canvas, *cv_font, 0x8000, y, s);
				}
			}
			
			row += SHEIGHT / 200;
			if (row >= ROW_SPACING)
				row = 0;
			break;
			}

		case event_type::window_close:
			songs_set_volume(CGameCfg.MusicVolume);
			songs_play_song( SONG_TITLE, 1 );
			break;
		default:
			break;
	}
	return window_event_result::ignored;
}

static void credits_show_common(RAIIPHYSFS_File file, const int have_bin_file)
{
	palette_array_t backdrop_palette;
	auto cr = window_create<credits_window>(grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, std::move(file), have_bin_file);

	set_screen_mode(SCREEN_MENU);
#if defined(DXX_BUILD_DESCENT_II)
	gr_use_palette_table( "credits.256" );
#endif

	pcx_read_bitmap_or_default(STARS_BACKGROUND, cr->backdrop, backdrop_palette);
	songs_play_song( SONG_CREDITS, 1 );

	gr_remap_bitmap_good(cr->backdrop,backdrop_palette, -1, -1);

	show_fullscr(cr->w_canv, cr->backdrop);
	gr_palette_load( gr_palette );

	key_flush();
	event_process_all();
}

}

void credits_show(const char *const filename)
{
	if (auto &&file = PHYSFSX_openReadBuffered(filename).first)
		credits_show_common(std::move(file), 1);
}

void credits_show()
{
	const auto &&credits_file = CREDITS_FILE;
	int have_bin_file = 0;
	auto &&[file, physfserr] = PHYSFSX_openReadBuffered(credits_file.data());
	if (!file)
	{
		char nfile[32];
		snprintf(nfile, sizeof(nfile), "%.*sxb", static_cast<int>(credits_file.size()), credits_file.data());
		auto &&[file2, physfserr2] = PHYSFSX_openReadBuffered(nfile);
		if (!file2)
			Error("Failed to open CREDITS.TEX and CREDITS.TXB file: \"%s\", \"%s\"\n", PHYSFS_getErrorByCode(physfserr), PHYSFS_getErrorByCode(physfserr2));
		file = std::move(file2);
		have_bin_file = 1;
	}
	credits_show_common(std::move(file), have_bin_file);
}

}
