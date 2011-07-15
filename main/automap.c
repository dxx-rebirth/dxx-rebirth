/* $Id: automap.c,v 1.1.1.1 2006/03/17 19:56:52 zicodxx Exp $ */
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
 * Routines for displaying the auto-map.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef OGL
#include "ogl_init.h"
#endif

#include "error.h"
#include "3d.h"
#include "inferno.h"
#include "u_mem.h"
#include "render.h"
#include "object.h"
#include "vclip.h"
#include "game.h"
#include "polyobj.h"
#include "sounds.h"
#include "player.h"
#include "bm.h"
#include "key.h"
#include "newmenu.h"
#include "menu.h"
#include "screens.h"
#include "textures.h"
#include "mouse.h"
#include "timer.h"
#include "segpoint.h"
#include "joy.h"
#include "iff.h"
#include "pcx.h"
#include "palette.h"
#include "wall.h"
#include "hostage.h"
#include "fuelcen.h"
#include "gameseq.h"
#include "gamefont.h"
#ifdef NETWORK
#include "multi.h"
#endif
#include "kconfig.h"
#include "endlevel.h"
#include "text.h"
#include "gauges.h"
#include "powerup.h"
#include "switch.h"
#include "automap.h"
#include "cntrlcen.h"
#include "timer.h"
#include "config.h"
#include "rbaudio.h"
#include "window.h"
#include "playsave.h"

#define LEAVE_TIME 0x4000

#define EF_USED     1   // This edge is used
#define EF_DEFINING 2   // A structure defining edge that should always draw.
#define EF_FRONTIER 4   // An edge between the known and the unknown.
#define EF_SECRET   8   // An edge that is part of a secret wall.
#define EF_GRATE    16  // A grate... draw it all the time.
#define EF_NO_FADE  32  // An edge that doesn't fade with distance
#define EF_TOO_FAR  64  // An edge that is too far away

typedef struct Edge_info {
	int verts[2];       // 8  bytes
	ubyte sides[4];     // 4  bytes
	int   segnum[4];    // 16 bytes  // This might not need to be stored... If you can access the normals of a side.
	ubyte flags;        // 1  bytes  // See the EF_??? defines above.
	ubyte color;        // 1  bytes
	ubyte num_faces;    // 1  bytes  // 31 bytes...
} Edge_info;

typedef struct automap
{
	fix64			entry_time;
	fix64			t1, t2;
	int			leave_mode;
	int			pause_game;
	vms_angvec		tangles;
	ushort			old_wiggle; // keep 4 byte aligned
	int			max_segments_away;
	int			segment_limit;
	
	// Edge list variables
	int			num_edges;
	int			max_edges; //set each frame
	int			highest_edge_index;
	Edge_info		*edges;
	int			*drawingListBright;
	
	// Screen canvas variables
	grs_canvas		automap_view;
	
	grs_bitmap		automap_background;
	
	// Rendering variables
	fix			zoom;
	vms_vector		view_target;
	vms_vector		view_position;
	fix			farthest_dist;
	vms_matrix		viewMatrix;
	fix			viewDist;
	
	int			wall_normal_color;
	int			wall_door_color;
	int			wall_door_blue;
	int			wall_door_gold;
	int			wall_door_red;
	int			wall_revealed_color;
	int			hostage_color;
	int			font_color_20;
	int			green_31;
	int			white_63;
	int			blue_48;
	int			red_48;
	control_info controls;
} automap;

#define MAX_EDGES_FROM_VERTS(v)     ((v)*4)
#define MAX_EDGES 6000  // Determined by loading all the levels by John & Mike, Feb 9, 1995

#define K_WALL_NORMAL_COLOR     BM_XRGB(29, 29, 29 )
#define K_WALL_DOOR_COLOR       BM_XRGB(5, 27, 5 )
#define K_WALL_DOOR_BLUE        BM_XRGB(0, 0, 31)
#define K_WALL_DOOR_GOLD        BM_XRGB(31, 31, 0)
#define K_WALL_DOOR_RED         BM_XRGB(31, 0, 0)
#define K_HOSTAGE_COLOR         BM_XRGB(0, 31, 0 )
#define K_FONT_COLOR_20         BM_XRGB(20, 20, 20 )
#define K_GREEN_31              BM_XRGB(0, 31, 0)

int Automap_active = 0;

void init_automap_colors(automap *am)
{
	am->wall_normal_color = K_WALL_NORMAL_COLOR;
	am->wall_door_color = K_WALL_DOOR_COLOR;
	am->wall_door_blue = K_WALL_DOOR_BLUE;
	am->wall_door_gold = K_WALL_DOOR_GOLD;
	am->wall_door_red = K_WALL_DOOR_RED;
	am->hostage_color = K_HOSTAGE_COLOR;
	am->font_color_20 = K_FONT_COLOR_20;
	am->green_31 = K_GREEN_31;

	am->white_63 = gr_find_closest_color_current(63,63,63);
	am->blue_48 = gr_find_closest_color_current(0,0,48);
	am->red_48 = gr_find_closest_color_current(48,0,0);
}

// Segment visited list
ubyte Automap_visited[MAX_SEGMENTS];

