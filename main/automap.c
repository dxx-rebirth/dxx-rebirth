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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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
#include "network.h"
#include "kconfig.h"
#include "multi.h"
#include "endlevel.h"
#include "text.h"
#include "gauges.h"
#include "songs.h"
#include "powerup.h"
#include "network.h" 
#include "newmenu.h"
#include "cntrlcen.h"
#include "timer.h"
#include "automap.h"
#include "config.h"

#define EF_USED     1   // This edge is used
#define EF_DEFINING 2   // A structure defining edge that should always draw.
#define EF_FRONTIER 4   // An edge between the known and the unknown.
#define EF_SECRET   8   // An edge that is part of a secret wall.
#define EF_GRATE    16  // A grate... draw it all the time.
#define EF_NO_FADE  32  // An edge that doesn't fade with distance
#define EF_TOO_FAR  64  // An edge that is too far away

typedef struct Edge_info {
	short verts[2];     // 4 bytes
	ubyte sides[4];     // 4 bytes
	short segnum[4];    // 8 bytes  // This might not need to be stored... If you can access the normals of a side.
	ubyte flags;        // 1 bytes  // See the EF_??? defines above.
	ubyte color;        // 1 bytes
	ubyte num_faces;    // 1 bytes  // 19 bytes...
} Edge_info;

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

int Wall_normal_color;
int Wall_door_color;
int Wall_door_blue;
int Wall_door_gold;
int Wall_door_red;
int Hostage_color;
int Font_color_20;
int Green_31;
int White_63;
int Blue_48;
int Red_48;

void init_automap_colors(void)
{
	Wall_normal_color = K_WALL_NORMAL_COLOR;
	Wall_door_color = K_WALL_DOOR_COLOR;
	Wall_door_blue = K_WALL_DOOR_BLUE;
	Wall_door_gold = K_WALL_DOOR_GOLD;
	Wall_door_red = K_WALL_DOOR_RED;
	Hostage_color = K_HOSTAGE_COLOR;
	Font_color_20 = K_FONT_COLOR_20;
	Green_31 = K_GREEN_31;
	White_63 = gr_find_closest_color_current(63,63,63);
	Blue_48 = gr_find_closest_color_current(0,0,48);
	Red_48 = gr_find_closest_color_current(48,0,0);
}

// Segment visited list
ubyte Automap_visited[MAX_SEGMENTS];

// Edge list variables
static int Num_edges=0;
static int Max_edges; //set each frame
static int Highest_edge_index = -1;
static Edge_info Edges[MAX_EDGES];
static short DrawingListBright[MAX_EDGES];

// Map movement defines
#define PITCH_DEFAULT		9000
#define ZOOM_DEFAULT		i2f(20*10)
#define ZOOM_MIN_VALUE		i2f(20*5)
#define ZOOM_MAX_VALUE		i2f(20*100)

#define SLIDE_SPEED 		(350)
#define ZOOM_SPEED_FACTOR	(500)
#define ROT_SPEED_DIVISOR	(115000)

// Screen anvas variables
static grs_canvas Automap_view;

grs_bitmap Automap_background;

// Flags
static int Automap_cheat = 0;		// If set, show everything

// Rendering variables
static fix Automap_zoom = 0x9000;
static vms_vector view_target;
static fix Automap_farthest_dist = (F1_0 * 20 * 50);		// 50 segments away
static vms_matrix ViewMatrix;
static fix ViewDist=0;

//	Function Prototypes
void adjust_segment_limit(int SegmentLimit);
void draw_all_edges(void);
void automap_build_edge_list(void);

#define	MAX_DROP_MULTI	2
#define	MAX_DROP_SINGLE	9

extern vms_vector Matrix_scale;		//how the matrix is currently scaled

# define automap_draw_line g3_draw_line

void automap_clear_visited()	
{
	int i;
	for (i=0; i<MAX_SEGMENTS; i++ )
		Automap_visited[i] = 0;
}

char		name_level[128];

