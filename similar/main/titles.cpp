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
 * Routines to display title screens...
 *
 */


#include "dxxsconf.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if DXX_USE_OGL
#include "ogl_init.h"
#endif
#include "pstypes.h"
#include "timer.h"
#include "key.h"
#include "gr.h"
#include "palette.h"
#include "iff.h"
#include "pcx.h"
#include "physfsx.h"
#include "u_mem.h"
#include "joy.h"
#include "titles.h"
#include "gamefont.h"
#include "gameseq.h"
#include "dxxerror.h"
#include "robot.h"
#include "textures.h"
#include "screens.h"
#include "multi.h"
#include "player.h"
#include "digi.h"
#include "text.h"
#include "kmatrix.h"
#include "piggy.h"
#include "songs.h"
#include "newmenu.h"
#include "state.h"
#if defined(DXX_BUILD_DESCENT_II)
#include "movie.h"
#endif
#include "menu.h"
#include "mouse.h"
#include "console.h"
#include "args.h"
#include "strutil.h"

#include "compiler-range_for.h"
#include "partial_range.h"
#include <memory>

#if defined(DXX_BUILD_DESCENT_I)
constexpr std::true_type EMULATING_D1{};
#endif

#if defined(DXX_BUILD_DESCENT_II)
#define	SHAREWARE_ENDING_FILENAME	"ending.tex"
#endif
#define DEFAULT_BRIEFING_BKG		"brief03.pcx"

namespace dcx {

namespace {

static std::array<color_t, 7> Briefing_text_colors;
static color_t *Current_color;
static color_t Erase_color;

// added by Jan Bobrowski for variable-size menu screen
static int rescale_x(const grs_bitmap &cv_bitmap, int x)
{
	return x * cv_bitmap.bm_w / 320;
}

static int rescale_y(const grs_bitmap &cv_bitmap, int y)
{
	return y * cv_bitmap.bm_h / 200;
}

static int get_message_num(const char *&message)
{
	char *p;
	auto num = strtoul(message, &p, 10);
	while (*p && *p++ != '\n')		//	Get and drop eoln
		;
	message = p;
	return num;
}

enum class title_load_location : uint8_t
{
	anywhere,
	from_hog_only,
};

struct title_screen : window
{
	grs_main_bitmap title_bm;
	fix64 timer;
	using window::window;
	virtual window_event_result event_handler(const d_event &) override;
};

window_event_result title_screen::event_handler(const d_event &event)
{
	window_event_result result;

	switch (event.type)
	{
		case EVENT_MOUSE_BUTTON_DOWN:
			if (event_mouse_get_button(event) != 0)
				return window_event_result::ignored;
			else
			{
				return window_event_result::close;
			}
			break;

		case EVENT_KEY_COMMAND:
			if ((result = call_default_handler(event)) == window_event_result::ignored)
				{
					return window_event_result::close;
				}
			return result;

#if DXX_MAX_BUTTONS_PER_JOYSTICK
		case EVENT_JOYSTICK_BUTTON_DOWN:
				return window_event_result::close;
#endif

		case EVENT_IDLE:
			timer_delay2(50);

			if (timer_query() > timer)
			{
				return window_event_result::close;
			}
			break;

		case EVENT_WINDOW_DRAW:
			gr_set_default_canvas();
			show_fullscr(*grd_curcanv, title_bm);
			break;

		case EVENT_WINDOW_CLOSE:
			break;

		default:
			break;
	}
	return window_event_result::ignored;
}

static void show_title_screen(const char *filename)
{
	char new_filename[PATH_MAX] = "";

	auto ts = window_create<title_screen>(grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT);

	strcat(new_filename,filename);
	filename = new_filename;

	ts->timer = timer_query() + i2f(3);
	const auto pcx_error = pcx_read_bitmap_or_default(filename, ts->title_bm, gr_palette);
	if (pcx_error != pcx_result::SUCCESS)
	{
		con_printf(CON_URGENT, "%s:%u: error: loading briefing screen <%s> failed: PCX load error: %s (%u)", __FILE__, __LINE__, filename, pcx_errormsg(pcx_error), static_cast<unsigned>(pcx_error));
	}

	gr_palette_load( gr_palette );
	event_process_all();
}

static void show_title_screen(const char *const filename, title_load_location /* from_hog_only */)
{
	show_title_screen(filename);
}

static void show_first_found_title_screen(const char *oem, const char *share, const char *macshare)
{
	const char *filename = oem;
	if ((PHYSFSX_exists(filename, 1)) ||
		(filename = share, PHYSFSX_exists(filename, 1)) ||
		(filename = macshare, PHYSFSX_exists(filename, 1))
		)
		show_title_screen(filename, title_load_location::from_hog_only);
}

}

}

namespace dsx {
#if defined(DXX_BUILD_DESCENT_II)
int intro_played;
namespace {
static int DefineBriefingBox(const grs_bitmap &, const char *&buf);
}
#endif

void show_titles(void)
{
#if defined(DXX_BUILD_DESCENT_I)
	songs_play_song(SONG_TITLE, 1);
#endif
	if (CGameArg.SysNoTitles)
		return;
#if defined(DXX_BUILD_DESCENT_I)

	show_first_found_title_screen(
		"macplay.pcx",	// Mac Shareware
		"mplaycd.pcx",	// Mac Registered
		"iplogo1.pcx"	// PC. Only down here because it's lowres ;-)
	);
	const bool resolution_at_least_640_480 = (SWIDTH >= 640 && SHEIGHT >= 480);
	auto &logo_hires_pcx = "logoh.pcx";
	auto &descent_hires_pcx = "descenth.pcx";
	show_title_screen((resolution_at_least_640_480 && PHYSFSX_exists(logo_hires_pcx, 1)) ? logo_hires_pcx : "logo.pcx", title_load_location::from_hog_only);
	show_title_screen((resolution_at_least_640_480 && PHYSFSX_exists(descent_hires_pcx, 1)) ? descent_hires_pcx : "descent.pcx", title_load_location::from_hog_only);
#elif defined(DXX_BUILD_DESCENT_II)
	int played=MOVIE_NOT_PLAYED;    //default is not played
	int song_playing = 0;

#define MOVIE_REQUIRED 1	//(!is_D2_OEM && !is_SHAREWARE && !is_MAC_SHARE)	// causes segfault

	const auto hiresmode = HIRESMODE;
	{       //show bundler screens
		played=MOVIE_NOT_PLAYED;        //default is not played

		played = PlayMovie(NULL, "pre_i.mve",0);

		if (!played) {
			char filename[12];
			strcpy(filename, hiresmode ? "pre_i1b.pcx" : "pre_i1.pcx");

			while (PHYSFSX_exists(filename,0))
			{
				show_title_screen(filename, title_load_location::anywhere);
				filename[5]++;
			}
		}
	}

	played = PlayMovie("intro.tex", "intro.mve",MOVIE_REQUIRED);

	if (played != MOVIE_NOT_PLAYED)
		intro_played = 1;
	else
	{                                               //didn't get intro movie, try titles

		played = PlayMovie(NULL, "titles.mve",MOVIE_REQUIRED);

		if (played == MOVIE_NOT_PLAYED)
		{
			con_puts(CON_DEBUG, "Playing title song...");
			songs_play_song( SONG_TITLE, 1);
			song_playing = 1;
			con_puts(CON_DEBUG, "Showing logo screens...");

			show_first_found_title_screen(
				hiresmode ? "iplogo1b.pcx" : "iplogo1.pcx", // OEM
				"iplogo1.pcx", // SHAREWARE
				"mplogo.pcx" // MAC SHAREWARE
				);
			show_first_found_title_screen(
				hiresmode ? "logob.pcx" : "logo.pcx", // OEM
				"logo.pcx", // SHAREWARE
				"plogo.pcx" // MAC SHAREWARE
				);
		}
	}

	{       //show bundler movie or screens
		played=MOVIE_NOT_PLAYED;        //default is not played

		//check if OEM movie exists, so we don't stop the music if it doesn't
		if (RAIIPHYSFS_File{PHYSFS_openRead("oem.mve")})
		{
			played = PlayMovie(NULL, "oem.mve",0);
			song_playing = 0;               //movie will kill sound
		}

		if (!played)
		{
			char filename[12];
			strcpy(filename, hiresmode ? "oem1b.pcx" : "oem1.pcx");
			while (PHYSFSX_exists(filename,0))
			{
				show_title_screen(filename, title_load_location::anywhere);
				filename[3]++;
			}
		}
	}

	if (!song_playing)
	{
		con_puts(CON_DEBUG, "Playing title song...");
		songs_play_song( SONG_TITLE, 1);
	}
	con_puts(CON_DEBUG, "Showing logo screen...");
	const auto filename = hiresmode ? "descentb.pcx" : "descent.pcx";
	if (PHYSFSX_exists(filename,1))
		show_title_screen(filename, title_load_location::from_hog_only);
#endif
}

}

