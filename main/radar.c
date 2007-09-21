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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 *
 * Routines for drawing the radar.
 *
 */


#include <stdlib.h>

#include "error.h"
#include "3d.h"
#include "inferno.h"
#include "object.h"
#include "vclip.h"
#include "game.h"
#include "mono.h"
#include "polyobj.h"
#include "sounds.h"
#include "player.h"
#include "bm.h"
#include "hostage.h"
#include "multi.h"
#include "network.h"
#include "gauges.h"

int show_radar=0;
int Network_allow_radar=0;
short Hostage_monitor_size, Hostage_monitor_x, Hostage_monitor_y;

static fix radx, rady, rox, roy, cenx, ceny;

typedef struct blip	{
	short x, y;
	ubyte c;
} blip;

#define MAX_BLIPS 1000

blip Blips[MAX_BLIPS];
int N_blips = 0;
fix Radar_farthest_dist = (F1_0 * 20 * 15);             // 15 segments away

void radar_plot_object( object * objp, int hue )
{
	ubyte flags;
	g3s_point pos;
	int color;
	fix dist, rscale, zdist, distance;
	int xpos, ypos;

	flags = g3_rotate_point(&pos,&objp->pos);
	dist = vm_vec_mag_quick(&pos.p3_vec);

        // Make distance be 1.0 to 0.0, where 0.0 is maximum segments away;
	if ( dist >= Radar_farthest_dist ) return;
	distance = F1_0 - fixdiv( dist, Radar_farthest_dist );
	color = f2i( distance*31 );

	zdist = fix_sqrt( fixmul(pos.p3_x,pos.p3_x)+fixmul(pos.p3_y,pos.p3_y) );
	if (zdist < 100 ) return;		// Watch for divide overflow

	rscale = fix_acos( fixdiv(pos.p3_z,dist) )/2;

	xpos = f2i(fixmul( rox+fixmul(fixdiv(pos.p3_x,zdist),rscale), radx));
	ypos = f2i(fixmul( roy-fixmul(fixdiv(pos.p3_y,zdist),rscale), rady));

	if ( xpos < Hostage_monitor_x ) xpos = Hostage_monitor_x;
	if ( ypos < Hostage_monitor_y ) ypos = Hostage_monitor_y;
        if ( xpos > Hostage_monitor_x+Hostage_monitor_size-1 ) xpos = Hostage_monitor_x+Hostage_monitor_size-1;
        if ( ypos > Hostage_monitor_y+Hostage_monitor_size-1 ) ypos = Hostage_monitor_y+Hostage_monitor_size-1;

	Blips[N_blips].c = gr_fade_table[hue+color*256];
	Blips[N_blips].x = xpos; Blips[N_blips].y = ypos;
	N_blips++;
}

