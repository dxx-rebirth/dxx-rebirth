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
#include <cinttypes>
#include <string.h>
#include <stdarg.h>
#include <ranges>
#include <errno.h>

#include "pstypes.h"
#include "console.h"
#include "physfsx.h"
#include "gr.h"
#include "palette.h"
#include "fmtcheck.h"

#include "inferno.h"
#include "dxxerror.h"
#include "object.h"
#include "wall.h"
#include "gameseg.h"
#include "robot.h"
#include "newmenu.h"
#include "textures.h"
#include "text.h"
#include "bm.h"
#include "switch.h"
#include "fuelcen.h"
#include "powerup.h"
#include "polyobj.h"
#include "gamesave.h"
#include "piggy.h"

#include "compiler-range_for.h"
#include "d_enumerate.h"
#include "d_levelstate.h"
#include "d_underlying_value.h"
#include "d_zip.h"
#include "segiter.h"

#if DXX_USE_EDITOR

namespace dsx {
namespace {
#if DXX_BUILD_DESCENT == 1
using perm_tmap_buffer_type = enumerated_array<int, MAX_TEXTURES, bitmap_index>;
using level_tmap_buffer_type = enumerated_array<int8_t, MAX_TEXTURES, bitmap_index>;
using wall_buffer_type = std::array<int, MAX_WALL_ANIMS>;
#elif DXX_BUILD_DESCENT == 2
using perm_tmap_buffer_type = enumerated_array<int, MAX_BITMAP_FILES, bitmap_index>;
using level_tmap_buffer_type = enumerated_array<int8_t, MAX_BITMAP_FILES, bitmap_index>;
using wall_buffer_type = std::array<int, MAX_BITMAP_FILES>;
#endif
}
}

namespace dcx {
const std::array<char[10], 7> Wall_names{{
	"NORMAL   ",
	"BLASTABLE",
	"DOOR     ",
	"ILLUSION ",
	"OPEN     ",
	"CLOSED   ",
	"EXTERNAL "
}};
}

namespace {
static void dump_used_textures_level(PHYSFS_File *my_file, int level_num, const char *Gamesave_current_filename);
static void say_totals(fvcobjptridx &vcobjptridx, PHYSFS_File *my_file, const char *level_name);
}

namespace dsx {
namespace {
const std::array<char[9], MAX_OBJECT_TYPES> Object_type_names{{
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
#if DXX_BUILD_DESCENT == 2
	"MARKER  ",
#endif
}};

// ----------------------------------------------------------------------------
static const char *object_types(const object_base &objp)
{
	const auto type{objp.type};
	assert(type == OBJ_NONE || type < MAX_OBJECT_TYPES);
	return &Object_type_names[type][0];
}
}
}

namespace {

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

static void err_puts(PHYSFS_File *f, const std::span<const char> str)
{
	++Errors_in_mine;
	con_puts(CON_CRITICAL, str);
	PHYSFSX_puts(f, str);
}

template <std::size_t len>
static void err_puts_literal(PHYSFS_File *f, const char (&str)[len])
{
	err_puts(f, {str, len - 1});
}

void err_printf(PHYSFS_File *my_file, const char * format) = delete;

dxx_compiler_attribute_format_printf(2, 3)
static void err_printf(PHYSFS_File *my_file, const char * format, ... )
{
	va_list	args;
	char		message[256];

	va_start(args, format );
	const std::size_t len = std::max(vsnprintf(message, sizeof(message), format, args), 0);
	va_end(args);
	err_puts(my_file, {message, len});
}

static void warning_puts(PHYSFS_File *f, const std::span<const char> str)
{
	con_puts(CON_URGENT, str);
	PHYSFSX_puts(f, str);
}

void warning_printf(PHYSFS_File *my_file, const char *format) = delete;

static void warning_printf(PHYSFS_File *my_file, const char * format, ... ) dxx_compiler_attribute_format_printf(2, 3);
static void warning_printf(PHYSFS_File *my_file, const char * format, ... )
{
	va_list	args;
	char		message[256];

	va_start(args, format );
	const std::size_t written = std::max(vsnprintf(message, sizeof(message), format, args), 0);
	va_end(args);
	warning_puts(my_file, {message, written});
}
}