namespace dcx {
namespace {

//-----------------------------------------------------------------------------
struct briefing_screen {
	char    bs_name[13];                //  filename, eg merc01.  Assumes .lbm suffix.
	sbyte   level_num;
	sbyte   message_num;
	short   text_ulx, text_uly;         //  upper left x,y of text window
	short   text_width, text_height;    //  width and height of text window
};

#define	ENDING_LEVEL_NUM_OEMSHARE 0x7f
#define	ENDING_LEVEL_NUM_REGISTER 0x7e

}

static grs_subcanvas_ptr create_spinning_robot_sub_canvas(grs_canvas &canvas)
{
	return gr_create_sub_canvas(canvas, rescale_x(canvas.cv_bitmap, 138), rescale_y(canvas.cv_bitmap, 55), rescale_x(canvas.cv_bitmap, 166), rescale_y(canvas.cv_bitmap, 138));
}

static void get_message_name(const char *&message, std::array<char, 32> &result, const char *const trailer)
{
	auto p = message;
	for (; *p == ' '; ++p)
	{
	}
	const auto e = std::prev(result.end(), sizeof(".bbm"));
	auto i = result.begin();
	char c;
	for (; (c = *p) && c != ' ' && c != '\n'; ++p)
	{
		*i++ = c;
		if (i == e)
			/* Avoid buffer overflow; this was not present in the
			 * original code.
			 */
			break;
	}
	/* This is inconsistent.  If the copy loop terminated on a newline,
	 * then `p` points to the newline and the next loop is skipped.  If
	 * the copy loop terminated on a null, the next loop is attempted,
	 * but exits immediately.  In both those cases, `p` is unchanged.
	 * If the copy loop terminated on any other character, the next loop
	 * will advance `p` to point to a null (consistent with the copy
	 * loop terminating on a null) or past the newline (inconsistent
	 * with the copy loop).
	 *
	 * This inconsistency was present in the prior version of the code,
	 * and is retained in case there exist briefings which rely on this
	 * inconsistency.
	 */
	if (c != '\n')
		while ((c = *p) && (++p, c) != '\n')		//	Get and drop eoln
		{
		}
	message = p;
	strcpy(i, trailer);
}
}