void radar_render_frame()	
{
	int i,color;
	object * objp;

	Hostage_monitor_size = (SWIDTH / 6);
	Hostage_monitor_x = (SWIDTH/320);

	switch (Cockpit_mode)
	{
		case CM_FULL_SCREEN:
			Hostage_monitor_y = 35*(SHEIGHT/200);
			break;
		case CM_FULL_COCKPIT:
			Hostage_monitor_y = 15*(SHEIGHT/200); //(grd_curscreen->sc_h) / 2.2;
			break;
		case CM_STATUS_BAR:
			Hostage_monitor_y = (SHEIGHT/200);
			break;
		case CM_REAR_VIEW: //no radar in rear view or letterbox!
		case CM_LETTERBOX:
			return;
		}

	if (hostage_is_vclip_playing())
		return;

	gr_set_current_canvas(NULL);

	// Draw a box to get more contrast on the radar
	gr_setcolor(BM_XRGB(200,200,200));
#ifdef OGL
	Gr_scanline_darkening_level = 2*7;
#endif
	gr_rect(Hostage_monitor_x,
		Hostage_monitor_y,
		Hostage_monitor_x+Hostage_monitor_size,
		Hostage_monitor_y+Hostage_monitor_size);
	Gr_scanline_darkening_level = GR_FADE_LEVELS;


	gr_setcolor( BM_XRGB( 0, 31, 0 ) );
	
        gr_ucircle( i2f(Hostage_monitor_x+Hostage_monitor_size/2), i2f(Hostage_monitor_y+Hostage_monitor_size/2),     i2f(Hostage_monitor_size)/2);

	//other stuff added 9/14/98 by Victor Rachels for fun.
	gr_circle( i2f(Hostage_monitor_x+Hostage_monitor_size/2), i2f(Hostage_monitor_y+Hostage_monitor_size/2),     i2f(Hostage_monitor_size) / 8 );
	gr_upixel((Hostage_monitor_x+Hostage_monitor_size/2), (Hostage_monitor_y+Hostage_monitor_size/2) );
	gr_uline(i2f(Hostage_monitor_x+Hostage_monitor_size/10),i2f(Hostage_monitor_y+Hostage_monitor_size/2),i2f(Hostage_monitor_x+Hostage_monitor_size*2/10),i2f(Hostage_monitor_y+Hostage_monitor_size/2));
	gr_uline(i2f(Hostage_monitor_x+Hostage_monitor_size-Hostage_monitor_size/10),i2f(Hostage_monitor_y+Hostage_monitor_size/2),i2f(Hostage_monitor_x+Hostage_monitor_size-Hostage_monitor_size*2/10),i2f(Hostage_monitor_y+Hostage_monitor_size/2));


        N_blips = 0;

        radx = i2f(Hostage_monitor_size*4)/2;
        rady = i2f(Hostage_monitor_size*4)/2;
        cenx = i2f(Hostage_monitor_x)+i2f(Hostage_monitor_size)/2;
        ceny = i2f(Hostage_monitor_y)+i2f(Hostage_monitor_size)/2;

	rox = fixdiv( cenx, radx );
	roy = fixdiv( ceny, rady );

	objp = Objects;
	for (i=0;i<=Highest_object_index;i++) {
		switch( objp->type )	{
		case OBJ_PLAYER:
			if ( i != Players[Player_num].objnum )	{
#ifdef NETWORK
				if (Game_mode & GM_TEAM)
					color = get_team(i);
				else
#endif
                                        color = i;
				radar_plot_object( objp, gr_getcolor(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b) );
			}
			break;
		case OBJ_HOSTAGE:
			radar_plot_object( objp, BM_XRGB(0,31,0) );
			break;
		case OBJ_POWERUP:
			if(!(Game_mode & GM_MULTI))
				radar_plot_object( objp, BM_XRGB(0,0,31) );
			break;
		case OBJ_ROBOT:
			radar_plot_object( objp, BM_XRGB(31,0,0) );
			break;
		case OBJ_CNTRLCEN:
			radar_plot_object( objp, BM_XRGB(31,31,31) );
			break;
		default:
			break;
		}
		objp++;
	}

	// Draw new blips...
	for (i=0; i<N_blips; i++ )	{
		gr_setcolor( Blips[i].c );
		gr_upixel( Blips[i].x, Blips[i].y );
	}

	// Draw a blue frame around the radar
	gr_setcolor(BM_XRGB(0,0,255));
	gr_rect(Hostage_monitor_x-(SWIDTH/640),
		Hostage_monitor_y,
		Hostage_monitor_x,
		Hostage_monitor_y+Hostage_monitor_size);
	gr_rect(Hostage_monitor_x+Hostage_monitor_size,
		Hostage_monitor_y,
		Hostage_monitor_x+Hostage_monitor_size+(SWIDTH/640),
		Hostage_monitor_y+Hostage_monitor_size);
	gr_rect(Hostage_monitor_x,
		Hostage_monitor_y-(SHEIGHT/480),
		Hostage_monitor_x+Hostage_monitor_size,
		Hostage_monitor_y);
	gr_rect(Hostage_monitor_x,
		Hostage_monitor_y+Hostage_monitor_size,
		Hostage_monitor_x+Hostage_monitor_size,
		Hostage_monitor_y+Hostage_monitor_size+(SHEIGHT/480));
}