// ----------------------------------------------------------------------------
namespace dsx {
namespace {
static void write_exit_text(fvcsegptridx &vcsegptridx, fvcwallptridx &vcwallptridx, PHYSFS_File *my_file)
{
	int	count;

	PHYSFSX_puts_literal(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_puts_literal(my_file, "Exit stuff\n");

	//	---------- Find exit triggers ----------
	count=0;
	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	auto &vctrgptridx = Triggers.vcptridx;
	range_for (const auto &&t, vctrgptridx)
	{
		if (trigger_is_exit(t))
		{
			int	count2;

			const auto i = t.get_unchecked_index();
			PHYSFSX_printf(my_file, "Trigger %2i, is an exit trigger with %i links.\n", underlying_value(i), t->num_links);
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
					PHYSFSX_printf(my_file, "Exit trigger %i is in segment %i, on side %i, bound to wall %hu\n", underlying_value(i), w->segnum, underlying_value(w->sidenum), underlying_value(wallnum_t{w}));
				}
			}
			if (count2 == 0)
				err_printf(my_file, "Error: Trigger %i is not bound to any wall.", underlying_value(i));
			else if (count2 > 1)
				err_printf(my_file, "Error: Trigger %i is bound to %i walls.", underlying_value(i), count2);

		}
	}

	if (count == 0)
		err_puts_literal(my_file, "Error: No exit trigger in this mine.");
	else if (count != 1)
		err_puts_literal(my_file, "Error: More than one exit trigger in this mine.");
	else
		PHYSFSX_puts_literal(my_file, "\n");

	//	---------- Find exit doors ----------
	count = 0;
	range_for (const auto &&segp, vcsegptridx)
	{
		for (const auto &&[sidenum, child_segnum] : enumerate(segp->shared_segment::children))
		{
			if (child_segnum == segment_exit)
			{
				PHYSFSX_printf(my_file, "Segment %3hu, side %u is an exit door.\n", segp.get_unchecked_index(), underlying_value(sidenum));
				count++;
			}
		}
	}

	if (count == 0)
		err_puts_literal(my_file, "Error: No external wall in this mine.");
	else if (count != 1) {
#if DXX_BUILD_DESCENT == 1
		warning_printf(my_file, "Warning: %i external walls in this mine.", count);
		warning_printf(my_file, "(If %i are secret exits, then no problem.)", count-1); 
#endif
	} else
		PHYSFSX_puts_literal(my_file, "\n");
}
}
}