// Map movement defines
#define PITCH_DEFAULT 9000
#define ZOOM_DEFAULT i2f(20*10)
#define ZOOM_MIN_VALUE i2f(20*5)
#define ZOOM_MAX_VALUE i2f(20*100)

#define SLIDE_SPEED 			(350)
#define ZOOM_SPEED_FACTOR		(500)	//(1500)
#define ROT_SPEED_DIVISOR		(115000)

// Function Prototypes
void adjust_segment_limit(automap *am, int SegmentLimit);
void draw_all_edges(automap *am);
void automap_build_edge_list(automap *am);
// extern
void check_and_fix_matrix(vms_matrix *m);

#define	MAX_DROP_MULTI	2
#define	MAX_DROP_SINGLE	9

extern vms_vector Matrix_scale; //how the matrix is currently scaled

# define automap_draw_line g3_draw_line

void automap_clear_visited()	
{
	int i;
	for (i=0; i<MAX_SEGMENTS; i++ )
		Automap_visited[i] = 0;
}

void draw_player( object * obj )
{
	vms_vector arrow_pos, head_pos;
	g3s_point sphere_point, arrow_point, head_point;

	// Draw Console player -- shaped like a ellipse with an arrow.
	g3_rotate_point(&sphere_point,&obj->pos);
	g3_draw_sphere(&sphere_point,obj->size);

	// Draw shaft of arrow
	vm_vec_scale_add( &arrow_pos, &obj->pos, &obj->orient.fvec, obj->size*3 );
	g3_rotate_point(&arrow_point,&arrow_pos);
	automap_draw_line(&sphere_point, &arrow_point);

	// Draw right head of arrow
	vm_vec_scale_add( &head_pos, &obj->pos, &obj->orient.fvec, obj->size*2 );
	vm_vec_scale_add2( &head_pos, &obj->orient.rvec, obj->size*1 );
	g3_rotate_point(&head_point,&head_pos);
	automap_draw_line(&arrow_point, &head_point);

	// Draw left head of arrow
	vm_vec_scale_add( &head_pos, &obj->pos, &obj->orient.fvec, obj->size*2 );
	vm_vec_scale_add2( &head_pos, &obj->orient.rvec, obj->size*(-1) );
	g3_rotate_point(&head_point,&head_pos);
	automap_draw_line(&arrow_point, &head_point);

	// Draw player's up vector
	vm_vec_scale_add( &arrow_pos, &obj->pos, &obj->orient.uvec, obj->size*2 );
	g3_rotate_point(&arrow_point,&arrow_pos);
	automap_draw_line(&sphere_point, &arrow_point);
}

void name_frame(automap *am)
{
	char		name_level[128];
	
	if (Current_level_num > 0)
		sprintf(name_level, "%s %i: ",TXT_LEVEL, Current_level_num);
	else
		name_level[0] = 0;

	strcat(name_level, Current_level_name);

	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(am->green_31,-1);
	gr_printf((SWIDTH/64),(SHEIGHT/48),"%s", name_level);
}

void draw_automap(automap *am)
{
	int i;
	int color;
	object * objp;
	g3s_point sphere_point;
	
	if ( am->leave_mode==0 && am->controls.automap_state && (timer_query()-am->entry_time)>LEAVE_TIME)
		am->leave_mode = 1;

	gr_set_current_canvas(NULL);
	show_fullscr(&am->automap_background);
	gr_set_curfont(HUGE_FONT);
	gr_set_fontcolor(BM_XRGB(20, 20, 20), -1);
	if (!MacHog)
		gr_printf((SWIDTH/8), (SHEIGHT/16), TXT_AUTOMAP);
	else
		gr_printf(80*(SWIDTH/640.0), 36*(SHEIGHT/480.0), TXT_AUTOMAP);
	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(BM_XRGB(20, 20, 20), -1);
	if (!MacHog)
	{
		gr_printf((SWIDTH/4.923), (SHEIGHT/1.126), TXT_TURN_SHIP);
		gr_printf((SWIDTH/4.923), (SHEIGHT/1.083), TXT_SLIDE_UPDOWN);
		gr_printf((SWIDTH/4.923), (SHEIGHT/1.043), "F9/F10 Changes viewing distance");
	}
	else
	{
		// for the Mac automap they're shown up the top, hence the different layout
		gr_printf(265*(SWIDTH/640.0), 27*(SHEIGHT/480.0), TXT_TURN_SHIP);
		gr_printf(265*(SWIDTH/640.0), 44*(SHEIGHT/480.0), TXT_SLIDE_UPDOWN);
		gr_printf(265*(SWIDTH/640.0), 61*(SHEIGHT/480.0), "F9/F10 Changes viewing distance");
	}
	
	gr_set_current_canvas(&am->automap_view);

	gr_clear_canvas(BM_XRGB(0,0,0));

	g3_start_frame();
	render_start_frame();

	if (!PlayerCfg.AutomapFreeFlight)
		vm_vec_scale_add(&am->view_position,&am->view_target,&am->viewMatrix.fvec,-am->viewDist);

	g3_set_view_matrix(&am->view_position,&am->viewMatrix,am->zoom);

	draw_all_edges(am);

	// Draw player...
#ifdef NETWORK
	if (Game_mode & GM_TEAM)
		color = get_team(Player_num);
	else
#endif	
		color = Player_num;	// Note link to above if!

	gr_setcolor(BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b));
	draw_player(&Objects[Players[Player_num].objnum]);

	// Draw player(s)...
