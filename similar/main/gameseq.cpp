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
 * Routines for EndGame, EndLevel, etc.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_MSC_VER) && !defined(macintosh)
#include <unistd.h>
#endif
#include <time.h>

#ifdef OGL
#include "ogl_init.h"
#endif

#include "inferno.h"
#include "game.h"
#include "player.h"
#include "key.h"
#include "object.h"
#include "physics.h"
#include "dxxerror.h"
#include "joy.h"
#include "iff.h"
#include "pcx.h"
#include "timer.h"
#include "render.h"
#include "laser.h"
#include "screens.h"
#include "textures.h"
#include "slew.h"
#include "gauges.h"
#include "texmap.h"
#include "3d.h"
#include "effects.h"
#include "menu.h"
#include "gameseg.h"
#include "wall.h"
#include "ai.h"
#include "fuelcen.h"
#include "switch.h"
#include "digi.h"
#include "gamesave.h"
#include "scores.h"
#include "u_mem.h"
#include "palette.h"
#include "morph.h"
#include "lighting.h"
#include "newdemo.h"
#include "titles.h"
#include "collide.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "gameseq.h"
#include "gamefont.h"
#include "newmenu.h"
#include "endlevel.h"
#  include "multi.h"
#include "playsave.h"
#include "ctype.h"
#include "fireball.h"
#include "kconfig.h"
#include "config.h"
#include "robot.h"
#include "automap.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "text.h"
#include "piggy.h"
#include "texmerge.h"
#include "paging.h"
#include "mission.h"
#include "state.h"
#include "songs.h"
#include "gamepal.h"
#include "controls.h"
#include "credits.h"
#include "gamemine.h"
#ifdef EDITOR
#include "editor/editor.h"
#endif
#include "strutil.h"
#include "rle.h"
#include "byteutil.h"
#include "segment.h"
#include "gameseg.h"
#include "fmtcheck.h"

#include "compiler-range_for.h"
#include "highest_valid.h"

#if defined(DXX_BUILD_DESCENT_I)
#include "custom.h"
#define GLITZ_BACKGROUND	Menu_pcx_name

static int AdvanceLevel(int secret_flag);
#elif defined(DXX_BUILD_DESCENT_II)
#include "movie.h"
#define GLITZ_BACKGROUND	STARS_BACKGROUND

static void StartNewLevelSecret(int level_num, int page_in_textures);
static void InitPlayerPosition(int random_flag);
static void DoEndGame(void);
static void filter_objects_from_level();
static void AdvanceLevel(int secret_flag);
#endif
static void StartLevel(int random_flag);
static void copy_defaults_to_robot_all(void);

//Current_level_num starts at 1 for the first level
//-1,-2,-3 are secret levels
//0 means not a real level loaded
int	Current_level_num=0,Next_level_num;
PHYSFSX_gets_line_t<LEVEL_NAME_LEN> Current_level_name;

// Global variables describing the player
unsigned	N_players=1;	// Number of players ( >1 means a net game, eh?)
playernum_t Player_num;	// The player number who is on the console.
array<player, MAX_PLAYERS + DXX_PLAYER_HEADER_ADD_EXTRA_PLAYERS> Players;   // Misc player info
#if defined(DXX_BUILD_DESCENT_II)
int	First_secret_visit = 1;
#endif
obj_position	Player_init[MAX_PLAYERS];

// Global variables telling what sort of game we have
int NumNetPlayerPositions = -1;
int	Do_appearance_effect=0;

//--------------------------------------------------------------------
static void verify_console_object()
{
	Assert(Player_num < Players.size());
	Assert( Players[Player_num].objnum != object_none );
	ConsoleObject = &Objects[Players[Player_num].objnum];
	Assert( ConsoleObject->type==OBJ_PLAYER );
	Assert( get_player_id(ConsoleObject)==Player_num );
}

static int count_number_of_robots()
{
	int robot_count;
	robot_count = 0;
	range_for (auto i, highest_valid(Objects))
	{
		if (Objects[i].type == OBJ_ROBOT)
			robot_count++;
	}

	return robot_count;
}


static int count_number_of_hostages()
{
	int count;
	count = 0;
	range_for (auto i, highest_valid(Objects))
	{
		if (Objects[i].type == OBJ_HOSTAGE)
			count++;
	}

	return count;
}

//added 10/12/95: delete buddy bot if coop game.  Probably doesn't really belong here. -MT
static void gameseq_init_network_players()
{
	int k,j;

	// Initialize network player start locations and object numbers

	ConsoleObject = &Objects[0];
	k = 0;
	j = 0;
	range_for (auto i, highest_valid(Objects))
	{
		const auto o = vobjptridx(i);
		if (( o->type==OBJ_PLAYER )	|| (o->type == OBJ_GHOST) || (o->type == OBJ_COOP))
		{
			if ( (!(Game_mode & GM_MULTI_COOP) && ((o->type == OBJ_PLAYER)||(o->type==OBJ_GHOST))) ||
	           ((Game_mode & GM_MULTI_COOP) && ((j == 0) || ( o->type==OBJ_COOP ))) )
			{
				o->type=OBJ_PLAYER;
				Player_init[k].pos = o->pos;
				Player_init[k].orient = o->orient;
				Player_init[k].segnum = o->segnum;
				Players[k].objnum = i;
				set_player_id(o, k);
				k++;
			}
			else
				obj_delete(o);
			j++;
		}
		if ((o->type==OBJ_ROBOT) && robot_is_companion(&Robot_info[get_robot_id(o)]) && (Game_mode & GM_MULTI))
			obj_delete(o);		//kill the buddy in netgames
	}
	NumNetPlayerPositions = k;
}

void gameseq_remove_unused_players()
{
	int i;

	// 'Remove' the unused players

	if (Game_mode & GM_MULTI)
	{
		for (i=0; i < NumNetPlayerPositions; i++)
		{
			if ((!Players[i].connected) || (i >= N_players))
			{
				multi_make_player_ghost(i);
			}
		}
	}
	else
	{		// Note link to above if!!!
		for (i=1; i < NumNetPlayerPositions; i++)
		{
			obj_delete(Players[i].objnum);
		}
	}
}

fix StartingShields=INITIAL_SHIELDS;

// Setup player for new game
void init_player_stats_game(ubyte pnum)
{
	Players[pnum].score = 0;
	Players[pnum].last_score = 0;
	Players[pnum].lives = INITIAL_LIVES;
	Players[pnum].level = 1;
	Players[pnum].time_level = 0;
	Players[pnum].time_total = 0;
	Players[pnum].hours_level = 0;
	Players[pnum].hours_total = 0;
	Players[pnum].killer_objnum = object_none;
	Players[pnum].net_killed_total = 0;
	Players[pnum].net_kills_total = 0;
	Players[pnum].num_kills_level = 0;
	Players[pnum].num_kills_total = 0;
	Players[pnum].num_robots_level = 0;
	Players[pnum].num_robots_total = 0;
	Players[pnum].KillGoalCount = 0;
	Players[pnum].hostages_rescued_total = 0;
	Players[pnum].hostages_level = 0;
	Players[pnum].hostages_total = 0;
	Players[pnum].laser_level = 0;
	Players[pnum].flags = 0;

	init_player_stats_new_ship(pnum);
#if defined(DXX_BUILD_DESCENT_II)
	if (pnum == Player_num)
		First_secret_visit = 1;
#endif
}

static void init_ammo_and_energy(void)
{
	if (Players[Player_num].energy < INITIAL_ENERGY)
		Players[Player_num].energy = INITIAL_ENERGY;
	if (Players[Player_num].shields < StartingShields)
		Players[Player_num].shields = StartingShields;

//	for (i=0; i<MAX_SECONDARY_WEAPONS; i++)
//		if (Players[Player_num].secondary_ammo[i] < Default_secondary_ammo_level[i])
//			Players[Player_num].secondary_ammo[i] = Default_secondary_ammo_level[i];
	if (Players[Player_num].secondary_ammo[0] < 2 + NDL - Difficulty_level)
		Players[Player_num].secondary_ammo[0] = 2 + NDL - Difficulty_level;
}

