/*
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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "pstypes.h"
#include "console.h"
#include "key.h"
#include "gr.h"
#include "palette.h"
#include "fmtcheck.h"

#include "inferno.h"
#ifdef EDITOR
#include "editor/editor.h"
#endif
#include "dxxerror.h"
#include "object.h"
#include "wall.h"
#include "gamemine.h"
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

#ifdef EDITOR
static void dump_used_textures_level(PHYSFS_file *my_file, int level_num);
static void say_totals(PHYSFS_file *my_file, const char *level_name);

// ----------------------------------------------------------------------------
static const char	*object_types(int objnum)
{
	int	type = Objects[objnum].type;

	Assert((type == OBJ_NONE) || ((type >= 0) && (type < MAX_OBJECT_TYPES)));
	return Object_type_names[type];
}

// ----------------------------------------------------------------------------
static char	*object_ids(int objnum)
{
	int	type = Objects[objnum].type;

	switch (type) {
		case OBJ_ROBOT:
			return Robot_names[get_robot_id(&Objects[objnum])];
			break;
		case OBJ_POWERUP:
			return Powerup_names[get_powerup_id(&Objects[objnum])];
			break;
	}

	return	NULL;
}

static void err_puts(PHYSFS_file *f, const char *str, size_t len) __attribute_nonnull();
static void err_puts(PHYSFS_file *f, const char *str, size_t len)
#define err_puts(A1,S,...)	(err_puts(A1,S, _dxx_call_puts_parameter2(1, ## __VA_ARGS__, strlen(S))))
{
	con_puts(CON_CRITICAL, str, len);
	PHYSFSX_puts(f, str);
	Errors_in_mine++;
}

template <size_t len>
static void err_puts_literal(PHYSFS_file *f, const char (&str)[len]) __attribute_nonnull();
template <size_t len>
static void err_puts_literal(PHYSFS_file *f, const char (&str)[len])
{
	err_puts(f, str, len);
}

static void err_printf(PHYSFS_file *my_file, const char * format, ... ) __attribute_format_printf(2, 3);
static void err_printf(PHYSFS_file *my_file, const char * format, ... )
#define err_printf(A1,F,...)	dxx_call_printf_checked(err_printf,err_puts_literal,(A1),(F),##__VA_ARGS__)
{
	va_list	args;
	char		message[256];

	va_start(args, format );
	size_t len = vsnprintf(message,sizeof(message),format,args);
	va_end(args);
	err_puts(my_file, message, len);
}

static void warning_puts(PHYSFS_file *f, const char *str, size_t len) __attribute_nonnull();
static void warning_puts(PHYSFS_file *f, const char *str, size_t len)
#define warning_puts(A1,S,...)	(warning_puts(A1,S, _dxx_call_puts_parameter2(1, ## __VA_ARGS__, strlen(S))))
{
	con_puts(CON_URGENT, str, len);
	PHYSFSX_puts(f, str);
}

template <size_t len>
static void warning_puts_literal(PHYSFS_file *f, const char (&str)[len]) __attribute_nonnull();
template <size_t len>
static void warning_puts_literal(PHYSFS_file *f, const char (&str)[len])
{
	warning_puts(f, str, len);
}

static void warning_printf(PHYSFS_file *my_file, const char * format, ... ) __attribute_format_printf(2, 3);
static void warning_printf(PHYSFS_file *my_file, const char * format, ... )
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
static void write_exit_text(PHYSFS_file *my_file)
{
	int	i, j, count;

	PHYSFSX_printf(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_printf(my_file, "Exit stuff\n");

	//	---------- Find exit triggers ----------
	count=0;
	for (i=0; i<Num_triggers; i++)
		if (trigger_is_exit(&Triggers[i])) {
			int	count2;

			PHYSFSX_printf(my_file, "Trigger %2i, is an exit trigger with %i links.\n", i, Triggers[i].num_links);
			count++;
			if (Triggers[i].num_links != 0)
				err_printf(my_file, "Error: Exit triggers must have 0 links, this one has %i links.", Triggers[i].num_links);

			//	Find wall pointing to this trigger.
			count2 = 0;
			for (j=0; j<Num_walls; j++)
				if (Walls[j].trigger == i) {
					count2++;
					PHYSFSX_printf(my_file, "Exit trigger %i is in segment %i, on side %i, bound to wall %i\n", i, Walls[j].segnum, Walls[j].sidenum, j);
				}
			if (count2 == 0)
				err_printf(my_file, "Error: Trigger %i is not bound to any wall.", i);
			else if (count2 > 1)
				err_printf(my_file, "Error: Trigger %i is bound to %i walls.", i, count2);

		}

	if (count == 0)
		err_printf(my_file, "Error: No exit trigger in this mine.");
	else if (count != 1)
		err_printf(my_file, "Error: More than one exit trigger in this mine.");
	else
		PHYSFSX_printf(my_file, "\n");

	//	---------- Find exit doors ----------
	count = 0;
	for (i=0; i<=Highest_segment_index; i++)
		for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
			if (Segments[i].children[j] == -2) {
				PHYSFSX_printf(my_file, "Segment %3i, side %i is an exit door.\n", i, j);
				count++;
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

// ----------------------------------------------------------------------------
static void write_key_text(PHYSFS_file *my_file)
{
	int	i;
	int	red_count, blue_count, gold_count;
	int	red_count2, blue_count2, gold_count2;
	int	blue_segnum=-1, blue_sidenum=-1, red_segnum=-1, red_sidenum=-1, gold_segnum=-1, gold_sidenum=-1;
	int	connect_side;

	PHYSFSX_printf(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_printf(my_file, "Key stuff:\n");

	red_count = 0;
	blue_count = 0;
	gold_count = 0;

	for (i=0; i<Num_walls; i++) {
		if (Walls[i].keys & KEY_BLUE) {
			PHYSFSX_printf(my_file, "Wall %i (seg=%i, side=%i) is keyed to the blue key.\n", i, Walls[i].segnum, Walls[i].sidenum);
			if (blue_segnum == -1) {
				blue_segnum = Walls[i].segnum;
				blue_sidenum = Walls[i].sidenum;
				blue_count++;
			} else {
				connect_side = find_connect_side(&Segments[Walls[i].segnum], &Segments[blue_segnum]);
				if (connect_side != blue_sidenum) {
					warning_printf(my_file, "Warning: This blue door at seg %i, is different than the one at seg %i, side %i", Walls[i].segnum, blue_segnum, blue_sidenum);
					blue_count++;
				}
			}
		}
		if (Walls[i].keys & KEY_RED) {
			PHYSFSX_printf(my_file, "Wall %i (seg=%i, side=%i) is keyed to the red key.\n", i, Walls[i].segnum, Walls[i].sidenum);
			if (red_segnum == -1) {
				red_segnum = Walls[i].segnum;
				red_sidenum = Walls[i].sidenum;
				red_count++;
			} else {
				connect_side = find_connect_side(&Segments[Walls[i].segnum], &Segments[red_segnum]);
				if (connect_side != red_sidenum) {
					warning_printf(my_file, "Warning: This red door at seg %i, is different than the one at seg %i, side %i", Walls[i].segnum, red_segnum, red_sidenum);
					red_count++;
				}
			}
		}
		if (Walls[i].keys & KEY_GOLD) {
			PHYSFSX_printf(my_file, "Wall %i (seg=%i, side=%i) is keyed to the gold key.\n", i, Walls[i].segnum, Walls[i].sidenum);
			if (gold_segnum == -1) {
				gold_segnum = Walls[i].segnum;
				gold_sidenum = Walls[i].sidenum;
				gold_count++;
			} else {
				connect_side = find_connect_side(&Segments[Walls[i].segnum], &Segments[gold_segnum]);
				if (connect_side != gold_sidenum) {
					warning_printf(my_file, "Warning: This gold door at seg %i, is different than the one at seg %i, side %i", Walls[i].segnum, gold_segnum, gold_sidenum);
					gold_count++;
				}
			}
		}
	}

	if (blue_count > 1)
		warning_printf(my_file, "Warning: %i doors are keyed to the blue key.", blue_count);

	if (red_count > 1)
		warning_printf(my_file, "Warning: %i doors are keyed to the red key.", red_count);

	if (gold_count > 1)
		warning_printf(my_file, "Warning: %i doors are keyed to the gold key.", gold_count);

	red_count2 = 0;
	blue_count2 = 0;
	gold_count2 = 0;

	for (i=0; i<=Highest_object_index; i++) {
		if (Objects[i].type == OBJ_POWERUP)
			if (get_powerup_id(&Objects[i]) == POW_KEY_BLUE) {
				PHYSFSX_printf(my_file, "The BLUE key is object %i in segment %i\n", i, Objects[i].segnum);
				blue_count2++;
			}
		if (Objects[i].type == OBJ_POWERUP)
			if (get_powerup_id(&Objects[i]) == POW_KEY_RED) {
				PHYSFSX_printf(my_file, "The RED key is object %i in segment %i\n", i, Objects[i].segnum);
				red_count2++;
			}
		if (Objects[i].type == OBJ_POWERUP)
			if (get_powerup_id(&Objects[i]) == POW_KEY_GOLD) {
				PHYSFSX_printf(my_file, "The GOLD key is object %i in segment %i\n", i, Objects[i].segnum);
				gold_count2++;
			}

		if (Objects[i].contains_count) {
			if (Objects[i].contains_type == OBJ_POWERUP) {
				switch (Objects[i].contains_id) {
					case POW_KEY_BLUE:
						PHYSFSX_printf(my_file, "The BLUE key is contained in object %i (a %s %s) in segment %i\n", i, Object_type_names[Objects[i].type], Robot_names[get_robot_id(&Objects[i])], Objects[i].segnum);
						blue_count2 += Objects[i].contains_count;
						break;
					case POW_KEY_GOLD:
						PHYSFSX_printf(my_file, "The GOLD key is contained in object %i (a %s %s) in segment %i\n", i, Object_type_names[Objects[i].type], Robot_names[get_robot_id(&Objects[i])], Objects[i].segnum);
						gold_count2 += Objects[i].contains_count;
						break;
					case POW_KEY_RED:
						PHYSFSX_printf(my_file, "The RED key is contained in object %i (a %s %s) in segment %i\n", i, Object_type_names[Objects[i].type], Robot_names[get_robot_id(&Objects[i])], Objects[i].segnum);
						red_count2 += Objects[i].contains_count;
						break;
					default:
						break;
				}
			}
		}
	}

	if (blue_count)
		if (blue_count2 == 0)
			err_printf(my_file, "Error: There is a door keyed to the blue key, but no blue key!");

	if (red_count)
		if (red_count2 == 0)
			err_printf(my_file, "Error: There is a door keyed to the red key, but no red key!");

	if (gold_count)
		if (gold_count2 == 0)
			err_printf(my_file, "Error: There is a door keyed to the gold key, but no gold key!");

	if (blue_count2 > 1)
		err_printf(my_file, "Error: There are %i blue keys!", blue_count2);

	if (red_count2 > 1)
		err_printf(my_file, "Error: There are %i red keys!", red_count2);

	if (gold_count2 > 1)
		err_printf(my_file, "Error: There are %i gold keys!", gold_count2);
}

// ----------------------------------------------------------------------------
static void write_control_center_text(PHYSFS_file *my_file)
{
	int	i, count, objnum, count2;

	PHYSFSX_printf(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_printf(my_file, "Control Center stuff:\n");

	count = 0;
	for (i=0; i<=Highest_segment_index; i++)
		if (Segments[i].special == SEGMENT_IS_CONTROLCEN) {
			count++;
			PHYSFSX_printf(my_file, "Segment %3i is a control center.\n", i);
			objnum = Segments[i].objects;
			count2 = 0;
			while (objnum != -1) {
				if (Objects[objnum].type == OBJ_CNTRLCEN)
					count2++;
				objnum = Objects[objnum].next;
			}
			if (count2 == 0)
				PHYSFSX_printf(my_file, "No control center object in control center segment.\n");
			else if (count2 != 1)
				PHYSFSX_printf(my_file, "%i control center objects in control center segment.\n", count2);
		}

	if (count == 0)
		err_printf(my_file, "Error: No control center in this mine.");
	else if (count != 1)
		err_printf(my_file, "Error: More than one control center in this mine.");
}

// ----------------------------------------------------------------------------
static void write_fuelcen_text(PHYSFS_file *my_file)
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
static void write_segment_text(PHYSFS_file *my_file)
{
	int	i, objnum;

	PHYSFSX_printf(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_printf(my_file, "Segment stuff:\n");

	for (i=0; i<=Highest_segment_index; i++) {

		PHYSFSX_printf(my_file, "Segment %4i: ", i);
		if (Segments[i].special != 0)
			PHYSFSX_printf(my_file, "special = %3i (%s), value = %3i ", Segments[i].special, Special_names[Segments[i].special], Segments[i].value);

		if (Segments[i].matcen_num != -1)
			PHYSFSX_printf(my_file, "matcen = %3i, ", Segments[i].matcen_num);
		
		PHYSFSX_printf(my_file, "\n");
	}

	for (i=0; i<=Highest_segment_index; i++) {
		int	depth;

		objnum = Segments[i].objects;
		PHYSFSX_printf(my_file, "Segment %4i: ", i);
		depth=0;
		if (objnum != -1) {
			PHYSFSX_printf(my_file, "Objects: ");
			while (objnum != -1) {
				PHYSFSX_printf(my_file, "[%8s %8s %3i] ", object_types(objnum), object_ids(objnum), objnum);
				objnum = Objects[objnum].next;
				if (depth++ > 30) {
					PHYSFSX_printf(my_file, "\nAborted after %i links\n", depth);
					break;
				}
			}
		}
		PHYSFSX_printf(my_file, "\n");
	}
}

// ----------------------------------------------------------------------------
// This routine is bogus.  It assumes that all centers are matcens,
// which is not true.  The setting of segnum is bogus.
static void write_matcen_text(PHYSFS_file *my_file)
{
	int	i, j, k;

	PHYSFSX_printf(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_printf(my_file, "Materialization centers:\n");
	for (i=0; i<Num_robot_centers; i++) {
		int	trigger_count=0, segnum, fuelcen_num;

		PHYSFSX_printf(my_file, "FuelCenter[%02i].Segment = %04i  ", i, Station[i].segnum);
		PHYSFSX_printf(my_file, "Segment[%04i].matcen_num = %02i  ", Station[i].segnum, Segments[Station[i].segnum].matcen_num);

		fuelcen_num = RobotCenters[i].fuelcen_num;
		if (Station[fuelcen_num].Type != SEGMENT_IS_ROBOTMAKER)
			err_printf(my_file, "Error: Matcen %i corresponds to Station %i, which has type %i (%s).", i, fuelcen_num, Station[fuelcen_num].Type, Special_names[Station[fuelcen_num].Type]);

		segnum = Station[fuelcen_num].segnum;

		//	Find trigger for this materialization center.
		for (j=0; j<Num_triggers; j++) {
			if (trigger_is_matcen(&Triggers[j])) {
				for (k=0; k<Triggers[j].num_links; k++)
					if (Triggers[j].seg[k] == segnum) {
						PHYSFSX_printf(my_file, "Trigger = %2i  ", j );
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
static void write_wall_text(PHYSFS_file *my_file)
{
	int	i, j;
	sbyte wall_flags[MAX_WALLS];

	PHYSFSX_printf(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_printf(my_file, "Walls:\n");
	for (i=0; i<Num_walls; i++) {
		int	segnum, sidenum;

		PHYSFSX_printf(my_file, "Wall %03i: seg=%3i, side=%2i, linked_wall=%3i, type=%s, flags=%4x, hps=%3i, trigger=%2i, clip_num=%2i, keys=%2i, state=%i\n", i,
			Walls[i].segnum, Walls[i].sidenum, Walls[i].linked_wall, Wall_names[Walls[i].type], Walls[i].flags, Walls[i].hps >> 16, Walls[i].trigger, Walls[i].clip_num, Walls[i].keys, Walls[i].state);

#if defined(DXX_BUILD_DESCENT_II)
		if (Walls[i].trigger >= Num_triggers)
			PHYSFSX_printf(my_file, "Wall %03d points to invalid trigger %d\n",i,Walls[i].trigger);
#endif

		segnum = Walls[i].segnum;
		sidenum = Walls[i].sidenum;

		if (Segments[segnum].sides[sidenum].wall_num != i)
			err_printf(my_file, "Error: Wall %i points at segment %i, side %i, but that segment doesn't point back (it's wall_num = %i)", i, segnum, sidenum, Segments[segnum].sides[sidenum].wall_num);
	}

	for (unsigned i=0; i<sizeof(wall_flags)/sizeof(wall_flags[0]); i++)
		wall_flags[i] = 0;

	for (i=0; i<=Highest_segment_index; i++) {
		segment	*segp = &Segments[i];
		for (j=0; j<MAX_SIDES_PER_SEGMENT; j++) {
			side	*sidep = &segp->sides[j];
			if (sidep->wall_num != -1)
			{
				if (wall_flags[sidep->wall_num])
					err_printf(my_file, "Error: Wall %i appears in two or more segments, including segment %i, side %i.", sidep->wall_num, i, j);
				else
					wall_flags[sidep->wall_num] = 1;
			}
		}
	}

}

// ----------------------------------------------------------------------------
static void write_player_text(PHYSFS_file *my_file)
{
	int	i, num_players=0;

	PHYSFSX_printf(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_printf(my_file, "Players:\n");
	for (i=0; i<=Highest_object_index; i++) {
		if (Objects[i].type == OBJ_PLAYER) {
			num_players++;
			PHYSFSX_printf(my_file, "Player %2i is object #%3i in segment #%3i.\n", get_player_id(&Objects[i]), i, Objects[i].segnum);
		}
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (num_players != MAX_PLAYERS)
		err_printf(my_file, "Error: %i player objects.  %i are required.", num_players, MAX_PLAYERS);
#endif
	if (num_players > MAX_MULTI_PLAYERS)
		err_printf(my_file, "Error: %i player objects.  %i are required.", num_players, MAX_PLAYERS);
}

// ----------------------------------------------------------------------------
static void write_trigger_text(PHYSFS_file *my_file)
{
	int	i, j, w;

	PHYSFSX_printf(my_file, "-----------------------------------------------------------------------------\n");
	PHYSFSX_printf(my_file, "Triggers:\n");
	for (i=0; i<Num_triggers; i++) {
#if defined(DXX_BUILD_DESCENT_I)
		PHYSFSX_printf(my_file, "Trigger %03i: type=%3i flags=%04x, value=%08x, time=%8x, linknum=%i, num_links=%i ", i, 
                        Triggers[i].type, Triggers[i].flags, (unsigned int) (Triggers[i].value), (unsigned int) (Triggers[i].time), Triggers[i].link_num, Triggers[i].num_links);
#elif defined(DXX_BUILD_DESCENT_II)
		PHYSFSX_printf(my_file, "Trigger %03i: type=%02x flags=%04x, value=%08x, time=%8x, num_links=%i ", i,
			Triggers[i].type, Triggers[i].flags, Triggers[i].value, Triggers[i].time, Triggers[i].num_links);
#endif

		for (j=0; j<Triggers[i].num_links; j++)
			PHYSFSX_printf(my_file, "[%03i:%i] ", Triggers[i].seg[j], Triggers[i].side[j]);

		//	Find which wall this trigger is connected to.
		for (w=0; w<Num_walls; w++)
			if (Walls[w].trigger == i)
				break;

		if (w == Num_walls)
			err_printf(my_file, "Error: Trigger %i is not connected to any wall, so it can never be triggered.", i);
		else
			PHYSFSX_printf(my_file, "Attached to seg:side = %i:%i, wall %i\n", Walls[w].segnum, Walls[w].sidenum, Segments[Walls[w].segnum].sides[Walls[w].sidenum].wall_num);

	}

}

// ----------------------------------------------------------------------------
void write_game_text_file(const char *filename)
{
	char	my_filename[128];
	int	namelen;
	PHYSFS_file	* my_file;

	Errors_in_mine = 0;

	namelen = strlen(filename);

	Assert (namelen > 4);

	Assert (filename[namelen-4] == '.');

	strcpy(my_filename, filename);
	strcpy( &my_filename[namelen-4], ".txm");

	my_file = PHYSFSX_openWriteBuffered( my_filename );

	if (!my_file)	{
		gr_palette_load(gr_palette);
		nm_messagebox( NULL, 1, "Ok", "ERROR: Unable to open %s\nErrno = %i", my_filename, errno);

		return;
	}

	dump_used_textures_level(my_file, 0);
	say_totals(my_file, Gamesave_current_filename);

	PHYSFSX_printf(my_file, "\nNumber of segments:   %4i\n", Highest_segment_index+1);
	PHYSFSX_printf(my_file, "Number of objects:    %4i\n", Highest_object_index+1);
	PHYSFSX_printf(my_file, "Number of walls:      %4i\n", Num_walls);
	PHYSFSX_printf(my_file, "Number of open doors: %4i\n", Num_open_doors);
	PHYSFSX_printf(my_file, "Number of triggers:   %4i\n", Num_triggers);
	PHYSFSX_printf(my_file, "Number of matcens:    %4i\n", Num_robot_centers);
	PHYSFSX_printf(my_file, "\n");

	write_segment_text(my_file);

	write_fuelcen_text(my_file);

	write_matcen_text(my_file);

	write_player_text(my_file);

	write_wall_text(my_file);

	write_trigger_text(my_file);

	write_exit_text(my_file);

	//	---------- Find control center segment ----------
	write_control_center_text(my_file);

	//	---------- Show keyed walls ----------
	write_key_text(my_file);

	{
		int r;
		r = PHYSFS_close(my_file);
		if (!r)
			Int3();
	}
}

#if defined(DXX_BUILD_DESCENT_II)
//	Adam: Change NUM_ADAM_LEVELS to the number of levels.
#define	NUM_ADAM_LEVELS	30

//	Adam: Stick the names here.
static const char Adam_level_names[NUM_ADAM_LEVELS][13] = {
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

int	Ignore_tmap_num2_error;
#endif

// ----------------------------------------------------------------------------
static void determine_used_textures_level(int load_level_flag, int shareware_flag, int level_num, int *tmap_buf, int *wall_buf, sbyte *level_tmap_buf, int max_tmap)
{
	int	segnum, sidenum;
	int	i, j;

#if defined(DXX_BUILD_DESCENT_I)
	for (i=0; i<max_tmap; i++)
		tmap_buf[i] = 0;

	if (load_level_flag) {
		if (shareware_flag)
			load_level(Shareware_level_names[level_num]);
		else
			load_level(Registered_level_names[level_num]);
	}

	for (segnum=0; segnum<=Highest_segment_index; segnum++)
         {
		segment	*segp = &Segments[segnum];

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++)
                 {
			side	*sidep = &segp->sides[sidenum];

			if (sidep->wall_num != -1) {
				int clip_num = Walls[sidep->wall_num].clip_num;
				if (clip_num != -1) {

					int num_frames = WallAnims[clip_num].num_frames;

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
	int objnum=max_tmap;
	Assert(shareware_flag != -17);

	for (i=0; i<MAX_BITMAP_FILES; i++)
		tmap_buf[i] = 0;

	if (load_level_flag) {
		load_level(Adam_level_names[level_num]);
	}


	//	Process robots.
	for (objnum=0; objnum<=Highest_object_index; objnum++) {
		object *objp = &Objects[objnum];

		if (objp->render_type == RT_POLYOBJ) {
			polymodel *po = &Polygon_models[objp->rtype.pobj_info.model_num];

			for (i=0; i<po->n_textures; i++) {

				int	tli = ObjBitmaps[ObjBitmapPtrs[po->first_texture+i]].index;

				if ((tli < MAX_BITMAP_FILES) && (tli >= 0)) {
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
	for (segnum=0; segnum<=Highest_segment_index; segnum++) {
		segment	*segp = &Segments[segnum];

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
			side	*sidep = &segp->sides[sidenum];

			if (sidep->wall_num != -1) {
				int clip_num = Walls[sidep->wall_num].clip_num;
				if (clip_num != -1) {

					// -- int num_frames = WallAnims[clip_num].num_frames;

					wall_buf[clip_num] = 1;

					for (j=0; j<1; j++) {	//	Used to do through num_frames, but don't really want all the door01#3 stuff.
						int	tmap_num;

						tmap_num = Textures[WallAnims[clip_num].frames[j]].index;
						Assert((tmap_num >= 0) && (tmap_num < MAX_BITMAP_FILES));
						tmap_buf[tmap_num]++;
						if (level_tmap_buf[tmap_num] == -1)
							level_tmap_buf[tmap_num] = level_num;
					}
				}
			} else if (segp->children[sidenum] == -1) {

				if (sidep->tmap_num >= 0)
				{
					if (sidep->tmap_num < MAX_BITMAP_FILES) {
						Assert(Textures[sidep->tmap_num].index < MAX_BITMAP_FILES);
						tmap_buf[Textures[sidep->tmap_num].index]++;
						if (level_tmap_buf[Textures[sidep->tmap_num].index] == -1)
							level_tmap_buf[Textures[sidep->tmap_num].index] = level_num;
					} else
						Int3();	//	Error, bogus texture map.  Should not be greater than max_tmap.
				}

				if ((sidep->tmap_num2 & 0x3fff) != 0)
				{
					if ((sidep->tmap_num2 & 0x3fff) < MAX_BITMAP_FILES) {
						Assert(Textures[sidep->tmap_num2 & 0x3fff].index < MAX_BITMAP_FILES);
						tmap_buf[Textures[sidep->tmap_num2 & 0x3fff].index]++;
						if (level_tmap_buf[Textures[sidep->tmap_num2 & 0x3fff].index] == -1)
							level_tmap_buf[Textures[sidep->tmap_num2 & 0x3fff].index] = level_num;
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

// ----------------------------------------------------------------------------
static void merge_buffers(int *dest, int *src, int num)
{
	int	i;

	for (i=0; i<num; i++)
		if (src[i])
			dest[i] += src[i];
}

// ----------------------------------------------------------------------------
static void say_used_tmaps(PHYSFS_file *my_file, int *tb)
{
	int	i;
#if defined(DXX_BUILD_DESCENT_I)
	int	count = 0;

	for (i=0; i<Num_tmaps; i++)
		if (tb[i]) {
			PHYSFSX_printf(my_file, "[%3i %8s (%4i)] ", i, TmapInfo[i].filename, tb[i]);
			if (count++ >= 4) {
				PHYSFSX_printf(my_file, "\n");
				count = 0;
			}
		}
#elif defined(DXX_BUILD_DESCENT_II)
	for (i=0; i<MAX_BITMAP_FILES; i++)
		if (tb[i]) {
			PHYSFSX_printf(my_file, "[%3i %8s (%4i)]\n", i, AllBitmaps[i].name, tb[i]);
		}
#endif
}

#if defined(DXX_BUILD_DESCENT_I)
//	-----------------------------------------------------------------------------
static void say_used_once_tmaps(PHYSFS_file *my_file, int *tb, sbyte *tb_lnum)
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

			PHYSFSX_printf(my_file, "Texture %3i %8s used only once on level %s\n", i, TmapInfo[i].filename, level_name);
		}
}
#endif

// ----------------------------------------------------------------------------
static void say_unused_tmaps(PHYSFS_file *my_file, int *tb)
{
	int	i;
	int	count = 0;

#if defined(DXX_BUILD_DESCENT_I)
	for (i=0; i<Num_tmaps; i++)
#elif defined(DXX_BUILD_DESCENT_II)
	for (i=0; i<MAX_BITMAP_FILES; i++)
#endif
		if (!tb[i]) {
			if (GameBitmaps[Textures[i].index].bm_data == bogus_data)
				PHYSFSX_printf(my_file, "U");
			else
				PHYSFSX_printf(my_file, " ");

#if defined(DXX_BUILD_DESCENT_I)
			PHYSFSX_printf(my_file, "[%3i %8s] ", i, TmapInfo[i].filename);
#elif defined(DXX_BUILD_DESCENT_II)
			PHYSFSX_printf(my_file, "[%3i %8s] ", i, AllBitmaps[i].name);
#endif
			if (count++ >= 4) {
				PHYSFSX_printf(my_file, "\n");
				count = 0;
			}
		}
}

#if defined(DXX_BUILD_DESCENT_I)
// ----------------------------------------------------------------------------
static void say_unused_walls(PHYSFS_file *my_file, int *tb)
{
	int	i;
	for (i=0; i<Num_wall_anims; i++)
		if (!tb[i])
			PHYSFSX_printf(my_file, "Wall %3i is unused.\n", i);
}
#endif

static void say_totals(PHYSFS_file *my_file, const char *level_name)
{
	int	i;		//, objnum;
	int	total_robots = 0;
	int	objects_processed = 0;

	int	used_objects[MAX_OBJECTS];

	PHYSFSX_printf(my_file, "\nLevel %s\n", level_name);

	for (i=0; i<MAX_OBJECTS; i++)
		used_objects[i] = 0;

	while (objects_processed < Highest_object_index+1) {
		int	j, objtype, objid, objcount, cur_obj_val, min_obj_val, min_objnum;

		//	Find new min objnum.
		min_obj_val = 0x7fff0000;
		min_objnum = -1;

		for (j=0; j<=Highest_object_index; j++) {
			if (!used_objects[j] && Objects[j].type!=OBJ_NONE) {
				cur_obj_val = Objects[j].type * 1000 + Objects[j].id;
				if (cur_obj_val < min_obj_val) {
					min_objnum = j;
					min_obj_val = cur_obj_val;
				}
			}
		}
		if ((min_objnum == -1) || (Objects[min_objnum].type == 255))
			break;

		objcount = 0;

		objtype = Objects[min_objnum].type;
		objid = Objects[min_objnum].id;

		for (i=0; i<=Highest_object_index; i++) {
			if (!used_objects[i]) {

				if (((Objects[i].type == objtype) && (Objects[i].id == objid)) ||
						((Objects[i].type == objtype) && (objtype == OBJ_PLAYER)) ||
						((Objects[i].type == objtype) && (objtype == OBJ_COOP)) ||
						((Objects[i].type == objtype) && (objtype == OBJ_HOSTAGE))) {
					if (Objects[i].type == OBJ_ROBOT)
						total_robots++;
					used_objects[i] = 1;
					objcount++;
					objects_processed++;
				}
			}
		}

		if (objcount) {
			PHYSFSX_printf(my_file, "Object: ");
			PHYSFSX_printf(my_file, "%8s %8s %3i\n", object_types(min_objnum), object_ids(min_objnum), objcount);
		}
	}

	PHYSFSX_printf(my_file, "Total robots = %3i\n", total_robots);
}

#if defined(DXX_BUILD_DESCENT_II)
int	First_dump_level = 0;
int	Last_dump_level = NUM_ADAM_LEVELS-1;
#endif

// ----------------------------------------------------------------------------
static void say_totals_all(void)
{
	int	i;
	PHYSFS_file	*my_file;

	my_file = PHYSFSX_openWriteBuffered( "levels.all" );

	if (!my_file)	{
		gr_palette_load(gr_palette);
		nm_messagebox( NULL, 1, "Ok", "ERROR: Unable to open levels.all\nErrno=%i", errno );

		return;
	}

#if defined(DXX_BUILD_DESCENT_I)
	for (i=0; i<NUM_SHAREWARE_LEVELS; i++) {
		load_level(Shareware_level_names[i]);
		say_totals(my_file, Shareware_level_names[i]);
	}

	for (i=0; i<NUM_REGISTERED_LEVELS; i++) {
		load_level(Registered_level_names[i]);
		say_totals(my_file, Registered_level_names[i]);
	}
#elif defined(DXX_BUILD_DESCENT_II)
	for (i=First_dump_level; i<=Last_dump_level; i++) {
		load_level(Adam_level_names[i]);
		say_totals(my_file, Adam_level_names[i]);
	}
#endif

	PHYSFS_close(my_file);
}

static void dump_used_textures_level(PHYSFS_file *my_file, int level_num)
{
#if defined(DXX_BUILD_DESCENT_I)
	int	temp_tmap_buf[MAX_TEXTURES];
	sbyte	level_tmap_buf[MAX_TEXTURES];
	int	temp_wall_buf[MAX_WALL_ANIMS];
#elif defined(DXX_BUILD_DESCENT_II)
	int	temp_tmap_buf[MAX_BITMAP_FILES];
	sbyte level_tmap_buf[MAX_BITMAP_FILES];
	int	temp_wall_buf[MAX_BITMAP_FILES];
#endif

	for (unsigned i=0; i<(sizeof(level_tmap_buf)/sizeof(level_tmap_buf[0])); i++)
		level_tmap_buf[i] = -1;

	determine_used_textures_level(0, 1, level_num, temp_tmap_buf, temp_wall_buf, level_tmap_buf, sizeof(level_tmap_buf)/sizeof(level_tmap_buf[0]));
	PHYSFSX_printf(my_file, "\nTextures used in [%s]\n", Gamesave_current_filename);
	say_used_tmaps(my_file, temp_tmap_buf);
}

// ----------------------------------------------------------------------------
void dump_used_textures_all(void)
{
	PHYSFS_file	*my_file;
	int	i;
#if defined(DXX_BUILD_DESCENT_I)
	int	temp_tmap_buf[MAX_TEXTURES];
	int	perm_tmap_buf[MAX_TEXTURES];
	sbyte	level_tmap_buf[MAX_TEXTURES];
	int	temp_wall_buf[MAX_WALL_ANIMS];
	int	perm_wall_buf[MAX_WALL_ANIMS];
#elif defined(DXX_BUILD_DESCENT_II)
	int	temp_tmap_buf[MAX_BITMAP_FILES];
	int	perm_tmap_buf[MAX_BITMAP_FILES];
	sbyte level_tmap_buf[MAX_BITMAP_FILES];
	int	temp_wall_buf[MAX_BITMAP_FILES];
#endif

say_totals_all();

	my_file = PHYSFSX_openWriteBuffered( "textures.dmp" );

	if (!my_file)	{
		gr_palette_load(gr_palette);
		nm_messagebox( NULL, 1, "Ok", "ERROR: Can't open textures.dmp\nErrno=%i", errno);

		return;
	}

	for (unsigned i=0; i<sizeof(perm_tmap_buf)/sizeof(perm_tmap_buf[0]); i++) {
		perm_tmap_buf[i] = 0;
		level_tmap_buf[i] = -1;
	}

#if defined(DXX_BUILD_DESCENT_I)
	for (i=0; i<MAX_WALL_ANIMS; i++) {
		perm_wall_buf[i] = 0;
	}

	for (i=0; i<NUM_SHAREWARE_LEVELS; i++) {
		determine_used_textures_level(1, 1, i, temp_tmap_buf, temp_wall_buf, level_tmap_buf, sizeof(level_tmap_buf)/sizeof(level_tmap_buf[0]));
		PHYSFSX_printf(my_file, "\nTextures used in [%s]\n", Shareware_level_names[i]);
		say_used_tmaps(my_file, temp_tmap_buf);
		merge_buffers(perm_tmap_buf, temp_tmap_buf, MAX_TEXTURES);
		merge_buffers(perm_wall_buf, temp_wall_buf, MAX_WALL_ANIMS);
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
		determine_used_textures_level(1, 0, i, temp_tmap_buf, temp_wall_buf, level_tmap_buf, sizeof(level_tmap_buf)/sizeof(level_tmap_buf[0]));
#if defined(DXX_BUILD_DESCENT_I)
		PHYSFSX_printf(my_file, "\nTextures used in [%s]\n", Registered_level_names[i]);
#elif defined(DXX_BUILD_DESCENT_II)
		PHYSFSX_printf(my_file, "\nTextures used in [%s]\n", Adam_level_names[i]);
#endif
		say_used_tmaps(my_file, temp_tmap_buf);
		merge_buffers(perm_tmap_buf, temp_tmap_buf, sizeof(perm_tmap_buf)/sizeof(perm_tmap_buf[0]));
	}

	PHYSFSX_printf(my_file, "\n\nUsed textures in all (including registered) mines:\n");
	say_used_tmaps(my_file, perm_tmap_buf);

	PHYSFSX_printf(my_file, "\nUnused textures in all (including registered) mines:\n");
	say_unused_tmaps(my_file, perm_tmap_buf);

	PHYSFS_close(my_file);
}

#endif