#ifdef NETWORK
	if ( (Game_mode & (GM_TEAM | GM_MULTI_COOP)) || (Netgame.game_flags & NETGAME_FLAG_SHOW_MAP) )	{
		for (i=0; i<N_players; i++)		{
			if ( (i != Player_num) && ((Game_mode & GM_MULTI_COOP) || (get_team(Player_num) == get_team(i)) || (Netgame.game_flags & NETGAME_FLAG_SHOW_MAP)) )	{
				if ( Objects[Players[i].objnum].type == OBJ_PLAYER )	{
					if (Game_mode & GM_TEAM)
						color = get_team(i);
					else
						color = i;
					gr_setcolor(BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b));
					draw_player(&Objects[Players[i].objnum]);
				}
			}
		}
	}
#endif

	objp = &Objects[0];
	for (i=0;i<=Highest_object_index;i++,objp++) {
		switch( objp->type )	{
		case OBJ_HOSTAGE:
			gr_setcolor(am->hostage_color);
			g3_rotate_point(&sphere_point,&objp->pos);
			g3_draw_sphere(&sphere_point,objp->size);	
			break;
		case OBJ_POWERUP:
			if ( Automap_visited[objp->segnum] )	{
				if ( (objp->id==POW_KEY_RED) || (objp->id==POW_KEY_BLUE) || (objp->id==POW_KEY_GOLD) )	{
					switch (objp->id) {
					case POW_KEY_RED:		gr_setcolor(BM_XRGB(63, 5, 5));	break;
					case POW_KEY_BLUE:	gr_setcolor(BM_XRGB(5, 5, 63)); break;
					case POW_KEY_GOLD:	gr_setcolor(BM_XRGB(63, 63, 10)); break;
					default:
						Error("Illegal key type: %i", objp->id);
					}
					g3_rotate_point(&sphere_point,&objp->pos);
					g3_draw_sphere(&sphere_point,objp->size*4);	
				}
			}
			break;
		}
	}

	g3_end_frame();

	name_frame(am);

	if (PlayerCfg.MouseFlightSim && PlayerCfg.MouseFSIndicator)
		show_mousefs_indicator(am->controls.raw_mouse_axis[0], am->controls.raw_mouse_axis[1], am->controls.raw_mouse_axis[2], GWIDTH-(GHEIGHT/8), GHEIGHT-(GHEIGHT/8), GHEIGHT/5);

	am->t2 = timer_query();
	while (am->t2 - am->t1 < F1_0 / (GameCfg.VSync?MAXIMUM_FPS:GameArg.SysMaxFPS)) // ogl is fast enough that the automap can read the input too fast and you start to turn really slow.  So delay a bit (and free up some cpu :)
	{
		if (GameArg.SysUseNiceFPS && !GameCfg.VSync)
			timer_delay(f1_0 / GameArg.SysMaxFPS - (am->t2 - am->t1));
		timer_update();
		am->t2 = timer_query();
	}
	if (am->pause_game)
	{
		FrameTime=am->t2-am->t1;
		FixedStepCalc();
	}
	am->t1 = am->t2;
}

extern int set_segment_depths(int start_seg, ubyte *segbuf);

#define MAP_BACKGROUND_FILENAME "MAP.PCX"

int automap_key_command(window *wind, d_event *event, automap *am)
{
	int c = event_key_get(event);

	switch (c)
	{
		case KEY_PRINT_SCREEN: {
			gr_set_current_canvas(NULL);
			save_screen_shot(1);
			return 1;
		}
			
		case KEY_ESC:
			if (am->leave_mode==0)
			{
				window_close(wind);
				return 1;
			}
			return 1;
			
		case KEY_ALTED+KEY_F:           // Alt+F shows full map, if cheats enabled
			if (cheats.enabled) 	 
			{
				cheats.fullautomap = !cheats.fullautomap;
				automap_build_edge_list(am);
			}
			return 1;
#ifndef NDEBUG
		case KEY_DEBUGGED+KEY_F: 	{
				int i;
				
				for (i=0; i<=Highest_segment_index; i++ )
					Automap_visited[i] = 1;
				automap_build_edge_list(am);
				am->max_segments_away = set_segment_depths(Objects[Players[Player_num].objnum].segnum, Automap_visited);
				am->segment_limit = am->max_segments_away;
				adjust_segment_limit(am, am->segment_limit);
			}
			return 1;
#endif
			
		case KEY_F9:
			if (am->segment_limit > 1) 		{
				am->segment_limit--;
				adjust_segment_limit(am, am->segment_limit);
			}
			return 1;
		case KEY_F10:
			if (am->segment_limit < am->max_segments_away) 	{
				am->segment_limit++;
				adjust_segment_limit(am, am->segment_limit);
			}
			return 1;
	}
	
	return 0;
}

