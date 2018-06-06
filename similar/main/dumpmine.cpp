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
 * Functions to dump text description of mine.
 * An editor-only function, called at mine load time.
 * To be read by a human to verify the correctness and completeness of a mine.
 *
 */

#include <bitset>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "pstypes.h"
#include "console.h"
#include "physfsx.h"
#include "key.h"
#include "gr.h"
#include "palette.h"
#include "fmtcheck.h"

#include "inferno.h"
#if DXX_USE_EDITOR
#include "editor/editor.h"
#endif
#include "dxxerror.h"
#include "object.h"
#include "wall.h"
#include "gamemine.h"
#include "gameseg.h"
#include "robot.h"
#include "player.h"
#include "newmenu.h"
#include "textures.h"

#include "bm.h"
#include "menu.h"
#include "switch.h"
#include "fuelcen.h"
#include "powerup.h"
#include "gameseq.h"
#include "polyobj.h"
#include "gamesave.h"
#include "piggy.h"

#include "compiler-range_for.h"
#include "partial_range.h"
#include "segiter.h"

#if DXX_USE_EDITOR

namespace dsx {
namespace {
#if defined(DXX_BUILD_DESCENT_I)
using perm_tmap_buffer_type = array<int, MAX_TEXTURES>;
using level_tmap_buffer_type = array<int8_t, MAX_TEXTURES>;
using wall_buffer_type = array<int, MAX_WALL_ANIMS>;
#elif defined(DXX_BUILD_DESCENT_II)
using perm_tmap_buffer_type = array<int, MAX_BITMAP_FILES>;
using level_tmap_buffer_type = array<int8_t, MAX_BITMAP_FILES>;
using wall_buffer_type = array<int, MAX_BITMAP_FILES>;
#endif
}
}

namespace dcx {
const array<char[10], 7> Wall_names{{
	"NORMAL   ",
	"BLASTABLE",
	"DOOR     ",
	"ILLUSION ",
	"OPEN     ",
	"CLOSED   ",
	"EXTERNAL "
}};
}

static void dump_used_textures_level(PHYSFS_File *my_file, int level_num);
static void say_totals(fvcobjptridx &vcobjptridx, PHYSFS_File *my_file, const char *level_name);

namespace dsx {
const array<char[9], MAX_OBJECT_TYPES> Object_type_names{{
	"WALL    ",
	"FIREBALL",
	"ROBOT   ",
	"HOSTAGE ",
	"PLAYER  ",
	"WEAPON  ",
	"CAMERA  ",
	"POWERUP ",
	"DEBRIS  ",
	"CNTRLCEN",
	"FLARE   ",
	"CLUTTER ",
	"GHOST   ",
	"LIGHT   ",
	"COOP    ",
#if defined(DXX_BUILD_DESCENT_II)
	"MARKER  ",
#endif
}};

// ----------------------------------------------------------------------------
static const char *object_types(const object_base &objp)
{
	const auto type = objp.type;
	assert(type == OBJ_NONE || type < MAX_OBJECT_TYPES);
	return &Object_type_names[type][0];
}
}

// ----------------------------------------------------------------------------
static const char *object_ids(const object_base &objp)
{
	switch (objp.type)
	{
		case OBJ_ROBOT:
			return Robot_names[get_robot_id(objp)].data();
		case OBJ_POWERUP:
			return Powerup_names[get_powerup_id(objp)].data();
		default:
			return nullptr;
	}
}

static void err_puts(PHYSFS_File *f, const char *str, size_t len) __attribute_nonnull();
static void err_puts(PHYSFS_File *f, const char *str, size_t len)
#define err_puts(A1,S,...)	(err_puts(A1,S, _dxx_call_puts_parameter2(1, ## __VA_ARGS__, strlen(S))))
{
	con_puts(CON_CRITICAL, str, len);
	PHYSFSX_puts(f, str);
	Errors_in_mine++;
}

template <size_t len>
static void err_puts_literal(PHYSFS_File *f, const char (&str)[len]) __attribute_nonnull();
template <size_t len>
static void err_puts_literal(PHYSFS_File *f, const char (&str)[len])
{
	err_puts(f, str, len);
}

static void err_printf(PHYSFS_File *my_file, const char * format, ... ) __attribute_format_printf(2, 3);
static void err_printf(PHYSFS_File *my_file, const char * format, ... )
#define err_printf(A1,F,...)	dxx_call_printf_checked(err_printf,err_puts_literal,(A1),(F),##__VA_ARGS__)
{
	va_list	args;
	char		message[256];

	va_start(args, format );
	size_t len = vsnprintf(message,sizeof(message),format,args);
	va_end(args);
	err_puts(my_file, message, len);
}

static void warning_puts(PHYSFS_File *f, const char *str, size_t len) __attribute_nonnull();
static void warning_puts(PHYSFS_File *f, const char *str, size_t len)
#define warning_puts(A1,S,...)	(warning_puts(A1,S, _dxx_call_puts_parameter2(1, ## __VA_ARGS__, strlen(S))))
{
	con_puts(CON_URGENT, str, len);
	PHYSFSX_puts(f, str);
}

template <size_t len>
static void warning_puts_literal(PHYSFS_File *f, const char (&str)[len]) __attribute_nonnull();
template <size_t len>
static void warning_puts_literal(PHYSFS_File *f, const char (&str)[len])
{
	warning_puts(f, str, len);
}