namespace {

class key_stat
{
	const char *const label;
	unsigned wall_count{0}, powerup_count = 0;
	segnum_t seg = segment_none;
	sidenum_t side{};
public:
	key_stat(const char *const p) :
		label(p)
	{
	}
	void check_wall(const segment_array &segments, PHYSFS_File *const fp, const vcwallptridx_t wpi, const wall_key key)
	{
		auto &w = *wpi;
		if (!(w.keys & key))
			return;
		PHYSFSX_printf(fp, "Wall %hu (seg=%i, side=%i) is keyed to the %s key.\n", underlying_value(wallnum_t{wpi}), w.segnum, underlying_value(w.sidenum), label);
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
			warning_printf(fp, "Warning: This door at seg %i, is different than the one at seg %i, side %i", w.segnum, seg, underlying_value(side));
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

// ----------------------------------------------------------------------------
static void write_key_text(fvcobjptridx &vcobjptridx, segment_array &segments, fvcwallptridx &vcwallptridx, PHYSFS_File *my_file)
{
	PHYSFSX_puts_literal(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_puts_literal(my_file, "Key stuff:\n");

	key_stat blue("blue"), gold("gold"), red("red");

	range_for (const auto &&w, vcwallptridx)
	{
		blue.check_wall(segments, my_file, w, wall_key::blue);
		gold.check_wall(segments, my_file, w, wall_key::gold);
		red.check_wall(segments, my_file, w, wall_key::red);
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
				(id == powerup_type_t::POW_KEY_BLUE && (blue.found_powerup(), color = "BLUE", true)) ||
				(id == powerup_type_t::POW_KEY_RED && (red.found_powerup(), color = "RED", true)) ||
				(id == powerup_type_t::POW_KEY_GOLD && (gold.found_powerup(), color = "GOLD", true))
			)
			{
				PHYSFSX_printf(my_file, "The %s key is object %hu in segment %i\n", color, static_cast<objnum_t>(objp), objp->segnum);
			}
		}

		if (const auto contains_count = objp->contains.count)
		{
			if (objp->contains.type == contained_object_type::powerup)
			{
				const char *color;
				const auto id = objp->contains.id.powerup;
				if (
					(id == powerup_type_t::POW_KEY_BLUE && (blue.found_powerup(contains_count), color = "BLUE", true)) ||
					(id == powerup_type_t::POW_KEY_RED && (red.found_powerup(contains_count), color = "RED", true)) ||
					(id == powerup_type_t::POW_KEY_GOLD && (gold.found_powerup(contains_count), color = "GOLD", true))
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
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptridx = Objects.vcptridx;
	int	count, count2;

	PHYSFSX_puts_literal(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_puts_literal(my_file, "Control Center stuff:\n");

	count = 0;
	range_for (const auto &&segp, vcsegptridx)
	{
		if (segp->special == segment_special::controlcen)
		{
			count++;
			PHYSFSX_printf(my_file, "Segment %3hu is a control center.\n", static_cast<uint16_t>(segp));
			count2 = 0;
			for (auto &objp : objects_in<const object_base>(segp, vcobjptridx, vcsegptr))
			{
				if (objp.type == OBJ_CNTRLCEN)
					count2++;
			}
			if (count2 == 0)
				PHYSFSX_puts_literal(my_file, "No control center object in control center segment.\n");
			else if (count2 != 1)
				PHYSFSX_printf(my_file, "%i control center objects in control center segment.\n", count2);
		}
	}

	if (count == 0)
		err_puts_literal(my_file, "Error: No control center in this mine.");
	else if (count != 1)
		err_puts_literal(my_file, "Error: More than one control center in this mine.");
}

// ----------------------------------------------------------------------------
static void write_fuelcen_text(PHYSFS_File *my_file)
{
	auto &Station = LevelUniqueFuelcenterState.Station;

	PHYSFSX_puts_literal(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_puts_literal(my_file, "Fuel Center stuff: (Note: This means fuel, repair, materialize, control centers!)\n");

	const auto Num_fuelcenters = LevelUniqueFuelcenterState.Num_fuelcenters;
	for (auto &&[i, f] : enumerate(partial_const_range(Station, Num_fuelcenters)))
	{
		PHYSFSX_printf(my_file, "Fuelcenter %u: Type=%i (%s), segment = %3i\n", underlying_value(i), underlying_value(f.Type), Special_names[f.Type], f.segnum);
		if (Segments[f.segnum].special != f.Type)
			err_printf(my_file, "Error: Conflicting data: Segment %i has special type %i (%s), expected to be %i", f.segnum, underlying_value(Segments[f.segnum].special), Special_names[Segments[f.segnum].special], underlying_value(f.Type));
	}
}

// ----------------------------------------------------------------------------
static void write_segment_text(fvcsegptridx &vcsegptridx, PHYSFS_File *my_file)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptridx = Objects.vcptridx;
	PHYSFSX_puts_literal(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_puts_literal(my_file, "Segment stuff:\n");

	range_for (const auto &&segp, vcsegptridx)
	{
		PHYSFSX_printf(my_file, "Segment %4hu:", static_cast<uint16_t>(segp));
		if (segp->special != segment_special::nothing)
			PHYSFSX_printf(my_file, " special = %3i (%s), station_idx=%3u", underlying_value(segp->special), Special_names[segp->special], underlying_value(segp->station_idx));
		if (segp->matcen_num != materialization_center_number::None)
			PHYSFSX_printf(my_file, " matcen = %3i", underlying_value(segp->matcen_num));
		PHYSFSX_puts_literal(my_file, "\n");
	}

	range_for (const auto &&segp, vcsegptridx)
	{
		int	depth;

		PHYSFSX_printf(my_file, "Segment %4hu: ", static_cast<uint16_t>(segp));
		depth=0;
		PHYSFSX_puts_literal(my_file, "Objects: ");
		range_for (const auto objp, objects_in(segp, vcobjptridx, vcsegptr))
			{
				short objnum = objp;
				PHYSFSX_printf(my_file, "[%8s %8s %3i] ", object_types(objp), object_ids(objp), objnum);
				if (depth++ > 30) {
					PHYSFSX_printf(my_file, "\nAborted after %i links\n", depth);
					break;
				}
			}
		PHYSFSX_puts_literal(my_file, "\n");
	}
}

// ----------------------------------------------------------------------------
static void write_matcen_text(PHYSFS_File *my_file)
{
	PHYSFSX_puts_literal(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_puts_literal(my_file, "Materialization centers:\n");
	auto &RobotCenters = LevelSharedRobotcenterState.RobotCenters;
	auto &Station = LevelUniqueFuelcenterState.Station;
	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	auto &vctrgptridx = Triggers.vcptridx;
	const auto Num_robot_centers = LevelSharedRobotcenterState.Num_robot_centers;
	for (auto &&[i, robotcen] : enumerate(partial_const_range(RobotCenters, Num_robot_centers)))
	{
		const auto fuelcen_num = robotcen.fuelcen_num;
		auto &station = Station[fuelcen_num];
		if (station.Type != segment_special::robotmaker)
		{
			err_printf(my_file, "Error: Matcen %u corresponds to Station %u, which has type %i (%s).", underlying_value(i), underlying_value(fuelcen_num), underlying_value(station.Type), Special_names[station.Type]);
			continue;
		}
		int trigger_count{0};

		const auto segnum{station.segnum};
		PHYSFSX_printf(my_file, "FuelCenter[%02i].Segment = %04i  ", underlying_value(i), segnum);
		PHYSFSX_printf(my_file, "Segment[%04i].matcen_num = %02i  ", segnum, underlying_value(Segments[segnum].matcen_num));

		//	Find trigger for this materialization center.
		range_for (auto &&t, vctrgptridx)
		{
			if (trigger_is_matcen(t))
			{
				range_for (auto &k, partial_const_range(t->seg, t->num_links))
					if (k == segnum)
					{
						PHYSFSX_printf(my_file, "Trigger = %2i  ", underlying_value(t.get_unchecked_index()));
						trigger_count++;
					}
			}
		}
		PHYSFSX_puts_literal(my_file, "\n");

		if (trigger_count == 0)
			err_printf(my_file, "Error: Matcen %i in segment %i has no trigger!", underlying_value(i), segnum);
	}
}

}

// ----------------------------------------------------------------------------
namespace dsx {
namespace {
static void write_wall_text(fvcsegptridx &vcsegptridx, fvcwallptridx &vcwallptridx, PHYSFS_File *my_file)
{
	enumerated_array<int8_t, MAX_WALLS, wallnum_t> wall_flags;

	PHYSFSX_puts_literal(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_puts_literal(my_file, "Walls:\n");
#if DXX_BUILD_DESCENT == 2
	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
#endif
	range_for (auto &&wp, vcwallptridx)
	{
		auto &w = *wp;
		const auto i = underlying_value(wallnum_t{wp});
		PHYSFSX_printf(my_file, "Wall %03hu: seg=%3i, side=%2i, linked_wall=%3hu, type=%s, flags=%4x, hps=%3i, trigger=%2i, clip_num=%2i, keys=%2i, state=%i\n", i, w.segnum, underlying_value(w.sidenum), underlying_value(wallnum_t{w.linked_wall}), Wall_names[w.type], underlying_value(w.flags), w.hps >> 16, underlying_value(w.trigger), w.clip_num, underlying_value(w.keys), underlying_value(w.state));

#if DXX_BUILD_DESCENT == 2
		if (const auto utw = underlying_value(w.trigger); utw >= Triggers.get_count())
			PHYSFSX_printf(my_file, "Wall %03hu points to invalid trigger %u\n", i, utw);
#endif

		auto segnum = w.segnum;
		const auto sidenum{w.sidenum};

		if (const auto actual_wall_num = Segments[segnum].shared_segment::sides[sidenum].wall_num; actual_wall_num != wp)
			err_printf(my_file, "Error: Wall %hu points at segment %i, side %i, but that segment doesn't point back (it's wall_num = %hi)", i, segnum, underlying_value(sidenum), underlying_value(actual_wall_num));
	}

	wall_flags = {};

	range_for (const auto &&segp, vcsegptridx)
	{
		for (const auto &&[idx, value] : enumerate(segp->shared_segment::sides))
		{
			const auto sidep = &value;
			if (sidep->wall_num != wall_none)
			{
				if (auto &wf = wall_flags[sidep->wall_num])
					err_printf(my_file, "Error: Wall %hu appears in two or more segments, including segment %hu, side %u.", underlying_value(sidep->wall_num), segp.get_unchecked_index(), underlying_value(idx));
				else
					wf = 1;
			}
		}
	}

}

// ----------------------------------------------------------------------------
static void write_player_text(fvcobjptridx &vcobjptridx, PHYSFS_File *my_file)
{
	int num_players{0};

	PHYSFSX_puts_literal(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_puts_literal(my_file, "Players:\n");
	range_for (const auto &&objp, vcobjptridx)
	{
		if (objp->type == OBJ_PLAYER)
		{
			num_players++;
			PHYSFSX_printf(my_file, "Player %2i is object #%3hu in segment #%3i.\n", get_player_id(objp), static_cast<uint16_t>(objp), objp->segnum);
		}
	}

#if DXX_BUILD_DESCENT == 2
	if (num_players != MAX_PLAYERS)
		err_printf(my_file, "Error: %i player objects.  %i are required.", num_players, MAX_PLAYERS);
#endif
	if (num_players > MAX_MULTI_PLAYERS)
		err_printf(my_file, "Error: %i player objects.  %i are required.", num_players, MAX_PLAYERS);
}

// ----------------------------------------------------------------------------
static void write_trigger_text(PHYSFS_File *my_file)
{
	PHYSFSX_puts_literal(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_puts_literal(my_file, "Triggers:\n");
	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	auto &vctrgptridx = Triggers.vcptridx;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	for (auto &&t : vctrgptridx)
	{
		const auto i = static_cast<trgnum_t>(t);
#if DXX_BUILD_DESCENT == 1
		PHYSFSX_printf(my_file, "Trigger %03i: flags=%04x, value=%08x, time=%8x, num_links=%i ", underlying_value(i), t->flags, static_cast<unsigned>(t->value), 0, t->num_links);
#elif DXX_BUILD_DESCENT == 2
		PHYSFSX_printf(my_file, "Trigger %03i: type=%02x flags=%04x, value=%08x, time=%8x, num_links=%i ", underlying_value(i),
			static_cast<uint8_t>(t->type), static_cast<uint8_t>(t->flags), t->value, 0, t->num_links);
#endif

		for (const auto &&[seg, side] : zip(partial_range(t->seg, t->num_links), t->side))
			PHYSFSX_printf(my_file, "[%03i:%i] ", seg, underlying_value(side));

		//	Find which wall this trigger is connected to.
		const auto &&we = vcwallptr.end();
		const auto &&wi{std::ranges::find(vcwallptr.begin(), we, i, &wall::trigger)};
		if (wi == we)
			err_printf(my_file, "Error: Trigger %i is not connected to any wall, so it can never be triggered.", underlying_value(i));
		else
		{
			auto &w{*wi};
			const auto wseg{w.segnum};
			const auto wside{w.sidenum};
			PHYSFSX_printf(my_file, "Attached to seg:side = %i:%i, wall %hi\n", wseg, underlying_value(wside), underlying_value(vcsegptr(wseg)->shared_segment::sides[wside].wall_num));
		}
	}
}
}
}

// ----------------------------------------------------------------------------
void write_game_text_file(const char *filename)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptridx = Objects.vcptridx;
	char	my_filename[128];
	int	namelen;
	Errors_in_mine = 0;

	namelen = strlen(filename);

	Assert (namelen > 4);

	Assert (filename[namelen-4] == '.');

	strcpy(my_filename, filename);
	strcpy( &my_filename[namelen-4], ".txm");

	auto &&[my_file, physfserr] = PHYSFSX_openWriteBuffered(my_filename);
	if (!my_file)	{
		gr_palette_load(gr_palette);
		nm_messagebox(menu_title{nullptr}, {TXT_OK}, "ERROR: Unable to open %s\n%s", my_filename, PHYSFS_getErrorByCode(physfserr));
		return;
	}

	dump_used_textures_level(my_file, 0, filename);
	say_totals(vcobjptridx, my_file, filename);

	PHYSFSX_printf(my_file, "\nNumber of segments:   %4i\n", Highest_segment_index+1);
	PHYSFSX_printf(my_file, "Number of objects:    %4i\n", Objects.get_count());
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptridx = Walls.vcptridx;
	PHYSFSX_printf(my_file, "Number of walls:      %4i\n", Walls.get_count());
	auto &ActiveDoors = LevelUniqueWallSubsystemState.ActiveDoors;
	PHYSFSX_printf(my_file, "Number of open doors: %4i\n", ActiveDoors.get_count());
	{
	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	PHYSFSX_printf(my_file, "Number of triggers:   %4i\n", Triggers.get_count());
	}
	PHYSFSX_printf(my_file, "Number of matcens:    %4i\n\n", LevelSharedRobotcenterState.Num_robot_centers);

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

namespace dsx {
namespace {

#if DXX_BUILD_DESCENT == 1
const char Shareware_level_names[NUM_SHAREWARE_LEVELS][12] = {
	"level01.rdl",
	"level02.rdl",
	"level03.rdl",
	"level04.rdl",
	"level05.rdl",
	"level06.rdl",
	"level07.rdl"
};

const char Registered_level_names[NUM_REGISTERED_LEVELS][12] = {
	"level08.rdl",
	"level09.rdl",
	"level10.rdl",
	"level11.rdl",
	"level12.rdl",
	"level13.rdl",
	"level14.rdl",
	"level15.rdl",
	"level16.rdl",
	"level17.rdl",
	"level18.rdl",
	"level19.rdl",
	"level20.rdl",
	"level21.rdl",
	"level22.rdl",
	"level23.rdl",
	"level24.rdl",
	"level25.rdl",
	"level26.rdl",
	"level27.rdl",
	"levels1.rdl",
	"levels2.rdl",
	"levels3.rdl"
};
#endif

#if DXX_BUILD_DESCENT == 2
static int Ignore_tmap_num2_error;
#endif

// ----------------------------------------------------------------------------
#if DXX_BUILD_DESCENT == 1
#define determine_used_textures_level(load_level_flag,shareware_flag,level_num,tmap_buf,wall_buffer_type,level_tmap_buf,max_tmap)	determine_used_textures_level(load_level_flag,shareware_flag,level_num,tmap_buf,wall_buffer_type,level_tmap_buf,max_tmap)
#endif
static void determine_used_textures_level(int load_level_flag, int shareware_flag, int level_num, perm_tmap_buffer_type &tmap_buf, wall_buffer_type &wall_buf, level_tmap_buffer_type &level_tmap_buf, int max_tmap)
{
#if DXX_BUILD_DESCENT == 2
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
#endif
	int	j;

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &WallAnims = GameSharedState.WallAnims;
#if DXX_BUILD_DESCENT == 1
	tmap_buf = {};

	if (load_level_flag) {
		load_level(shareware_flag ? Shareware_level_names[level_num] : Registered_level_names[level_num]);
	}

	range_for (const cscusegment segp, vcsegptr)
	{
		range_for (const auto &&z, zip(segp.s.sides, segp.u.sides))
		{
			auto &sside = std::get<0>(z);
			if (sside.wall_num != wall_none)
			{
				const auto clip_num = Walls.vcptr(sside.wall_num)->clip_num;
				if (clip_num != -1) {

					const auto num_frames = WallAnims[clip_num].num_frames;

					wall_buf[clip_num] = 1;

					for (j=0; j<num_frames; j++) {
						const bitmap_index tmap_num{WallAnims[clip_num].frames[j]};
						tmap_buf[tmap_num]++;
						if (level_tmap_buf[tmap_num] == -1)
							level_tmap_buf[tmap_num] = level_num + (!shareware_flag) * NUM_SHAREWARE_LEVELS;
					}
				}
			}

			auto &uside = std::get<1>(z);
			if (const bitmap_index tmap1idx{get_texture_index(uside.tmap_num)}; underlying_value(tmap1idx) < max_tmap)
			{
				++ tmap_buf[tmap1idx];
				if (auto &t = level_tmap_buf[tmap1idx]; t == -1)
					t = level_num + (!shareware_flag) * NUM_SHAREWARE_LEVELS;
			}
                                else
                                 {
					Int3(); //	Error, bogus texture map.  Should not be greater than max_tmap.
                                 }

			if (const bitmap_index tmap_num2{get_texture_index(uside.tmap_num2)}; tmap_num2 != bitmap_index{})
                         {
				if (underlying_value(tmap_num2) < max_tmap) {
					++tmap_buf[tmap_num2];
					if (level_tmap_buf[tmap_num2] == -1)
						level_tmap_buf[tmap_num2] = level_num + (!shareware_flag) * NUM_SHAREWARE_LEVELS;
				} else
					Int3();	//	Error, bogus texture map.  Should not be greater than max_tmap.
                         }
                 }
         }
#elif DXX_BUILD_DESCENT == 2
	(void)load_level_flag;
	(void)max_tmap;
	(void)shareware_flag;

	tmap_buf = {};

	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	//	Process robots.
	for (auto &obj : vcobjptr)
	{
		if (obj.render_type == render_type::RT_POLYOBJ)
		{
			const polymodel *const po{&Polygon_models[obj.rtype.pobj_info.model_num]};

			for (unsigned i = 0; i < po->n_textures; ++i)
			{
				const auto tli = ObjBitmaps[ObjBitmapPtrs[po->first_texture + i]];
				if (tmap_buf.valid_index(tli))
				{
					tmap_buf[tli]++;
					if (auto &ltb = level_tmap_buf[tli]; ltb == -1)
						ltb = level_num;
				} else
					Int3();	//	Hmm, It seems this texture is bogus!
			}
		}
	}


	Ignore_tmap_num2_error = 0;

	//	Process walls and segment sides.
	range_for (const csmusegment segp, vmsegptr)
	{
		range_for (const auto &&z, zip(segp.s.sides, segp.u.sides, segp.s.children))
		{
			auto &sside = std::get<0>(z);
			auto &uside = std::get<1>(z);
			const auto child = std::get<2>(z);
			if (sside.wall_num != wall_none) {
				const auto clip_num = Walls.vcptr(sside.wall_num)->clip_num;
				if (clip_num != -1) {

					// -- int num_frames = WallAnims[clip_num].num_frames;

					wall_buf[clip_num] = 1;

					for (j=0; j<1; j++) {	//	Used to do through num_frames, but don't really want all the door01#3 stuff.
						const auto tmap_num = Textures[WallAnims[clip_num].frames[j]];
						assert(tmap_buf.valid_index(tmap_num));
						tmap_buf[tmap_num]++;
						if (auto &ltb = level_tmap_buf[tmap_num]; ltb == -1)
							ltb = level_num;
					}
				}
			} else if (child == segment_none) {
				{
					const auto tmap1idx = get_texture_index(uside.tmap_num);
					if (tmap1idx < Textures.size()) {
						const auto ti = Textures[tmap1idx];
						assert(tmap_buf.valid_index(ti));
						++tmap_buf[ti];
						if (auto &ltb = level_tmap_buf[ti]; ltb == -1)
							ltb = level_num;
					} else
						Int3();	//	Error, bogus texture map.  Should not be greater than max_tmap.
				}

				if (const auto masked_tmap_num2 = get_texture_index(uside.tmap_num2))
				{
					if (masked_tmap_num2 < Textures.size())
					{
						const auto ti = Textures[masked_tmap_num2];
						assert(tmap_buf.valid_index(ti));
						++tmap_buf[ti];
						if (level_tmap_buf[ti] == -1)
							level_tmap_buf[ti] = level_num;
					} else {
						if (!Ignore_tmap_num2_error)
							Int3();	//	Error, bogus texture map.  Should not be greater than max_tmap.
						Ignore_tmap_num2_error = 1;
						uside.tmap_num2 = texture2_value::None;
					}
				}
			}
		}
	}
#endif
}
}
}

namespace {
// ----------------------------------------------------------------------------
template <std::size_t N>
static void merge_buffers(std::array<int, N> &dest, const std::array<int, N> &src)
{
	std::transform(dest.begin(), dest.end(), src.begin(), dest.begin(), std::plus<int>());
}
}

// ----------------------------------------------------------------------------
namespace dsx {
namespace {
static void say_used_tmaps(PHYSFS_File *const my_file, const perm_tmap_buffer_type &tb)
{
#if DXX_BUILD_DESCENT == 1
	int count{0};

	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	const auto Num_tmaps = LevelUniqueTmapInfoState.Num_tmaps;
	for (const uint16_t i : xrange(Num_tmaps))
	{
		const bitmap_index bi{i};
		if (tb[bi]) {
			PHYSFSX_printf(my_file, "[%3i %8s (%4i)]%s", i, TmapInfo[i].filename.data(), tb[bi], count++ >= 4 ? (count = 0, "\n") : " ");
		}
	}
#elif DXX_BUILD_DESCENT == 2
	for (const uint16_t i : xrange(std::size(tb)))
	{
		const bitmap_index bi{i};
		if (tb[bi]) {
			PHYSFSX_printf(my_file, "[%3i %8s (%4i)]\n", i, AllBitmaps[bi].name.data(), tb[bi]);
		}
	}
#endif
}

#if DXX_BUILD_DESCENT == 1
//	-----------------------------------------------------------------------------
static void say_used_once_tmaps(PHYSFS_File *const my_file, const perm_tmap_buffer_type &tb, const level_tmap_buffer_type &tb_lnum)
{
	const char	*level_name;

	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	const auto Num_tmaps = LevelUniqueTmapInfoState.Num_tmaps;
	for (const uint16_t i : xrange(Num_tmaps))
	{
		const bitmap_index bi{i};
		if (tb[bi] == 1)
		{
			const auto level_num = tb_lnum[bi];
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
}
#endif

// ----------------------------------------------------------------------------
static void say_unused_tmaps(PHYSFS_File *my_file, perm_tmap_buffer_type &perm_tmap_buf)
{
#if DXX_BUILD_DESCENT == 1
	const unsigned bound = LevelUniqueTmapInfoState.Num_tmaps;
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	auto &tmap_name_source = TmapInfo;
#elif DXX_BUILD_DESCENT == 2
	const unsigned bound = MAX_BITMAP_FILES;
	auto &tmap_name_source = AllBitmaps;
#endif
	unsigned count{0};
	for (auto &&[i, tb, texture, tmap_name] : enumerate(zip(partial_range(perm_tmap_buf, bound), Textures, tmap_name_source)))
		if (!tb)
		{
			const char usage_indicator = (GameBitmaps[texture].bm_data == bogus_data.data())
				? 'U'
				: ' ';

#if DXX_BUILD_DESCENT == 1
			const auto filename = static_cast<const char *>(tmap_name.filename);
#elif DXX_BUILD_DESCENT == 2
			const auto filename = tmap_name.name.data();
#endif
			const char sep = (count++ >= 4)
				? (count = 0, '\n')
				: ' ';
			PHYSFSX_printf(my_file, "%c[%3" PRIuFAST32 " %8s]%c", usage_indicator, i, filename, sep);
		}
}

#if DXX_BUILD_DESCENT == 1
// ----------------------------------------------------------------------------
static void say_unused_walls(PHYSFS_File *my_file, const wall_buffer_type &tb)
{
	int	i;
	for (i=0; i<Num_wall_anims; i++)
		if (!tb[i])
			PHYSFSX_printf(my_file, "Wall %3i is unused.\n", i);
}
#endif
}
}

namespace {

static void say_totals(fvcobjptridx &vcobjptridx, PHYSFS_File *my_file, const char *level_name)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	int total_robots{0};
	int objects_processed{0};

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

}

namespace dsx {
namespace {

// ----------------------------------------------------------------------------
static void say_totals_all(void)
{
	auto &&[my_file, physfserr] = PHYSFSX_openWriteBuffered("levels.all");
	if (!my_file)	{
		gr_palette_load(gr_palette);
		nm_messagebox(menu_title{nullptr}, {TXT_OK}, "ERROR: Unable to open levels.all\n%s", PHYSFS_getErrorByCode(physfserr));
		return;
	}

#if DXX_BUILD_DESCENT == 1
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptridx = Objects.vcptridx;
	for (unsigned i = 0; i < NUM_SHAREWARE_LEVELS; ++i)
	{
		load_level(Shareware_level_names[i]);
		say_totals(vcobjptridx, my_file, Shareware_level_names[i]);
	}

	for (unsigned i = 0; i < NUM_REGISTERED_LEVELS; ++i)
	{
		load_level(Registered_level_names[i]);
		say_totals(vcobjptridx, my_file, Registered_level_names[i]);
	}
#endif
}
}
}

namespace {
static void dump_used_textures_level(PHYSFS_File *my_file, int level_num, const char *const Gamesave_current_filename)
{
	perm_tmap_buffer_type temp_tmap_buf;
	level_tmap_buffer_type level_tmap_buf;

	level_tmap_buf.fill(-1);

	wall_buffer_type temp_wall_buf;
	determine_used_textures_level(0, 1, level_num, temp_tmap_buf, temp_wall_buf, level_tmap_buf, level_tmap_buf.size());
	PHYSFSX_printf(my_file, "\nTextures used in [%s]\n", Gamesave_current_filename);
	say_used_tmaps(my_file, temp_tmap_buf);
}
}

// ----------------------------------------------------------------------------
namespace dsx {
void dump_used_textures_all(void)
{
say_totals_all();

	auto &&[my_file, physfserr] = PHYSFSX_openWriteBuffered("textures.dmp");
	if (!my_file)	{
		gr_palette_load(gr_palette);
		nm_messagebox(menu_title{nullptr}, {TXT_OK}, "ERROR: Can't open textures.dmp\n%s", PHYSFS_getErrorByCode(physfserr));
		return;
	}

	perm_tmap_buffer_type perm_tmap_buf{};
	level_tmap_buffer_type level_tmap_buf;
	level_tmap_buf.fill(-1);

#if DXX_BUILD_DESCENT == 1
	perm_tmap_buffer_type temp_tmap_buf;
	wall_buffer_type perm_wall_buf{};

	for (unsigned i = 0; i < NUM_SHAREWARE_LEVELS; ++i)
	{
		wall_buffer_type temp_wall_buf;
		determine_used_textures_level(1, 1, i, temp_tmap_buf, temp_wall_buf, level_tmap_buf, level_tmap_buf.size());
		PHYSFSX_printf(my_file, "\nTextures used in [%s]\n", Shareware_level_names[i]);
		say_used_tmaps(my_file, temp_tmap_buf);
		merge_buffers(perm_tmap_buf, temp_tmap_buf);
		merge_buffers(perm_wall_buf, temp_wall_buf);
	}

	PHYSFSX_puts_literal(my_file, "\n\nUsed textures in all shareware mines:\n");
	say_used_tmaps(my_file, perm_tmap_buf);

	PHYSFSX_puts_literal(my_file, "\nUnused textures in all shareware mines:\n");
	say_unused_tmaps(my_file, perm_tmap_buf);

	PHYSFSX_puts_literal(my_file, "\nTextures used exactly once in all shareware mines:\n");
	say_used_once_tmaps(my_file, perm_tmap_buf, level_tmap_buf);

	PHYSFSX_puts_literal(my_file, "\nWall anims (eg, doors) unused in all shareware mines:\n");
	say_unused_walls(my_file, perm_wall_buf);

	for (unsigned i = 0; i < NUM_REGISTERED_LEVELS; ++i)
	{
		wall_buffer_type temp_wall_buf;
		determine_used_textures_level(1, 0, i, temp_tmap_buf, temp_wall_buf, level_tmap_buf, level_tmap_buf.size());
		PHYSFSX_printf(my_file, "\nTextures used in [%s]\n", Registered_level_names[i]);
		say_used_tmaps(my_file, temp_tmap_buf);
		merge_buffers(perm_tmap_buf, temp_tmap_buf);
	}
#endif

	PHYSFSX_puts_literal(my_file, "\n\nUsed textures in all (including registered) mines:\n");
	say_used_tmaps(my_file, perm_tmap_buf);

	PHYSFSX_puts_literal(my_file, "\nUnused textures in all (including registered) mines:\n");
	say_unused_tmaps(my_file, perm_tmap_buf);
}
}

#endif