int automap_process_input(window *wind, d_event *event, automap *am)
{
	vms_matrix tempm;

	Controls = am->controls;
	kconfig_read_controls(event, 1);
	am->controls = Controls;
	memset(&Controls, 0, sizeof(control_info));

	if ( !am->controls.automap_state && (am->leave_mode==1) )
	{
		window_close(wind);
		return 1;
	}
	
	if ( am->controls.automap_count > 0)
	{
		am->controls.automap_count = 0;
		if (am->leave_mode==0)
		{
			window_close(wind);
			return 1;
		}
	}
	
	if (PlayerCfg.AutomapFreeFlight)
	{
		if ( am->controls.fire_primary_count > 0)
		{
			// Reset orientation
			am->viewMatrix = Objects[Players[Player_num].objnum].orient;
			vm_vec_scale_add(&am->view_position, &Objects[Players[Player_num].objnum].pos, &am->viewMatrix.fvec, -ZOOM_DEFAULT );
			am->controls.fire_primary_count = 0;
		}
		
		if (am->controls.pitch_time || am->controls.heading_time || am->controls.bank_time)
		{
			vms_angvec tangles;
			vms_matrix new_m;

			tangles.p = fixdiv( am->controls.pitch_time, ROT_SPEED_DIVISOR );
			tangles.h = fixdiv( am->controls.heading_time, ROT_SPEED_DIVISOR );
			tangles.b = fixdiv( am->controls.bank_time, ROT_SPEED_DIVISOR*2 );

			vm_angles_2_matrix(&tempm, &tangles);
			vm_matrix_x_matrix(&new_m,&am->viewMatrix,&tempm);
			am->viewMatrix = new_m;
			check_and_fix_matrix(&am->viewMatrix);
		}
		
		if ( am->controls.forward_thrust_time || am->controls.vertical_thrust_time || am->controls.sideways_thrust_time )
		{
			vm_vec_scale_add2( &am->view_position, &am->viewMatrix.fvec, am->controls.forward_thrust_time*ZOOM_SPEED_FACTOR );
			vm_vec_scale_add2( &am->view_position, &am->viewMatrix.uvec, am->controls.vertical_thrust_time*SLIDE_SPEED );
			vm_vec_scale_add2( &am->view_position, &am->viewMatrix.rvec, am->controls.sideways_thrust_time*SLIDE_SPEED );
			
			// Crude wrapping check
			if (am->view_position.x >  F1_0*32000) am->view_position.x =  F1_0*32000;
			if (am->view_position.x < -F1_0*32000) am->view_position.x = -F1_0*32000;
			if (am->view_position.y >  F1_0*32000) am->view_position.y =  F1_0*32000;
			if (am->view_position.y < -F1_0*32000) am->view_position.y = -F1_0*32000;
			if (am->view_position.z >  F1_0*32000) am->view_position.z =  F1_0*32000;
			if (am->view_position.z < -F1_0*32000) am->view_position.z = -F1_0*32000;
		}
	}
	else
	{
		if ( am->controls.fire_primary_count > 0)
		{
			// Reset orientation
			am->viewDist = ZOOM_DEFAULT;
			am->tangles.p = PITCH_DEFAULT;
			am->tangles.h  = 0;
			am->tangles.b  = 0;
			am->view_target = Objects[Players[Player_num].objnum].pos;
			am->controls.fire_primary_count = 0;
		}

		am->viewDist -= am->controls.forward_thrust_time*ZOOM_SPEED_FACTOR;
		am->tangles.p += fixdiv( am->controls.pitch_time, ROT_SPEED_DIVISOR );
		am->tangles.h  += fixdiv( am->controls.heading_time, ROT_SPEED_DIVISOR );
		am->tangles.b  += fixdiv( am->controls.bank_time, ROT_SPEED_DIVISOR*2 );

		if ( am->controls.vertical_thrust_time || am->controls.sideways_thrust_time )
		{
			vms_angvec      tangles1;
			vms_vector      old_vt;

			old_vt = am->view_target;
			tangles1 = am->tangles;
			vm_angles_2_matrix(&tempm,&tangles1);
			vm_matrix_x_matrix(&am->viewMatrix,&Objects[Players[Player_num].objnum].orient,&tempm);
			vm_vec_scale_add2( &am->view_target, &am->viewMatrix.uvec, am->controls.vertical_thrust_time*SLIDE_SPEED );
			vm_vec_scale_add2( &am->view_target, &am->viewMatrix.rvec, am->controls.sideways_thrust_time*SLIDE_SPEED );
			if ( vm_vec_dist_quick( &am->view_target, &Objects[Players[Player_num].objnum].pos) > i2f(1000) )
				am->view_target = old_vt;
		}

		vm_angles_2_matrix(&tempm,&am->tangles);
		vm_matrix_x_matrix(&am->viewMatrix,&Objects[Players[Player_num].objnum].orient,&tempm);

		if ( am->viewDist < ZOOM_MIN_VALUE ) am->viewDist = ZOOM_MIN_VALUE;
		if ( am->viewDist > ZOOM_MAX_VALUE ) am->viewDist = ZOOM_MAX_VALUE;
	}
	
	return 0;
}