void name_frame()
{
	if (Current_level_num > 0)
		sprintf(name_level, "%s %i: ",TXT_LEVEL, Current_level_num);
	else
		name_level[0] = 0;

	strcat(name_level, Current_level_name);

	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(Green_31,-1);
	gr_printf((SWIDTH/64),(SHEIGHT/48),"%s", name_level);
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

void draw_automap(int flip)
{
	int i;
	int color;
	object * objp;
	vms_vector viewer_position;
	g3s_point sphere_point;

	gr_set_current_canvas(NULL);
	show_fullscr(&Automap_background);
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
	
	gr_set_current_canvas(&Automap_view);

	gr_clear_canvas(BM_XRGB(0,0,0));

	g3_start_frame();
	render_start_frame();

	vm_vec_scale_add(&viewer_position,&view_target,&ViewMatrix.fvec,-ViewDist );

	g3_set_view_matrix(&viewer_position,&ViewMatrix,Automap_zoom);

	draw_all_edges();

	// Draw player...
#ifdef NETWORK
	if (Game_mode & GM_TEAM)
		color = get_team(Player_num);
	else
#endif	
		color = Player_num; // Note link to above if!

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
			gr_setcolor(Hostage_color);
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

	name_frame();

	if (flip)
		gr_flip();
}

#define LEAVE_TIME 0x4000
#define WINDOW_WIDTH		288

extern void GameLoop(int, int );
extern int set_segment_depths(int start_seg, ubyte *segbuf);
int Automap_active = 0;

#define MAP_BACKGROUND_FILENAME "MAP.PCX"

void do_automap( int key_code )	{
	int done=0;
	vms_matrix	tempm;
	vms_angvec	tangles;
	int leave_mode=0;
	int first_time=1;
	int pcx_error;
#ifndef NDEBUG
	int i;
#endif
	int c;
	fix entry_time;
	int pause_game=1;	// Set to 1 if everything is paused during automap...No pause during net.
	fix t1, t2;
	control_info saved_control_info;
	int Max_segments_away = 0;
	int SegmentLimit = 1;
	ubyte pal[256*3];
	
	Automap_active = 1;

	init_automap_colors();

	key_code = key_code;	// disable warning...

	if ((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME) && (!Endlevel_sequence))
		pause_game = 0;

	if (pause_game)
		stop_time();

	Max_edges = min(MAX_EDGES_FROM_VERTS(Num_vertices),MAX_EDGES);	//make maybe smaller than max

	gr_set_current_canvas(NULL);

	automap_build_edge_list();

	if ( ViewDist==0 )
		ViewDist = ZOOM_DEFAULT;
	ViewMatrix = Objects[Players[Player_num].objnum].orient;

	tangles.p = PITCH_DEFAULT;
	tangles.h  = 0;
	tangles.b  = 0;

	done = 0;

	view_target = Objects[Players[Player_num].objnum].pos;

	t1 = entry_time = timer_get_fixed_seconds();
	t2 = t1;

	//Fill in Automap_visited from Objects[Players[Player_num].objnum].segnum
	Max_segments_away = set_segment_depths(Objects[Players[Player_num].objnum].segnum, Automap_visited);
	SegmentLimit = Max_segments_away;

	adjust_segment_limit(SegmentLimit);

	// ZICO - code from above to show frame in OGL correctly. Redundant, but better readable.
	// KREATOR - Now applies to all platforms so double buffering is supported
	gr_init_bitmap_data (&Automap_background);
	pcx_error = pcx_read_bitmap(MAP_BACKGROUND_FILENAME, &Automap_background, BM_LINEAR, pal);
	if (pcx_error != PCX_ERROR_NONE)
		Error("File %s - PCX error: %s", MAP_BACKGROUND_FILENAME, pcx_errormsg(pcx_error));
	gr_remap_bitmap_good(&Automap_background, pal, -1, -1);
	if (!MacHog)
		gr_init_sub_canvas(&Automap_view, &grd_curscreen->sc_canvas, (SWIDTH/23), (SHEIGHT/6), (SWIDTH/1.1), (SHEIGHT/1.45));
	else
		gr_init_sub_canvas(&Automap_view, &grd_curscreen->sc_canvas, 38*(SWIDTH/640.0), 77*(SHEIGHT/480.0), 564*(SWIDTH/640.0), 381*(SHEIGHT/480.0));

	while(!done)	{
		if ( leave_mode==0 && Controls.automap_state && (timer_get_fixed_seconds()-entry_time)>LEAVE_TIME)
			leave_mode = 1;

		if ( !Controls.automap_state && (leave_mode==1) )
			done=1;

		if (!pause_game)	{
			ushort old_wiggle;
			saved_control_info = Controls;					// Save controls so we can zero them
			memset(&Controls,0,sizeof(control_info));			// Clear everything...
			old_wiggle = ConsoleObject->mtype.phys_info.flags & PF_WIGGLE;	// Save old wiggle
			ConsoleObject->mtype.phys_info.flags &= ~PF_WIGGLE;		// Turn off wiggle
#ifdef NETWORK
			if (multi_menu_poll())
				done = 1;
#endif
			ConsoleObject->mtype.phys_info.flags |= old_wiggle;		// Restore wiggle
			Controls = saved_control_info;
		}

		controls_read_all();

		if ( Controls.automap_down_count ) {
			if (leave_mode==0)
				done = 1;
			c = 0;
		}

		//see if redbook song needs to be restarted
		songs_check_redbook_repeat();
		
		while( (c=key_inkey()) )	{
			switch( c ) {
#ifndef NDEBUG
			case KEY_BACKSP: Int3(); break;
#endif
	
			case KEY_PRINT_SCREEN: {
				gr_set_current_canvas(NULL);
				save_screen_shot(1);
				break;
			}
	
			case KEY_ESC:
				if (leave_mode==0)
					done = 1;
				 break;
			case KEY_ALTED+KEY_F:           // Alt+F shows full map, if cheats enabled
				if (Cheats_enabled) 	 
				{
					uint t;
					t = Players[Player_num].flags;
					Players[Player_num].flags |= PLAYER_FLAGS_MAP_ALL_CHEAT;
					automap_build_edge_list();
					Players[Player_num].flags=t;
				}
				break;
#ifndef NDEBUG
		  	case KEY_DEBUGGED+KEY_F: 	{
				for (i=0; i<=Highest_segment_index; i++ )
					Automap_visited[i] = 1;
				automap_build_edge_list();
				Max_segments_away = set_segment_depths(Objects[Players[Player_num].objnum].segnum, Automap_visited);
				SegmentLimit = Max_segments_away;
				adjust_segment_limit(SegmentLimit);
				}
				break;
#endif

			case KEY_F9:
				if (SegmentLimit > 1) 		{
					SegmentLimit--;
					adjust_segment_limit(SegmentLimit);
				}
				break;
			case KEY_F10:
				if (SegmentLimit < Max_segments_away) 	{
					SegmentLimit++;
					adjust_segment_limit(SegmentLimit);
				}
				break;
			case KEY_ALTED+KEY_ENTER:
			case KEY_ALTED+KEY_PADENTER:
				gr_toggle_fullscreen();
				break;
			}
		}

		if ( Controls.fire_primary_down_count )	{
			// Reset orientation
			ViewDist = ZOOM_DEFAULT;
			tangles.p = PITCH_DEFAULT;
			tangles.h  = 0;
			tangles.b  = 0;
			view_target = Objects[Players[Player_num].objnum].pos;
		}

		ViewDist -= Controls.forward_thrust_time*ZOOM_SPEED_FACTOR;

		tangles.p += fixdiv( Controls.pitch_time, ROT_SPEED_DIVISOR );
		tangles.h  += fixdiv( Controls.heading_time, ROT_SPEED_DIVISOR );
		tangles.b  += fixdiv( Controls.bank_time, ROT_SPEED_DIVISOR*2 );
		
		if ( Controls.vertical_thrust_time || Controls.sideways_thrust_time )	{
			vms_angvec	tangles1;
			vms_vector	old_vt;
			old_vt = view_target;
			tangles1 = tangles;
			vm_angles_2_matrix(&tempm,&tangles1);
			vm_matrix_x_matrix(&ViewMatrix,&Objects[Players[Player_num].objnum].orient,&tempm);
			vm_vec_scale_add2( &view_target, &ViewMatrix.uvec, Controls.vertical_thrust_time*SLIDE_SPEED );
			vm_vec_scale_add2( &view_target, &ViewMatrix.rvec, Controls.sideways_thrust_time*SLIDE_SPEED );
			if ( vm_vec_dist_quick( &view_target, &Objects[Players[Player_num].objnum].pos) > i2f(1000) )	{
				view_target = old_vt;
			}
		}

		vm_angles_2_matrix(&tempm,&tangles);
		vm_matrix_x_matrix(&ViewMatrix,&Objects[Players[Player_num].objnum].orient,&tempm);

		if ( ViewDist < ZOOM_MIN_VALUE ) ViewDist = ZOOM_MIN_VALUE;
		if ( ViewDist > ZOOM_MAX_VALUE ) ViewDist = ZOOM_MAX_VALUE;

		draw_automap(GameArg.DbgUseDoubleBuffer);

		if ( first_time )	{
			first_time = 0;
			gr_palette_load( gr_palette );
		}

		t2 = timer_get_fixed_seconds();
		while (t2 - t1 < F1_0 / (GameCfg.VSync?MAXIMUM_FPS:GameArg.SysMaxFPS)) // ogl is fast enough that the automap can read the input too fast and you start to turn really slow.  So delay a bit (and free up some cpu :)
		{
			if (GameArg.SysUseNiceFPS && !GameCfg.VSync)
				timer_delay(f1_0 / GameArg.SysMaxFPS - (t2 - t1));
			t2 = timer_get_fixed_seconds();
		}
		if (pause_game)
		{
			FrameTime=t2-t1;
			FixedStepCalc();
		}
		t1 = t2;
	}

#ifdef OGL
		gr_free_bitmap_data(&Automap_background);
#endif

	game_flush_inputs();
	
	if (pause_game)
		start_time();

	Automap_active = 0;
}

void adjust_segment_limit(int SegmentLimit)
{
	int i,e1;
	Edge_info * e;

	for (i=0; i<=Highest_edge_index; i++ )	{
		e = &Edges[i];
		e->flags |= EF_TOO_FAR;
		for (e1=0; e1<e->num_faces; e1++ )	{
			if ( Automap_visited[e->segnum[e1]] <= SegmentLimit )	{
				e->flags &= (~EF_TOO_FAR);
				break;
			}
		}
	}
}

void draw_all_edges()	
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

	for (i=0; i<=Highest_edge_index; i++ )	{
		//e = &Edges[Edge_used_list[i]];
		e = &Edges[i];
		if (!(e->flags & EF_USED)) continue;

		if ( e->flags & EF_TOO_FAR) continue;

		if (e->flags&EF_FRONTIER) { 		// A line that is between what we have seen and what we haven't
			if ( (!(e->flags&EF_SECRET))&&(e->color==Wall_normal_color))
				continue;		// If a line isn't secret and is normal color, then don't draw it
		}

		cc=rotate_list(2,e->verts);
		distance = Segment_points[e->verts[1]].p3_z;

		if (min_distance>distance )
			min_distance = distance;

		if (!cc.and) 	{	//all off screen?
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
				DrawingListBright[nbright++] = e-Edges;
			} else if ( e->flags&(EF_DEFINING|EF_GRATE) )	{
				if ( nfacing == 0 )	{
					if ( e->flags & EF_NO_FADE )
						gr_setcolor( e->color );
					else
						gr_setcolor( gr_fade_table[e->color+256*8] );
					g3_draw_line( &Segment_points[e->verts[0]], &Segment_points[e->verts[1]] );
				} 	else {
					DrawingListBright[nbright++] = e-Edges;
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
					v1 = Edges[DrawingListBright[j]].verts[0];
					v2 = Edges[DrawingListBright[j+incr]].verts[0];

					if (Segment_points[v1].p3_z < Segment_points[v2].p3_z) {
						// If not in correct order, them swap 'em
						t=DrawingListBright[j+incr];
						DrawingListBright[j+incr]=DrawingListBright[j];
						DrawingListBright[j]=t;
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
		e = &Edges[DrawingListBright[i]];
		p1 = &Segment_points[e->verts[0]];
		p2 = &Segment_points[e->verts[1]];
		dist = p1->p3_z - min_distance;
		// Make distance be 1.0 to 0.0, where 0.0 is 10 segments away;
		if ( dist < 0 ) dist=0;
		if ( dist >= Automap_farthest_dist ) continue;

		if ( e->flags & EF_NO_FADE )	{
			gr_setcolor( e->color );
		} else {
			dist = F1_0 - fixdiv( dist, Automap_farthest_dist );
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
static int automap_find_edge(int v0,int v1,Edge_info **edge_ptr)
{
	long vv, evv;
	short hash,oldhash;
	int ret, ev0, ev1;

	vv = (v1<<16) + v0;

	oldhash = hash = ((v0*5+v1) % Max_edges);

	ret = -1;

	while (ret==-1) {
		ev0 = (int)(Edges[hash].verts[0]);
		ev1 = (int)(Edges[hash].verts[1]);
		evv = (ev1<<16)+ev0;
		if (Edges[hash].num_faces == 0 ) ret=0;
		else if (evv == vv) ret=1;
		else {
			if (++hash==Max_edges) hash=0;
			if (hash==oldhash) Error("Edge list full!");
		}
	}

	*edge_ptr = &Edges[hash];

	if (ret == 0)
		return -1;
	else
		return hash;

}


void add_one_edge( short va, short vb, ubyte color, ubyte side, short segnum, int hidden, int grate, int no_fade )	{
	int found;
	Edge_info *e;
	short tmp;

	if ( Num_edges >= Max_edges)	{
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

	found = automap_find_edge(va,vb,&e);
		
	if (found == -1) {
		e->verts[0] = va;
		e->verts[1] = vb;
		e->color = color;
		e->num_faces = 1;
		e->flags = EF_USED | EF_DEFINING;			// Assume a normal line
		e->sides[0] = side;
		e->segnum[0] = segnum;
		//Edge_used_list[Num_edges] = e-Edges;
		if ( (e-Edges) > Highest_edge_index )
			Highest_edge_index = e - Edges;
		Num_edges++;
	} else {
		if ( color != Wall_normal_color )
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

void add_one_unknown_edge( short va, short vb )	{
	int found;
	Edge_info *e;
	short tmp;

	if ( va > vb )	{
		tmp = va;
		va = vb;
		vb = tmp;
	}

	found = automap_find_edge(va,vb,&e);
	if (found != -1) 	
		e->flags|=EF_FRONTIER;		// Mark as a border edge
}

#ifndef _GAMESEQ_H
extern obj_position Player_init[];
#endif

void add_segment_edges(segment *seg)
{
	int 	is_grate, no_fade;
	ubyte	color;
	int	sn;
	int	segnum = seg-Segments;
	int	hidden_flag;
	int ttype,trigger_num;
	
	for (sn=0;sn<MAX_SIDES_PER_SEGMENT;sn++) {
		short	vertex_list[4];

		hidden_flag = 0;

		is_grate = 0;
		no_fade = 0;

		color = 255;
		if (seg->children[sn] == -1) {
			color = Wall_normal_color;
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
		
			trigger_num = Walls[seg->sides[sn].wall_num].trigger;
			ttype = Triggers[trigger_num].type;

			switch( Walls[seg->sides[sn].wall_num].type )	{
			case WALL_DOOR:
				if (Walls[seg->sides[sn].wall_num].keys == KEY_BLUE) {
					no_fade = 1;
					color = Wall_door_blue;
				} else if (Walls[seg->sides[sn].wall_num].keys == KEY_GOLD) {
					no_fade = 1;
					color = Wall_door_gold;
				} else if (Walls[seg->sides[sn].wall_num].keys == KEY_RED) {
					no_fade = 1;
					color = Wall_door_red;
				} else if (!(WallAnims[Walls[seg->sides[sn].wall_num].clip_num].flags & WCF_HIDDEN)) {
					int	connected_seg = seg->children[sn];
					if (connected_seg != -1) {
						int connected_side = find_connect_side(seg, &Segments[connected_seg]);
						int	keytype = Walls[Segments[connected_seg].sides[connected_side].wall_num].keys;
						if ((keytype != KEY_BLUE) && (keytype != KEY_GOLD) && (keytype != KEY_RED))
							color = Wall_door_color;
						else {
							switch (Walls[Segments[connected_seg].sides[connected_side].wall_num].keys) {
								case KEY_BLUE:	color = Wall_door_blue;	no_fade = 1; break;
								case KEY_GOLD:	color = Wall_door_gold;	no_fade = 1; break;
								case KEY_RED:	color = Wall_door_red;	no_fade = 1; break;
								default:	Error("Inconsistent data.  Supposed to be a colored wall, but not blue, gold or red.\n");
							}
						}

					}
				} else {
					color = Wall_normal_color;
					hidden_flag = 1;
				}
				break;
			case WALL_CLOSED:
				// Make grates draw properly
				if (WALL_IS_DOORWAY(seg,sn) & WID_RENDPAST_FLAG)
					is_grate = 1;
				else
					hidden_flag = 1;
				color = Wall_normal_color;
				break;
			case WALL_BLASTABLE:
				// Hostage doors
				color = Wall_door_color;	
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
			add_one_edge( vertex_list[0], vertex_list[1], color, sn, segnum, hidden_flag, 0, no_fade );
			add_one_edge( vertex_list[1], vertex_list[2], color, sn, segnum, hidden_flag, 0, no_fade );
			add_one_edge( vertex_list[2], vertex_list[3], color, sn, segnum, hidden_flag, 0, no_fade );
			add_one_edge( vertex_list[3], vertex_list[0], color, sn, segnum, hidden_flag, 0, no_fade );

			if ( is_grate )	{
				add_one_edge( vertex_list[0], vertex_list[2], color, sn, segnum, hidden_flag, 1, no_fade );
				add_one_edge( vertex_list[1], vertex_list[3], color, sn, segnum, hidden_flag, 1, no_fade );
			}
		}
	}
}


// Adds all the edges from a segment we haven't visited yet.

void add_unknown_segment_edges(segment *seg)
{
	int sn;
	int segnum = seg-Segments;
	
	for (sn=0;sn<MAX_SIDES_PER_SEGMENT;sn++) {
		short	vertex_list[4];

		// Only add edges that have no children
		if (seg->children[sn] == -1) {
			get_side_verts(vertex_list,segnum,sn);
			add_one_unknown_edge( vertex_list[0], vertex_list[1] );
			add_one_unknown_edge( vertex_list[1], vertex_list[2] );
			add_one_unknown_edge( vertex_list[2], vertex_list[3] );
			add_one_unknown_edge( vertex_list[3], vertex_list[0] );
		}
	}
}

void automap_build_edge_list()
{	
	int	i,e1,e2,s;
	Edge_info * e;

	Automap_cheat = 0;

	if ( Players[Player_num].flags & PLAYER_FLAGS_MAP_ALL_CHEAT )
		Automap_cheat = 1;		// Damn cheaters...

	// clear edge list
	for (i=0; i<Max_edges; i++) {
		Edges[i].num_faces = 0;
		Edges[i].flags = 0;
	}
	Num_edges = 0;
	Highest_edge_index = -1;

	if (Automap_cheat || (Players[Player_num].flags & PLAYER_FLAGS_MAP_ALL) )	{
		// Cheating, add all edges as visited
		for (s=0; s<=Highest_segment_index; s++)
#ifdef EDITOR
			if (Segments[s].segnum != -1)
#endif
			{
				add_segment_edges(&Segments[s]);
			}
	} else {
		// Not cheating, add visited edges, and then unvisited edges
		for (s=0; s<=Highest_segment_index; s++)
#ifdef EDITOR
			if (Segments[s].segnum != -1)
#endif
				if (Automap_visited[s]) {
					add_segment_edges(&Segments[s]);
				}
	
		for (s=0; s<=Highest_segment_index; s++)
#ifdef EDITOR
			if (Segments[s].segnum != -1)
#endif
				if (!Automap_visited[s]) {
					add_unknown_segment_edges(&Segments[s]);
				}
	}

	// Find unnecessary lines (These are lines that don't have to be drawn because they have small curvature)
	for (i=0; i<=Highest_edge_index; i++ )	{
		e = &Edges[i];
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