#if defined(DXX_BUILD_DESCENT_II)
extern	ubyte	Last_afterburner_state;
#endif

// Setup player for new level (After completion of previous level)
void init_player_stats_level(int secret_flag)
{
#if defined(DXX_BUILD_DESCENT_I)
	secret_flag = 0;
#endif
	// int	i;

	Players[Player_num].last_score = Players[Player_num].score;

	Players[Player_num].level = Current_level_num;

	if (!Network_rejoined) {
		Players[Player_num].time_level = 0;
		Players[Player_num].hours_level = 0;
	}

	Players[Player_num].killer_objnum = object_none;

	Players[Player_num].num_kills_level = 0;
	Players[Player_num].num_robots_level = count_number_of_robots();
	Players[Player_num].num_robots_total += Players[Player_num].num_robots_level;

	Players[Player_num].hostages_level = count_number_of_hostages();
	Players[Player_num].hostages_total += Players[Player_num].hostages_level;
	Players[Player_num].hostages_on_board = 0;

	if (!secret_flag) {
		init_ammo_and_energy();

		Players[Player_num].flags &= (~KEY_BLUE);
		Players[Player_num].flags &= (~KEY_RED);
		Players[Player_num].flags &= (~KEY_GOLD);

		Players[Player_num].flags &= ~(PLAYER_FLAGS_INVULNERABLE | PLAYER_FLAGS_CLOAKED);
#if defined(DXX_BUILD_DESCENT_II)
		Players[Player_num].flags &= ~(PLAYER_FLAGS_MAP_ALL);
#endif

		Players[Player_num].cloak_time = 0;
		Players[Player_num].invulnerable_time = 0;

		if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
			Players[Player_num].flags |= (KEY_BLUE | KEY_RED | KEY_GOLD);
	}

	Player_is_dead = 0; // Added by RH
	Dead_player_camera = NULL;
	Players[Player_num].homing_object_dist = -F1_0; // Added by RH

	// properly init these cursed globals
	Next_flare_fire_time = Last_laser_fired_time = Next_laser_fire_time = Next_missile_fire_time = GameTime64;
#if defined(DXX_BUILD_DESCENT_II)
	Controls.state.afterburner = 0;
	Last_afterburner_state = 0;

	digi_kill_sound_linked_to_object(vcobjptridx(Players[Player_num].objnum));
#endif
	init_gauges();
#if defined(DXX_BUILD_DESCENT_II)
	Missile_viewer = NULL;
#endif
}

// Setup player for a brand-new ship
void init_player_stats_new_ship(ubyte pnum)
{
	int i;

	if (pnum == Player_num)
	{
		if (Newdemo_state == ND_STATE_RECORDING)
		{
			newdemo_record_laser_level(Players[Player_num].laser_level, 0);
			newdemo_record_player_weapon(0, 0);
			newdemo_record_player_weapon(1, 0);
		}
		Primary_weapon = 0;
		Secondary_weapon = 0;
		dead_player_end(); //player no longer dead
		Player_is_dead = 0;
		Player_exploded = 0;
		Player_eggs_dropped = 0;
		Dead_player_camera = 0;
		Global_laser_firing_count=0;
#if defined(DXX_BUILD_DESCENT_II)
		for (i=0; i<MAX_PRIMARY_WEAPONS; i++)
			Primary_last_was_super[i] = 0;
		for (i=1; i<MAX_SECONDARY_WEAPONS; i++)
			Secondary_last_was_super[i] = 0;
		Afterburner_charge = 0;
		Controls.state.afterburner = 0;
		Last_afterburner_state = 0;
		Missile_viewer=NULL; //reset missile camera if out there
		Missile_viewer_sig=-1;
		init_ai_for_ship();
#endif
	}

	Players[pnum].energy = INITIAL_ENERGY;
	Players[pnum].shields = StartingShields;
	Players[pnum].laser_level = 0;
	Players[pnum].killer_objnum = object_none;
	Players[pnum].hostages_on_board = 0;
	Players[pnum].vulcan_ammo = 0;
	for (i=1; i<MAX_SECONDARY_WEAPONS; i++)
		Players[pnum].secondary_ammo[i] = 0;
	Players[pnum].secondary_ammo[0] = 2 + NDL - Difficulty_level;
	Players[pnum].primary_weapon_flags = HAS_LASER_FLAG;
	Players[pnum].secondary_weapon_flags = HAS_CONCUSSION_FLAG;
	Players[pnum].flags &= ~(PLAYER_FLAGS_QUAD_LASERS | PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE);
#if defined(DXX_BUILD_DESCENT_II)
	Players[pnum].flags &= ~(PLAYER_FLAGS_AFTERBURNER | PLAYER_FLAGS_MAP_ALL | PLAYER_FLAGS_CONVERTER | PLAYER_FLAGS_AMMO_RACK | PLAYER_FLAGS_HEADLIGHT | PLAYER_FLAGS_HEADLIGHT_ON | PLAYER_FLAGS_FLAG);
#endif
	Players[pnum].cloak_time = 0;
	Players[pnum].invulnerable_time = 0;
	Players[pnum].homing_object_dist = -F1_0; // Added by RH
	digi_kill_sound_linked_to_object(vcobjptridx(Players[pnum].objnum));
}

#ifdef EDITOR
//reset stuff so game is semi-normal when playing from editor
void editor_reset_stuff_on_level()
{
	gameseq_init_network_players();
	init_player_stats_level(0);
	Viewer = ConsoleObject;
	ConsoleObject = Viewer = &Objects[Players[Player_num].objnum];
	set_player_id(ConsoleObject, Player_num);
	ConsoleObject->control_type = CT_FLYING;
	ConsoleObject->movement_type = MT_PHYSICS;
	Game_suspended = 0;
	verify_console_object();
	Control_center_destroyed = 0;
	if (Newdemo_state != ND_STATE_PLAYBACK)
		gameseq_remove_unused_players();
	init_cockpit();
	init_robots_for_level();
	init_ai_objects();
	init_morphs();
	init_all_matcens();
	init_player_stats_new_ship(Player_num);
#if defined(DXX_BUILD_DESCENT_II)
	init_controlcen_for_level();
	automap_clear_visited();
	init_stuck_objects();
	init_thief_for_level();

	Slide_segs_computed = 0;
#endif
	if (!Game_wind)
		Game_wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, game_handler, unused_window_userdata);
}
#endif

//do whatever needs to be done when a player dies in multiplayer

static void DoGameOver()
{
	if (PLAYING_BUILTIN_MISSION)
		scores_maybe_add_player(0);

	if (Game_wind)
		window_close(Game_wind);		// Exit out of game loop
}

//update various information about the player
void update_player_stats()
{
	Players[Player_num].time_level += FrameTime;	//the never-ending march of time...
	if ( Players[Player_num].time_level > i2f(3600) )	{
		Players[Player_num].time_level -= i2f(3600);
		Players[Player_num].hours_level++;
	}

	Players[Player_num].time_total += FrameTime;	//the never-ending march of time...
	if ( Players[Player_num].time_total > i2f(3600) )	{
		Players[Player_num].time_total -= i2f(3600);
		Players[Player_num].hours_total++;
	}
}