int automap_handler(window *wind, d_event *event, automap *am)
{
	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
			game_flush_inputs();
			event_toggle_focus(1);
			key_toggle_repeat(0);
			break;

		case EVENT_WINDOW_DEACTIVATED:
			event_toggle_focus(0);
			key_toggle_repeat(1);
			break;

		case EVENT_IDLE:
		case EVENT_JOYSTICK_BUTTON_UP:
		case EVENT_JOYSTICK_BUTTON_DOWN:
		case EVENT_JOYSTICK_MOVED:
		case EVENT_MOUSE_BUTTON_UP:
		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_MOVED:
			automap_process_input(wind, event, am);
			break;
		case EVENT_KEY_COMMAND:
		case EVENT_KEY_RELEASE:
		{
			int kret = automap_key_command(wind, event, am);
			if (!kret)
				automap_process_input(wind, event, am);
			return kret;
		}
			
		case EVENT_WINDOW_DRAW:
			draw_automap(am);
			break;
			
		case EVENT_WINDOW_CLOSE:
			if (!am->pause_game)
				ConsoleObject->mtype.phys_info.flags |= am->old_wiggle;		// Restore wiggle
			event_toggle_focus(0);
			key_toggle_repeat(1);
#ifdef OGL
			gr_free_bitmap_data(&am->automap_background);
#endif
			d_free(am->edges);
			d_free(am->drawingListBright);
			d_free(am);
			window_set_visible(Game_wind, 1);
			Automap_active = 0;
			return 0;	// continue closing
			break;

		default:
			return 0;
			break;
	}
	
	return 1;
}

void do_automap( int key_code )
{
	int pcx_error;
	ubyte pal[256*3];
	window *automap_wind = NULL;
	automap *am;
	
	MALLOC(am, automap, 1);
	
	if (am)
	{
		memset(am, 0, sizeof(automap));
		automap_wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, (int (*)(window *, d_event *, void *)) automap_handler, am);
	}

	if (automap_wind == NULL)
	{
		Warning("Out of memory");
		return;
	}

	am->leave_mode = 0;
	am->pause_game = 1; // Set to 1 if everything is paused during automap...No pause during net.
	am->max_segments_away = 0;
	am->segment_limit = 1;
	am->num_edges = 0;
	am->highest_edge_index = -1;
	am->max_edges = Num_segments*12;
	MALLOC(am->edges, Edge_info, am->max_edges);
	MALLOC(am->drawingListBright, int, am->max_edges);
	if (!am->edges || !am->drawingListBright)
	{
		if (am->edges)
			d_free(am->edges);
		if (am->drawingListBright)
			d_free(am->drawingListBright);

		Warning("Out of memory");
		return;
	}
	
	am->zoom = 0x9000;
	am->farthest_dist = (F1_0 * 20 * 50); // 50 segments away
	am->viewDist = 0;

	init_automap_colors(am);

	key_code = key_code;	// disable warning...

	if ((Game_mode & GM_MULTI) && (!Endlevel_sequence))
		am->pause_game = 0;

	if (am->pause_game) {
		window_set_visible(Game_wind, 0);
	}
	if (!am->pause_game)	{
		am->old_wiggle = ConsoleObject->mtype.phys_info.flags & PF_WIGGLE;	// Save old wiggle
		ConsoleObject->mtype.phys_info.flags &= ~PF_WIGGLE;		// Turn off wiggle
	}

	//Max_edges = min(MAX_EDGES_FROM_VERTS(Num_vertices),MAX_EDGES); //make maybe smaller than max

	gr_set_current_canvas(NULL);

	automap_build_edge_list(am);

	if ( am->viewDist==0 )
		am->viewDist = ZOOM_DEFAULT;

	am->viewMatrix = Objects[Players[Player_num].objnum].orient;
	am->tangles.p = PITCH_DEFAULT;
	am->tangles.h  = 0;
	am->tangles.b  = 0;
	am->view_target = Objects[Players[Player_num].objnum].pos;
	
	if (PlayerCfg.AutomapFreeFlight)
		vm_vec_scale_add(&am->view_position, &Objects[Players[Player_num].objnum].pos, &am->viewMatrix.fvec, -ZOOM_DEFAULT );

	am->t1 = am->entry_time = timer_query();
	am->t2 = am->t1;

	//Fill in Automap_visited from Objects[Players[Player_num].objnum].segnum
	am->max_segments_away = set_segment_depths(Objects[Players[Player_num].objnum].segnum, Automap_visited);
	am->segment_limit = am->max_segments_away;

	adjust_segment_limit(am, am->segment_limit);

	// ZICO - code from above to show frame in OGL correctly. Redundant, but better readable.
	// KREATOR - Now applies to all platforms so double buffering is supported
	gr_init_bitmap_data (&am->automap_background);
	pcx_error = pcx_read_bitmap(MAP_BACKGROUND_FILENAME, &am->automap_background, BM_LINEAR, pal);
	if (pcx_error != PCX_ERROR_NONE)
		Error("File %s - PCX error: %s", MAP_BACKGROUND_FILENAME, pcx_errormsg(pcx_error));
	gr_remap_bitmap_good(&am->automap_background, pal, -1, -1);
	if (!MacHog)
		gr_init_sub_canvas(&am->automap_view, &grd_curscreen->sc_canvas, (SWIDTH/23), (SHEIGHT/6), (SWIDTH/1.1), (SHEIGHT/1.45));
	else
		gr_init_sub_canvas(&am->automap_view, &grd_curscreen->sc_canvas, 38*(SWIDTH/640.0), 77*(SHEIGHT/480.0), 564*(SWIDTH/640.0), 381*(SHEIGHT/480.0));

	gr_palette_load( gr_palette );
	Automap_active = 1;
}