namespace dsx {
namespace {

#if defined(DXX_BUILD_DESCENT_II)

static std::array<briefing_screen, 60> Briefing_screens{{
	{"brief03.pcx",0,3,8,8,257,177}
}}; // default=0!!!
#endif

constexpr briefing_screen D1_Briefing_screens_full[] = {
	{ "brief01.pcx",   0,  1,  13, 140, 290,  59 },
	{ "brief02.pcx",   0,  2,  27,  34, 257, 177 },
	{ "brief03.pcx",   0,  3,  20,  22, 257, 177 },
	{ "brief02.pcx",   0,  4,  27,  34, 257, 177 },
	{ "moon01.pcx",    1,  5,  10,  10, 300, 170 }, // level 1
	{ "moon01.pcx",    2,  6,  10,  10, 300, 170 }, // level 2
	{ "moon01.pcx",    3,  7,  10,  10, 300, 170 }, // level 3
	{ "venus01.pcx",   4,  8,  15, 15, 300,  200 }, // level 4
	{ "venus01.pcx",   5,  9,  15, 15, 300,  200 }, // level 5
	{ "brief03.pcx",   6, 10,  20,  22, 257, 177 },
	{ "merc01.pcx",    6, 11,  10, 15, 300, 200 },  // level 6
	{ "merc01.pcx",    7, 12,  10, 15, 300, 200 },  // level 7
	{ "brief03.pcx",   8, 13,  20,  22, 257, 177 },
	{ "mars01.pcx",    8, 14,  10, 100, 300,  200 }, // level 8
	{ "mars01.pcx",    9, 15,  10, 100, 300,  200 }, // level 9
	{ "brief03.pcx",  10, 16,  20,  22, 257, 177 },
	{ "mars01.pcx",   10, 17,  10, 100, 300,  200 }, // level 10
	{ "jup01.pcx",    11, 18,  10, 40, 300,  200 }, // level 11
	{ "jup01.pcx",    12, 19,  10, 40, 300,  200 }, // level 12
	{ "brief03.pcx",  13, 20,  20,  22, 257, 177 },
	{ "jup01.pcx",    13, 21,  10, 40, 300,  200 }, // level 13
	{ "jup01.pcx",    14, 22,  10, 40, 300,  200 }, // level 14
	{ "saturn01.pcx", 15, 23,  10, 40, 300,  200 }, // level 15
	{ "brief03.pcx",  16, 24,  20,  22, 257, 177 },
	{ "saturn01.pcx", 16, 25,  10, 40, 300,  200 }, // level 16
	{ "brief03.pcx",  17, 26,  20,  22, 257, 177 },
	{ "saturn01.pcx", 17, 27,  10, 40, 300,  200 }, // level 17
	{ "uranus01.pcx", 18, 28,  100, 100, 300,  200 }, // level 18
	{ "uranus01.pcx", 19, 29,  100, 100, 300,  200 }, // level 19
	{ "uranus01.pcx", 20, 30,  100, 100, 300,  200 }, // level 20
	{ "uranus01.pcx", 21, 31,  100, 100, 300,  200 }, // level 21
	{ "neptun01.pcx", 22, 32,  10, 20, 300,  200 }, // level 22
	{ "neptun01.pcx", 23, 33,  10, 20, 300,  200 }, // level 23
	{ "neptun01.pcx", 24, 34,  10, 20, 300,  200 }, // level 24
	{ "pluto01.pcx",  25, 35,  10, 20, 300,  200 }, // level 25
	{ "pluto01.pcx",  26, 36,  10, 20, 300,  200 }, // level 26
	{ "pluto01.pcx",  27, 37,  10, 20, 300,  200 }, // level 27
	{ "aster01.pcx",  -1, 38,  10, 90, 300,  200 }, // secret level -1
	{ "aster01.pcx",  -2, 39,  10, 90, 300,  200 }, // secret level -2
	{ "aster01.pcx",  -3, 40,  10, 90, 300,  200 }, // secret level -3
	{ "end01.pcx",   ENDING_LEVEL_NUM_OEMSHARE,  1,  23, 40, 320, 200 },   //  OEM and shareware end
	{ "end02.pcx",   ENDING_LEVEL_NUM_REGISTER,  1,  5, 5, 300, 200 },    // registered end
	{ "end01.pcx",   ENDING_LEVEL_NUM_REGISTER,  2,  23, 40, 320, 200 },  // registered end
	{ "end03.pcx",   ENDING_LEVEL_NUM_REGISTER,  3,  5, 5, 300, 200 },    // registered end
};

constexpr briefing_screen D1_Briefing_screens_share[] = {
	{ "brief01.pcx",   0,  1,  13, 140, 290,  59 },
	{ "brief02.pcx",   0,  2,  27,  34, 257, 177 },
	{ "brief03.pcx",   0,  3,  20,  22, 257, 177 },
	{ "brief02.pcx",   0,  4,  27,  34, 257, 177 },
	{ "moon01.pcx",    1,  5,  10,  10, 300, 170 }, // level 1
	{ "moon01.pcx",    2,  6,  10,  10, 300, 170 }, // level 2
	{ "moon01.pcx",    3,  7,  10,  10, 300, 170 }, // level 3
	{ "venus01.pcx",   4,  8,  15, 15, 300,  200 }, // level 4
	{ "venus01.pcx",   5,  9,  15, 15, 300,  200 }, // level 5
	{ "brief03.pcx",   6, 10,  20,  22, 257, 177 },
	{ "merc01.pcx",    6, 10,  10, 15, 300, 200 }, // level 6
	{ "merc01.pcx",    7, 11,  10, 15, 300, 200 }, // level 7
	{ "end01.pcx",   ENDING_LEVEL_NUM_OEMSHARE,  1,  23, 40, 320, 200 }, // shareware end
};

struct msgstream
{
	int x;
	int y;
	color_t color;
	char ch;
};

constexpr const briefing_screen *get_d1_briefing_screens(const unsigned descent_hog_size)
{
	if (descent_hog_size == D1_SHAREWARE_MISSION_HOGSIZE || descent_hog_size == D1_SHAREWARE_10_MISSION_HOGSIZE)
		return D1_Briefing_screens_share;
	return D1_Briefing_screens_full;
}

#if defined(DXX_BUILD_DESCENT_I)
using briefing_screen_deleter = std::default_delete<briefing_screen>;
#elif defined(DXX_BUILD_DESCENT_II)
class briefing_screen_deleter : std::default_delete<briefing_screen>
{
	typedef std::default_delete<briefing_screen> base_deleter;
public:
	briefing_screen_deleter() = default;
	briefing_screen_deleter(base_deleter &&b) : base_deleter(std::move(b)) {}
	void operator()(briefing_screen *const p) const
	{
		if (p >= &Briefing_screens.front() && p <= &Briefing_screens.back())
			return;
		this->base_deleter::operator()(p);
	}
};
#endif

struct briefing : window
{
	using window::window;
	virtual window_event_result event_handler(const d_event &) override;
	unsigned streamcount;
	short	level_num;
	short	cur_screen;
	std::unique_ptr<briefing_screen, briefing_screen_deleter> screen;
	grs_main_bitmap background;
#if defined(DXX_BUILD_DESCENT_II)
	int		got_z;
	RAIIdigi_sound		hum_channel, printing_channel;
	MVESTREAM_ptr_t pMovie;
#endif
	std::unique_ptr<char[]>	text;
	const char	*message;
	int		text_x, text_y;
	std::array<msgstream, 2048> messagestream;
	short	tab_stop;
	ubyte	flashing_cursor;
	ubyte	new_page;
	int		new_screen;
#if defined(DXX_BUILD_DESCENT_II)
	ubyte	dumb_adjust;
	ubyte	line_adjustment;
	char	robot_playing;
	short	chattering;
#endif
	fix64		start_time;
	fix64		delay_count;
	int		robot_num;
	grs_subcanvas_ptr	robot_canv;
	vms_angvec	robot_angles;
	std::array<char, 32> bitmap_name;
	grs_main_bitmap  guy_bitmap;
	sbyte   door_dir, door_div_count, animating_bitmap_type;
	sbyte	prev_ch;
	std::array<char, 16> background_name;
};

static void briefing_init(briefing *br, short level_num)
{
	br->level_num = level_num;
	if (EMULATING_D1 && (br->level_num == 1))
		br->level_num = 0;	// for start of game stuff

	br->cur_screen = 0;
	br->background_name.back() = 0;
	strncpy(br->background_name.data(), DEFAULT_BRIEFING_BKG, br->background_name.size() - 1);
#if defined(DXX_BUILD_DESCENT_II)
	br->robot_playing = 0;
#endif
	br->robot_num = 0;
	br->robot_angles = {};
	br->bitmap_name[0] = '\0';
	br->door_dir = 1;
	br->door_div_count = 0;
	br->animating_bitmap_type = 0;
}

//-----------------------------------------------------------------------------
//	Load Descent briefing text.
static int load_screen_text(const d_fname &filename, std::unique_ptr<char[]> &buf)
{
	int len, have_binary = 0;
	auto e = end(filename);
	auto ext = std::find(begin(filename), e, '.');
	if (ext == e)
		return (0);
	if (!d_stricmp(&*ext, ".txb"))
		have_binary = 1;
	
	auto tfile = PHYSFSX_openReadBuffered(filename);
	if (!tfile)
		return (0);

	len = PHYSFS_fileLength(tfile);
	buf = std::make_unique<char[]>(len + 1);
	PHYSFS_read(tfile, buf.get(), 1, len);
#if defined(DXX_BUILD_DESCENT_I)
	const auto endbuf = &buf[len];
#elif defined(DXX_BUILD_DESCENT_II)
	const auto endbuf = std::remove(&buf[0], &buf[len], 13);
#endif
	*endbuf = 0;

	if (have_binary)
		decode_text(buf.get(), len);

	return (1);
}

#if defined(DXX_BUILD_DESCENT_II)
static void set_briefing_fontcolor(struct briefing &br);
static int get_new_message_num(const char *&message)
{
	char *p;
	const auto num = strtoul(message, &p, 10);
	message = p + 1;
	return num;
}
#endif

// Return a pointer to the start of text for screen #screen_num.
static const char * get_briefing_message(const briefing *br, int screen_num)
{
	const char	*tptr = br->text.get();
	int	cur_screen=0;
	int	ch;

	Assert(screen_num >= 0);

	while ( (*tptr != 0 ) && (screen_num != cur_screen)) {
		ch = *tptr++;
		if (ch == '$') {
			ch = *tptr++;
			if (ch == 'S')
				cur_screen = get_message_num(tptr);
		}
	}

	if (screen_num!=cur_screen)
		return (NULL);

	return tptr;
}

static void init_char_pos(briefing *br, int x, int y)
{
	br->text_x = x;
	br->text_y = y;
}

// Make sure the text stays on the screen
// Return 1 if new page required
// 0 otherwise
static int check_text_pos(briefing *br)
{
	if (br->text_x > br->screen->text_ulx + br->screen->text_width)
	{
		br->text_x = br->screen->text_ulx;
		br->text_y += br->screen->text_uly;
	}

	if (br->text_y > br->screen->text_uly + br->screen->text_height)
	{
		br->new_page = 1;
		return 1;
	}

	return 0;
}

static void put_char_delay(const grs_font &cv_font, briefing *const br, const char ch)
{
	char str[2];
	int	w;

	str[0] = ch; str[1] = '\0';
	if (br->delay_count && (timer_query() < br->start_time + br->delay_count))
	{
		br->message--;		// Go back to same character
		return;
	}

	if (br->streamcount >= br->messagestream.size())
		return;
	br->messagestream[br->streamcount].x = br->text_x;
	br->messagestream[br->streamcount].y = br->text_y;
	br->messagestream[br->streamcount].color = *Current_color;
	br->messagestream[br->streamcount].ch = ch;
	br->streamcount++;

	br->prev_ch = ch;
	gr_get_string_size(cv_font, str, &w, nullptr, nullptr);
	br->text_x += w;

#if defined(DXX_BUILD_DESCENT_II)
	if (!EMULATING_D1 && !br->chattering) {
		br->printing_channel.reset(digi_start_sound(digi_xlat_sound(SOUND_BRIEFING_PRINTING), F1_0, 0xFFFF/2, 1, -1, -1, sound_object_none));
		br->chattering=1;
	}
#endif

	br->start_time = timer_query();
}

static void init_spinning_robot(grs_canvas &canvas, briefing &br);
__attribute_warn_unused_result
static int load_briefing_screen(grs_canvas &, briefing *br, const char *fname);

// Process a character for the briefing,
// including special characters preceded by a '$'.
// Return 1 when page is finished, 0 otherwise
static int briefing_process_char(grs_canvas &canvas, briefing *const br)
{
	auto &game_font = *GAME_FONT;
	char ch = *br->message++;
	if (ch == '$') {
		ch = *br->message++;
#if defined(DXX_BUILD_DESCENT_II)
		if (ch=='D') {
			br->cur_screen = DefineBriefingBox(canvas.cv_bitmap, br->message);
			br->screen.reset(&Briefing_screens[br->cur_screen]);
			init_char_pos(br, br->screen->text_ulx, br->screen->text_uly);
			br->line_adjustment=0;
			br->prev_ch = 10;                                   // read to eoln
		} else if (ch=='U') {
			br->cur_screen = get_message_num(br->message);
			br->screen.reset(&Briefing_screens[br->cur_screen]);
			init_char_pos(br, br->screen->text_ulx, br->screen->text_uly);
			br->prev_ch = 10;                                   // read to eoln
		} else
#endif
		if (ch == 'C') {
			auto cc = get_message_num(br->message) - 1;
			if (cc < 0)
				cc = 0;
			else if (cc > Briefing_text_colors.size() - 1)
				cc = Briefing_text_colors.size() - 1;
			Current_color = &Briefing_text_colors[cc];
			br->prev_ch = 10;
		} else if (ch == 'F') {     // toggle flashing cursor
			br->flashing_cursor = !br->flashing_cursor;
			br->prev_ch = 10;
			while (*br->message++ != 10)
				;
		} else if (ch == 'T') {
			br->tab_stop = get_message_num(br->message);
			br->prev_ch = 10;							//	read to eoln
		} else if (ch == 'R') {
			br->robot_canv.reset();
#if defined(DXX_BUILD_DESCENT_II)
			if (br->robot_playing) {
				DeInitRobotMovie(br->pMovie);
				br->robot_playing=0;
			}
#endif

			if (EMULATING_D1) {
				init_spinning_robot(canvas, *br);
				br->robot_num = get_message_num(br->message);
#if defined(DXX_BUILD_DESCENT_II)
				while (*br->message++ != 10)
					;
#endif
			} else {
#if defined(DXX_BUILD_DESCENT_II)
				char spinRobotName[]="RBA.MVE", kludge;  // matt don't change this!

				kludge=*br->message++;
				spinRobotName[2]=kludge; // ugly but proud

				br->robot_playing=InitRobotMovie(spinRobotName, br->pMovie);

				// gr_remap_bitmap_good( &grd_curcanv->cv_bitmap, pal, -1, -1 );

				if (br->robot_playing) {
					RotateRobot(br->pMovie);
					set_briefing_fontcolor(*br);
				}
#endif
			}
			br->prev_ch = 10;                           // read to eoln
		}
		else if ((ch == 'N' && (br->animating_bitmap_type = 0, true)) ||
				(ch == 'O' && (br->animating_bitmap_type = 1, true)))
		{
			br->robot_canv.reset();
			br->prev_ch = 10;
			get_message_name(br->message, br->bitmap_name, "#0");
		} else if (ch=='A') {
#if defined(DXX_BUILD_DESCENT_II)
			br->line_adjustment=1-br->line_adjustment;
#endif
		} else if (ch=='Z') {
#if defined(DXX_BUILD_DESCENT_II)
			std::array<char, 15> fname, fnameb;
			br->got_z=1;
			br->dumb_adjust=1;
			auto p = br->message;
			const auto begin_filename = p;
			const char *separator = nullptr;
			for (char c; (c = *p) != 0 && c != '\n'; ++p)
			{
				if (c == '.' && !separator)
					separator = p;
			}
			const auto end_filename = p;
			/* This is inconsistent, but preserves the original game
			 * logic.
			 */
			if (*p != 10)
				while (*p++ != 10)    //  Get and drop eoln
					;
			br->message = p;
			/* In the original game, if a separator was not found, the
			 * game would have undefined behavior.  Define that case to
			 * mean an early return.
			 */
			if (!separator)
				return 1;
			const std::size_t idx_end_filename = std::distance(begin_filename, end_filename);
			if (idx_end_filename >= fname.size() - 1)
				return 1;
			const std::size_t idx_separator = std::distance(begin_filename, separator);
			if (idx_separator >= fnameb.size() - 6)
				return 1;
			fname[idx_end_filename] = 0;
			std::copy(begin_filename, end_filename, std::begin(fname));
			std::copy_n(begin_filename, idx_separator, std::begin(fnameb));
			std::copy_n("b.pcx", 6, std::next(std::begin(fnameb), idx_separator));
			auto &use_fname = ((HIRESMODE && PHYSFSX_exists(fnameb.data(), 1)) || !PHYSFSX_exists(fname.data(), 1))
				? fnameb
				: fname;
			if (!load_briefing_screen(*grd_curcanv, br, use_fname.data()))
				return 1;
#endif
		} else if (ch == 'B') {
			std::array<char, 32> bitmap_name;
			palette_array_t		temp_palette;
			int		iff_error;
			br->robot_canv.reset();
			get_message_name(br->message, bitmap_name, ".bbm");
			br->guy_bitmap.reset();
			iff_error = iff_read_bitmap(&bitmap_name[0], br->guy_bitmap, &temp_palette);
#if defined(DXX_BUILD_DESCENT_II)
			gr_remap_bitmap_good( br->guy_bitmap, temp_palette, -1, -1 );
#endif
			Assert(iff_error == IFF_NO_ERROR);
			(void)iff_error;

			br->prev_ch = 10;
		} else if (ch == 'S') {
#if defined(DXX_BUILD_DESCENT_II)
			br->chattering = 0;
			br->printing_channel.reset();
#endif

			br->new_screen = 1;
			return 1;
		} else if (ch == 'P') {		//	New page.
#if defined(DXX_BUILD_DESCENT_II)
			if (!br->got_z) {
				Int3(); // Hey ryan!!!! You gotta load a screen before you start
				// printing to it! You know, $Z !!!
				if (!load_briefing_screen(*grd_curcanv, br, HIRESMODE ? "end01b.pcx" : "end01.pcx"))
					return 1;
			}

			br->chattering = 0;
			br->printing_channel.reset();
#endif

			br->new_page = 1;

			while (*br->message != 10) {
				br->message++;	//	drop carriage return after special escape sequence
			}
			br->message++;
			br->prev_ch = 10;

			return 1;
		}
#if defined(DXX_BUILD_DESCENT_II)
		else if (ch == ':') {
			br->prev_ch = 10;
			auto p = br->message;
			/* Legacy clients do not understand $: and will instead show
			 * the remainder of the line.  However, if the next two
			 * characters after $: are $F, legacy clients will treat
			 * the $F as above, toggle the flashing_cursor flag, then
			 * discard the rest of the line.  This special case allows
			 * briefing authors to hide the directive from legacy
			 * clients, but get the same state of flashing_cursor for
			 * both legacy and aware clients.  Briefing authors will
			 * likely need an additional $F nearby to balance this
			 * toggle, but no additional code here is needed to support
			 * that.
			 *
			 * The trailing colon is cosmetic, so that the compatibility
			 * $F is not directly adjacent to the directive.
			 */
			if (!strncmp(p, "$F:", 3))
			{
				br->flashing_cursor = !br->flashing_cursor;
				p += 3;
			}
			auto &rotate_robot_label = "Rebirth.rotate.robot ";
			constexpr auto rotate_robot_len = sizeof(rotate_robot_label) - 1;
			if (!strncmp(p, rotate_robot_label, rotate_robot_len))
			{
				char *p2;
				const auto id = strtoul(p + rotate_robot_len, &p2, 10);
				if (*p2 == '\n')
				{
					p = p2;
					br->robot_canv.reset();
					if (br->robot_playing)
					{
						br->robot_playing = 0;
						DeInitRobotMovie(br->pMovie);
					}
					init_spinning_robot(canvas, *br);
					/* This modifies the appearance of the frame, which
					 * is unfortunate.  However, without it, all robots
					 * come out blue shifted.
					 */
					gr_use_palette_table("groupa.256");
					br->robot_num = id;
				}
			}
			else
			{
				const char *p2 = p;
				/* Suppress non-printing characters.  No need to support
				 * encodings.
				 */
				for (char c; (c = *p2) >= ' ' && c <= '~'; ++p2)
				{
				}
				con_printf(CON_VERBOSE, "warning: unknown briefing directive \"%.*s\"", DXX_ptrdiff_cast_int(p2 - p), p);
			}
			for (char c; (c = *p) && (++p, c) != '\n';)
			{
				/* Discard through newline.  On break, *p is '\0' or
				 * p[-1] is '\n'.
				 */
			}
			br->message = p;
		}
#endif
		else if (ch == '$' || ch == ';') // Print a $/;
			put_char_delay(game_font, br, ch);
	} else if (ch == '\t') {		//	Tab
		const auto &&fspacx = FSPACX();
		if (br->text_x - br->screen->text_ulx < fspacx(br->tab_stop))
			br->text_x = br->screen->text_ulx + fspacx(br->tab_stop);
	} else if ((ch == ';') && (br->prev_ch == 10)) {
		while (*br->message++ != 10)
			;
		br->prev_ch = 10;
	} else if (ch == '\\') {
		br->prev_ch = ch;
	} else if (ch == 10) {
		if (br->prev_ch != '\\') {
			br->prev_ch = ch;
#if defined(DXX_BUILD_DESCENT_II)
			if (br->dumb_adjust)
				br->dumb_adjust--;
			else
#endif
				br->text_y += FSPACY(5)+FSPACY(5)*3/5;
			br->text_x = br->screen->text_ulx;
			if (br->text_y > br->screen->text_uly + br->screen->text_height) {
#if defined(DXX_BUILD_DESCENT_I)
				const auto descent_hog_size = PHYSFSX_fsize("descent.hog");
				auto &bs = get_d1_briefing_screens(descent_hog_size)[br->cur_screen];
#elif defined(DXX_BUILD_DESCENT_II)
				auto &bs = Briefing_screens[br->cur_screen];
#endif
				if (!load_briefing_screen(*grd_curcanv, br, bs.bs_name))
					return 1;
				br->text_x = br->screen->text_ulx;
				br->text_y = br->screen->text_uly;
			}
		} else {
			if (ch == 13)		//Can this happen? Above says ch==10
				Int3();
			br->prev_ch = ch;
		}
	} else {
#if defined(DXX_BUILD_DESCENT_II)
		if (!br->got_z) {
			LevelError("briefing wrote to screen without using $Z to load a screen; loading default.");
			//Int3(); // Hey ryan!!!! You gotta load a screen before you start
			// printing to it! You know, $Z !!!
			if (!load_briefing_screen(*grd_curcanv, br, HIRESMODE ? "end01b.pcx" : "end01.pcx"))
				return 1;
		}
#endif
		put_char_delay(game_font, br, ch);
	}

	return 0;
}

#if defined(DXX_BUILD_DESCENT_I)
static void set_briefing_fontcolor()
#elif defined(DXX_BUILD_DESCENT_II)
static void set_briefing_fontcolor(briefing &br)
#endif
{
	struct rgb
	{
		int r, g, b;
	};
	std::array<rgb, 3> colors;
	if (EMULATING_D1) {
		//green
		colors[0] = {0, 54, 0};
		//white
		colors[1] = {42, 38, 32};
		//Begin D1X addition
		//red
		colors[2] = {63, 0, 0};
	}
	else
	{
		colors[0] = {0, 40, 0};
		colors[1] = {40, 33, 35};
		colors[2] = {8, 31, 54};
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (br.robot_playing)
	{
		colors[0] = {0, 31, 0};
	}
#endif
	Briefing_text_colors[0] = gr_find_closest_color_current(colors[0].r, colors[0].g, colors[0].b);
	Briefing_text_colors[1] = gr_find_closest_color_current(colors[1].r, colors[1].g, colors[1].b);
	Briefing_text_colors[2] = gr_find_closest_color_current(colors[2].r, colors[2].g, colors[2].b);

	//blue
	Briefing_text_colors[3] = gr_find_closest_color_current( 0, 0, 54);
	//gray
	Briefing_text_colors[4] = gr_find_closest_color_current( 14, 14, 14);
	//yellow
	Briefing_text_colors[5] = gr_find_closest_color_current( 54, 54, 0);
	//purple
	Briefing_text_colors[6] = gr_find_closest_color_current( 0, 54, 54);
	//End D1X addition

	Erase_color = gr_find_closest_color_current(0, 0, 0);
}
}

}

static void redraw_messagestream(grs_canvas &canvas, const grs_font &cv_font, const msgstream &stream, unsigned &lastcolor)
{
	char msgbuf[2] = {stream.ch, 0};
	if (lastcolor != stream.color)
	{
		lastcolor = stream.color;
		gr_set_fontcolor(canvas, stream.color, -1);
	}
	gr_string(canvas, cv_font, stream.x + 1, stream.y, msgbuf);
}

namespace dsx {
namespace {
static void flash_cursor(grs_canvas &canvas, const grs_font &cv_font, briefing *const br, const int cursor_flag)
{
	if (cursor_flag == 0)
		return;
	gr_set_fontcolor(canvas, (timer_query() % (F1_0 / 2)) > F1_0 / 4 ? *Current_color : Erase_color, -1);
	gr_string(canvas, cv_font, br->text_x, br->text_y, "_");
}

#define EXIT_DOOR_MAX   14
#define OTHER_THING_MAX 10      // Adam: This is the number of frames in your new animating thing.
#define DOOR_DIV_INIT   6

//-----------------------------------------------------------------------------
static void show_animated_bitmap(grs_canvas &canvas, briefing *br)
{
	grs_bitmap	*bitmap_ptr;
#if DXX_USE_OGL
	float scale = 1.0;

	if ((static_cast<float>(SWIDTH)/320) < (static_cast<float>(SHEIGHT)/200))
		scale = (static_cast<float>(SWIDTH)/320);
	else
		scale = (static_cast<float>(SHEIGHT)/200);
#endif

	// Only plot every nth frame.
	if (br->door_div_count) {
		if (br->bitmap_name[0] != 0) {
			bitmap_index bi;
			bi = piggy_find_bitmap(&br->bitmap_name[0]);
			bitmap_ptr = &GameBitmaps[bi.index];
			PIGGY_PAGE_IN( bi );
#if DXX_USE_OGL
			ogl_ubitmapm_cs(canvas, rescale_x(canvas.cv_bitmap, 220), rescale_y(canvas.cv_bitmap, 45), bitmap_ptr->bm_w * scale, bitmap_ptr->bm_h * scale, *bitmap_ptr, 255, F1_0);
#else
			gr_bitmapm(canvas, rescale_x(canvas.cv_bitmap, 220), rescale_y(canvas.cv_bitmap, 45), *bitmap_ptr);
#endif
		}
		br->door_div_count--;
		return;
	}

	br->door_div_count = DOOR_DIV_INIT;

	if (br->bitmap_name[0] != 0) {
		char		*pound_signp;
		int		num, dig1, dig2;
		bitmap_index bi;
		grs_subcanvas_ptr bitmap_canv;

		switch (br->animating_bitmap_type) {
			case 0:
				bitmap_canv = gr_create_sub_canvas(canvas, rescale_x(canvas.cv_bitmap, 220), rescale_y(canvas.cv_bitmap, 45), 64, 64);
				break;
			case 1:
				bitmap_canv = gr_create_sub_canvas(canvas, rescale_x(canvas.cv_bitmap, 220), rescale_y(canvas.cv_bitmap, 45), 94, 94);
				break; // Adam: Change here for your new animating bitmap thing. 94, 94 are bitmap size.
			default:	Int3(); // Impossible, illegal value for br->animating_bitmap_type
		}

		auto &subcanvas = *bitmap_canv.get();

		pound_signp = strchr(&br->bitmap_name[0], '#');
		Assert(pound_signp != NULL);

		dig1 = *(pound_signp+1);
		dig2 = *(pound_signp+2);
		if (dig2 == 0)
			num = dig1-'0';
		else
			num = (dig1-'0')*10 + (dig2-'0');

		switch (br->animating_bitmap_type) {
			case 0:
				num += br->door_dir;
				if (num > EXIT_DOOR_MAX) {
					num = EXIT_DOOR_MAX;
					br->door_dir = -1;
				} else if (num < 0) {
					num = 0;
					br->door_dir = 1;
				}
				break;
			case 1:
				num++;
				if (num > OTHER_THING_MAX)
					num = 0;
				break;
		}

		Assert(num < 100);
		if (num >= 10) {
			*(pound_signp+1) = (num / 10) + '0';
			*(pound_signp+2) = (num % 10) + '0';
			*(pound_signp+3) = 0;
		} else {
			*(pound_signp+1) = (num % 10) + '0';
			*(pound_signp+2) = 0;
		}

		bi = piggy_find_bitmap(&br->bitmap_name[0]);
		bitmap_ptr = &GameBitmaps[bi.index];
		PIGGY_PAGE_IN( bi );
#if DXX_USE_OGL
		ogl_ubitmapm_cs(subcanvas, 0, 0, bitmap_ptr->bm_w*scale, bitmap_ptr->bm_h*scale, *bitmap_ptr, 255, F1_0);
#else
		gr_bitmapm(subcanvas, 0, 0, *bitmap_ptr);
#endif

		switch (br->animating_bitmap_type) {
			case 0:
				if (num == EXIT_DOOR_MAX) {
					br->door_dir = -1;
					br->door_div_count = 64;
				} else if (num == 0) {
					br->door_dir = 1;
					br->door_div_count = 64;
				}
				break;
			case 1:
				break;
		}
	}
}

}

}

//-----------------------------------------------------------------------------
static void show_briefing_bitmap(grs_canvas &canvas, grs_bitmap *bmp)
{
	const bool hiresmode = HIRESMODE;
	const auto w = static_cast<float>(SWIDTH) / (hiresmode ? 640 : 320);
	const auto h = static_cast<float>(SHEIGHT) / (hiresmode ? 480 : 200);
	const float scale = (w < h) ? w : h;

	auto bitmap_canv = gr_create_sub_canvas(canvas, rescale_x(canvas.cv_bitmap, 220), rescale_y(canvas.cv_bitmap, 55), bmp->bm_w*scale, bmp->bm_h*scale);
	show_fullscr(*bitmap_canv, *bmp);
}

//-----------------------------------------------------------------------------
namespace dsx {
namespace {
static void init_spinning_robot(grs_canvas &canvas, briefing &br) //(int x,int y,int w,int h)
{
	br.robot_canv = create_spinning_robot_sub_canvas(canvas);
}

static void show_spinning_robot_frame(briefing *br, int robot_num)
{
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	if (robot_num != -1) {
		br->robot_angles.p = br->robot_angles.b = 0;
		br->robot_angles.h += 150;

		Assert(Robot_info[robot_num].model_num != -1);
		draw_model_picture(*br->robot_canv.get(), Robot_info[robot_num].model_num, br->robot_angles);
	}
}

//-----------------------------------------------------------------------------
#define KEY_DELAY_DEFAULT       ((F1_0*20)/1000)

__attribute_warn_unused_result
static int init_new_page(briefing *br)
{
	br->new_page = 0;
	br->robot_num = -1;

	const auto r = load_briefing_screen(*grd_curcanv, br, br->background_name.data());
	if (!r)
		return r;
	br->text_x = br->screen->text_ulx;
	br->text_y = br->screen->text_uly;

	br->streamcount=0;
	br->guy_bitmap.reset();

#if defined(DXX_BUILD_DESCENT_II)
	if (br->robot_playing)
	{
		DeInitRobotMovie(br->pMovie);
		br->robot_playing=0;
	}
#endif

	br->start_time = 0;
	br->delay_count = KEY_DELAY_DEFAULT;
	return r;
}

#if defined(DXX_BUILD_DESCENT_II)
static int DefineBriefingBox(const grs_bitmap &cv_bitmap, const char *&buf)
{
	int i=0;
	char name[20];

	const auto n = get_new_message_num (buf);

	assert(n < Briefing_screens.size());

	while (*buf != ' ')
	{
		name[i++]= *buf;
		++buf;
	}

	name[i]='\0';   // slap a delimiter on this guy

	strcpy (Briefing_screens[n].bs_name,name);
	Briefing_screens[n].level_num=get_new_message_num (buf);
	Briefing_screens[n].message_num=get_new_message_num (buf);
	Briefing_screens[n].text_ulx=get_new_message_num (buf);
	Briefing_screens[n].text_uly=get_new_message_num (buf);
	Briefing_screens[n].text_width=get_new_message_num (buf);
	Briefing_screens[n].text_height = get_message_num (buf);  // NOTICE!!!

	Briefing_screens[n].text_ulx = rescale_x(cv_bitmap, Briefing_screens[n].text_ulx);
	Briefing_screens[n].text_uly = rescale_y(cv_bitmap, Briefing_screens[n].text_uly);
	Briefing_screens[n].text_width = rescale_x(cv_bitmap, Briefing_screens[n].text_width);
	Briefing_screens[n].text_height = rescale_y(cv_bitmap, Briefing_screens[n].text_height);

	return (n);
}
#endif

static void free_briefing_screen(briefing *br);

//	-----------------------------------------------------------------------------
//	loads a briefing screen
static int load_briefing_screen(grs_canvas &canvas, briefing *const br, const char *const fname)
{
#if defined(DXX_BUILD_DESCENT_I)
	const auto descent_hog_size = PHYSFSX_fsize("descent.hog");
	char forigin[PATH_MAX];
	decltype(br->background_name) fname2a;

	free_briefing_screen(br);

	snprintf(fname2a.data(), fname2a.size(), "%s", fname);
	snprintf(forigin, sizeof(forigin), "%s", PHYSFS_getRealDir(fname));
	d_strlwr(forigin);

	// check if we have a hires version of this image (not included in PC-version by default)
	// Also if this hires image comes via external AddOn pack, only apply if requested image would be loaded from descent.hog - not a seperate mission which might want to show something else.
	if (SWIDTH >= 640 && SHEIGHT >= 480 && (strstr(forigin,"descent.hog") != NULL))
	{
		const auto fb = fname2a.begin();
		auto ptr = fb;
		for (; const char c = *ptr; ++ptr)
		{
			if (c == '.')
			{
				*ptr = 0;
				break;
			}
		}
		auto &hires_pcx = "h.pcx";
		const size_t len_fname2 = std::distance(fb, ptr);
		if (len_fname2 + sizeof(hires_pcx) < fname2a.size())
		{
			strcpy(ptr, hires_pcx);
			if (!PHYSFSX_exists(fname2a.data(), 1))
				snprintf(fname2a.data(), fname2a.size(), "%s", fname);
		}
	}
	br->background_name = fname2a;
	const auto fname2 = fname2a.data();

	pcx_result pcx_error;
	if ((!d_stricmp(fname2, "brief02.pcx") || !d_stricmp(fname2, "brief02h.pcx")) && cheats.baldguy &&
		bald_guy_load("btexture.xxx", br->background, gr_palette) == pcx_result::SUCCESS)
	{
	}
	else if ((pcx_error = pcx_read_bitmap_or_default(fname2, br->background, gr_palette)) != pcx_result::SUCCESS)
	{
		con_printf(CON_URGENT, "%s:%u: error: loading briefing screen <%s> failed: PCX load error: %s (%u)", __FILE__, __LINE__, fname2, pcx_errormsg(pcx_error), static_cast<unsigned>(pcx_error));
	}

	// Hack: Make sure black parts of robot are shown black
	if (MacPig && gr_palette[0].r == 63 &&
		(!d_stricmp(fname2, "brief03.pcx") || !d_stricmp(fname2, "end01.pcx") ||
		!d_stricmp(fname2, "brief03h.pcx") || !d_stricmp(fname2, "end01h.pcx")
		))
	{
		swap_0_255(br->background);
		gr_palette[0].r = gr_palette[0].g = gr_palette[0].b = 0;
		gr_palette[255].r = gr_palette[255].g = gr_palette[255].b = 63;
	}
	show_fullscr(canvas, br->background);
	gr_palette_load(gr_palette);

	set_briefing_fontcolor();

	br->screen = std::make_unique<briefing_screen>(get_d1_briefing_screens(descent_hog_size)[br->cur_screen]);
	br->screen->text_ulx = rescale_x(canvas.cv_bitmap, br->screen->text_ulx);
	br->screen->text_uly = rescale_y(canvas.cv_bitmap, br->screen->text_uly);
	br->screen->text_width = rescale_x(canvas.cv_bitmap, br->screen->text_width);
	br->screen->text_height = rescale_y(canvas.cv_bitmap, br->screen->text_height);
	init_char_pos(br, br->screen->text_ulx, br->screen->text_uly);
#elif defined(DXX_BUILD_DESCENT_II)
	free_briefing_screen(br);
	const auto bndata = br->background_name.data();
	if (fname != bndata)
	{
		br->background_name.back() = 0;
		strncpy(bndata, fname, br->background_name.size() - 1);
	}

	pcx_result pcx_error;
	if ((pcx_error = pcx_read_bitmap_or_default(fname, br->background, gr_palette)) != pcx_result::SUCCESS)
	{
		con_printf(CON_URGENT, "%s:%u: error: loading briefing screen <%s> failed: PCX load error: %s (%u)", __FILE__, __LINE__, fname, pcx_errormsg(pcx_error), static_cast<unsigned>(pcx_error));
		return 0;
	}
	show_fullscr(canvas, br->background);
	if (EMULATING_D1 && !d_stricmp(fname, "brief03.pcx")) // HACK, FIXME: D1 missions should use their own palette (PALETTE.256), but texture replacements not complete
		gr_use_palette_table("groupa.256");

	gr_palette_load(gr_palette);

	set_briefing_fontcolor(*br);

	if (EMULATING_D1)
	{
		br->got_z = 1;
		br->screen = std::make_unique<briefing_screen>(Briefing_screens[br->cur_screen]);
		br->screen->text_ulx = rescale_x(canvas.cv_bitmap, br->screen->text_ulx);
		br->screen->text_uly = rescale_y(canvas.cv_bitmap, br->screen->text_uly);
		br->screen->text_width = rescale_x(canvas.cv_bitmap, br->screen->text_width);
		br->screen->text_height = rescale_y(canvas.cv_bitmap, br->screen->text_height);
		init_char_pos(br, br->screen->text_ulx, br->screen->text_uly);
	}

#endif
	return 1;
}

static void free_briefing_screen(briefing *br)
{
	br->background.reset();
#if defined(DXX_BUILD_DESCENT_II)
	if (br->robot_playing)
	{
		DeInitRobotMovie(br->pMovie);
		br->robot_playing=0;
	}
#endif
	br->robot_canv.reset();
#if defined(DXX_BUILD_DESCENT_II)
	br->printing_channel.reset();
#endif
	if (EMULATING_D1)
		br->screen.reset();
}

__attribute_warn_unused_result
static int new_briefing_screen(briefing *br, int first)
{
	br->new_screen = 0;
	const auto descent_hog_size = PHYSFSX_fsize("descent.hog");
	const auto num_d1_briefing_screens = (
		(descent_hog_size == D1_SHAREWARE_MISSION_HOGSIZE || descent_hog_size == D1_SHAREWARE_10_MISSION_HOGSIZE)
		? std::size(D1_Briefing_screens_share)
		: std::size(D1_Briefing_screens_full)
	);
#if defined(DXX_BUILD_DESCENT_I)

	if (!first)
		br->cur_screen++;

	auto &&d1_briefing_screens = get_d1_briefing_screens(descent_hog_size);
	while (br->cur_screen < num_d1_briefing_screens && d1_briefing_screens[br->cur_screen].level_num != br->level_num)
	{
		br->cur_screen++;
		if (br->cur_screen == num_d1_briefing_screens && br->level_num == 0)
		{
			// Showed the pre-game briefing, now show level 1 briefing
			br->level_num++;
			br->cur_screen = 0;
		}
	}

	if (br->cur_screen == num_d1_briefing_screens)
		return 0;		// finished

	if (!load_briefing_screen(*grd_curcanv, br, d1_briefing_screens[br->cur_screen].bs_name))
		return 0;

	br->message = get_briefing_message(br, d1_briefing_screens[br->cur_screen].message_num);
#elif defined(DXX_BUILD_DESCENT_II)
	br->got_z = 0;

	if (EMULATING_D1)
	{
		auto &&d1_briefing_screens = get_d1_briefing_screens(descent_hog_size);
		if (!first)
			br->cur_screen++;
		else
			for (int i = 0; i < num_d1_briefing_screens; i++)
				Briefing_screens[i] = d1_briefing_screens[i];

		while (br->cur_screen < num_d1_briefing_screens && Briefing_screens[br->cur_screen].level_num != br->level_num)
		{
			br->cur_screen++;
			if (br->cur_screen == num_d1_briefing_screens && br->level_num == 0)
			{
				// Showed the pre-game briefing, now show level 1 briefing
				br->level_num++;
				br->cur_screen = 0;
			}
		}

		if (br->cur_screen == num_d1_briefing_screens)
			return 0;		// finished

		if (!load_briefing_screen(*grd_curcanv, br, Briefing_screens[br->cur_screen].bs_name))
			return 0;
	}
	else if (first)
	{
		br->cur_screen = br->level_num;
		br->screen.reset(&Briefing_screens[0]);
		init_char_pos(br, br->screen->text_ulx, br->screen->text_uly-(8*(1+HIRESMODE)));
	}
	else
		return 0;	// finished

	br->message = get_briefing_message(br, EMULATING_D1 ? Briefing_screens[br->cur_screen].message_num : br->cur_screen);
	br->printing_channel.reset();
	br->dumb_adjust = 0;
	br->line_adjustment = 1;
	br->chattering = 0;
	br->robot_playing=0;
#endif

	if (br->message==NULL)
		return 0;

	Current_color = &Briefing_text_colors.front();
	br->streamcount = 0;
	br->tab_stop = 0;
	br->flashing_cursor = 0;
	br->new_page = 0;
	br->start_time = 0;
	br->delay_count = KEY_DELAY_DEFAULT;
	br->robot_num = -1;
	br->bitmap_name[0] = 0;
	br->guy_bitmap.reset();
	br->prev_ch = -1;

#if defined(DXX_BUILD_DESCENT_II)
	if (songs_is_playing() == -1 && !br->hum_channel)
		br->hum_channel.reset(digi_start_sound(digi_xlat_sound(SOUND_BRIEFING_HUM), F1_0/2, 0xFFFF/2, 1, -1, -1, sound_object_none));
#endif

	return 1;
}

}

//-----------------------------------------------------------------------------
window_event_result briefing::event_handler(const d_event &event)
{
	window_event_result result;

	switch (event.type)
	{
		case EVENT_WINDOW_ACTIVATED:
		case EVENT_WINDOW_DEACTIVATED:
			key_flush();
			break;

		case EVENT_MOUSE_BUTTON_DOWN:
			if (event_mouse_get_button(event) == 0)
			{
				if (this->new_screen)
				{
					if (!new_briefing_screen(this, 0))
					{
						return window_event_result::close;
					}
				}
				else if (this->new_page)
				{
					if (!init_new_page(this))
						return window_event_result::close;
				}
				else
					this->delay_count = 0;
				return window_event_result::handled;
			}
			break;

#if DXX_MAX_BUTTONS_PER_JOYSTICK
		case EVENT_JOYSTICK_BUTTON_DOWN:
			// using joy_translate_menu_key doesn't work here for unclear
			// reasons, so we build a reasonable facsimile right here
			if (event_joystick_get_button(event) == 1)
				return window_event_result::close;
			if (this->new_screen)
			{
				if (!new_briefing_screen(this, 0))
				{
					return window_event_result::close;
				}
			}
			else if (this->new_page)
			{
				if (!init_new_page(this))
					return window_event_result::close;
			}
			else
				this->delay_count = 0;
			return window_event_result::handled;
#endif

		case EVENT_KEY_COMMAND:
		{
			int key = event_key_get(event);

			switch (key)
			{
#if defined(DXX_BUILD_DESCENT_I)
				case KEY_ALTED + KEY_B: // B - ALTED... BALT... BALD... get it? 
					cheats.baldguy = !cheats.baldguy;
					break;
#endif
				case KEY_ESC:
					return window_event_result::close;
				case KEY_SPACEBAR:
				case KEY_ENTER:
					this->delay_count = 0;
					DXX_BOOST_FALLTHROUGH;
				default:
					if ((result = call_default_handler(event)) != window_event_result::ignored)
						return result;
					else if (this->new_screen)
					{
						if (!new_briefing_screen(this, 0))
						{
							return window_event_result::close;
						}
					}
					else if (this->new_page)
					{
						if (!init_new_page(this))
							return window_event_result::close;
					}
					break;
			}
			break;
		}

		case EVENT_WINDOW_DRAW:
		{
			gr_set_default_canvas();
			auto &canvas = *grd_curcanv;

			timer_delay2(50);

			if (!(this->new_screen || this->new_page))
				while (!briefing_process_char(canvas, this) && !this->delay_count)
				{
					check_text_pos(this);
					if (this->new_page)
						break;
				}
			check_text_pos(this);

			if (this->background.bm_data)
				show_fullscr(canvas, this->background);

			if (this->guy_bitmap.bm_data)
				show_briefing_bitmap(canvas, &this->guy_bitmap);
			if (this->bitmap_name[0] != 0)
				show_animated_bitmap(canvas, this);
#if defined(DXX_BUILD_DESCENT_II)
			if (this->robot_playing)
				RotateRobot(this->pMovie);
#endif
			if (this->robot_num != -1)
				show_spinning_robot_frame(this, this->robot_num);

			auto &game_font = *GAME_FONT;

			gr_set_fontcolor(canvas, *Current_color, -1);
			{
				unsigned lastcolor = ~0u;
				range_for (const auto b, partial_const_range(this->messagestream, this->streamcount))
					redraw_messagestream(canvas, game_font, b, lastcolor);
			}

			if (this->new_page || this->new_screen)
				flash_cursor(canvas, game_font, this, this->flashing_cursor);
			else if (this->flashing_cursor)
				gr_string(canvas, game_font, this->text_x, this->text_y, "_");
			break;
		}
		case EVENT_WINDOW_CLOSE:
			free_briefing_screen(this);
#if defined(DXX_BUILD_DESCENT_II)
			this->hum_channel.reset();
#endif
			break;

		default:
			break;
	}
	return window_event_result::ignored;
}

void do_briefing_screens(const d_fname &filename, int level_num)
{
	if (!*static_cast<const char *>(filename))
		return;

	auto br = window_create<briefing>(grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT);
	briefing_init(br, level_num);

	if (!load_screen_text(filename, br->text))
	{
		return;
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (!(EMULATING_D1 || is_SHAREWARE || is_MAC_SHARE || is_D2_OEM || !PLAYING_BUILTIN_MISSION))
		songs_stop_all();
	else
#endif
	{
		if ((songs_is_playing() != SONG_BRIEFING) && (songs_is_playing() != SONG_ENDGAME))
			songs_play_song( SONG_BRIEFING, 1 );
	}

#if defined(DXX_BUILD_DESCENT_I)
	set_screen_mode( SCREEN_MENU );
#elif defined(DXX_BUILD_DESCENT_II)
	// set screen correctly for robot movies
	set_screen_mode( SCREEN_MOVIE );
#endif

	gr_set_default_canvas();

	if (!new_briefing_screen(br, 1))
	{
		window_close(br);
	}
	else
	// Stay where we are in the stack frame until briefing done
	// Too complicated otherwise
		event_process_all();
}

void do_end_briefing_screens(const d_fname &filename)
{
	int level_num_screen = Current_level_num;

	if (!*static_cast<const char *>(filename))
		return; // no filename, no ending

	if (EMULATING_D1)
	{
		unsigned song;
		if (d_stricmp(filename, BIMD1_ENDING_FILE_OEM) == 0)
		{
			song = SONG_ENDGAME;
			level_num_screen = ENDING_LEVEL_NUM_OEMSHARE;
		}
		else if (d_stricmp(filename, BIMD1_ENDING_FILE_SHARE) == 0)
		{
			song = SONG_BRIEFING;
			level_num_screen = ENDING_LEVEL_NUM_OEMSHARE;
		}
		else
		{
			song = SONG_ENDGAME;
			level_num_screen = ENDING_LEVEL_NUM_REGISTER;
		}
		songs_play_song(song, 1);
	}
#if defined(DXX_BUILD_DESCENT_II)
	else if (PLAYING_BUILTIN_MISSION)
	{
		unsigned song;
		if ((d_stricmp(filename, BIMD2_ENDING_FILE_OEM) == 0 && (song = SONG_TITLE, true)) ||
			(d_stricmp(filename, BIMD2_ENDING_FILE_SHARE) == 0 && (song = SONG_ENDGAME, true)))
		{
			songs_play_song(song, 1);
			level_num_screen = 1;
		}
	}
	else
	{
		songs_play_song( SONG_ENDGAME, 1 );
		level_num_screen = Last_level + 1;
	}
#endif

	do_briefing_screens(filename, level_num_screen);
}

}