//go through this level and start any eclip sounds
static void set_sound_sources()
{
	int sidenum;

	digi_init_sounds();		//clear old sounds
#if defined(DXX_BUILD_DESCENT_II)
	Dont_start_sound_objects = 1;
#endif

	range_for (auto segnum, highest_valid(Segments))
	{
		auto seg = &Segments[segnum];
		for (sidenum=0;sidenum<MAX_SIDES_PER_SEGMENT;sidenum++) {
			int tm,ec,sn;

#if defined(DXX_BUILD_DESCENT_I)
			if ((tm=seg->sides[sidenum].tmap_num2) != 0)
				if ((ec=TmapInfo[tm&0x3fff].eclip_num)!=-1)
#elif defined(DXX_BUILD_DESCENT_II)
			auto wid = WALL_IS_DOORWAY(seg, sidenum);
			if (wid & WID_RENDER_FLAG)
				if ((((tm=seg->sides[sidenum].tmap_num2) != 0) && ((ec=TmapInfo[tm&0x3fff].eclip_num)!=-1)) || ((ec=TmapInfo[seg->sides[sidenum].tmap_num].eclip_num)!=-1))
#endif
					if ((sn=Effects[ec].sound_num)!=-1) {
#if defined(DXX_BUILD_DESCENT_II)
						auto csegnum = seg->children[sidenum];

						//check for sound on other side of wall.  Don't add on
						//both walls if sound travels through wall.  If sound
						//does travel through wall, add sound for lower-numbered
						//segment.

						if (IS_CHILD(csegnum) && csegnum < segnum) {
							if (wid & (WID_FLY_FLAG|WID_RENDPAST_FLAG)) {
								auto csegp = &Segments[seg->children[sidenum]];
								auto csidenum = find_connect_side(seg, csegp);

								if (csegp->sides[csidenum].tmap_num2 == seg->sides[sidenum].tmap_num2)
									continue;		//skip this one
							}
						}
#endif

						const auto pnt = compute_center_point_on_side(seg,sidenum);
						digi_link_sound_to_pos(sn,segnum,sidenum,pnt,1, F1_0/2);
					}
		}
	}

#if defined(DXX_BUILD_DESCENT_II)
	Dont_start_sound_objects = 0;
#endif
}


//fix flash_dist=i2f(1);
fix flash_dist=fl2f(.9);

//create flash for player appearance
void create_player_appearance_effect(const vobjptridx_t player_obj)
{
	const auto pos = (player_obj == Viewer)
		? vm_vec_scale_add(player_obj->pos, player_obj->orient.fvec, fixmul(player_obj->size,flash_dist))
		: player_obj->pos;

	const objptridx_t effect_obj = object_create_explosion(player_obj->segnum, pos, player_obj->size, VCLIP_PLAYER_APPEARANCE );

	if (effect_obj) {
		effect_obj->orient = player_obj->orient;

		if ( Vclip[VCLIP_PLAYER_APPEARANCE].sound_num > -1 )
			digi_link_sound_to_object( Vclip[VCLIP_PLAYER_APPEARANCE].sound_num, effect_obj, 0, F1_0);
	}
}

//
// New Game sequencing functions
//

//get level filename. level numbers start at 1.  Secret levels are -1,-2,-3
static const d_fname &get_level_file(int level_num)
{
	if (level_num<0)                //secret level
		return Secret_level_names[-level_num-1];
	else                                    //normal level
		return Level_names[level_num-1];
}

// routine to calculate the checksum of the segments.
static void do_checksum_calc(ubyte *b, int len, unsigned int *s1, unsigned int *s2)
{

	while(len--) {
		*s1 += *b++;
		if (*s1 >= 255) *s1 -= 255;
		*s2 += *s1;
	}
}

static ushort netmisc_calc_checksum()
{
	int i, j, k;
	unsigned int sum1,sum2;
	short s;
	int t;

	sum1 = sum2 = 0;
	for (i = 0; i < Highest_segment_index + 1; i++) {
		for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++) {
			do_checksum_calc((unsigned char *)&(Segments[i].sides[j].get_type()), 1, &sum1, &sum2);
			s = INTEL_SHORT(Segments[i].sides[j].wall_num);
			do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
			s = INTEL_SHORT(Segments[i].sides[j].tmap_num);
			do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
			s = INTEL_SHORT(Segments[i].sides[j].tmap_num2);
			do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
			for (k = 0; k < 4; k++) {
				t = INTEL_INT(((int)Segments[i].sides[j].uvls[k].u));
				do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
				t = INTEL_INT(((int)Segments[i].sides[j].uvls[k].v));
				do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
				t = INTEL_INT(((int)Segments[i].sides[j].uvls[k].l));
				do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
			}
			for (k = 0; k < 2; k++) {
				t = INTEL_INT(((int)Segments[i].sides[j].normals[k].x));
				do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
				t = INTEL_INT(((int)Segments[i].sides[j].normals[k].y));
				do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
				t = INTEL_INT(((int)Segments[i].sides[j].normals[k].z));
				do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
			}
		}
		for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++) {
			s = INTEL_SHORT(Segments[i].children[j]);
			do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
		}
		for (j = 0; j < MAX_VERTICES_PER_SEGMENT; j++) {
			s = INTEL_SHORT(Segments[i].verts[j]);
			do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
		}
		s = INTEL_SHORT(Segments[i].objects);
		do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
#if defined(DXX_BUILD_DESCENT_I)
		do_checksum_calc((unsigned char *)&(Segments[i].special), 1, &sum1, &sum2);
		do_checksum_calc((unsigned char *)&(Segments[i].matcen_num), 1, &sum1, &sum2);
		s = INTEL_SHORT(Segments[i].value);
		do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
		t = INTEL_INT(((int)Segments[i].static_light));
		do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
		s = INTEL_SHORT(0); // no matter if we need alignment on our platform, if we have editor we MUST consider this integer to get the same checksum as non-editor games calculate
		do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
#endif
	}
	sum2 %= 255;
	return ((sum1<<8)+ sum2);
}

#if defined(DXX_BUILD_DESCENT_II)
// load just the hxm file
void load_level_robots(int level_num)
{
	Assert(level_num <= Last_level  && level_num >= Last_secret_level  && level_num != 0);
	const d_fname &level_name = get_level_file(level_num);
	if (Robot_replacements_loaded) {
		free_polygon_models();
		load_mission_ham();
		Robot_replacements_loaded = 0;
	}
	load_robot_replacements(level_name);
}
#endif

//load a level off disk. level numbers start at 1.  Secret levels are -1,-2,-3
void LoadLevel(int level_num,int page_in_textures)
{
	player save_player;

	save_player = Players[Player_num];

	Assert(level_num <= Last_level  && level_num >= Last_secret_level  && level_num != 0);
	const d_fname &level_name = get_level_file(level_num);
#if defined(DXX_BUILD_DESCENT_I)
	if (!load_level(level_name))
		Current_level_num=level_num;

	gr_use_palette_table( "palette.256" );
#elif defined(DXX_BUILD_DESCENT_II)
	gr_set_current_canvas(NULL);
	gr_clear_canvas(BM_XRGB(0, 0, 0));		//so palette switching is less obvious

	int load_ret = load_level(level_name);		//actually load the data from disk!

	if (load_ret)
		Error("Couldn't load level file <%s>, error = %d",static_cast<const char *>(level_name),load_ret);

	Current_level_num=level_num;

	load_palette(Current_level_palette,1,1);		//don't change screen
#endif

	show_boxed_message(TXT_LOADING, 0);
#ifdef RELEASE
	timer_delay(F1_0);
#endif

	load_endlevel_data(level_num);
#if defined(DXX_BUILD_DESCENT_I)
	load_custom_data(level_name);
#elif defined(DXX_BUILD_DESCENT_II)
	if (EMULATING_D1)
		load_d1_bitmap_replacements();
	else
		load_bitmap_replacements(level_name);

	load_level_robots(level_num);

	if ( page_in_textures )
		piggy_load_level_data();
#endif

	my_segments_checksum = netmisc_calc_checksum();

	reset_network_objects();

	Players[Player_num] = save_player;

	set_sound_sources();

	songs_play_level_song( Current_level_num, 0 );

	gr_palette_load(gr_palette);		//actually load the palette
#if defined(DXX_BUILD_DESCENT_I)
	if ( page_in_textures )
		piggy_load_level_data();
#endif
}

//sets up Player_num & ConsoleObject
void InitPlayerObject()
{
	Assert(Player_num<MAX_PLAYERS);

	if (Player_num != 0 )	{
		Players[0] = Players[Player_num];
		Player_num = 0;
	}

	Players[Player_num].objnum = object_first;

	ConsoleObject = &Objects[Players[Player_num].objnum];

	ConsoleObject->type				= OBJ_PLAYER;
	set_player_id(ConsoleObject, Player_num);
	ConsoleObject->control_type	= CT_FLYING;
	ConsoleObject->movement_type	= MT_PHYSICS;
}