void adjust_segment_limit(automap *am, int SegmentLimit)
{
	int i,e1;
	Edge_info * e;

	for (i=0; i<=am->highest_edge_index; i++ )	{
		e = &am->edges[i];
		e->flags |= EF_TOO_FAR;
		for (e1=0; e1<e->num_faces; e1++ )	{
			if ( Automap_visited[e->segnum[e1]] <= SegmentLimit )	{
				e->flags &= (~EF_TOO_FAR);
				break;
			}
		}
	}
}

void draw_all_edges(automap *am)	
{
	g3s_codes cc;
	int i,j,nbright;
	ubyte nfacing,nnfacing;
	Edge_info *e;
	vms_vector *tv1;
	fix distance;
	fix min_distance = 0x7fffffff;
	g3s_point *p1, *p2;
	
	
	nbright=0;

	for (i=0; i<=am->highest_edge_index; i++ )	{
		//e = &am->edges[Edge_used_list[i]];
		e = &am->edges[i];
		if (!(e->flags & EF_USED)) continue;

		if ( e->flags & EF_TOO_FAR) continue;

		if (e->flags&EF_FRONTIER) { 	// A line that is between what we have seen and what we haven't
			if ( (!(e->flags&EF_SECRET))&&(e->color==am->wall_normal_color))
				continue; 	// If a line isn't secret and is normal color, then don't draw it
		}

		cc=rotate_list(2,e->verts);
		distance = Segment_points[e->verts[1]].p3_z;

		if (min_distance>distance )
			min_distance = distance;

		if (!cc.and) {			//all off screen?
			nfacing = nnfacing = 0;
			tv1 = &Vertices[e->verts[0]];
			j = 0;
			while( j<e->num_faces && (nfacing==0 || nnfacing==0) )	{
#ifdef COMPACT_SEGS
				vms_vector temp_v;
				get_side_normal(&Segments[e->segnum[j]], e->sides[j], 0, &temp_v );
				if (!g3_check_normal_facing( tv1, &temp_v ) )
#else
				if (!g3_check_normal_facing( tv1, &Segments[e->segnum[j]].sides[e->sides[j]].normals[0] ) )
#endif
					nfacing++;
				else
					nnfacing++;
				j++;
			}

			if ( nfacing && nnfacing )	{
				// a contour line
				am->drawingListBright[nbright++] = e-am->edges;
			} else if ( e->flags&(EF_DEFINING|EF_GRATE) )	{
				if ( nfacing == 0 )	{
					if ( e->flags & EF_NO_FADE )
						gr_setcolor( e->color );
					else
						gr_setcolor( gr_fade_table[e->color+256*8] );
					g3_draw_line( &Segment_points[e->verts[0]], &Segment_points[e->verts[1]] );
				} 	else {
					am->drawingListBright[nbright++] = e-am->edges;
				}
			}
		}
	}
		
	if ( min_distance < 0 ) min_distance = 0;

	// Sort the bright ones using a shell sort
	{
		int t;
		int i, j, incr, v1, v2;
	
		incr = nbright / 2;
		while( incr > 0 )	{
			for (i=incr; i<nbright; i++ )	{
				j = i - incr;
				while (j>=0 )	{
					// compare element j and j+incr
					v1 = am->edges[am->drawingListBright[j]].verts[0];
					v2 = am->edges[am->drawingListBright[j+incr]].verts[0];

					if (Segment_points[v1].p3_z < Segment_points[v2].p3_z) {
						// If not in correct order, them swap 'em
						t=am->drawingListBright[j+incr];
						am->drawingListBright[j+incr]=am->drawingListBright[j];
						am->drawingListBright[j]=t;
						j -= incr;
					}
					else
						break;
				}
			}
			incr = incr / 2;
		}
	}
					
	// Draw the bright ones
	for (i=0; i<nbright; i++ )	{
		int color;
		fix dist;
		e = &am->edges[am->drawingListBright[i]];
		p1 = &Segment_points[e->verts[0]];
		p2 = &Segment_points[e->verts[1]];
		dist = p1->p3_z - min_distance;
		// Make distance be 1.0 to 0.0, where 0.0 is 10 segments away;
		if ( dist < 0 ) dist=0;
		if ( dist >= am->farthest_dist ) continue;

		if ( e->flags & EF_NO_FADE )	{
			gr_setcolor( e->color );
		} else {
			dist = F1_0 - fixdiv( dist, am->farthest_dist );
			color = f2i( dist*31 );
			gr_setcolor( gr_fade_table[e->color+color*256] );	
		}
		g3_draw_line( p1, p2 );
	}
}


//==================================================================
//
// All routines below here are used to build the Edge list
//
//==================================================================