static void warning_printf(PHYSFS_File *my_file, const char * format, ... ) __attribute_format_printf(2, 3);
static void warning_printf(PHYSFS_File *my_file, const char * format, ... )
#define warning_printf(A1,F,...)	dxx_call_printf_checked(warning_printf,warning_puts_literal,(A1),(F),##__VA_ARGS__)
{
	va_list	args;
	char		message[256];

	va_start(args, format );
	vsnprintf(message,sizeof(message),format,args);
	va_end(args);
	warning_puts(my_file, message);
}

// ----------------------------------------------------------------------------
namespace dsx {
static void write_exit_text(fvcsegptridx &vcsegptridx, fvcwallptridx &vcwallptridx, PHYSFS_File *my_file)
{
	int	j, count;

	PHYSFSX_printf(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_printf(my_file, "Exit stuff\n");

	//	---------- Find exit triggers ----------
	count=0;
	range_for (const auto &&t, vctrgptridx)
	{
		if (trigger_is_exit(t))
		{
			int	count2;

			const auto i = t.get_unchecked_index();
			PHYSFSX_printf(my_file, "Trigger %2i, is an exit trigger with %i links.\n", i, t->num_links);
			count++;
			if (t->num_links != 0)
				err_printf(my_file, "Error: Exit triggers must have 0 links, this one has %i links.", t->num_links);

			//	Find wall pointing to this trigger.
			count2 = 0;
			range_for (const auto &&w, vcwallptridx)
			{
				if (w->trigger == i)
				{
					count2++;
					PHYSFSX_printf(my_file, "Exit trigger %i is in segment %i, on side %i, bound to wall %i\n", i, w->segnum, w->sidenum, static_cast<wallnum_t>(w));
				}
			}
			if (count2 == 0)
				err_printf(my_file, "Error: Trigger %i is not bound to any wall.", i);
			else if (count2 > 1)
				err_printf(my_file, "Error: Trigger %i is bound to %i walls.", i, count2);

		}
	}

	if (count == 0)
		err_printf(my_file, "Error: No exit trigger in this mine.");
	else if (count != 1)
		err_printf(my_file, "Error: More than one exit trigger in this mine.");
	else
		PHYSFSX_printf(my_file, "\n");

	//	---------- Find exit doors ----------
	count = 0;
	range_for (const auto &&segp, vcsegptridx)
	{
		for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
			if (segp->children[j] == segment_exit)
			{
				PHYSFSX_printf(my_file, "Segment %3hu, side %i is an exit door.\n", static_cast<uint16_t>(segp), j);
				count++;
			}
	}

	if (count == 0)
		err_printf(my_file, "Error: No external wall in this mine.");
	else if (count != 1) {
#if defined(DXX_BUILD_DESCENT_I)
		warning_printf(my_file, "Warning: %i external walls in this mine.", count);
		warning_printf(my_file, "(If %i are secret exits, then no problem.)", count-1); 
#endif
	} else
		PHYSFSX_printf(my_file, "\n");
}
}

namespace {

class key_stat
{
	const char *const label;
	unsigned wall_count = 0, powerup_count = 0;
	segnum_t seg = segment_none;
	uint8_t side = 0;
public:
	key_stat(const char *const p) :
		label(p)
	{
	}
	void check_wall(const segment_array &segments, PHYSFS_File *const fp, const vcwallptridx_t wpi, const wall_key_t key)
	{
		auto &w = *wpi;
		if (!(w.keys & key))
			return;
		PHYSFSX_printf(fp, "Wall %i (seg=%i, side=%i) is keyed to the %s key.\n", static_cast<wallnum_t>(wpi), w.segnum, w.sidenum, label);
		if (seg == segment_none)
		{
			seg = w.segnum;
			side = w.sidenum;
		}
		else
		{
			const auto &&connect_side = find_connect_side(segments.vcptridx(w.segnum), segments.vcptr(seg));
			if (connect_side == side)
				return;
			warning_printf(fp, "Warning: This door at seg %i, is different than the one at seg %i, side %i", w.segnum, seg, side);
		}
		++wall_count;
	}
	void report_walls(PHYSFS_File *const fp) const
	{
		if (wall_count > 1)
			warning_printf(fp, "Warning: %i doors are keyed to the %s key.", wall_count, label);
	}
	void found_powerup(const unsigned amount = 1)
	{
		powerup_count += amount;
	}
	void report_keys(PHYSFS_File *const fp) const
	{
		if (!powerup_count)
		{
			if (wall_count)
				err_printf(fp, "Error: There is a door keyed to the %s key, but no %s key!", label, label);
		}
		else if (powerup_count > 1)
			err_printf(fp, "Error: There are %i %s keys!", powerup_count, label);
	}
};

}

// ----------------------------------------------------------------------------
static void write_key_text(fvcobjptridx &vcobjptridx, segment_array &segments, fvcwallptridx &vcwallptridx, PHYSFS_File *my_file)
{
	PHYSFSX_printf(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_printf(my_file, "Key stuff:\n");

	key_stat blue("blue"), gold("gold"), red("red");

	range_for (const auto &&w, vcwallptridx)
	{
		blue.check_wall(segments, my_file, w, KEY_BLUE);
		gold.check_wall(segments, my_file, w, KEY_GOLD);
		red.check_wall(segments, my_file, w, KEY_RED);
	}

	blue.report_walls(my_file);
	gold.report_walls(my_file);
	red.report_walls(my_file);

	range_for (const auto &&objp, vcobjptridx)
	{
		if (objp->type == OBJ_POWERUP)
		{
			const char *color;
			const auto id = get_powerup_id(objp);
			if (
				(id == POW_KEY_BLUE && (blue.found_powerup(), color = "BLUE", true)) ||
				(id == POW_KEY_RED && (red.found_powerup(), color = "RED", true)) ||
				(id == POW_KEY_GOLD && (gold.found_powerup(), color = "GOLD", true))
			)
			{
				PHYSFSX_printf(my_file, "The %s key is object %hu in segment %i\n", color, static_cast<objnum_t>(objp), objp->segnum);
			}
		}

		if (const auto contains_count = objp->contains_count)
		{
			if (objp->contains_type == OBJ_POWERUP)
			{
				const char *color;
				const auto id = objp->contains_id;
				if (
					(id == POW_KEY_BLUE && (blue.found_powerup(contains_count), color = "BLUE", true)) ||
					(id == POW_KEY_RED && (red.found_powerup(contains_count), color = "RED", true)) ||
					(id == POW_KEY_GOLD && (gold.found_powerup(contains_count), color = "GOLD", true))
				)
					PHYSFSX_printf(my_file, "The %s key is contained in object %hu (a %s %s) in segment %hu\n", color, static_cast<objnum_t>(objp), object_types(objp), Robot_names[get_robot_id(objp)].data(), objp->segnum);
			}
		}
	}

	blue.report_keys(my_file);
	gold.report_keys(my_file);
	red.report_keys(my_file);
}

// ----------------------------------------------------------------------------
static void write_control_center_text(fvcsegptridx &vcsegptridx, PHYSFS_File *my_file)
{
	int	count, count2;

	PHYSFSX_printf(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_printf(my_file, "Control Center stuff:\n");

	count = 0;
	range_for (const auto &&segp, vcsegptridx)
	{
		if (segp->special == SEGMENT_IS_CONTROLCEN)
		{
			count++;
			PHYSFSX_printf(my_file, "Segment %3hu is a control center.\n", static_cast<uint16_t>(segp));
			count2 = 0;
			range_for (const auto objp, objects_in(segp, vcobjptridx, vcsegptr))
			{
				if (objp->type == OBJ_CNTRLCEN)
					count2++;
			}
			if (count2 == 0)
				PHYSFSX_printf(my_file, "No control center object in control center segment.\n");
			else if (count2 != 1)
				PHYSFSX_printf(my_file, "%i control center objects in control center segment.\n", count2);
		}
	}

	if (count == 0)
		err_printf(my_file, "Error: No control center in this mine.");
	else if (count != 1)
		err_printf(my_file, "Error: More than one control center in this mine.");
}

// ----------------------------------------------------------------------------
static void write_fuelcen_text(PHYSFS_File *my_file)
{
	int	i;

	PHYSFSX_printf(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_printf(my_file, "Fuel Center stuff: (Note: This means fuel, repair, materialize, control centers!)\n");

	for (i=0; i<Num_fuelcenters; i++) {
		PHYSFSX_printf(my_file, "Fuelcenter %i: Type=%i (%s), segment = %3i\n", i, Station[i].Type, Special_names[Station[i].Type], Station[i].segnum);
		if (Segments[Station[i].segnum].special != Station[i].Type)
			err_printf(my_file, "Error: Conflicting data: Segment %i has special type %i (%s), expected to be %i", Station[i].segnum, Segments[Station[i].segnum].special, Special_names[Segments[Station[i].segnum].special], Station[i].Type);
	}
}

// ----------------------------------------------------------------------------
static void write_segment_text(fvcsegptridx &vcsegptridx, PHYSFS_File *my_file)
{
	PHYSFSX_printf(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_printf(my_file, "Segment stuff:\n");

	range_for (const auto &&segp, vcsegptridx)
	{
		PHYSFSX_printf(my_file, "Segment %4hu:", static_cast<uint16_t>(segp));
		if (segp->special != 0)
			PHYSFSX_printf(my_file, " special = %3i (%s), station_idx=%3i", segp->special, Special_names[segp->special], segp->station_idx);
		if (segp->matcen_num != -1)
			PHYSFSX_printf(my_file, " matcen = %3i", segp->matcen_num);
		PHYSFSX_printf(my_file, "\n");
	}

	range_for (const auto &&segp, vcsegptridx)
	{
		int	depth;

		PHYSFSX_printf(my_file, "Segment %4hu: ", static_cast<uint16_t>(segp));
		depth=0;
			PHYSFSX_printf(my_file, "Objects: ");
		range_for (const auto objp, objects_in(segp, vcobjptridx, vcsegptr))
			{
				short objnum = objp;
				PHYSFSX_printf(my_file, "[%8s %8s %3i] ", object_types(objp), object_ids(objp), objnum);
				if (depth++ > 30) {
					PHYSFSX_printf(my_file, "\nAborted after %i links\n", depth);
					break;
				}
			}
		PHYSFSX_printf(my_file, "\n");
	}
}

// ----------------------------------------------------------------------------
// This routine is bogus.  It assumes that all centers are matcens,
// which is not true.  The setting of segnum is bogus.
static void write_matcen_text(PHYSFS_File *my_file)
{
	int	i;

	PHYSFSX_printf(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_printf(my_file, "Materialization centers:\n");
	for (i=0; i<Num_robot_centers; i++) {
		int	trigger_count=0, fuelcen_num;

		PHYSFSX_printf(my_file, "FuelCenter[%02i].Segment = %04i  ", i, Station[i].segnum);
		PHYSFSX_printf(my_file, "Segment[%04i].matcen_num = %02i  ", Station[i].segnum, Segments[Station[i].segnum].matcen_num);

		fuelcen_num = RobotCenters[i].fuelcen_num;
		if (Station[fuelcen_num].Type != SEGMENT_IS_ROBOTMAKER)
			err_printf(my_file, "Error: Matcen %i corresponds to Station %i, which has type %i (%s).", i, fuelcen_num, Station[fuelcen_num].Type, Special_names[Station[fuelcen_num].Type]);

		auto segnum = Station[fuelcen_num].segnum;

		//	Find trigger for this materialization center.
		range_for (auto &&t, vctrgptridx)
		{
			if (trigger_is_matcen(t))
			{
				range_for (auto &k, partial_const_range(t->seg, t->num_links))
					if (k == segnum)
					{
						PHYSFSX_printf(my_file, "Trigger = %2i  ", t.get_unchecked_index());
						trigger_count++;
					}
			}
		}
		PHYSFSX_printf(my_file, "\n");

		if (trigger_count == 0)
			err_printf(my_file, "Error: Matcen %i in segment %i has no trigger!", i, segnum);

	}
}

// ----------------------------------------------------------------------------
namespace dsx {
static void write_wall_text(fvcsegptridx &vcsegptridx, fvcwallptridx &vcwallptridx, PHYSFS_File *my_file)
{
	int	j;
	array<int8_t, MAX_WALLS> wall_flags;

	PHYSFSX_printf(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_printf(my_file, "Walls:\n");
	range_for (auto &&wp, vcwallptridx)
	{
		auto &w = *wp;
		int	sidenum;

		const auto i = static_cast<wallnum_t>(wp);
		PHYSFSX_printf(my_file, "Wall %03i: seg=%3i, side=%2i, linked_wall=%3i, type=%s, flags=%4x, hps=%3i, trigger=%2i, clip_num=%2i, keys=%2i, state=%i\n", i,
			w.segnum, w.sidenum, w.linked_wall, Wall_names[w.type], w.flags, w.hps >> 16, w.trigger, w.clip_num, w.keys, w.state);

#if defined(DXX_BUILD_DESCENT_II)
		if (w.trigger >= Num_triggers)
			PHYSFSX_printf(my_file, "Wall %03d points to invalid trigger %d\n",i,w.trigger);
#endif

		auto segnum = w.segnum;
		sidenum = w.sidenum;

		if (Segments[segnum].sides[sidenum].wall_num != wp)
			err_printf(my_file, "Error: Wall %u points at segment %i, side %i, but that segment doesn't point back (it's wall_num = %hi)", i, segnum, sidenum, static_cast<int16_t>(Segments[segnum].sides[sidenum].wall_num));
	}

	wall_flags = {};

	range_for (const auto &&segp, vcsegptridx)
	{
		for (j=0; j<MAX_SIDES_PER_SEGMENT; j++) {
			const auto sidep = &segp->sides[j];
			if (sidep->wall_num != wall_none)
			{
				if (wall_flags[sidep->wall_num])
					err_printf(my_file, "Error: Wall %hu appears in two or more segments, including segment %hu, side %i.", static_cast<int16_t>(sidep->wall_num), static_cast<segnum_t>(segp), j);
				else
					wall_flags[sidep->wall_num] = 1;
			}
		}
	}

}
}

// ----------------------------------------------------------------------------
namespace dsx {
static void write_player_text(fvcobjptridx &vcobjptridx, PHYSFS_File *my_file)
{
	int	num_players=0;

	PHYSFSX_printf(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_printf(my_file, "Players:\n");
	range_for (const auto &&objp, vcobjptridx)
	{
		if (objp->type == OBJ_PLAYER)
		{
			num_players++;
			PHYSFSX_printf(my_file, "Player %2i is object #%3hu in segment #%3i.\n", get_player_id(objp), static_cast<uint16_t>(objp), objp->segnum);
		}
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (num_players != MAX_PLAYERS)
		err_printf(my_file, "Error: %i player objects.  %i are required.", num_players, MAX_PLAYERS);
#endif
	if (num_players > MAX_MULTI_PLAYERS)
		err_printf(my_file, "Error: %i player objects.  %i are required.", num_players, MAX_PLAYERS);
}
}

// ----------------------------------------------------------------------------
namespace dsx {
static void write_trigger_text(PHYSFS_File *my_file)
{
	PHYSFSX_printf(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_printf(my_file, "Triggers:\n");
	range_for (auto &&t, vctrgptridx)
	{
		const auto i = static_cast<trgnum_t>(t);
#if defined(DXX_BUILD_DESCENT_I)
		PHYSFSX_printf(my_file, "Trigger %03i: flags=%04x, value=%08x, time=%8x, linknum=%i, num_links=%i ", i, 
                        t->flags, static_cast<unsigned>(t->value), 0, t->link_num, t->num_links);
#elif defined(DXX_BUILD_DESCENT_II)
		PHYSFSX_printf(my_file, "Trigger %03i: type=%02x flags=%04x, value=%08x, time=%8x, num_links=%i ", i,
			t->type, t->flags, t->value, 0, t->num_links);
#endif

		for (unsigned j = 0; j < t->num_links; ++j)
			PHYSFSX_printf(my_file, "[%03i:%i] ", t->seg[j], t->side[j]);

		//	Find which wall this trigger is connected to.
		const auto &&we = vcwallptr.end();
		const auto &&wi = std::find_if(vcwallptr.begin(), we, [i](const wall *const p) { return p->trigger == i; });
		if (wi == we)
			err_printf(my_file, "Error: Trigger %i is not connected to any wall, so it can never be triggered.", i);
		else
		{
			const auto &&w = *wi;
			PHYSFSX_printf(my_file, "Attached to seg:side = %i:%i, wall %hi\n", w->segnum, w->sidenum, static_cast<int16_t>(vcsegptr(w->segnum)->sides[w->sidenum].wall_num));
		}
	}
}
}

// ----------------------------------------------------------------------------
void write_game_text_file(const char *filename)
{
	char	my_filename[128];
	int	namelen;
	Errors_in_mine = 0;

	namelen = strlen(filename);

	Assert (namelen > 4);

	Assert (filename[namelen-4] == '.');

	strcpy(my_filename, filename);
	strcpy( &my_filename[namelen-4], ".txm");

	auto my_file = PHYSFSX_openWriteBuffered(my_filename);
	if (!my_file)	{
		gr_palette_load(gr_palette);
		nm_messagebox( NULL, 1, "Ok", "ERROR: Unable to open %s\nErrno = %i", my_filename, errno);

		return;
	}

	dump_used_textures_level(my_file, 0);
	say_totals(vcobjptridx, my_file, Gamesave_current_filename);

	PHYSFSX_printf(my_file, "\nNumber of segments:   %4i\n", Highest_segment_index+1);
	PHYSFSX_printf(my_file, "Number of objects:    %4i\n", Highest_object_index+1);
	PHYSFSX_printf(my_file, "Number of walls:      %4i\n", Num_walls);
	PHYSFSX_printf(my_file, "Number of open doors: %4i\n", ActiveDoors.get_count());
	PHYSFSX_printf(my_file, "Number of triggers:   %4i\n", Num_triggers);
	PHYSFSX_printf(my_file, "Number of matcens:    %4i\n", Num_robot_centers);
	PHYSFSX_printf(my_file, "\n");

	write_segment_text(vcsegptridx, my_file);

	write_fuelcen_text(my_file);

	write_matcen_text(my_file);

	write_player_text(vcobjptridx, my_file);

	write_wall_text(vcsegptridx, vcwallptridx, my_file);

	write_trigger_text(my_file);

	write_exit_text(vcsegptridx, vcwallptridx, my_file);

	//	---------- Find control center segment ----------
	write_control_center_text(vcsegptridx, my_file);

	//	---------- Show keyed walls ----------
	write_key_text(vcobjptridx, Segments, vcwallptridx, my_file);
}

#if defined(DXX_BUILD_DESCENT_II)
//	Adam: Change NUM_ADAM_LEVELS to the number of levels.
#define	NUM_ADAM_LEVELS	30

//	Adam: Stick the names here.
constexpr char Adam_level_names[NUM_ADAM_LEVELS][13] = {
	"D2LEVA-1.LVL",
	"D2LEVA-2.LVL",
	"D2LEVA-3.LVL",
	"D2LEVA-4.LVL",
	"D2LEVA-S.LVL",

	"D2LEVB-1.LVL",
	"D2LEVB-2.LVL",
	"D2LEVB-3.LVL",
	"D2LEVB-4.LVL",
	"D2LEVB-S.LVL",

	"D2LEVC-1.LVL",
	"D2LEVC-2.LVL",
	"D2LEVC-3.LVL",
	"D2LEVC-4.LVL",
	"D2LEVC-S.LVL",

	"D2LEVD-1.LVL",
	"D2LEVD-2.LVL",
	"D2LEVD-3.LVL",
	"D2LEVD-4.LVL",
	"D2LEVD-S.LVL",

	"D2LEVE-1.LVL",
	"D2LEVE-2.LVL",
	"D2LEVE-3.LVL",
	"D2LEVE-4.LVL",
	"D2LEVE-S.LVL",

	"D2LEVF-1.LVL",
	"D2LEVF-2.LVL",
	"D2LEVF-3.LVL",
	"D2LEVF-4.LVL",
	"D2LEVF-S.LVL",
};

static int Ignore_tmap_num2_error;
#endif

// ----------------------------------------------------------------------------
namespace dsx {
static void determine_used_textures_level(int load_level_flag, int shareware_flag, int level_num, perm_tmap_buffer_type &tmap_buf, wall_buffer_type &wall_buf, level_tmap_buffer_type &level_tmap_buf, int max_tmap)
{
	int	sidenum;
	int	j;

#if defined(DXX_BUILD_DESCENT_I)
	tmap_buf = {};

	if (load_level_flag) {
		load_level(shareware_flag ? Shareware_level_names[level_num] : Registered_level_names[level_num]);
	}

	range_for (const auto &&segp, vcsegptr)
         {
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++)
                 {
			const auto sidep = &segp->sides[sidenum];
			if (sidep->wall_num != wall_none) {
				int clip_num = Walls[sidep->wall_num].clip_num;
				if (clip_num != -1) {

					const auto num_frames = WallAnims[clip_num].num_frames;

					wall_buf[clip_num] = 1;

					for (j=0; j<num_frames; j++) {
						int	tmap_num;

						tmap_num = WallAnims[clip_num].frames[j];
						tmap_buf[tmap_num]++;
						if (level_tmap_buf[tmap_num] == -1)
							level_tmap_buf[tmap_num] = level_num + (!shareware_flag) * NUM_SHAREWARE_LEVELS;
					}
				}
			}

			if (sidep->tmap_num >= 0)
                         {
				if (sidep->tmap_num < max_tmap)
                                 {
					tmap_buf[sidep->tmap_num]++;
					if (level_tmap_buf[sidep->tmap_num] == -1)
						level_tmap_buf[sidep->tmap_num] = level_num + (!shareware_flag) * NUM_SHAREWARE_LEVELS;
                                 }
                                else
                                 {
					Int3(); //	Error, bogus texture map.  Should not be greater than max_tmap.
                                 }
                         }

			if ((sidep->tmap_num2 & 0x3fff) != 0)
                         {
				if ((sidep->tmap_num2 & 0x3fff) < max_tmap) {
					tmap_buf[sidep->tmap_num2 & 0x3fff]++;
					if (level_tmap_buf[sidep->tmap_num2 & 0x3fff] == -1)
						level_tmap_buf[sidep->tmap_num2 & 0x3fff] = level_num + (!shareware_flag) * NUM_SHAREWARE_LEVELS;
				} else
					Int3();	//	Error, bogus texture map.  Should not be greater than max_tmap.
                         }
                 }
         }
#elif defined(DXX_BUILD_DESCENT_II)
	(void)max_tmap;
	(void)shareware_flag;

	tmap_buf = {};

	if (load_level_flag) {
		load_level(Adam_level_names[level_num]);
	}


	//	Process robots.
	range_for (const auto &&objp, vcobjptr)
	{
		if (objp->render_type == RT_POLYOBJ) {
			polymodel *po = &Polygon_models[objp->rtype.pobj_info.model_num];

			for (unsigned i = 0; i < po->n_textures; ++i)
			{
				unsigned tli = ObjBitmaps[ObjBitmapPtrs[po->first_texture+i]].index;

				if (tli < tmap_buf.size())
				{
					tmap_buf[tli]++;
					if (level_tmap_buf[tli] == -1)
						level_tmap_buf[tli] = level_num;
				} else
					Int3();	//	Hmm, It seems this texture is bogus!
			}

		}
	}


	Ignore_tmap_num2_error = 0;

	//	Process walls and segment sides.
	range_for (const auto &&segp, vmsegptr)
	{
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
			const auto sidep = &segp->sides[sidenum];
			if (sidep->wall_num != wall_none) {
				int clip_num = Walls[sidep->wall_num].clip_num;
				if (clip_num != -1) {

					// -- int num_frames = WallAnims[clip_num].num_frames;

					wall_buf[clip_num] = 1;

					for (j=0; j<1; j++) {	//	Used to do through num_frames, but don't really want all the door01#3 stuff.
						unsigned tmap_num = Textures[WallAnims[clip_num].frames[j]].index;
						Assert(tmap_num < tmap_buf.size());
						tmap_buf[tmap_num]++;
						if (level_tmap_buf[tmap_num] == -1)
							level_tmap_buf[tmap_num] = level_num;
					}
				}
			} else if (segp->children[sidenum] == segment_none) {

				if (sidep->tmap_num >= 0)
				{
					if (sidep->tmap_num < Textures.size()) {
						const auto ti = Textures[sidep->tmap_num].index;
						assert(ti < tmap_buf.size());
						++tmap_buf[ti];
						if (level_tmap_buf[ti] == -1)
							level_tmap_buf[ti] = level_num;
					} else
						Int3();	//	Error, bogus texture map.  Should not be greater than max_tmap.
				}

				if (const auto masked_tmap_num2 = (sidep->tmap_num2 & 0x3fff))
				{
					if (masked_tmap_num2 < Textures.size())
					{
						const auto ti = Textures[masked_tmap_num2].index;
						assert(ti < tmap_buf.size());
						++tmap_buf[ti];
						if (level_tmap_buf[ti] == -1)
							level_tmap_buf[ti] = level_num;
					} else {
						if (!Ignore_tmap_num2_error)
							Int3();	//	Error, bogus texture map.  Should not be greater than max_tmap.
						Ignore_tmap_num2_error = 1;
						sidep->tmap_num2 = 0;
					}
				}
			}
		}
	}
#endif
}
}

// ----------------------------------------------------------------------------
template <std::size_t N>
static void merge_buffers(array<int, N> &dest, const array<int, N> &src)
{
	std::transform(dest.begin(), dest.end(), src.begin(), dest.begin(), std::plus<int>());
}

// ----------------------------------------------------------------------------
namespace dsx {
static void say_used_tmaps(PHYSFS_File *const my_file, const perm_tmap_buffer_type &tb)
{
	int	i;
#if defined(DXX_BUILD_DESCENT_I)
	int	count = 0;

	for (i=0; i<Num_tmaps; i++)
		if (tb[i]) {
			PHYSFSX_printf(my_file, "[%3i %8s (%4i)] ", i, static_cast<const char *>(TmapInfo[i].filename), tb[i]);
			if (count++ >= 4) {
				PHYSFSX_printf(my_file, "\n");
				count = 0;
			}
		}
#elif defined(DXX_BUILD_DESCENT_II)
	for (i = 0; i < tb.size(); ++i)
		if (tb[i]) {
			PHYSFSX_printf(my_file, "[%3i %8s (%4i)]\n", i, AllBitmaps[i].name, tb[i]);
		}
#endif
}
}

#if defined(DXX_BUILD_DESCENT_I)
//	-----------------------------------------------------------------------------
static void say_used_once_tmaps(PHYSFS_File *const my_file, const perm_tmap_buffer_type &tb, const level_tmap_buffer_type &tb_lnum)
{
	int	i;
	const char	*level_name;

	for (i=0; i<Num_tmaps; i++)
		if (tb[i] == 1) {
			int	level_num = tb_lnum[i];
			if (level_num >= NUM_SHAREWARE_LEVELS) {
				Assert((level_num - NUM_SHAREWARE_LEVELS >= 0) && (level_num - NUM_SHAREWARE_LEVELS < NUM_REGISTERED_LEVELS));
				level_name = Registered_level_names[level_num - NUM_SHAREWARE_LEVELS];
			} else {
				Assert((level_num >= 0) && (level_num < NUM_SHAREWARE_LEVELS));
				level_name = Shareware_level_names[level_num];
			}

			PHYSFSX_printf(my_file, "Texture %3i %8s used only once on level %s\n", i, static_cast<const char *>(TmapInfo[i].filename), level_name);
		}
}
#endif

// ----------------------------------------------------------------------------
namespace dsx {
static void say_unused_tmaps(PHYSFS_File *my_file, perm_tmap_buffer_type &tb)
{
	int	i;
	int	count = 0;

#if defined(DXX_BUILD_DESCENT_I)
	const unsigned bound = Num_tmaps;
#elif defined(DXX_BUILD_DESCENT_II)
	const unsigned bound = MAX_BITMAP_FILES;
#endif
	for (i=0; i < bound; i++)
		if (!tb[i]) {
			if (GameBitmaps[Textures[i].index].bm_data == bogus_data.data())
				PHYSFSX_printf(my_file, "U");
			else
				PHYSFSX_printf(my_file, " ");

#if defined(DXX_BUILD_DESCENT_I)
			PHYSFSX_printf(my_file, "[%3i %8s] ", i, static_cast<const char *>(TmapInfo[i].filename));
#elif defined(DXX_BUILD_DESCENT_II)
			PHYSFSX_printf(my_file, "[%3i %8s] ", i, AllBitmaps[i].name);
#endif
			if (count++ >= 4) {
				PHYSFSX_printf(my_file, "\n");
				count = 0;
			}
		}
}
}

#if defined(DXX_BUILD_DESCENT_I)
// ----------------------------------------------------------------------------
static void say_unused_walls(PHYSFS_File *my_file, const wall_buffer_type &tb)
{
	int	i;
	for (i=0; i<Num_wall_anims; i++)
		if (!tb[i])
			PHYSFSX_printf(my_file, "Wall %3i is unused.\n", i);
}
#endif

static void say_totals(fvcobjptridx &vcobjptridx, PHYSFS_File *my_file, const char *level_name)
{
	int	total_robots = 0;
	int	objects_processed = 0;

	PHYSFSX_printf(my_file, "\nLevel %s\n", level_name);
	std::bitset<MAX_OBJECTS> used_objects;
	while (objects_processed < Highest_object_index+1) {
		int	objtype, objid, objcount, min_obj_val;

		//	Find new min objnum.
		min_obj_val = 0x7fff0000;
		const object_base *min_objp = nullptr;

		range_for (const auto &&objp, vcobjptridx)
		{
			if (!used_objects[objp] && objp->type != OBJ_NONE)
			{
				const auto cur_obj_val = (objp->type << 10) + objp->id;
				if (cur_obj_val < min_obj_val) {
					min_objp = &*objp;
					min_obj_val = cur_obj_val;
				}
			}
		}
		if (!min_objp || min_objp->type == OBJ_NONE)
			break;

		objcount = 0;

		objtype = min_objp->type;
		objid = min_objp->id;

		range_for (const auto &&objp, vcobjptridx)
		{
			if (auto &&uo = used_objects[objp])
			{
			}
			else
			{
				if ((objp->type == objtype && objp->id == objid) ||
						(objp->type == objtype && objtype == OBJ_PLAYER) ||
						(objp->type == objtype && objtype == OBJ_COOP) ||
						(objp->type == objtype && objtype == OBJ_HOSTAGE)) {
					if (objp->type == OBJ_ROBOT)
						total_robots++;
					uo = true;
					objcount++;
					objects_processed++;
				}
			}
		}

		if (objcount) {
			PHYSFSX_printf(my_file, "Object: %8s %8s %3i\n", object_types(*min_objp), object_ids(*min_objp), objcount);
		}
	}

	PHYSFSX_printf(my_file, "Total robots = %3i\n", total_robots);
}

#if defined(DXX_BUILD_DESCENT_II)
int	First_dump_level = 0;
int	Last_dump_level = NUM_ADAM_LEVELS-1;
#endif

// ----------------------------------------------------------------------------
namespace dsx {
static void say_totals_all(void)
{
	int	i;
	auto my_file = PHYSFSX_openWriteBuffered("levels.all");
	if (!my_file)	{
		gr_palette_load(gr_palette);
		nm_messagebox( NULL, 1, "Ok", "ERROR: Unable to open levels.all\nErrno=%i", errno );

		return;
	}

#if defined(DXX_BUILD_DESCENT_I)
	for (i=0; i<NUM_SHAREWARE_LEVELS; i++) {
		load_level(Shareware_level_names[i]);
		say_totals(vcobjptridx, my_file, Shareware_level_names[i]);
	}

	for (i=0; i<NUM_REGISTERED_LEVELS; i++) {
		load_level(Registered_level_names[i]);
		say_totals(vcobjptridx, my_file, Registered_level_names[i]);
	}
#elif defined(DXX_BUILD_DESCENT_II)
	for (i=First_dump_level; i<=Last_dump_level; i++) {
		load_level(Adam_level_names[i]);
		say_totals(vcobjptridx, my_file, Adam_level_names[i]);
	}
#endif
}
}

static void dump_used_textures_level(PHYSFS_File *my_file, int level_num)
{
	perm_tmap_buffer_type temp_tmap_buf;
	level_tmap_buffer_type level_tmap_buf;

	level_tmap_buf.fill(-1);

	wall_buffer_type temp_wall_buf;
	determine_used_textures_level(0, 1, level_num, temp_tmap_buf, temp_wall_buf, level_tmap_buf, sizeof(level_tmap_buf)/sizeof(level_tmap_buf[0]));
	PHYSFSX_printf(my_file, "\nTextures used in [%s]\n", Gamesave_current_filename);
	say_used_tmaps(my_file, temp_tmap_buf);
}

// ----------------------------------------------------------------------------
namespace dsx {
void dump_used_textures_all(void)
{
	int	i;

say_totals_all();

	auto my_file = PHYSFSX_openWriteBuffered("textures.dmp");

	if (!my_file)	{
		gr_palette_load(gr_palette);
		nm_messagebox( NULL, 1, "Ok", "ERROR: Can't open textures.dmp\nErrno=%i", errno);

		return;
	}

	perm_tmap_buffer_type perm_tmap_buf{};
	level_tmap_buffer_type level_tmap_buf;
	level_tmap_buf.fill(-1);

	perm_tmap_buffer_type temp_tmap_buf;
#if defined(DXX_BUILD_DESCENT_I)
	wall_buffer_type perm_wall_buf{};

	for (i=0; i<NUM_SHAREWARE_LEVELS; i++) {
		wall_buffer_type temp_wall_buf;
		determine_used_textures_level(1, 1, i, temp_tmap_buf, temp_wall_buf, level_tmap_buf, sizeof(level_tmap_buf)/sizeof(level_tmap_buf[0]));
		PHYSFSX_printf(my_file, "\nTextures used in [%s]\n", Shareware_level_names[i]);
		say_used_tmaps(my_file, temp_tmap_buf);
		merge_buffers(perm_tmap_buf, temp_tmap_buf);
		merge_buffers(perm_wall_buf, temp_wall_buf);
	}

	PHYSFSX_printf(my_file, "\n\nUsed textures in all shareware mines:\n");
	say_used_tmaps(my_file, perm_tmap_buf);

	PHYSFSX_printf(my_file, "\nUnused textures in all shareware mines:\n");
	say_unused_tmaps(my_file, perm_tmap_buf);

	PHYSFSX_printf(my_file, "\nTextures used exactly once in all shareware mines:\n");
	say_used_once_tmaps(my_file, perm_tmap_buf, level_tmap_buf);

	PHYSFSX_printf(my_file, "\nWall anims (eg, doors) unused in all shareware mines:\n");
	say_unused_walls(my_file, perm_wall_buf);

	for (i=0; i<NUM_REGISTERED_LEVELS; i++)
#elif defined(DXX_BUILD_DESCENT_II)
	for (i=First_dump_level; i<=Last_dump_level; i++)
#endif
	{
		wall_buffer_type temp_wall_buf;
		determine_used_textures_level(1, 0, i, temp_tmap_buf, temp_wall_buf, level_tmap_buf, sizeof(level_tmap_buf)/sizeof(level_tmap_buf[0]));
#if defined(DXX_BUILD_DESCENT_I)
		PHYSFSX_printf(my_file, "\nTextures used in [%s]\n", Registered_level_names[i]);
#elif defined(DXX_BUILD_DESCENT_II)
		PHYSFSX_printf(my_file, "\nTextures used in [%s]\n", Adam_level_names[i]);
#endif
		say_used_tmaps(my_file, temp_tmap_buf);
		merge_buffers(perm_tmap_buf, temp_tmap_buf);
	}

	PHYSFSX_printf(my_file, "\n\nUsed textures in all (including registered) mines:\n");
	say_used_tmaps(my_file, perm_tmap_buf);

	PHYSFSX_printf(my_file, "\nUnused textures in all (including registered) mines:\n");
	say_unused_tmaps(my_file, perm_tmap_buf);
}
}

#endif