//starts a new game on the given level
void StartNewGame(int start_level)
{
	state_quick_item = -1;	// for first blind save, pick slot to save in

	Game_mode = GM_NORMAL;

	Next_level_num = 0;

	InitPlayerObject();				//make sure player's object set up

	init_player_stats_game(Player_num);		//clear all stats

	N_players = 1;

#if defined(DXX_BUILD_DESCENT_II)
	if (start_level < 0)
		StartNewLevelSecret(start_level, 0);
	else
#endif
		StartNewLevel(start_level);

	Players[Player_num].starting_level = start_level;		// Mark where they started

	game_disable_cheats();
#if defined(DXX_BUILD_DESCENT_II)
	init_seismic_disturbances();
#endif
}

//	-----------------------------------------------------------------------------
//	Does the bonus scoring.
//	Call with dead_flag = 1 if player died, but deserves some portion of bonus (only skill points), anyway.
void DoEndLevelScoreGlitz(int network)
{
	int level_points, skill_points, energy_points, shield_points, hostage_points;
	int	all_hostage_points;
	int	endgame_points;
	char	all_hostage_text[64];
	char	endgame_text[64];
	#define N_GLITZITEMS 9
	char				m_str[N_GLITZITEMS][30];
	newmenu_item	m[N_GLITZITEMS];
	int				i,c;
	char				title[128];
	int				is_last_level;
#if defined(DXX_BUILD_DESCENT_I)
	gr_palette_load( gr_palette );
#elif defined(DXX_BUILD_DESCENT_II)
	int				mine_level;

	//	Compute level player is on, deal with secret levels (negative numbers)
	mine_level = Players[Player_num].level;
	if (mine_level < 0)
		mine_level *= -(Last_level/N_secret_levels);
#endif

	level_points = Players[Player_num].score-Players[Player_num].last_score;

	if (!cheats.enabled) {
		if (Difficulty_level > 1) {
#if defined(DXX_BUILD_DESCENT_I)
			skill_points = level_points*(Difficulty_level-1)/2;
#elif defined(DXX_BUILD_DESCENT_II)
			skill_points = level_points*(Difficulty_level)/4;
#endif
			skill_points -= skill_points % 100;
		} else
			skill_points = 0;

		hostage_points = Players[Player_num].hostages_on_board * 500 * (Difficulty_level+1);
#if defined(DXX_BUILD_DESCENT_I)
		shield_points = f2i(Players[Player_num].shields) * 10 * (Difficulty_level+1);
		energy_points = f2i(Players[Player_num].energy) * 5 * (Difficulty_level+1);
#elif defined(DXX_BUILD_DESCENT_II)
		shield_points = f2i(Players[Player_num].shields) * 5 * mine_level;
		energy_points = f2i(Players[Player_num].energy) * 2 * mine_level;

		shield_points -= shield_points % 50;
		energy_points -= energy_points % 50;
#endif
	} else {
		skill_points = 0;
		shield_points = 0;
		energy_points = 0;
		hostage_points = 0;
	}

	all_hostage_text[0] = 0;
	endgame_text[0] = 0;

	if (!cheats.enabled && (Players[Player_num].hostages_on_board == Players[Player_num].hostages_level)) {
		all_hostage_points = Players[Player_num].hostages_on_board * 1000 * (Difficulty_level+1);
		sprintf(all_hostage_text, "%s%i\n", TXT_FULL_RESCUE_BONUS, all_hostage_points);
	} else
		all_hostage_points = 0;

	if (!cheats.enabled && !(Game_mode & GM_MULTI) && (Players[Player_num].lives) && (Current_level_num == Last_level)) {		//player has finished the game!
		endgame_points = Players[Player_num].lives * 10000;
		sprintf(endgame_text, "%s%i\n", TXT_SHIP_BONUS, endgame_points);
		is_last_level=1;
	} else
		endgame_points = is_last_level = 0;

	add_bonus_points_to_score(skill_points + energy_points + shield_points + hostage_points + all_hostage_points + endgame_points);

	c = 0;
	sprintf(m_str[c++], "%s%i", TXT_SHIELD_BONUS, shield_points);		// Return at start to lower menu...
	sprintf(m_str[c++], "%s%i", TXT_ENERGY_BONUS, energy_points);
	sprintf(m_str[c++], "%s%i", TXT_HOSTAGE_BONUS, hostage_points);
	sprintf(m_str[c++], "%s%i", TXT_SKILL_BONUS, skill_points);

	sprintf(m_str[c++], "%s", all_hostage_text);
	if (!(Game_mode & GM_MULTI) && (Players[Player_num].lives) && (Current_level_num == Last_level))
		sprintf(m_str[c++], "%s", endgame_text);

	sprintf(m_str[c++], "%s%i\n", TXT_TOTAL_BONUS, shield_points+energy_points+hostage_points+skill_points+all_hostage_points+endgame_points);
	sprintf(m_str[c++], "%s%i", TXT_TOTAL_SCORE, Players[Player_num].score);

	for (i=0; i<c; i++) {
		nm_set_item_text(& m[i], m_str[i]);
	}

	auto current_level_num = Current_level_num;
	const auto txt_level = (current_level_num < 0) ? (current_level_num = -current_level_num, TXT_SECRET_LEVEL) : TXT_LEVEL;
	snprintf(title, sizeof(title), "%s%s %d %s\n%s %s", is_last_level?"\n\n\n":"\n", txt_level, current_level_num, TXT_COMPLETE, static_cast<const char *>(Current_level_name), TXT_DESTROYED);

	Assert(c <= N_GLITZITEMS);

	if ( network && (Game_mode & GM_NETWORK) )
		newmenu_do2(NULL, title, c, m, multi_endlevel_poll1, unused_newmenu_userdata, 0, GLITZ_BACKGROUND);
	else
		// NOTE LINK TO ABOVE!!!
		newmenu_do2(NULL, title, c, m, unused_newmenu_subfunction, unused_newmenu_userdata, 0, GLITZ_BACKGROUND);
}

#if defined(DXX_BUILD_DESCENT_II)
//	-----------------------------------------------------------------------------------------------------
//called when the player is starting a level (new game or new ship)
static void StartSecretLevel()
{
	Assert(!Player_is_dead);

	InitPlayerPosition(0);

	verify_console_object();

	ConsoleObject->control_type	= CT_FLYING;
	ConsoleObject->movement_type	= MT_PHYSICS;

	// -- WHY? -- disable_matcens();
	clear_transient_objects(0);		//0 means leave proximity bombs

	// create_player_appearance_effect(ConsoleObject);
	Do_appearance_effect = 1;

	ai_reset_all_paths();
	// -- NO? -- reset_time();

	reset_rear_view();
	Auto_fire_fusion_cannon_time = 0;
	Fusion_charge = 0;
}

//	Returns true if secret level has been destroyed.
int p_secret_level_destroyed(void)
{
	if (First_secret_visit) {
		return 0;		//	Never been there, can't have been destroyed.
	} else {
		if (PHYSFSX_exists(SECRETC_FILENAME,0))
		{
			return 0;
		} else {
			return 1;
		}
	}
}

//	-----------------------------------------------------------------------------------------------------
#define TXT_SECRET_RETURN  "Returning to level %i", Entered_from_level
#define TXT_SECRET_ADVANCE "Base level destroyed.\nAdvancing to level %i", Entered_from_level+1
#endif

static int draw_endlevel_background(newmenu *,const d_event &event, grs_bitmap *background)
{
	switch (event.type)
	{
		case EVENT_WINDOW_DRAW:
			gr_set_current_canvas(NULL);
			show_fullscr(*background);
			break;
			
		default:
			break;
	}
	
	return 0;
}

static void do_screen_message(const char *msg) __attribute_nonnull();
static void do_screen_message(const char *msg)
{
	grs_bitmap background;
	
	if (Game_mode & GM_MULTI)
		return;
	gr_init_bitmap_data(background);
	if (pcx_read_bitmap(GLITZ_BACKGROUND, background, BM_LINEAR, gr_palette) != PCX_ERROR_NONE)
		return;

	gr_palette_load(gr_palette);
	newmenu_item nm_message_items[1];
	nm_set_item_menu(& nm_message_items[0], TXT_OK);
	newmenu_do( NULL, msg, 1, nm_message_items, draw_endlevel_background, static_cast<grs_bitmap *>(&background));
	gr_free_bitmap_data(background);
}