//finds edge, filling in edge_ptr. if found old edge, returns index, else return -1
static int automap_find_edge(automap *am, int v0,int v1,Edge_info **edge_ptr)
{
	long vv, evv;
	int hash,oldhash;
	int ret, ev0, ev1;

	vv = (v1<<16) + v0;

	oldhash = hash = ((v0*5+v1) % am->max_edges);

	ret = -1;

	while (ret==-1) {
		ev0 = am->edges[hash].verts[0];
		ev1 = am->edges[hash].verts[1];
		evv = (ev1<<16)+ev0;
		if (am->edges[hash].num_faces == 0 ) ret=0;
		else if (evv == vv) ret=1;
		else {
			if (++hash==am->max_edges) hash=0;
			if (hash==oldhash) Error("Edge list full!");
		}
	}

	*edge_ptr = &am->edges[hash];

	if (ret == 0)
		return -1;
	else
		return hash;

}


void add_one_edge( automap *am, int va, int vb, ubyte color, ubyte side, int segnum, int hidden, int grate, int no_fade )	{
	int found;
	Edge_info *e;
	int tmp;

	if ( am->num_edges >= am->max_edges)	{
		// GET JOHN! (And tell him that his
		// MAX_EDGES_FROM_VERTS formula is hosed.)
		// If he's not around, save the mine,
		// and send him  mail so he can look
		// at the mine later. Don't modify it.
		// This is important if this happens.
		Int3();		// LOOK ABOVE!!!!!!
		return;
	}

	if ( va > vb )	{
		tmp = va;
		va = vb;
		vb = tmp;
	}

	found = automap_find_edge(am,va,vb,&e);
		
	if (found == -1) {
		e->verts[0] = va;
		e->verts[1] = vb;
		e->color = color;
		e->num_faces = 1;
		e->flags = EF_USED | EF_DEFINING;			// Assume a normal line
		e->sides[0] = side;
		e->segnum[0] = segnum;
		//Edge_used_list[am->num_edges] = e-am->edges;
		if ( (e-am->edges) > am->highest_edge_index )
			am->highest_edge_index = e - am->edges;
		am->num_edges++;
	} else {
		if ( color != am->wall_normal_color )
			e->color = color;
		if ( e->num_faces < 4 ) {
			e->sides[e->num_faces] = side;
			e->segnum[e->num_faces] = segnum;
			e->num_faces++;
		}
	}

	if ( grate )
		e->flags |= EF_GRATE;

	if ( hidden )
		e->flags|=EF_SECRET;		// Mark this as a hidden edge
	if ( no_fade )
		e->flags |= EF_NO_FADE;
}

void add_one_unknown_edge( automap *am, int va, int vb )
{
	int found;
	Edge_info *e;
	int tmp;

	if ( va > vb )	{
		tmp = va;
		va = vb;
		vb = tmp;
	}

	found = automap_find_edge(am,va,vb,&e);
	if (found != -1) 	
		e->flags|=EF_FRONTIER;		// Mark as a border edge
}

#ifndef _GAMESEQ_H
extern obj_position Player_init[];
#endif

void add_segment_edges(automap *am, segment *seg)
{
	int 	is_grate, no_fade;
	ubyte	color;
	int	sn;
	int	segnum = seg-Segments;
	int	hidden_flag;
	
	for (sn=0;sn<MAX_SIDES_PER_SEGMENT;sn++) {
		int	vertex_list[4];

		hidden_flag = 0;

		is_grate = 0;
		no_fade = 0;

		color = 255;
		if (seg->children[sn] == -1) {
			color = am->wall_normal_color;
		}

		switch( seg->special )	{
		case SEGMENT_IS_FUELCEN:
			color = BM_XRGB( 29, 27, 13 );
			break;
		case SEGMENT_IS_CONTROLCEN:
			if (Control_center_present)
				color = BM_XRGB( 29, 0, 0 );
			break;
		case SEGMENT_IS_ROBOTMAKER:
			color = BM_XRGB( 29, 0, 31 );
			break;
		}

		if (seg->sides[sn].wall_num > -1)	{
		
			switch( Walls[seg->sides[sn].wall_num].type )	{
			case WALL_DOOR:
				if (Walls[seg->sides[sn].wall_num].keys == KEY_BLUE) {
					no_fade = 1;
					color = am->wall_door_blue;
				} else if (Walls[seg->sides[sn].wall_num].keys == KEY_GOLD) {
					no_fade = 1;
					color = am->wall_door_gold;
				} else if (Walls[seg->sides[sn].wall_num].keys == KEY_RED) {
					no_fade = 1;
					color = am->wall_door_red;
				} else if (!(WallAnims[Walls[seg->sides[sn].wall_num].clip_num].flags & WCF_HIDDEN)) {
					int	connected_seg = seg->children[sn];
					if (connected_seg != -1) {
						int connected_side = find_connect_side(seg, &Segments[connected_seg]);
						int	keytype = Walls[Segments[connected_seg].sides[connected_side].wall_num].keys;
						if ((keytype != KEY_BLUE) && (keytype != KEY_GOLD) && (keytype != KEY_RED))
							color = am->wall_door_color;
						else {
							switch (Walls[Segments[connected_seg].sides[connected_side].wall_num].keys) {
								case KEY_BLUE:	color = am->wall_door_blue;	no_fade = 1; break;
								case KEY_GOLD:	color = am->wall_door_gold;	no_fade = 1; break;
								case KEY_RED:	color = am->wall_door_red;	no_fade = 1; break;
								default:	Error("Inconsistent data.  Supposed to be a colored wall, but not blue, gold or red.\n");
							}
						}

					}
				} else {
					color = am->wall_normal_color;
					hidden_flag = 1;
				}
				break;
			case WALL_CLOSED:
				// Make grates draw properly
				if (WALL_IS_DOORWAY(seg,sn) & WID_RENDPAST_FLAG)
					is_grate = 1;
				else
					hidden_flag = 1;
				color = am->wall_normal_color;
				break;
			case WALL_BLASTABLE:
				// Hostage doors
				color = am->wall_door_color;	
				break;
			}
		}
	
		if (segnum==Player_init[Player_num].segnum)
			color = BM_XRGB(31,0,31);

		if ( color != 255 )	{
			// If they have a map powerup, draw unvisited areas in dark blue.
			if (Players[Player_num].flags & PLAYER_FLAGS_MAP_ALL && (!Automap_visited[segnum]))	
				color = BM_XRGB( 0, 0, 25 );

			get_side_verts(vertex_list,segnum,sn);
			add_one_edge( am, vertex_list[0], vertex_list[1], color, sn, segnum, hidden_flag, 0, no_fade );
			add_one_edge( am, vertex_list[1], vertex_list[2], color, sn, segnum, hidden_flag, 0, no_fade );
			add_one_edge( am, vertex_list[2], vertex_list[3], color, sn, segnum, hidden_flag, 0, no_fade );
			add_one_edge( am, vertex_list[3], vertex_list[0], color, sn, segnum, hidden_flag, 0, no_fade );

			if ( is_grate )	{
				add_one_edge( am, vertex_list[0], vertex_list[2], color, sn, segnum, hidden_flag, 1, no_fade );
				add_one_edge( am, vertex_list[1], vertex_list[3], color, sn, segnum, hidden_flag, 1, no_fade );
			}
		}
	}
}