#if defined(DXX_BUILD_DESCENT_II)
static void do_screen_message_fmt(const char *fmt, ...) __attribute_format_printf(1, 2);
static void do_screen_message_fmt(const char *fmt, ...)
{
	va_list arglist;
	char msg[1024];
	va_start(arglist, fmt);
	vsnprintf(msg, sizeof(msg), fmt, arglist);
	va_end(arglist);
	do_screen_message(msg);
}
#define do_screen_message(F,...)	dxx_call_printf_checked(do_screen_message_fmt,do_screen_message,(),F,##__VA_ARGS__)

//	-----------------------------------------------------------------------------------------------------
// called when the player is starting a new level for normal game mode and restore state
//	Need to deal with whether this is the first time coming to this level or not.  If not the
//	first time, instead of initializing various things, need to do a game restore for all the
//	robots, powerups, walls, doors, etc.
static void StartNewLevelSecret(int level_num, int page_in_textures)
{
        ThisLevelTime=0;

	last_drawn_cockpit = -1;

	if (Newdemo_state == ND_STATE_PAUSED)
		Newdemo_state = ND_STATE_RECORDING;

	if (Newdemo_state == ND_STATE_RECORDING) {
		newdemo_set_new_level(level_num);
		newdemo_record_start_frame(FrameTime );
	} else if (Newdemo_state != ND_STATE_PLAYBACK) {

		set_screen_mode(SCREEN_MENU);

		if (First_secret_visit) {
			do_screen_message(TXT_SECRET_EXIT);
		} else {
			if (PHYSFSX_exists(SECRETC_FILENAME,0))
			{
				do_screen_message(TXT_SECRET_EXIT);
			} else {
				do_screen_message("Secret level already destroyed.\nAdvancing to level %i.", Current_level_num+1);
			}
		}
	}

	LoadLevel(level_num,page_in_textures);

	Assert(Current_level_num == level_num); // make sure level set right

	gameseq_init_network_players(); // Initialize the Players array for this level

	HUD_clear_messages();

	automap_clear_visited();

	Viewer = &Objects[Players[Player_num].objnum];

	gameseq_remove_unused_players();

	Game_suspended = 0;

	Control_center_destroyed = 0;

	init_cockpit();
	reset_palette_add();

	if (First_secret_visit || (Newdemo_state == ND_STATE_PLAYBACK)) {
		init_robots_for_level();
		init_ai_objects();
		init_smega_detonates();
		init_morphs();
		init_all_matcens();
		reset_special_effects();
		StartSecretLevel();
	} else {
		if (PHYSFSX_exists(SECRETC_FILENAME,0))
		{
			int	pw_save, sw_save;

			pw_save = Primary_weapon;
			sw_save = Secondary_weapon;
			state_restore_all(1, 1, SECRETC_FILENAME, 0);
			Primary_weapon = pw_save;
			Secondary_weapon = sw_save;
			reset_special_effects();
			StartSecretLevel();
			// -- No: This is only for returning to base level: set_pos_from_return_segment();
		} else {
			do_screen_message("Secret level already destroyed.\nAdvancing to level %i.", Current_level_num+1);
			return;
		}
	}

	if (First_secret_visit) {
		copy_defaults_to_robot_all();
	}

	init_controlcen_for_level();

	// Say player can use FLASH cheat to mark path to exit.
	Last_level_path_created = -1;

	First_secret_visit = 0;
}

int	Entered_from_level;

// ---------------------------------------------------------------------------------------------------------------
//	Called from switch.c when player is on a secret level and hits exit to return to base level.
void ExitSecretLevel(void)
{
	if (Newdemo_state == ND_STATE_PLAYBACK)
		return;

	if (Game_wind)
		window_set_visible(Game_wind, 0);

	if (!Control_center_destroyed) {
		state_save_all(2, SECRETC_FILENAME, 0);
	}

	if (PHYSFSX_exists(SECRETB_FILENAME,0))
	{
		int	pw_save, sw_save;

		do_screen_message(TXT_SECRET_RETURN);
		pw_save = Primary_weapon;
		sw_save = Secondary_weapon;
		state_restore_all(1, 1, SECRETB_FILENAME, 0);
		Primary_weapon = pw_save;
		Secondary_weapon = sw_save;
	} else {
		// File doesn't exist, so can't return to base level.  Advance to next one.
		if (Entered_from_level == Last_level)
			DoEndGame();
		else {
			do_screen_message(TXT_SECRET_ADVANCE);
			StartNewLevel(Entered_from_level+1);
		}
	}

	if (Game_wind)
		window_set_visible(Game_wind, 1);
	reset_time();
}

// ---------------------------------------------------------------------------------------------------------------
//	Set invulnerable_time and cloak_time in player struct to preserve amount of time left to
//	be invulnerable or cloaked.
void do_cloak_invul_secret_stuff(fix64 old_gametime)
{
	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) {
		fix64	time_used;

		time_used = old_gametime - Players[Player_num].invulnerable_time;
		Players[Player_num].invulnerable_time = GameTime64 - time_used;
	}

	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
		fix	time_used;

		time_used = old_gametime - Players[Player_num].cloak_time;
		Players[Player_num].cloak_time = GameTime64 - time_used;
	}
}

// ---------------------------------------------------------------------------------------------------------------
//	Called from switch.c when player passes through secret exit.  That means he was on a non-secret level and he
//	is passing to the secret level.
//	Do a savegame.
void EnterSecretLevel(void)
{
	int i;

	Assert(! (Game_mode & GM_MULTI) );

	if (Game_wind)
		window_set_visible(Game_wind, 0);

	digi_play_sample( SOUND_SECRET_EXIT, F1_0 );	// after above call which stops all sounds
	
	Entered_from_level = Current_level_num;

	if (Control_center_destroyed)
		DoEndLevelScoreGlitz(0);

	if (Newdemo_state != ND_STATE_PLAYBACK)
		state_save_all(1, NULL, 0);	//	Not between levels (ie, save all), IS a secret level, NO filename override

	//	Find secret level number to go to, stuff in Next_level_num.
	for (i=0; i<-Last_secret_level; i++)
		if (Secret_level_table[i]==Current_level_num) {
			Next_level_num = -(i+1);
			break;
		} else if (Secret_level_table[i] > Current_level_num) {	//	Allows multiple exits in same group.
			Next_level_num = -i;
			break;
		}

	if (! (i<-Last_secret_level))		//didn't find level, so must be last
		Next_level_num = Last_secret_level;

	// NMN 04/09/07  Do a REAL start level routine if we are playing a D1 level so we have
	//               briefings
	if (EMULATING_D1)
	{
		set_screen_mode(SCREEN_MENU);
		do_screen_message("Alternate Exit Found!\n\nProceeding to Secret Level!");
		StartNewLevel(Next_level_num);
	} else {
 	   	StartNewLevelSecret(Next_level_num, 1);
	}
	// END NMN

	// do_cloak_invul_stuff();
	if (Game_wind)
		window_set_visible(Game_wind, 1);
	reset_time();
}
#endif

//called when the player has finished a level
void PlayerFinishedLevel(int secret_flag)
{
	if (Game_wind)
		window_set_visible(Game_wind, 0);

	//credit the player for hostages
	Players[Player_num].hostages_rescued_total += Players[Player_num].hostages_on_board;
#if defined(DXX_BUILD_DESCENT_I)
	int	rval;
	int 	was_multi = 0;

	if (!(Game_mode & GM_MULTI) && (secret_flag)) {
		newmenu_item	m[1];

		nm_set_item_text(&m[0], " ");			//TXT_SECRET_EXIT;

		newmenu_do2(NULL, TXT_SECRET_EXIT, 1, m, unused_newmenu_subfunction, unused_newmenu_userdata, 0, Menu_pcx_name);
	}

// -- mk mk mk -- used to be here -- mk mk mk --

	if (Game_mode & GM_NETWORK)
         {
		if (secret_flag)
			Players[Player_num].connected = CONNECT_FOUND_SECRET; // Finished and went to secret level
		else
			Players[Player_num].connected = CONNECT_WAITING; // Finished but did not die
         }
	last_drawn_cockpit = -1;

	if (Current_level_num == Last_level) {
		if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
		{
			was_multi = 1;
			multi_endlevel_score();
			rval = AdvanceLevel(secret_flag);				//now go on to the next one (if one)
		}
		else
		{	// Note link to above else!
			rval = AdvanceLevel(secret_flag);				//now go on to the next one (if one)
			DoEndLevelScoreGlitz(0);		//give bonuses
		}
	} else {
		if (Game_mode & GM_MULTI)
			multi_endlevel_score();
		else
			DoEndLevelScoreGlitz(0);		//give bonuses
		rval = AdvanceLevel(secret_flag);				//now go on to the next one (if one)
	}

	if (!was_multi && rval) {
		if (PLAYING_BUILTIN_MISSION)
			scores_maybe_add_player(0);
		if (Game_wind)
			window_close(Game_wind);		// Exit out of game loop
	}
	else if (rval && Game_wind)
		window_close(Game_wind);
#elif defined(DXX_BUILD_DESCENT_II)

	Assert(!secret_flag);

	if (Game_mode & GM_NETWORK)
		Players[Player_num].connected = CONNECT_WAITING; // Finished but did not die

	last_drawn_cockpit = -1;

	AdvanceLevel(secret_flag);				//now go on to the next one (if one)
#endif
	if (Game_wind)
		window_set_visible(Game_wind, 1);
	reset_time();
}

#if defined(DXX_BUILD_DESCENT_I)
static int AdvanceLevel(int secret_flag)
{
	Control_center_destroyed = 0;

	#ifdef EDITOR
	if (Current_level_num == 0)
	{
		return 1;		//not a real level
	}
	#endif

	key_flush();

	if (Game_mode & GM_MULTI)
	{
		int result;
		result = multi_endlevel(&secret_flag); // Wait for other players to reach this point
		if (result) // failed to sync
		{
			return (Current_level_num == Last_level);
		}
	}

	key_flush();

	if (Current_level_num == Last_level) {		//player has finished the game!

		if ((Newdemo_state == ND_STATE_RECORDING) || (Newdemo_state == ND_STATE_PAUSED))
			newdemo_stop_recording();

		do_end_briefing_screens(Ending_text_filename);

		return 1;

	} else {

		Next_level_num = Current_level_num+1;		//assume go to next normal level

		if (secret_flag) {			//go to secret level instead
			int i;

			for (i=0;i<-Last_secret_level;i++)
				if (Secret_level_table[i]==Current_level_num) {
					Next_level_num = -(i+1);
					break;
				}
			Assert(i<-Last_secret_level);		//couldn't find which secret level
		}

		if (Current_level_num < 0)	{			//on secret level, where to go?

			Assert(!secret_flag);				//shouldn't be going to secret level
			Assert(Current_level_num<=-1 && Current_level_num>=Last_secret_level);

			Next_level_num = Secret_level_table[(-Current_level_num)-1]+1;
		}

		StartNewLevel(Next_level_num);

	}

	key_flush();

	return 0;
}

//called when the player has died
void DoPlayerDead()
{
	if (Game_wind)
		window_set_visible(Game_wind, 0);

	reset_palette_add();

	gr_palette_load (gr_palette);

	dead_player_end();		//terminate death sequence (if playing)

	#ifdef EDITOR
	if (Game_mode == GM_EDITOR) {			//test mine, not real level
		object * player = &Objects[Players[Player_num].objnum];
		//nm_messagebox( "You're Dead!", 1, "Continue", "Not a real game, though." );
		if (Game_wind)
			window_set_visible(Game_wind, 1);
		load_level("gamesave.lvl");
		init_player_stats_new_ship(Player_num);
		player->flags &= ~OF_SHOULD_BE_DEAD;
		StartLevel(0);
		return;
	}
	#endif

	if ( Game_mode&GM_MULTI )
	{
		multi_do_death(Players[Player_num].objnum);
	}
	else
	{				//Note link to above else!
		Players[Player_num].lives--;
		if (Players[Player_num].lives == 0)
		{
			DoGameOver();
			return;
		}
	}

	if ( Control_center_destroyed ) {

		//clear out stuff so no bonus
		Players[Player_num].hostages_on_board = 0;
		Players[Player_num].energy = 0;
		Players[Player_num].shields = 0;
		Players[Player_num].connected = CONNECT_DIED_IN_MINE;

		do_screen_message(TXT_DIED_IN_MINE); // Give them some indication of what happened

		int	rval;
		if (Current_level_num == Last_level) {
			if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
			{
				multi_endlevel_score();
				rval = AdvanceLevel(0);			//if finished, go on to next level
			}
			else
			{			// Note link to above else!
				rval = AdvanceLevel(0);			//if finished, go on to next level
				DoEndLevelScoreGlitz(0);
			}
			init_player_stats_new_ship(Player_num);
			last_drawn_cockpit = -1;
		} else {
			if (Game_mode & GM_MULTI)
				multi_endlevel_score();
			else
				DoEndLevelScoreGlitz(0);		// Note above link!

			rval = AdvanceLevel(0);			//if finished, go on to next level
			init_player_stats_new_ship(Player_num);
			last_drawn_cockpit = -1;
		}

		if (rval) {
			if (PLAYING_BUILTIN_MISSION)
				scores_maybe_add_player(0);
			if (Game_wind)
				window_close(Game_wind);		// Exit out of game loop
		}
	} else {
		init_player_stats_new_ship(Player_num);
		StartLevel(1);
	}

	if (Game_wind)
		window_set_visible(Game_wind, 1);
	reset_time();
}
#elif defined(DXX_BUILD_DESCENT_II)
#define MOVIE_REQUIRED 1

#define ENDMOVIE "end"

//called when the player has finished the last level
static void DoEndGame(void)
{
	if ((Newdemo_state == ND_STATE_RECORDING) || (Newdemo_state == ND_STATE_PAUSED))
		newdemo_stop_recording();

	set_screen_mode( SCREEN_MENU );

	gr_set_current_canvas(NULL);

	key_flush();

	if (PLAYING_BUILTIN_MISSION && !(Game_mode & GM_MULTI))
	{ //only built-in mission, & not multi
		int played=MOVIE_NOT_PLAYED;	//default is not played

		played = PlayMovie(ENDMOVIE ".tex", ENDMOVIE,MOVIE_REQUIRED);
		if (!played)
		{
			do_end_briefing_screens(Ending_text_filename);
		}
   }
   else if (!(Game_mode & GM_MULTI))    //not multi
   {
		char tname[FILENAME_LEN];

		do_end_briefing_screens (Ending_text_filename);

		//try doing special credits
		snprintf(tname, sizeof(tname), "%s.ctb", Current_mission_filename);
		credits_show(tname);
	}

	key_flush();

	if (Game_mode & GM_MULTI)
		multi_endlevel_score();
	else
		// NOTE LINK TO ABOVE
		DoEndLevelScoreGlitz(0);

	if (PLAYING_BUILTIN_MISSION && !((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))) {
		gr_set_current_canvas( NULL );
		gr_clear_canvas(BM_XRGB(0,0,0));
		load_palette(D2_DEFAULT_PALETTE,0,1);
		scores_maybe_add_player(0);
	}

	if (Game_wind)
		window_close(Game_wind);		// Exit out of game loop
}

//called to go to the next level (if there is one)
//if secret_flag is true, advance to secret level, else next normal one
//	Return true if game over.
static void AdvanceLevel(int secret_flag)
{
	Assert(!secret_flag);

	if (Current_level_num != Last_level) {
		if (Game_mode & GM_MULTI)
			multi_endlevel_score();
		else
			// NOTE LINK TO ABOVE!!!
			DoEndLevelScoreGlitz(0);		//give bonuses
	}

	Control_center_destroyed = 0;

	#ifdef EDITOR
	if (Current_level_num == 0)
	{
		window_close(Game_wind);		//not a real level
	}
	#endif

	if (Game_mode & GM_MULTI)	{
		int result;
		result = multi_endlevel(&secret_flag); // Wait for other players to reach this point
		if (result) // failed to sync
		{
			if (Current_level_num == Last_level)		//player has finished the game!
				if (Game_wind)
					window_close(Game_wind);		// Exit out of game loop

			return;
		}
	}

	if (Current_level_num == Last_level) {		//player has finished the game!

		DoEndGame();

	} else {
		//NMN 04/08/07 If we are in a secret level and playing a D1
		// 	       level, then use Entered_from_level # instead
		if (Current_level_num < 0 && EMULATING_D1)
		{
		  Next_level_num = Entered_from_level+1;		//assume go to next normal level
		} else {
		  Next_level_num = Current_level_num+1;		//assume go to next normal level
                }
		// END NMN

		StartNewLevel(Next_level_num);

	}
}