// Adds all the edges from a segment we haven't visited yet.

void add_unknown_segment_edges(automap *am, segment *seg)
{
	int sn;
	int segnum = seg-Segments;
	
	for (sn=0;sn<MAX_SIDES_PER_SEGMENT;sn++) {
		int	vertex_list[4];

		// Only add edges that have no children
		if (seg->children[sn] == -1) {
			get_side_verts(vertex_list,segnum,sn);
	
			add_one_unknown_edge( am, vertex_list[0], vertex_list[1] );
			add_one_unknown_edge( am, vertex_list[1], vertex_list[2] );
			add_one_unknown_edge( am, vertex_list[2], vertex_list[3] );
			add_one_unknown_edge( am, vertex_list[3], vertex_list[0] );
		}
	}
}

void automap_build_edge_list(automap *am)
{	
	int	i,e1,e2,s;
	Edge_info * e;

	// clear edge list
	for (i=0; i<am->max_edges; i++) {
		am->edges[i].num_faces = 0;
		am->edges[i].flags = 0;
	}
	am->num_edges = 0;
	am->highest_edge_index = -1;

	if (cheats.fullautomap || (Players[Player_num].flags & PLAYER_FLAGS_MAP_ALL) )	{
		// Cheating, add all edges as visited
		for (s=0; s<=Highest_segment_index; s++)
#ifdef EDITOR
			if (Segments[s].segnum != -1)
#endif
			{
				add_segment_edges(am, &Segments[s]);
			}
	} else {
		// Not cheating, add visited edges, and then unvisited edges
		for (s=0; s<=Highest_segment_index; s++)
#ifdef EDITOR
			if (Segments[s].segnum != -1)
#endif
				if (Automap_visited[s]) {
					add_segment_edges(am, &Segments[s]);
				}
	
		for (s=0; s<=Highest_segment_index; s++)
#ifdef EDITOR
			if (Segments[s].segnum != -1)
#endif
				if (!Automap_visited[s]) {
					add_unknown_segment_edges(am, &Segments[s]);
				}
	}

	// Find unnecessary lines (These are lines that don't have to be drawn because they have small curvature)
	for (i=0; i<=am->highest_edge_index; i++ )	{
		e = &am->edges[i];
		if (!(e->flags&EF_USED)) continue;

		for (e1=0; e1<e->num_faces; e1++ )	{
			for (e2=1; e2<e->num_faces; e2++ )	{
				if ( (e1 != e2) && (e->segnum[e1] != e->segnum[e2]) )	{
#ifdef COMPACT_SEGS
					vms_vector v1, v2;
					get_side_normal(&Segments[e->segnum[e1]], e->sides[e1], 0, &v1 );
					get_side_normal(&Segments[e->segnum[e2]], e->sides[e2], 0, &v2 );
					if ( vm_vec_dot(&v1,&v2) > (F1_0-(F1_0/10))  )	{
#else
					if ( vm_vec_dot( &Segments[e->segnum[e1]].sides[e->sides[e1]].normals[0], &Segments[e->segnum[e2]].sides[e->sides[e2]].normals[0] ) > (F1_0-(F1_0/10))  )	{
#endif
						e->flags &= (~EF_DEFINING);
						break;
					}
				}
			}
			if (!(e->flags & EF_DEFINING))
				break;
		}
	}
}