void DoPlayerDead()
{
	if (Game_wind)
		window_set_visible(Game_wind, 0);

	reset_palette_add();

	gr_palette_load (gr_palette);

	dead_player_end();		//terminate death sequence (if playing)

	#ifdef EDITOR
	if (Game_mode == GM_EDITOR) {			//test mine, not real level
		object * playerobj = &Objects[Players[Player_num].objnum];
		//nm_messagebox( "You're Dead!", 1, "Continue", "Not a real game, though." );
		if (Game_wind)
			window_set_visible(Game_wind, 1);
		load_level("gamesave.lvl");
		init_player_stats_new_ship(Player_num);
		playerobj->flags &= ~OF_SHOULD_BE_DEAD;
		StartLevel(0);
		return;
	}
	#endif

	if ( Game_mode&GM_MULTI )
	{
		multi_do_death(Players[Player_num].objnum);
	}
	else
	{				//Note link to above else!
		Players[Player_num].lives--;
		if (Players[Player_num].lives == 0)
		{
			DoGameOver();
			return;
		}
	}

	if ( Control_center_destroyed ) {

		//clear out stuff so no bonus
		Players[Player_num].hostages_on_board = 0;
		Players[Player_num].energy = 0;
		Players[Player_num].shields = 0;
		Players[Player_num].connected = CONNECT_DIED_IN_MINE;

		do_screen_message(TXT_DIED_IN_MINE); // Give them some indication of what happened

		if (Current_level_num < 0) {
			if (PHYSFSX_exists(SECRETB_FILENAME,0))
			{
				do_screen_message(TXT_SECRET_RETURN);
				state_restore_all(1, 2, SECRETB_FILENAME, 0);			//	2 means you died
				set_pos_from_return_segment();
				Players[Player_num].lives--;						//	re-lose the life, Players[Player_num].lives got written over in restore.
			} else {
				if (Entered_from_level == Last_level)
					DoEndGame();
				else {
					do_screen_message(TXT_SECRET_ADVANCE);
					StartNewLevel(Entered_from_level+1);
					init_player_stats_new_ship(Player_num);	//	New, MK, 05/29/96!, fix bug with dying in secret level, advance to next level, keep powerups!
				}
			}
		} else {

			AdvanceLevel(0);			//if finished, go on to next level

			init_player_stats_new_ship(Player_num);
			last_drawn_cockpit = -1;
		}

	} else if (Current_level_num < 0) {
		if (PHYSFSX_exists(SECRETB_FILENAME,0))
		{
			do_screen_message(TXT_SECRET_RETURN);
			if (!Control_center_destroyed)
				state_save_all(2, SECRETC_FILENAME, 0);
			state_restore_all(1, 2, SECRETB_FILENAME, 0);
			set_pos_from_return_segment();
			Players[Player_num].lives--;						//	re-lose the life, Players[Player_num].lives got written over in restore.
		} else {
			do_screen_message(TXT_DIED_IN_MINE); // Give them some indication of what happened
			if (Entered_from_level == Last_level)
				DoEndGame();
			else {
				do_screen_message(TXT_SECRET_ADVANCE);
				StartNewLevel(Entered_from_level+1);
				init_player_stats_new_ship(Player_num);	//	New, MK, 05/29/96!, fix bug with dying in secret level, advance to next level, keep powerups!
			}
		}
	} else {
		init_player_stats_new_ship(Player_num);
		StartLevel(1);
	}

	digi_sync_sounds();

	if (Game_wind)
		window_set_visible(Game_wind, 1);
	reset_time();
}
#endif

//called when the player is starting a new level for normal game mode and restore state
//	secret_flag set if came from a secret level
void StartNewLevelSub(int level_num, int page_in_textures, int secret_flag)
{
	if (!(Game_mode & GM_MULTI)) {
		last_drawn_cockpit = -1;
	}
#if defined(DXX_BUILD_DESCENT_I)
	secret_flag = 0;
#elif defined(DXX_BUILD_DESCENT_II)
   BigWindowSwitch=0;
#endif


	if (Newdemo_state == ND_STATE_PAUSED)
		Newdemo_state = ND_STATE_RECORDING;

	if (Newdemo_state == ND_STATE_RECORDING) {
		newdemo_set_new_level(level_num);
		newdemo_record_start_frame(FrameTime );
	}

	LoadLevel(level_num,page_in_textures);

	Assert(Current_level_num == level_num);	//make sure level set right

	gameseq_init_network_players(); // Initialize the Players array for
											  // this level

	Viewer = &Objects[Players[Player_num].objnum];

	Assert(N_players <= NumNetPlayerPositions);
		//If this assert fails, there's not enough start positions

	if (Game_mode & GM_NETWORK)
	{
		if(multi_level_sync()) // After calling this, Player_num is set
		{
			songs_play_song( SONG_TITLE, 1 ); // level song already plays but we fail to start level...
			return;
		}
	}

	HUD_clear_messages();

	automap_clear_visited();

	init_player_stats_level(secret_flag);

#if defined(DXX_BUILD_DESCENT_I)
	gr_use_palette_table( "palette.256" );
#elif defined(DXX_BUILD_DESCENT_II)
	load_palette(Current_level_palette,0,1);
#endif
	gr_palette_load(gr_palette);

	if ((Game_mode & GM_MULTI_COOP) && Network_rejoined)
	{
		int i;
		for (i = 0; i < N_players; i++)
			Players[i].flags |= Netgame.player_flags[i];
	}

	if (Game_mode & GM_MULTI)
	{
		multi_prep_level(); // Removes robots from level if necessary
	}

	gameseq_remove_unused_players();

	Game_suspended = 0;

	Control_center_destroyed = 0;

#if defined(DXX_BUILD_DESCENT_II)
	set_screen_mode(SCREEN_GAME);
#endif

	init_cockpit();
	init_robots_for_level();
	init_ai_objects();
	init_morphs();
	init_all_matcens();
	reset_palette_add();
#if defined(DXX_BUILD_DESCENT_II)
	init_smega_detonates();
	init_thief_for_level();
	init_stuck_objects();
	if (!(Game_mode & GM_MULTI))
		filter_objects_from_level();
#endif

	if (!(Game_mode & GM_MULTI) && !cheats.enabled)
		set_highest_level(Current_level_num);
	else
		read_player_file();		//get window sizes

	reset_special_effects();

#ifdef OGL
	gr_remap_mono_fonts();
	ogl_cache_level_textures();
#endif


	if (Network_rejoined == 1)
	{
		Network_rejoined = 0;
		StartLevel(1);
	}
	else
		StartLevel(0);		// Note link to above if!

	copy_defaults_to_robot_all();
	init_controlcen_for_level();

	//	Say player can use FLASH cheat to mark path to exit.
	Last_level_path_created = -1;

	// Initialise for palette_restore()
	// Also takes care of nm_draw_background() possibly being called
	if (!((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)))
		full_palette_save();

	if (!Game_wind)
		game();
}

void (bash_to_shield)(const vobjptr_t i)
{
	enum powerup_type_t type = (enum powerup_type_t) get_powerup_id(i);
	PowerupsInMine[type]=MaxPowerupsAllowed[type]=0;
	set_powerup_id(i, POW_SHIELD_BOOST);
	i->rtype.vclip_info.vclip_num = Powerup_info[get_powerup_id(i)].vclip_num;
	i->rtype.vclip_info.frametime = Vclip[i->rtype.vclip_info.vclip_num].frame_time;
}


#if defined(DXX_BUILD_DESCENT_II)
static void filter_objects_from_level()
 {
	range_for (auto i, highest_valid(Objects))
	{
		const auto objp = vobjptridx(i);
		if (objp->type==OBJ_POWERUP)
			if (objp->id==POW_FLAG_RED || objp->id==POW_FLAG_BLUE)
				bash_to_shield(objp);
   }

 }

struct intro_movie_t {
	int	level_num;
	char	movie_name[4];
};

static const intro_movie_t intro_movie[] = {
	{ 1,"pla"},
							{ 5,"plb"},
							{ 9,"plc"},
							{13,"pld"},
							{17,"ple"},
							{21,"plf"},
							{24,"plg"}};

#define NUM_INTRO_MOVIES (sizeof(intro_movie) / sizeof(*intro_movie))

static void ShowLevelIntro(int level_num)
{
	//if shareware, show a briefing?

	if (!(Game_mode & GM_MULTI)) {
		int i;

		palette_array_t save_pal;

		save_pal = gr_palette;

		if (PLAYING_BUILTIN_MISSION) {

			if (is_SHAREWARE || is_MAC_SHARE)
			{
				if (level_num==1)
					do_briefing_screens (Briefing_text_filename, 1);
			}
			else if (is_D2_OEM)
			{
				if (level_num == 1 && !intro_played)
					do_briefing_screens(Briefing_text_filename, 1);
			}
			else // full version
			{
				for (i=0;i<NUM_INTRO_MOVIES;i++)
				{
					if (intro_movie[i].level_num == level_num)
					{
						Screen_mode = -1;
						PlayMovie(NULL, intro_movie[i].movie_name,MOVIE_REQUIRED);
						break;
					}
				}

				do_briefing_screens (Briefing_text_filename,level_num);
			}
		}
		else	//not the built-in mission (maybe d1, too).  check for add-on briefing
		{
			do_briefing_screens(Briefing_text_filename, level_num);
		}

		gr_palette = save_pal;
	}
}

//	---------------------------------------------------------------------------
//	If starting a level which appears in the Secret_level_table, then set First_secret_visit.
//	Reason: On this level, if player goes to a secret level, he will be going to a different
//	secret level than he's ever been to before.
//	Sets the global First_secret_visit if necessary.  Otherwise leaves it unchanged.
static void maybe_set_first_secret_visit(int level_num)
{
	int	i;

	for (i=0; i<N_secret_levels; i++) {
		if (Secret_level_table[i] == level_num) {
			First_secret_visit = 1;
		}
	}
}
#endif

//called when the player is starting a new level for normal game model
//	secret_flag if came from a secret level
void StartNewLevel(int level_num)
{
	hide_menus();

	GameTime64 = 0;
	ThisLevelTime=0;

#if defined(DXX_BUILD_DESCENT_I)
	if (!(Game_mode & GM_MULTI)) {
		do_briefing_screens(Briefing_text_filename, level_num);
	}
#elif defined(DXX_BUILD_DESCENT_II)
	if (level_num > 0) {
		maybe_set_first_secret_visit(level_num);
	}

	ShowLevelIntro(level_num);
#endif

	StartNewLevelSub(level_num, 1, 0 );

}

//initialize the player object position & orientation (at start of game, or new ship)
static void InitPlayerPosition(int random_flag)
{
	int NewPlayer=0;

	if (! ((Game_mode & GM_MULTI) && !(Game_mode&GM_MULTI_COOP)) ) // If not deathmatch
		NewPlayer = Player_num;
	else if (random_flag == 1)
	{
		int i;
		uint_fast32_t trys=0;
		fix closest_dist = 0x7ffffff, dist;

		timer_update();
		d_srand((fix)timer_query());
		do {
			trys++;
			NewPlayer = d_rand() % NumNetPlayerPositions;

			closest_dist = 0x7fffffff;

			for (i=0; i<N_players; i++ )	{
				if ( (i!=Player_num) && (Objects[Players[i].objnum].type == OBJ_PLAYER) )	{
					dist = find_connected_distance(Objects[Players[i].objnum].pos, Objects[Players[i].objnum].segnum, Player_init[NewPlayer].pos, Player_init[NewPlayer].segnum, 15, WID_FLY_FLAG ); // Used to be 5, search up to 15 segments
					if ( (dist < closest_dist) && (dist >= 0) )	{
						closest_dist = dist;
					}
				}
			}

		} while ( (closest_dist<i2f(15*20)) && (trys<MAX_PLAYERS*2) );
	}
	else {
		// If deathmatch and not random, positions were already determined by sync packet
		reset_player_object();
		reset_cruise();
		return;
	}
	Assert(NewPlayer >= 0);
	Assert(NewPlayer < NumNetPlayerPositions);
	ConsoleObject->pos = Player_init[NewPlayer].pos;
	ConsoleObject->orient = Player_init[NewPlayer].orient;
	obj_relink(ConsoleObject-Objects,Player_init[NewPlayer].segnum);
	reset_player_object();
	reset_cruise();
}

//	-----------------------------------------------------------------------------------------------------
//	Initialize default parameters for one robot, copying from Robot_info to *objp.
//	What about setting size!?  Where does that come from?
void copy_defaults_to_robot(const vobjptr_t objp)
{
	robot_info	*robptr;
	int			objid;

	Assert(objp->type == OBJ_ROBOT);
	objid = get_robot_id(objp);
	Assert(objid < N_robot_types);

	robptr = &Robot_info[objid];

	//	Boost shield for Thief and Buddy based on level.
	objp->shields = robptr->strength;

#if defined(DXX_BUILD_DESCENT_II)
	if ((robot_is_thief(robptr)) || (robot_is_companion(robptr))) {
		objp->shields = (objp->shields * (abs(Current_level_num)+7))/8;

		if (robot_is_companion(robptr)) {
			//	Now, scale guide-bot hits by skill level
			switch (Difficulty_level) {
				case 0:	objp->shields = i2f(20000);	break;		//	Trainee, basically unkillable
				case 1:	objp->shields *= 3;				break;		//	Rookie, pretty dang hard
				case 2:	objp->shields *= 2;				break;		//	Hotshot, a bit tough
				default:	break;
			}
		}
	} else if (robptr->boss_flag)	//	MK, 01/16/95, make boss shields lower on lower diff levels.
		objp->shields = objp->shields/(NDL+3) * (Difficulty_level+4);

	//	Additional wimpification of bosses at Trainee
	if ((robptr->boss_flag) && (Difficulty_level == 0))
		objp->shields /= 2;
#endif
}

//	-----------------------------------------------------------------------------------------------------
//	Copy all values from the robot info structure to all instances of robots.
//	This allows us to change bitmaps.tbl and have these changes manifested in existing robots.
//	This function should be called at level load time.
static void copy_defaults_to_robot_all(void)
{
	range_for (auto i, highest_valid(Objects))
		if (Objects[i].type == OBJ_ROBOT)
			copy_defaults_to_robot(&Objects[i]);
}

//	-----------------------------------------------------------------------------------------------------
//called when the player is starting a level (new game or new ship)
static void StartLevel(int random_flag)
{
	Assert(!Player_is_dead);

	InitPlayerPosition(random_flag);

	verify_console_object();

	ConsoleObject->control_type	= CT_FLYING;
	ConsoleObject->movement_type	= MT_PHYSICS;

	// create_player_appearance_effect(ConsoleObject);
	Do_appearance_effect = 1;

	if (Game_mode & GM_MULTI)
	{
		if (Game_mode & GM_MULTI_COOP)
			multi_send_score();
	 	multi_send_reappear();
		multi_do_protocol_frame(1, 1);
	}
	else // in Singleplayer, after we died ...
	{
		disable_matcens(); // ... disable matcens and ...
		clear_transient_objects(0); // ... clear all transient objects.
#if defined(DXX_BUILD_DESCENT_II)
		clear_stuck_objects(); // and stuck ones.
#endif
	}

	ai_reset_all_paths();
	ai_init_boss_for_ship();

	reset_rear_view();
	Auto_fire_fusion_cannon_time = 0;
	Fusion_charge = 0;
}
