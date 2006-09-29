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
 * Inferno gauge drivers
 *
 */

#ifdef RCS
#pragma off (unreferenced)
static char rcsid[] = "$Id: gauges.c,v 1.2 2006/03/18 23:08:13 michaelstather Exp $";
#pragma on (unreferenced)
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "inferno.h"
#include "game.h"
#include "screens.h"
#include "gauges.h"
#include "physics.h"
#include "error.h"
#include "menu.h"
#include "mono.h"
#include "collide.h"
#include "newdemo.h"
#include "player.h"
#include "gamefont.h"
#include "hostage.h"
#include "bm.h"
#include "text.h"
#include "powerup.h"
#include "sounds.h"
#include "multi.h"
#include "network.h"
#include "endlevel.h"
#include "wall.h"
#include "text.h"
#include "render.h"
#include "piggy.h"
#include "pingstat.h"
#ifdef OGL
#include "ogl_init.h"
#endif

int Gauge_hud_mode=0;
bitmap_index Gauges[MAX_GAUGE_BMS]; // Array of all gauge bitmaps.
grs_canvas *Canv_LeftEnergyGauge;
grs_canvas *Canv_SBEnergyGauge;
grs_canvas *Canv_RightEnergyGauge;
grs_canvas *Canv_NumericalGauge;

//bitmap numbers for gauges
#define GAUGE_SHIELDS			0		//0..9, in decreasing order (100%,90%...0%)
#define GAUGE_INVULNERABLE		10		//10..19
#define N_INVULNERABLE_FRAMES		10
#define GAUGE_SPEED		   	20		//unused
#define GAUGE_ENERGY_LEFT		21
#define GAUGE_ENERGY_RIGHT		22
#define GAUGE_NUMERICAL			23
#define GAUGE_BLUE_KEY			24
#define GAUGE_GOLD_KEY			25
#define GAUGE_RED_KEY			26
#define GAUGE_BLUE_KEY_OFF		27
#define GAUGE_GOLD_KEY_OFF		28
#define GAUGE_RED_KEY_OFF		29
#define SB_GAUGE_BLUE_KEY		30
#define SB_GAUGE_GOLD_KEY		31
#define SB_GAUGE_RED_KEY		32
#define SB_GAUGE_BLUE_KEY_OFF		33
#define SB_GAUGE_GOLD_KEY_OFF		34
#define SB_GAUGE_RED_KEY_OFF		35
#define SB_GAUGE_ENERGY			36
#define GAUGE_LIVES			37
#define GAUGE_SHIPS			38
#define GAUGE_SHIPS_LAST		45
#define RETICLE_CROSS			46
#define RETICLE_PRIMARY			48
#define RETICLE_SECONDARY		51
#define RETICLE_LAST			55
#define GAUGE_HOMING_WARNING_ON		56
#define GAUGE_HOMING_WARNING_OFF	57
#define SML_RETICLE_CROSS		58
#define SML_RETICLE_PRIMARY		60
#define SML_RETICLE_SECONDARY		63
#define SML_RETICLE_LAST		67
#define KEY_ICON_BLUE			68
#define KEY_ICON_YELLOW			69
#define KEY_ICON_RED			70

//Coordinats for gauges
#define GAUGE_BLUE_KEY_X		45
#define GAUGE_BLUE_KEY_Y		152
#define GAUGE_GOLD_KEY_X		44
#define GAUGE_GOLD_KEY_Y		162
#define GAUGE_RED_KEY_X			43
#define GAUGE_RED_KEY_Y			172
#define SB_GAUGE_KEYS_X			11
#define SB_GAUGE_BLUE_KEY_Y		153
#define SB_GAUGE_GOLD_KEY_Y		169
#define SB_GAUGE_RED_KEY_Y		185
#define LEFT_ENERGY_GAUGE_X		70
#define LEFT_ENERGY_GAUGE_Y		131
#define LEFT_ENERGY_GAUGE_W		64
#define LEFT_ENERGY_GAUGE_H		8
#define RIGHT_ENERGY_GAUGE_X		190
#define RIGHT_ENERGY_GAUGE_Y		131
#define RIGHT_ENERGY_GAUGE_W		64
#define RIGHT_ENERGY_GAUGE_H		8
#define SB_ENERGY_GAUGE_X 		98
#define SB_ENERGY_GAUGE_Y 		155
#define SB_ENERGY_GAUGE_W 		17
#define SB_ENERGY_GAUGE_H 		41
#define SB_ENERGY_NUM_X 		(SB_ENERGY_GAUGE_X+2)
#define SB_ENERGY_NUM_Y 		190
#define SHIELD_GAUGE_X			146
#define SHIELD_GAUGE_Y			155
#define SHIELD_GAUGE_W			35
#define SHIELD_GAUGE_H			32 
#define SHIP_GAUGE_X 			(SHIELD_GAUGE_X+5)
#define SHIP_GAUGE_Y			(SHIELD_GAUGE_Y+5)
#define SB_SHIELD_GAUGE_X 		123
#define SB_SHIELD_GAUGE_Y 		163
#define SB_SHIP_GAUGE_X 		(SB_SHIELD_GAUGE_X+5)
#define SB_SHIP_GAUGE_Y 		(SB_SHIELD_GAUGE_Y+5)
#define SB_SHIELD_NUM_X 		(SB_SHIELD_GAUGE_X+12)
#define SB_SHIELD_NUM_Y 		156
#define NUMERICAL_GAUGE_X		154
#define NUMERICAL_GAUGE_Y		130
#define NUMERICAL_GAUGE_W		19
#define NUMERICAL_GAUGE_H		22
#define PRIMARY_W_PIC_X			64
#define PRIMARY_W_PIC_Y			154
#define PRIMARY_W_TEXT_X		COCKPITSCALE_X*87
#define PRIMARY_W_TEXT_Y		COCKPITSCALE_Y*157
#define PRIMARY_AMMO_X			COCKPITSCALE_X*93
#define PRIMARY_AMMO_Y			COCKPITSCALE_Y*171
#define SECONDARY_W_PIC_X		234
#define SECONDARY_W_PIC_Y		154
#define SECONDARY_W_TEXT_X		COCKPITSCALE_X*207
#define SECONDARY_W_TEXT_Y		COCKPITSCALE_Y*157
#define SECONDARY_AMMO_X		COCKPITSCALE_X*213
#define SECONDARY_AMMO_Y		COCKPITSCALE_Y*171
#define SB_LIVES_X                      266
#define SB_LIVES_Y                      185
#define SB_LIVES_LABEL_X		237
#define SB_LIVES_LABEL_Y		SB_LIVES_Y
#define SB_SCORE_RIGHT			301
#define SB_SCORE_Y                      158
#define SB_SCORE_LABEL_X		237
#define SB_SCORE_ADDED_RIGHT            301
#define SB_SCORE_ADDED_Y		165

static int score_display;
static fix score_time;

static int old_score[2]			= { -1, -1 };
static int old_energy[2]		= { -1, -1 };
static int old_shields[2]		= { -1, -1 };
static int old_flags[2]			= { -1, -1 };
static int old_weapon[2][2]		= {{ -1, -1 },{-1,-1}};
static int old_ammo_count[2][2]		= {{ -1, -1 },{-1,-1}};
static int old_cloak[2]			= { 0, 0 };
static int old_lives[2]			= { -1, -1 };
static int old_prox			= -1;

static int invulnerable_frame = 0;
static int cloak_fade_state;		//0=steady, -1 fading out, 1 fading in 

#define WS_SET				0 //in correct state
#define WS_FADING_OUT			1
#define WS_FADING_IN			2

int weapon_box_states[2];
fix weapon_box_fade_values[2];

#define FADE_SCALE	(2*i2f(GR_FADE_LEVELS)/REARM_TIME)		// fade out and back in REARM_TIME, in fade levels per seconds (int)

#ifdef OGL
#define COCKPITSCALE_X	((double)grd_curscreen->sc_w/320)
#define COCKPITSCALE_Y	((double)grd_curscreen->sc_h/200)
#define HUD_SCALE(v,s)	((int) ((double) (v) * (s) + 0.5))
#define HUD_SCALE_X(v)	HUD_SCALE(v,COCKPITSCALE_X)
#define HUD_SCALE_Y(v)	HUD_SCALE(v,COCKPITSCALE_Y)
#else
#define COCKPITSCALE_X 1
#define COCKPITSCALE_Y 1
#endif

inline void hud_bitblt (int x, int y, grs_bitmap *bm, int scale, int orient)
{
#ifdef OGL
ogl_ubitmapm_cs (
	(x < 0) ? -x : HUD_SCALE_X (x), 
	(y < 0) ? -y : HUD_SCALE_Y (y), 
	HUD_SCALE_X (bm->bm_w), 
	HUD_SCALE_Y (bm->bm_h), 
	bm, 
	-1,
	scale,
	orient
	);
#else
	gr_ubitmapm(x, y, bm);
#endif
}

typedef struct span {
	byte l,r;
} span;

//store delta x values from left of box
span weapon_window_left[] = {		//first span 67,154
		{4,53},
		{4,53},
		{4,53},
		{4,53},
		{4,53},
		{3,53},
		{3,53},
		{3,53},
		{3,53},
		{3,53},
		{3,53},
		{3,53},
		{3,53},
		{2,53},
		{2,53},
		{2,53},
		{2,53},
		{2,53},
		{2,53},
		{2,53},
		{2,53},
		{1,53},
		{1,53},
		{1,53},
		{1,53},
		{1,53},
		{1,53},
		{1,53},
		{1,53},
		{0,53},
		{0,53},
		{0,53},
		{0,53},
		{0,52},
		{1,52},
		{2,51},
		{3,51},
		{4,50},
		{5,50},
	};


//store delta x values from left of box
span weapon_window_right[] = {		//first span 207,154
		{208-202,255-202},
		{206-202,257-202},
		{205-202,258-202},
		{204-202,259-202},
		{203-202,260-202},
		{203-202,260-202},
		{203-202,260-202},
		{203-202,260-202},
		{203-202,260-202},
		{203-202,261-202},
		{203-202,261-202},
		{203-202,261-202},
		{203-202,261-202},
		{203-202,261-202},
		{203-202,261-202},
		{203-202,261-202},
		{203-202,261-202},
		{203-202,261-202},
		{203-202,262-202},
		{203-202,262-202},
		{203-202,262-202},
		{203-202,262-202},
		{203-202,262-202},
		{203-202,262-202},
		{203-202,262-202},
		{203-202,262-202},
		{204-202,263-202},
		{204-202,263-202},
		{204-202,263-202},
		{204-202,263-202},
		{204-202,263-202},
		{204-202,263-202},
		{204-202,263-202},
		{204-202,263-202},
		{204-202,263-202},
		{204-202,263-202},
		{204-202,263-202},
		{204-202,263-202},
		{205-202,263-202},
		{206-202,262-202},
		{207-202,261-202},
		{208-202,260-202},
		{211-202,255-202},
	};


#define N_LEFT_WINDOW_SPANS		(sizeof(weapon_window_left)/sizeof(*weapon_window_left))
#define N_RIGHT_WINDOW_SPANS		(sizeof(weapon_window_right)/sizeof(*weapon_window_right))
#define PRIMARY_W_BOX_LEFT		63
#define PRIMARY_W_BOX_TOP		151
#define PRIMARY_W_BOX_RIGHT		(PRIMARY_W_BOX_LEFT+58)
#define PRIMARY_W_BOX_BOT		(PRIMARY_W_BOX_TOP+N_LEFT_WINDOW_SPANS+3)
#define SECONDARY_W_BOX_LEFT		202
#define SECONDARY_W_BOX_TOP		151
#define SECONDARY_W_BOX_RIGHT		264
#define SECONDARY_W_BOX_BOT		(SECONDARY_W_BOX_TOP+N_RIGHT_WINDOW_SPANS-1)
#define SB_PRIMARY_W_BOX_LEFT		34
#define SB_PRIMARY_W_BOX_TOP		154
#define SB_PRIMARY_W_BOX_RIGHT		(SB_PRIMARY_W_BOX_LEFT+53)
#define SB_PRIMARY_W_BOX_BOT		(195)
#define SB_SECONDARY_W_BOX_LEFT		169
#define SB_SECONDARY_W_BOX_TOP		154
#define SB_SECONDARY_W_BOX_RIGHT	(SB_SECONDARY_W_BOX_LEFT+54)
#define SB_SECONDARY_W_BOX_BOT		196
#define SB_PRIMARY_W_PIC_X		(SB_PRIMARY_W_BOX_LEFT+1)
#define SB_PRIMARY_W_PIC_Y		154
#define SB_PRIMARY_W_TEXT_X		COCKPITSCALE_X*(SB_PRIMARY_W_BOX_LEFT+24)
#define SB_PRIMARY_W_TEXT_Y		COCKPITSCALE_Y*157
#define SB_PRIMARY_AMMO_X		COCKPITSCALE_X*((SB_PRIMARY_W_BOX_LEFT+33)-3)
#define SB_PRIMARY_AMMO_Y		COCKPITSCALE_Y*171
#define SB_SECONDARY_W_PIC_X		(SB_SECONDARY_W_BOX_LEFT+29)
#define SB_SECONDARY_W_PIC_Y		154
#define SB_SECONDARY_W_TEXT_X		COCKPITSCALE_X*(SB_SECONDARY_W_BOX_LEFT+2)
#define SB_SECONDARY_W_TEXT_Y		COCKPITSCALE_Y*157
#define SB_SECONDARY_AMMO_X		COCKPITSCALE_X*(SB_SECONDARY_W_BOX_LEFT+11)
#define SB_SECONDARY_AMMO_Y		COCKPITSCALE_Y*171

typedef struct gauge_box {
	int left,top;
	int right,bot;		//maximal box
	span *spanlist;		//list of left,right spans for copy
} gauge_box;

//first two are primary & secondary
//seconds two are the same for the status bar
gauge_box gauge_boxes[] = {
		{PRIMARY_W_BOX_LEFT,PRIMARY_W_BOX_TOP,PRIMARY_W_BOX_RIGHT,PRIMARY_W_BOX_BOT,weapon_window_left},
		{SECONDARY_W_BOX_LEFT,SECONDARY_W_BOX_TOP,SECONDARY_W_BOX_RIGHT,SECONDARY_W_BOX_BOT,weapon_window_right},

		{SB_PRIMARY_W_BOX_LEFT,SB_PRIMARY_W_BOX_TOP,SB_PRIMARY_W_BOX_RIGHT,SB_PRIMARY_W_BOX_BOT,NULL},
		{SB_SECONDARY_W_BOX_LEFT,SB_SECONDARY_W_BOX_TOP,SB_SECONDARY_W_BOX_RIGHT,SB_SECONDARY_W_BOX_BOT,NULL}
	};


int Color_0_31_0 = -1;

//copy a box from the off-screen buffer to the visible page
void copy_gauge_box(gauge_box *box,grs_bitmap *bm)
{
	if (box->spanlist) {
		int n_spans = box->bot-box->top+1;
		int cnt,y;

		for (cnt=0,y=box->top;cnt<n_spans;cnt++,y++)
			gr_bm_ubitblt(box->spanlist[cnt].r-box->spanlist[cnt].l+1,1,
						box->left+box->spanlist[cnt].l,y,box->left+box->spanlist[cnt].l,y,bm,&grd_curcanv->cv_bitmap);
	}
	else
	{
		gr_bm_ubitblt(box->right-box->left+1,box->bot-box->top+1,
						box->left,box->top,box->left,box->top,
						bm,&grd_curcanv->cv_bitmap);
	}
}

//fills in the coords of the hostage video window
void get_hostage_window_coords(int *x,int *y,int *w,int *h)
{
	if (Cockpit_mode == CM_STATUS_BAR) {
		*x = SB_SECONDARY_W_BOX_LEFT;
		*y = SB_SECONDARY_W_BOX_TOP;
		*w = SB_SECONDARY_W_BOX_RIGHT - SB_SECONDARY_W_BOX_LEFT + 1;
		*h = SB_SECONDARY_W_BOX_BOT - SB_SECONDARY_W_BOX_TOP + 1;
	}
	else {
		*x = SECONDARY_W_BOX_LEFT;
		*y = SECONDARY_W_BOX_TOP;
		*w = SECONDARY_W_BOX_RIGHT - SECONDARY_W_BOX_LEFT + 1;
		*h = SECONDARY_W_BOX_BOT - SECONDARY_W_BOX_TOP + 1;
	}

}

//these should be in gr.h 
#define cv_w  cv_bitmap.bm_w
#define cv_h  cv_bitmap.bm_h

extern int HUD_nmessages, hud_first;
extern char HUD_messages[HUD_MAX_NUM][HUD_MESSAGE_LENGTH+5]; 

void hud_show_score()
{
	char	score_str[20];
	int	w, h, aw;

	if ((HUD_nmessages > 0) && (strlen(HUD_messages[hud_first]) > 50))
		return;

	gr_set_curfont( GAME_FONT );

	if ( ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP)) ) {
		sprintf(score_str, "%s: %5d", TXT_KILLS, Players[Player_num].net_kills_total);
	} else {
		sprintf(score_str, "%s: %5d", TXT_SCORE, Players[Player_num].score);
	}
	gr_get_string_size(score_str, &w, &h, &aw );

	if (Color_0_31_0 == -1)
		Color_0_31_0 = gr_getcolor(0,31,0);
	gr_set_fontcolor(Color_0_31_0, -1);

	gr_printf(grd_curcanv->cv_w-w-2, 3, score_str);
}

void hud_show_score_added()
{
	int	color;
	int	w, h, aw;
	char	score_str[20];

	if ( (Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) ) 
		return;

	if (score_display == 0)
		return;

	gr_set_curfont( GAME_FONT );

	score_time -= FrameTime;
	if (score_time > 0) {
		color = f2i(score_time * 20) + 10;

		if (color < 10) color = 10;
		if (color > 31) color = 31;

		if (Cheats_enabled)
			sprintf(score_str, "%s", TXT_CHEATER);
		else
			sprintf(score_str, "%5d", score_display);

		gr_get_string_size(score_str, &w, &h, &aw );
		gr_set_fontcolor(gr_getcolor(0, color, 0),-1 );
		gr_printf(grd_curcanv->cv_w-w-FONTSCALE_X(12), FONTSCALE_Y(GAME_FONT->ft_h+5), score_str);
	} else {
		score_time = 0;
		score_display = 0;
	}
	
}

void sb_show_score()
{
	char	score_str[20];
	int x,y;
	int	w, h, aw;
	static int last_x[2]={SB_SCORE_RIGHT,SB_SCORE_RIGHT};
	int redraw_score;

	if ( (Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) ) 
		redraw_score = -99;
	else
		redraw_score = -1;

	gr_set_curfont( GAME_FONT );
	gr_set_fontcolor(gr_getcolor(0,20,0),-1 );

	if ( (Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) ) 
		gr_printf(COCKPITSCALE_X*SB_SCORE_LABEL_X,COCKPITSCALE_Y*SB_SCORE_Y,"%s:", TXT_KILLS);
	else
		gr_printf(COCKPITSCALE_X*SB_SCORE_LABEL_X,COCKPITSCALE_Y*SB_SCORE_Y,"%s:", TXT_SCORE);

	gr_set_curfont( GAME_FONT );
	if ( (Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) ) 
		sprintf(score_str, "%5d", Players[Player_num].net_kills_total);
	else
		sprintf(score_str, "%5d", Players[Player_num].score);
	gr_get_string_size(score_str, &w, &h, &aw );

	x = COCKPITSCALE_X*SB_SCORE_RIGHT-w-2;
	y = COCKPITSCALE_Y*SB_SCORE_Y;

	//erase old score
	gr_setcolor(BM_XRGB(0,0,0));

	if ( (Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) ) 
		gr_set_fontcolor(gr_getcolor(0,20,0),-1 );
	else
		gr_set_fontcolor(gr_getcolor(0,31,0),-1 );	

	gr_printf(x,y,score_str);

	last_x[VR_current_page] = x;
}

void sb_show_score_added()
{
	int	color;
	int w, h, aw;
	char	score_str[32];
	int x;
	static int last_x[2]={SB_SCORE_ADDED_RIGHT,SB_SCORE_ADDED_RIGHT};
	static	int last_score_display[2] = { -1, -1};

	if ( (Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) ) 
		return;

	if (score_display == 0)
		return;

	gr_set_curfont( GAME_FONT );

	score_time -= FrameTime;
	if (score_time > 0) {
		if (score_display != last_score_display[VR_current_page]) {
			gr_setcolor(BM_XRGB(0,0,0));
			last_score_display[VR_current_page] = score_display;
		}

		color = f2i(score_time * 20) + 10;

		if (color < 10) color = 10;
		if (color > 31) color = 31;

		if (Cheats_enabled)
			sprintf(score_str, "%s", TXT_CHEATER);
		else
			sprintf(score_str, "%5d", score_display);

		gr_get_string_size(score_str, &w, &h, &aw );

		x = COCKPITSCALE_X*SB_SCORE_ADDED_RIGHT-w-2;

		gr_set_fontcolor(gr_getcolor(0, color, 0),-1 );
		gr_printf(x, COCKPITSCALE_Y*SB_SCORE_ADDED_Y, score_str);

		last_x[VR_current_page] = x;

	} else {
		//erase old score
		gr_setcolor(BM_XRGB(0,0,0));
		score_time = 0;
		score_display = 0;
	}
}

fix Last_warning_beep_time[2] = {0,0}; // Time we last played homing missile warning beep.

//	-----------------------------------------------------------------------------
void play_homing_warning(void)
{
	fix	beep_delay;

	if (Endlevel_sequence || Player_is_dead)
		return;

	if (Players[Player_num].homing_object_dist >= 0) {
		beep_delay = Players[Player_num].homing_object_dist/128;
		if (beep_delay > F1_0)
			beep_delay = F1_0;
		else if (beep_delay < F1_0/8)
			beep_delay = F1_0/8;

		if (GameTime - Last_warning_beep_time[VR_current_page] > beep_delay/2) {
			digi_play_sample( SOUND_HOMING_WARNING, F1_0 );
			Last_warning_beep_time[VR_current_page] = GameTime;
		}
	}
}

int Last_homing_warning_shown[2]={-1,-1};

//	-----------------------------------------------------------------------------
void show_homing_warning(void)
{
	if ((Cockpit_mode == CM_STATUS_BAR) || (Endlevel_sequence)) {
		if (Last_homing_warning_shown[VR_current_page] == 1) {
			PIGGY_PAGE_IN( Gauges[GAUGE_HOMING_WARNING_OFF] );
			hud_bitblt( 7, 171, &GameBitmaps[Gauges[GAUGE_HOMING_WARNING_OFF].index], F1_0, 0);
			Last_homing_warning_shown[VR_current_page] = 0;
		}
		return;
	}

	gr_set_current_canvas( get_current_game_screen() );

	if (Players[Player_num].homing_object_dist >= 0) {

		if (GameTime & 0x4000) {
			if (Last_homing_warning_shown[VR_current_page] != 1) {
				PIGGY_PAGE_IN(Gauges[GAUGE_HOMING_WARNING_ON]);
				hud_bitblt( 7, 171, &GameBitmaps[Gauges[GAUGE_HOMING_WARNING_ON].index], F1_0, 0);
				Last_homing_warning_shown[VR_current_page] = 1;
			}
		} else {
			if (Last_homing_warning_shown[VR_current_page] != 0) {
				PIGGY_PAGE_IN(Gauges[GAUGE_HOMING_WARNING_OFF]);
				hud_bitblt( 7, 171, &GameBitmaps[Gauges[GAUGE_HOMING_WARNING_OFF].index], F1_0, 0);
				Last_homing_warning_shown[VR_current_page] = 0;
			}
		}
	} else if (Last_homing_warning_shown[VR_current_page] != 0) {
		PIGGY_PAGE_IN(Gauges[GAUGE_HOMING_WARNING_OFF]);
		hud_bitblt( 7, 171, &GameBitmaps[Gauges[GAUGE_HOMING_WARNING_OFF].index], F1_0, 0);
		Last_homing_warning_shown[VR_current_page] = 0;
	}

}

#define MAX_SHOWN_LIVES 4

void hud_show_homing_warning(void)
{
	if (Players[Player_num].homing_object_dist >= 0) {

		if (GameTime & 0x4000) {
			gr_set_curfont( GAME_FONT );
			gr_set_fontcolor(gr_getcolor(0,31,0),-1 );
			gr_printf(0x8000, grd_curcanv->cv_h-FONTSCALE_Y(GAME_FONT->ft_h+3),TXT_LOCK);
		}
	}
}

void hud_show_keys(void)
{
#ifndef SHAREWARE
	if (Players[Player_num].flags & PLAYER_FLAGS_BLUE_KEY) {
		PIGGY_PAGE_IN(Gauges[KEY_ICON_BLUE]);
		hud_bitblt(2,17,&GameBitmaps[Gauges[KEY_ICON_BLUE].index], F1_0, 0);
	}

	if (Players[Player_num].flags & PLAYER_FLAGS_GOLD_KEY) {
		PIGGY_PAGE_IN(Gauges[KEY_ICON_YELLOW]);
		hud_bitblt(10,17,&GameBitmaps[Gauges[KEY_ICON_YELLOW].index], F1_0, 0);
	}

	if (Players[Player_num].flags & PLAYER_FLAGS_RED_KEY) {
		PIGGY_PAGE_IN(Gauges[KEY_ICON_RED]);
		hud_bitblt(18,17,&GameBitmaps[Gauges[KEY_ICON_RED].index], F1_0, 0);

	}
#endif
}

void hud_show_energy(void)
{
	if (Gauge_hud_mode<2){
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(gr_getcolor(0,31,0),-1 );
		if (Game_mode & GM_MULTI)
		     gr_printf(2, (grd_curcanv->cv_h-FONTSCALE_Y(GAME_FONT->ft_h*5+3*5)),"%s: %i", TXT_ENERGY, f2ir(Players[Player_num].energy));
		else
		     gr_printf(2, (grd_curcanv->cv_h-FONTSCALE_Y(GAME_FONT->ft_h+3)),"%s: %i", TXT_ENERGY, f2ir(Players[Player_num].energy));
	}

	if (Newdemo_state==ND_STATE_RECORDING ) {
		int energy = f2ir(Players[Player_num].energy);

		if (energy != old_energy[VR_current_page]) {
#ifdef SHAREWARE
			newdemo_record_player_energy(energy);
#else
			newdemo_record_player_energy(old_energy[VR_current_page], energy);
#endif
			old_energy[VR_current_page] = energy;
	 	}
	}
}

void hud_show_weapons_mode1(int type,int vertical,int clear,int x,int y){
	int i,w,h,aw;
	char weapon_str[10];
	if (vertical){
		y=y+FONTSCALE_Y(GAME_FONT->ft_h*4);
		if (type==0 && clear)
		     x=x-3;//quick hack to prevent 10000 vulcan from going into the hud
	}
	if (type==0){
		for (i=4;i>=0;i--){
			if (Primary_weapon==i)
                             gr_set_fontcolor(gr_getcolor(20,0,0),-1);
			else{
				if (player_has_weapon(i,0) & HAS_WEAPON_FLAG)
				     gr_set_fontcolor(gr_getcolor(0,15,0),-1);
				else
				     gr_set_fontcolor(gr_getcolor(3,3,3),-1);
			}
			switch(i){
			  case 0:
				sprintf(weapon_str,"%c%i",
					  (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)?'Q':'L',
					  Players[Player_num].laser_level+1);
				break;
			  case 1:
				sprintf(weapon_str,"V%i",
					  f2i(Players[Player_num].primary_ammo[1] * VULCAN_AMMO_SCALE));
				break;
			  case 2:
				sprintf(weapon_str,"S");break;
			  case 3:
				sprintf(weapon_str,"P");break;
			  case 4:
				sprintf(weapon_str,"F");break;
				
			}
			gr_get_string_size(weapon_str, &w, &h, &aw );
			if (vertical){
				y-=h+FONTSCALE_X(2);
			}else
			     x-=w+FONTSCALE_Y(3);
			gr_printf(x, y, weapon_str);
		}
	}else{
		for (i=4;i>=0;i--){
			if (Secondary_weapon==i)
                             gr_set_fontcolor(gr_getcolor(20,0,0),-1);
			else{
				if (Players[Player_num].secondary_ammo[i]>0)
				     gr_set_fontcolor(gr_getcolor(0,15,0),-1);
				else
				     gr_set_fontcolor(gr_getcolor(0,6,0),-1);
			}
			sprintf(weapon_str,"%i",Players[Player_num].secondary_ammo[i]);
			gr_get_string_size(weapon_str, &w, &h, &aw );
			if (vertical){
				y-=h+FONTSCALE_X(2);
			}else
			     x-=w+FONTSCALE_Y(3);
			if (clear){
				gr_setcolor(BM_XRGB(0,0,0));
				gr_rect(x-2,y-4,x+8,y+4);
			}

			gr_printf(x, y, weapon_str);
		}
	}
	gr_set_fontcolor(gr_getcolor(0,31,0),-1 );
}

void hud_show_weapons(void)
{
	int	y;

	gr_set_curfont( GAME_FONT );
	gr_set_fontcolor(gr_getcolor(0,31,0),-1 );

	if (Game_mode & GM_MULTI)
		y = grd_curcanv->cv_h-FONTSCALE_Y(GAME_FONT->ft_h*4+3*4);
	else
		y = grd_curcanv->cv_h;

	if (Gauge_hud_mode==1){
		hud_show_weapons_mode1(0,0,0,grd_curcanv->cv_w,y-FONTSCALE_Y(GAME_FONT->ft_h*2+3*2));
		hud_show_weapons_mode1(1,0,0,grd_curcanv->cv_w,y-FONTSCALE_Y(GAME_FONT->ft_h+3));
	}

	else if (Gauge_hud_mode==2 || Gauge_hud_mode==3){
		int x1,x2;
		int	w, aw;
		gr_get_string_size("V1000", &w, &x1, &aw );
		gr_get_string_size("0 ", &x2, &x1, &aw);
		if (Gauge_hud_mode==2){
			y=grd_curcanv->cv_h-(grd_curcanv->cv_h/4);
			x1=grd_curcanv->cv_w/2-(w);
			x2=grd_curcanv->cv_w/2+x2;//originally /2+10
		}else{
			y=grd_curcanv->cv_h/1.75;
			x1=grd_curcanv->cv_w/2.1-(FONTSCALE_X(40)+w);
			x2=grd_curcanv->cv_w/1.9+(FONTSCALE_X(42)+x2);
		}
		hud_show_weapons_mode1(0,1,0,x1,y);
		hud_show_weapons_mode1(1,1,0,x2,y);
		gr_set_fontcolor(gr_getcolor(14,14,23),-1 );
		gr_printf(x2, y-(FONTSCALE_Y(GAME_FONT->ft_h*4+4)),"%i", f2ir(Players[Player_num].shields));
		gr_set_fontcolor(gr_getcolor(25,18,6),-1 );
		gr_printf(x1, y-(FONTSCALE_Y(GAME_FONT->ft_h*4+4)),"%i", f2ir(Players[Player_num].energy));
	}

	else{
		char    weapon_str[32], temp_str[20];
		int	w, h, aw;

		y -= FONTSCALE_Y(GAME_FONT->ft_h+3);
	
		switch (Primary_weapon) {
			case 0:
				if (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)
					sprintf(weapon_str, "%s %s %i", TXT_QUAD, TXT_LASER, Players[Player_num].laser_level+1);
				else
					sprintf(weapon_str, "%s %i", TXT_LASER, Players[Player_num].laser_level+1);
				break;
			case 1:
				sprintf(weapon_str, "%s: %i", TXT_W_VULCAN_S, f2i(Players[Player_num].primary_ammo[Primary_weapon] * VULCAN_AMMO_SCALE));
				break;
			case 2:
				strcpy(weapon_str, TXT_W_SPREADFIRE_S);
				break;
#ifndef SHAREWARE
			case 3:
				strcpy(weapon_str, TXT_W_PLASMA_S);
				break;
			case 4:
				strcpy(weapon_str, TXT_W_FUSION_S);
				break;
#endif
		}
		
		gr_get_string_size(weapon_str, &w, &h, &aw );
		gr_printf(grd_curcanv->cv_w-5-w, (y-FONTSCALE_Y(GAME_FONT->ft_h+3)), weapon_str);//originally y-8

		switch (Secondary_weapon) {
			case 0:		strcpy(weapon_str, TXT_CONCUSSION);	break;
			case 1:		strcpy(weapon_str, TXT_HOMING);		break;
			case 2:		strcpy(weapon_str, TXT_PROXBOMB   );	break;
#ifndef SHAREWARE
			case 3:		strcpy(weapon_str, TXT_SMART);		break;
			case 4:		strcpy(weapon_str, TXT_MEGA);		break;
#endif
			default:	Int3();	weapon_str[0] = 0;		break;
		}

		// Linux doesn't have an itoa() function, for reasons i can't imagine
		// so we use sprintf() instead...
		sprintf(temp_str," %u",Players[Player_num].secondary_ammo[Secondary_weapon]);
		strcat(weapon_str, temp_str);
		gr_get_string_size(weapon_str, &w, &h, &aw );
		gr_printf(grd_curcanv->cv_w-5-w, y, weapon_str);
		sprintf(weapon_str, "PROX %u",Players[Player_num].secondary_ammo[2]);
		gr_get_string_size(weapon_str, &w,&h, &aw );
		gr_printf(grd_curcanv->cv_w-5-w, (y-FONTSCALE_Y(GAME_FONT->ft_h*2+3*2)), weapon_str);//originally y-16
	}

#ifndef SHAREWARE
	if (Primary_weapon == VULCAN_INDEX) {
		if (Players[Player_num].primary_ammo[Primary_weapon] != old_ammo_count[0][VR_current_page]) {
			if (Newdemo_state == ND_STATE_RECORDING)
			     newdemo_record_primary_ammo(old_ammo_count[0][VR_current_page], Players[Player_num].primary_ammo[Primary_weapon]);
			old_ammo_count[0][VR_current_page] = Players[Player_num].primary_ammo[Primary_weapon];
		}
	}
	if (Players[Player_num].secondary_ammo[Secondary_weapon] != old_ammo_count[1][VR_current_page]) {
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_secondary_ammo(old_ammo_count[1][VR_current_page], Players[Player_num].secondary_ammo[Secondary_weapon]);
		old_ammo_count[1][VR_current_page] = Players[Player_num].secondary_ammo[Secondary_weapon];
	}
#endif
}

void hud_show_cloak_invuln(void)
{
	gr_set_fontcolor(gr_getcolor(0,31,0),-1 );

	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
		int	y = grd_curcanv->cv_h;

		if (Game_mode & GM_MULTI)
			y -= FONTSCALE_Y(GAME_FONT->ft_h*9+3*9);
		else
			y -= FONTSCALE_Y(GAME_FONT->ft_h*4+3*4);

		if ((Players[Player_num].cloak_time+CLOAK_TIME_MAX - GameTime > F1_0*3 ) || (GameTime & 0x8000))
			gr_printf(2, y, "%s", TXT_CLOAKED);
	}

	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) {
		int	y = grd_curcanv->cv_h;

		if (Game_mode & GM_MULTI)
			y -= FONTSCALE_Y(GAME_FONT->ft_h*10+3*10);
		else
			y -= FONTSCALE_Y(GAME_FONT->ft_h*5+3*5);

		if (((Players[Player_num].invulnerable_time + INVULNERABLE_TIME_MAX - GameTime) > F1_0*4) || (GameTime & 0x8000))
			gr_printf(2, y, "%s", TXT_INVULNERABLE);
	}

}

void hud_show_shield(void)
{
	if (Gauge_hud_mode<2){
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(gr_getcolor(0,31,0),-1 );
		if ( Players[Player_num].shields >= 0 )	{
			if (Game_mode & GM_MULTI)
			     gr_printf(2, (grd_curcanv->cv_h-FONTSCALE_Y(GAME_FONT->ft_h*6+3*6)),"%s: %i", TXT_SHIELD, f2ir(Players[Player_num].shields));
			else
			     gr_printf(2, (grd_curcanv->cv_h-FONTSCALE_Y(GAME_FONT->ft_h*2+3*2)),"%s: %i", TXT_SHIELD, f2ir(Players[Player_num].shields));
		} else {
			if (Game_mode & GM_MULTI)
			     gr_printf(2, (grd_curcanv->cv_h-FONTSCALE_Y(GAME_FONT->ft_h*6+3*6)),"%s: 0", TXT_SHIELD );
			else
			     gr_printf(2, (grd_curcanv->cv_h-FONTSCALE_Y(GAME_FONT->ft_h*2+3*2)),"%s: 0", TXT_SHIELD );
		}
	}

	if (Newdemo_state==ND_STATE_RECORDING ) {
		int shields = f2ir(Players[Player_num].shields);

		if (shields != old_shields[VR_current_page]) {
#ifdef SHAREWARE
			newdemo_record_player_shields(shields);
#else
			newdemo_record_player_shields(old_shields[VR_current_page], shields);
#endif
			old_shields[VR_current_page] = shields;
		}
	}
}

//draw the icons for number of lives
void hud_show_lives()
{
	if ((HUD_nmessages > 0) && (strlen(HUD_messages[hud_first]) > 50))
		return;

	if (Game_mode & GM_MULTI) {
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(gr_getcolor(0,31,0),-1 );
		gr_printf(FONTSCALE_X(10), 3, "%s: %d", TXT_DEATHS, Players[Player_num].net_killed_total);
	} 
	else if (Players[Player_num].lives > 1)  {
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(gr_getcolor(0,20,0),-1 );
		PIGGY_PAGE_IN(Gauges[GAUGE_LIVES]);
#ifndef OGL
		gr_ubitmapm(10,3,&GameBitmaps[Gauges[GAUGE_LIVES].index]);
		gr_printf(22, 3, "x %d", Players[Player_num].lives-1);
#else
		ogl_ubitmapm_cf(FONTSCALE_X(10),3,FONTSCALE_X((hiresfont && SWIDTH >= 640)?16:8),FONTSCALE_Y((hiresfont && SWIDTH >= 640)?14:7),&GameBitmaps[Gauges[GAUGE_LIVES].index],255,F1_0);
		gr_printf(FONTSCALE_X((hiresfont && SWIDTH >= 640)?35:22), 3, "x %d", Players[Player_num].lives-1);
#endif
	}
}

void sb_show_lives()
{
	int x,y;
	grs_bitmap * bm = &GameBitmaps[Gauges[GAUGE_LIVES].index];
	x = SB_LIVES_X;
	y = SB_LIVES_Y;

	gr_set_curfont( GAME_FONT );
	gr_set_fontcolor(gr_getcolor(0,20,0),-1 );
	if (Game_mode & GM_MULTI)
		gr_printf(COCKPITSCALE_X*SB_LIVES_LABEL_X,COCKPITSCALE_Y*SB_LIVES_LABEL_Y,"%s:", TXT_DEATHS);
	else
		gr_printf(COCKPITSCALE_X*SB_LIVES_LABEL_X,COCKPITSCALE_Y*SB_LIVES_LABEL_Y,"%s:", TXT_LIVES);

	if (Game_mode & GM_MULTI)
	{
		char killed_str[20];
		int w, h, aw;
		static int last_x[2] = {SB_SCORE_RIGHT,SB_SCORE_RIGHT};
		int x;

		sprintf(killed_str, "%5d", Players[Player_num].net_killed_total);
		gr_get_string_size(killed_str, &w, &h, &aw);
		gr_setcolor(BM_XRGB(0,0,0));
#ifndef OGL
		gr_rect(last_x[VR_current_page], y+1, SB_SCORE_RIGHT, y+GAME_FONT->ft_h);
#endif
		gr_set_fontcolor(gr_getcolor(0,20,0),-1);
		x = COCKPITSCALE_X*SB_SCORE_RIGHT-w-2;		
		gr_printf(x, COCKPITSCALE_Y*y+1, killed_str);
		last_x[VR_current_page] = x;
		return;
	}

	//erase old icons
	gr_setcolor(BM_XRGB(0,0,0));
	gr_rect(COCKPITSCALE_X*x, COCKPITSCALE_Y*y, COCKPITSCALE_X*x+32, (COCKPITSCALE_Y*y)+bm->bm_h);

	if (Players[Player_num].lives-1 > 0) {
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(gr_getcolor(0,20,0),-1 );
		PIGGY_PAGE_IN(Gauges[GAUGE_LIVES]);
#ifndef OGL
		gr_ubitmapm(x,y,&GameBitmaps[Gauges[GAUGE_LIVES].index]);
#else
		ogl_ubitmapm_cf(x*COCKPITSCALE_X,y*COCKPITSCALE_Y,FONTSCALE_X((hiresfont && SWIDTH >= 640)?16:8),FONTSCALE_Y((hiresfont && SWIDTH >= 640)?14:7),bm,255,F1_0);
#endif
		gr_printf(COCKPITSCALE_X*x+COCKPITSCALE_X*12, COCKPITSCALE_Y*y, "x %d", Players[Player_num].lives-1);
	}
}

#ifndef RELEASE

#ifdef PIGGY_USE_PAGING
extern int Piggy_bitmap_cache_next;
#endif

void show_time()
{
	int secs = f2i(Players[Player_num].time_level) % 60;
	int mins = f2i(Players[Player_num].time_level) / 60;

	gr_set_curfont( GAME_FONT );
	if (Color_0_31_0 == -1)
		Color_0_31_0 = gr_getcolor(0,31,0);
	gr_set_fontcolor(Color_0_31_0, -1 );
	gr_printf(grd_curcanv->cv_w-25,grd_curcanv->cv_h-28,"%d:%02d", mins, secs);

#ifdef PIGGY_USE_PAGING
	char text[25];
	int w,h,aw;
	sprintf( text, "%d KB", Piggy_bitmap_cache_next/1024 );
	gr_get_string_size( text, &w, &h, &aw );	
	gr_printf(grd_curcanv->cv_w-10-w,grd_curcanv->cv_h/2, text );
#endif

}
#endif

#define EXTRA_SHIP_SCORE	50000		//get new ship every this many points

void add_points_to_score(int points) 
{
	int prev_score;

	score_time += f1_0*2;
	score_display += points;
	if (score_time > f1_0*4) score_time = f1_0*4;

	if (points == 0 || Cheats_enabled)
		return;

	if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
		return;

	prev_score=Players[Player_num].score;

	Players[Player_num].score += points;

#ifndef SHAREWARE
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_player_score(points);
#endif

#ifndef SHAREWARE
#ifdef NETWORK
	if (Game_mode & GM_MULTI_COOP)
		multi_send_score();
#endif
#endif

	if (Game_mode & GM_MULTI)
		return;

	if (Players[Player_num].score/EXTRA_SHIP_SCORE != prev_score/EXTRA_SHIP_SCORE) {
		int snd;
		Players[Player_num].lives += Players[Player_num].score/EXTRA_SHIP_SCORE - prev_score/EXTRA_SHIP_SCORE;
		powerup_basic(20, 20, 20, 0, TXT_EXTRA_LIFE);
		if ((snd=Powerup_info[POW_EXTRA_LIFE].hit_sound) > -1 )
			digi_play_sample( snd, F1_0 );
	}
}

void add_bonus_points_to_score(int points) 
{
	int prev_score;

	if (points == 0 || Cheats_enabled)
		return;

	prev_score=Players[Player_num].score;

	Players[Player_num].score += points;


#ifndef SHAREWARE
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_player_score(points);
#endif

	if (Game_mode & GM_MULTI)
		return;

	if (Players[Player_num].score/EXTRA_SHIP_SCORE != prev_score/EXTRA_SHIP_SCORE) {
		int snd;
		Players[Player_num].lives += Players[Player_num].score/EXTRA_SHIP_SCORE - prev_score/EXTRA_SHIP_SCORE;
		if ((snd=Powerup_info[POW_EXTRA_LIFE].hit_sound) > -1 )
			digi_play_sample( snd, F1_0 );
	}
}

void init_gauge_canvases()
{
	Canv_LeftEnergyGauge = gr_create_sub_canvas( grd_curcanv, LEFT_ENERGY_GAUGE_X, LEFT_ENERGY_GAUGE_Y, LEFT_ENERGY_GAUGE_W, LEFT_ENERGY_GAUGE_H );
	Canv_SBEnergyGauge = gr_create_sub_canvas( grd_curcanv, SB_ENERGY_GAUGE_X, SB_ENERGY_GAUGE_Y, SB_ENERGY_GAUGE_W, SB_ENERGY_GAUGE_H );
	Canv_RightEnergyGauge = gr_create_sub_canvas( grd_curcanv, RIGHT_ENERGY_GAUGE_X, RIGHT_ENERGY_GAUGE_Y, RIGHT_ENERGY_GAUGE_W, RIGHT_ENERGY_GAUGE_H );
	Canv_NumericalGauge = gr_create_sub_canvas( grd_curcanv, NUMERICAL_GAUGE_X, NUMERICAL_GAUGE_Y, NUMERICAL_GAUGE_W, NUMERICAL_GAUGE_H );
}

void close_gauge_canvases()
{
	gr_free_sub_canvas( Canv_LeftEnergyGauge );
	gr_free_sub_canvas( Canv_SBEnergyGauge );
	gr_free_sub_canvas( Canv_RightEnergyGauge );
	gr_free_sub_canvas( Canv_NumericalGauge );
}

void init_gauges()
{
	int i;

	for (i=0; i<2; i++ )	{
		if ( ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP)) || ((Newdemo_state == ND_STATE_PLAYBACK) && (Newdemo_game_mode & GM_MULTI) && !(Newdemo_game_mode & GM_MULTI_COOP)) ) 
			old_score[i] = -99;
		else
			old_score[i]			= -1;
		old_energy[i]			= -1;
		old_shields[i]			= -1;
		old_flags[i]			= -1;
		old_cloak[i]			= -1;
		old_lives[i]			= -1;
	
		old_weapon[0][i] = old_weapon[1][i] = -1;
		old_ammo_count[0][i] = old_ammo_count[1][i] = -1;
	}

	cloak_fade_state = 0;
}

#ifdef OGL // ZICO - scalable
void draw_energy_bar(int energy)
{
	grs_bitmap *bm;
	int energy0;
	int x1, x2, y, yMax, i;
	int h0 = HUD_SCALE_X (LEFT_ENERGY_GAUGE_H - 1);
	int h1 = HUD_SCALE_Y (LEFT_ENERGY_GAUGE_H / 4);
	int h2 = HUD_SCALE_Y ((LEFT_ENERGY_GAUGE_H * 3) / 4);
	int w1 = HUD_SCALE_X (LEFT_ENERGY_GAUGE_W - 1);
	int w2 = HUD_SCALE_X (LEFT_ENERGY_GAUGE_W - 2);
	int w3 = HUD_SCALE_X (LEFT_ENERGY_GAUGE_W - 3);
	double eBarScale = (100.0 - (double) energy) * COCKPITSCALE_X * 0.075 / (double) HUD_SCALE_Y (LEFT_ENERGY_GAUGE_H);

	// Draw left energy bar
	PIGGY_PAGE_IN(Gauges[GAUGE_ENERGY_LEFT]);
	bm = &GameBitmaps[Gauges[GAUGE_ENERGY_LEFT].index];
	hud_bitblt (LEFT_ENERGY_GAUGE_X, LEFT_ENERGY_GAUGE_Y, bm, F1_0, 0);

	gr_setcolor(BM_XRGB(0,0,0));

	energy0 = HUD_SCALE_X (56);
	energy0 = energy0 - (energy * energy0) / 100;
	if (energy < 100) {
		for (i = 0; i < LEFT_ENERGY_GAUGE_H; i++) {
			yMax = HUD_SCALE_Y (i + 1);
			for (y = i; y <= yMax; y++) {
				x1 = h0 - y;
				x2 = x1 + energy0 + (int) ((double) y * eBarScale);
				if (y < h1) {
					if (x2 > w1) 
						x2 = w1;
					}
				else if (y < h2) {
					if (x2 > w2)
						x2 = w2;
					}
				else {
					if (x2 > w3) 
						x2 = w3;
					}
				if (x2 > x1)
					gr_rect(HUD_SCALE_X (LEFT_ENERGY_GAUGE_X) + x1, HUD_SCALE_Y (LEFT_ENERGY_GAUGE_Y) +y , HUD_SCALE_X (LEFT_ENERGY_GAUGE_X) + x2, HUD_SCALE_Y (LEFT_ENERGY_GAUGE_Y) +y +1);
				}
			}
		}
	gr_set_current_canvas( get_current_game_screen() );

	// Draw right energy bar
	PIGGY_PAGE_IN(Gauges[GAUGE_ENERGY_RIGHT]);
	bm = &GameBitmaps[Gauges[GAUGE_ENERGY_RIGHT].index];
	hud_bitblt (RIGHT_ENERGY_GAUGE_X-1, RIGHT_ENERGY_GAUGE_Y, bm, F1_0, 0);

	gr_setcolor(BM_XRGB(0,0,0));

	h0 = HUD_SCALE_X (RIGHT_ENERGY_GAUGE_W - RIGHT_ENERGY_GAUGE_H);
	w1 = HUD_SCALE_X (1);
	w2 = HUD_SCALE_X (2);
	if (energy < 100) {
		yMax = HUD_SCALE_Y (RIGHT_ENERGY_GAUGE_H);
		for (i = 0; i < RIGHT_ENERGY_GAUGE_H; i++) {
			yMax = HUD_SCALE_Y (i + 1);
			for (y = i; y <= yMax; y++) {
				x2 = h0 + y;
				x1 = x2 - energy0 - (int) ((double) y * eBarScale);
				if (y < h1) {
					if (x1 < 0) 
						x1 = 0;
					}
				else if (y < h2) {
					if (x1 < w1) 
						x1 = w1;
					}
				else {
					if (x1 < w2) 
						x1 = w2;
					}
				if (x2 > x1) 
					gr_rect(HUD_SCALE_X (RIGHT_ENERGY_GAUGE_X) + x1, HUD_SCALE_Y (RIGHT_ENERGY_GAUGE_Y) +y , HUD_SCALE_X (RIGHT_ENERGY_GAUGE_X) + x2, HUD_SCALE_Y (RIGHT_ENERGY_GAUGE_Y) +y +1);
				}
			}
		}
	gr_set_current_canvas( get_current_game_screen() );
}

#else // ZICO - SDL mode with non-scalable cockpit

void draw_energy_bar(int energy)
{
	int not_energy;
	int x1, x2, y;

	// Draw left energy bar
	PIGGY_PAGE_IN(Gauges[GAUGE_ENERGY_LEFT]);
	gr_ubitmapm( LEFT_ENERGY_GAUGE_X, LEFT_ENERGY_GAUGE_Y, &GameBitmaps[Gauges[GAUGE_ENERGY_LEFT].index] );
	gr_setcolor( 0 );

	not_energy = 61 - (energy*61)/100;

	if (energy < 100)
		for (y=0; y<8; y++) {
			x1 = 7 - y;
			x2 = 7 - y + not_energy;
	
			if ( y>=0 && y<2 ) if (x2 > LEFT_ENERGY_GAUGE_W - 1) x2 = LEFT_ENERGY_GAUGE_W - 1;
			if ( y>=2 && y<6 ) if (x2 > LEFT_ENERGY_GAUGE_W - 2) x2 = LEFT_ENERGY_GAUGE_W - 2;
			if ( y>=6 ) if (x2 > LEFT_ENERGY_GAUGE_W - 3) x2 = LEFT_ENERGY_GAUGE_W - 3;
			
			if (x2 > x1) gr_uscanline( LEFT_ENERGY_GAUGE_X+x1, LEFT_ENERGY_GAUGE_X+x2, LEFT_ENERGY_GAUGE_Y+y ); 
		}

	gr_set_current_canvas( get_current_game_screen() );

	// Draw right energy bar
	PIGGY_PAGE_IN(Gauges[GAUGE_ENERGY_RIGHT]);
	gr_ubitmapm( RIGHT_ENERGY_GAUGE_X, RIGHT_ENERGY_GAUGE_Y, &GameBitmaps[Gauges[GAUGE_ENERGY_RIGHT].index] );

	if (energy < 100)
		for (y=0; y<8; y++) {
			x1 = RIGHT_ENERGY_GAUGE_W - 8 + y - not_energy;
			x2 = RIGHT_ENERGY_GAUGE_W - 8 + y;
	
			if ( y>=0 && y<2 ) if (x1 < 0) x1 = 0;
			if ( y>=2 && y<6 ) if (x1 < 1) x1 = 1;
			if ( y>=6 ) if (x1 < 2) x1 = 2;
			
			if (x2 > x1) gr_uscanline( RIGHT_ENERGY_GAUGE_X+x1, RIGHT_ENERGY_GAUGE_X+x2, RIGHT_ENERGY_GAUGE_Y+y ); 
		}

	gr_set_current_canvas( get_current_game_screen() );

}
#endif

void draw_shield_bar(int shield)
{
	int bm_num = shield>=100?9:(shield / 10);

	PIGGY_PAGE_IN(Gauges[GAUGE_SHIELDS+9-bm_num]);
	hud_bitblt (SHIELD_GAUGE_X, SHIELD_GAUGE_Y, &GameBitmaps[Gauges[GAUGE_SHIELDS+9-bm_num].index], F1_0, 0);
}

#define CLOAK_FADE_WAIT_TIME  0x400

void draw_player_ship(int cloak_state,int old_cloak_state,int x, int y)
{
	static fix cloak_fade_timer=0;
	static int cloak_fade_value=GR_FADE_LEVELS-1;
	static int refade = 0;
	grs_bitmap *bm = NULL;

#ifdef NETWORK
	if (Game_mode & GM_TEAM)
	{
		PIGGY_PAGE_IN(Gauges[GAUGE_SHIPS+get_team(Player_num)]);
		bm = &GameBitmaps[Gauges[GAUGE_SHIPS+get_team(Player_num)].index];
	}
	else
        #endif
	{
		PIGGY_PAGE_IN(Gauges[GAUGE_SHIPS+Player_num]);
		bm = &GameBitmaps[Gauges[GAUGE_SHIPS+Player_num].index];
	}
	

	if (old_cloak_state==-1 && cloak_state)
			cloak_fade_value=0;

	if (!cloak_state) {
		cloak_fade_value=GR_FADE_LEVELS-1;
		cloak_fade_state = 0;
	}

	if (cloak_state==1 && old_cloak_state==0)
		cloak_fade_state = -1;

	if (cloak_state && GameTime > Players[Player_num].cloak_time + CLOAK_TIME_MAX - i2f(3)) //doing "about-to-uncloak" effect
		if (cloak_fade_state==0)
			cloak_fade_state = 2;
	

	if (cloak_fade_state)
		cloak_fade_timer -= FrameTime;

	while (cloak_fade_state && cloak_fade_timer < 0) {

		cloak_fade_timer += CLOAK_FADE_WAIT_TIME;
		cloak_fade_value += cloak_fade_state;

		if (cloak_fade_value >= GR_FADE_LEVELS-1) {
			cloak_fade_value = GR_FADE_LEVELS-1;
			if (cloak_fade_state == 2 && cloak_state)
				cloak_fade_state = -2;
			else
				cloak_fade_state = 0;
		}
		else if (cloak_fade_value <= 0) {
			cloak_fade_value = 0;
			if (cloak_fade_state == -2)
				cloak_fade_state = 2;
			else
				cloak_fade_state = 0;
		}
	}

	// To fade out both pages in a paged mode.
	if (refade) refade = 0;
	else if (cloak_state && old_cloak_state && !cloak_fade_state && !refade) {
		cloak_fade_state = -1;
		refade = 1;
	}

	gr_set_current_canvas(&VR_render_buffer[0]);
	gr_rect(COCKPITSCALE_X*x, COCKPITSCALE_Y*y, COCKPITSCALE_X*(x+bm->bm_w), COCKPITSCALE_Y*(y+bm->bm_h));
	hud_bitblt( x, y, bm, F1_0, 0);

	Gr_scanline_darkening_level = cloak_fade_value;
	gr_rect(COCKPITSCALE_X*x, COCKPITSCALE_Y*y, COCKPITSCALE_X*(x+bm->bm_w), COCKPITSCALE_Y*(y+bm->bm_h));
	Gr_scanline_darkening_level = GR_FADE_LEVELS;
	gr_set_current_canvas( get_current_game_screen() );
	gr_bm_ubitbltm( bm->bm_w, bm->bm_h, x, y, x, y, &VR_render_buffer[0].cv_bitmap, &grd_curcanv->cv_bitmap);
}

#define INV_FRAME_TIME	(f1_0/10)		//how long for each frame

void draw_numerical_display(int shield, int energy)
{
	int dx = NUMERICAL_GAUGE_X, dy = NUMERICAL_GAUGE_Y+1;
	int sw,sh,saw,ew,eh,eaw;

	gr_set_curfont( GAME_FONT );
	PIGGY_PAGE_IN(Gauges[GAUGE_NUMERICAL]);
	hud_bitblt (dx, dy, &GameBitmaps[Gauges[GAUGE_NUMERICAL].index], F1_0, 0);
	// cockpit is not 100% geometric so we need to divide shield and energy X position by 1.951 which should be most accurate
	// gr_get_string_size is used so we can get the numbers finally in the correct position with sw and ew
	gr_set_fontcolor(gr_getcolor(14,14,23),-1 );
	gr_get_string_size((shield>99)?"100":(shield>9)?"00":"0",&sw,&sh,&saw);
	gr_printf(	(grd_curscreen->sc_w/1.951)-(sw/2), 
			(grd_curscreen->sc_h/1.365),"%d",shield);

	gr_set_fontcolor(gr_getcolor(25,18,6),-1 );
	gr_get_string_size((energy>99)?"100":(energy>9)?"00":"0",&ew,&eh,&eaw);
	gr_printf(	(grd_curscreen->sc_w/1.951)-(ew/2),
			(grd_curscreen->sc_h/1.5),"%d",energy);

	gr_set_current_canvas( get_current_game_screen() );
}


void draw_keys()
{
	gr_set_current_canvas( get_current_game_screen() );

	if (Players[Player_num].flags & PLAYER_FLAGS_BLUE_KEY )	{
		PIGGY_PAGE_IN(Gauges[GAUGE_BLUE_KEY]);
                hud_bitblt( GAUGE_BLUE_KEY_X, GAUGE_BLUE_KEY_Y, &GameBitmaps[Gauges[GAUGE_BLUE_KEY].index], F1_0, 0 );
	} else {
		PIGGY_PAGE_IN(Gauges[GAUGE_BLUE_KEY_OFF]);
		hud_bitblt( GAUGE_BLUE_KEY_X, GAUGE_BLUE_KEY_Y, &GameBitmaps[Gauges[GAUGE_BLUE_KEY_OFF].index], F1_0, 0 );
	}

	if (Players[Player_num].flags & PLAYER_FLAGS_GOLD_KEY)	{
		PIGGY_PAGE_IN(Gauges[GAUGE_GOLD_KEY]);
		hud_bitblt( GAUGE_GOLD_KEY_X, GAUGE_GOLD_KEY_Y, &GameBitmaps[Gauges[GAUGE_GOLD_KEY].index], F1_0, 0 );
	} else {
		PIGGY_PAGE_IN(Gauges[GAUGE_GOLD_KEY_OFF]);
		hud_bitblt( GAUGE_GOLD_KEY_X, GAUGE_GOLD_KEY_Y, &GameBitmaps[Gauges[GAUGE_GOLD_KEY_OFF].index], F1_0, 0 );
	}

	if (Players[Player_num].flags & PLAYER_FLAGS_RED_KEY)	{
		PIGGY_PAGE_IN( Gauges[GAUGE_RED_KEY] );
		hud_bitblt( GAUGE_RED_KEY_X,  GAUGE_RED_KEY_Y,  &GameBitmaps[Gauges[GAUGE_RED_KEY].index], F1_0, 0 );
	} else {
		PIGGY_PAGE_IN(Gauges[GAUGE_RED_KEY_OFF]);
		hud_bitblt( GAUGE_RED_KEY_X,  GAUGE_RED_KEY_Y,  &GameBitmaps[Gauges[GAUGE_RED_KEY_OFF].index], F1_0, 0 );
	}
}


void draw_weapon_info_sub(int info_index,gauge_box *box,int pic_x,int pic_y,char *name,int text_x,int text_y)
{
	grs_bitmap *bm;
	char *p;

	//clear the window
	gr_setcolor(BM_XRGB(0,0,0));
	gr_rect(COCKPITSCALE_X*box->left,COCKPITSCALE_Y*box->top,COCKPITSCALE_X*box->right,COCKPITSCALE_Y*(box->bot+1));

	bm=&GameBitmaps[Weapon_info[info_index].picture.index];
	Assert(bm != NULL);

	PIGGY_PAGE_IN( Weapon_info[info_index].picture );
	hud_bitblt (pic_x, pic_y, bm, (Cockpit_mode == CM_FULL_SCREEN) ? 2 * F1_0 : F1_0, 0);

	if (Gauge_hud_mode==0)
	{
		gr_set_fontcolor(gr_getcolor(0,20,0),-1 );
		
		if ((p=strchr(name,'\n'))!=NULL) {
			*p=0;
			gr_printf(text_x,text_y,name);
			gr_printf(text_x,text_y+FONTSCALE_Y(grd_curcanv->cv_font->ft_h+1),p+1);
			*p='\n';
		} else
		     gr_printf(text_x,text_y,name);
		
		// For laser, show level and quadness
		if (info_index == 0) {
			char	temp_str[7];
			
			sprintf(temp_str, "%s: 0", TXT_LVL);
			
			temp_str[5] = Players[Player_num].laser_level+1 + '0';
			
			gr_printf(text_x,text_y+COCKPITSCALE_Y*8, temp_str);
			
			if (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS) {
				strcpy(temp_str, TXT_QUAD);
				gr_printf(text_x,text_y+COCKPITSCALE_Y*16, temp_str);
			}
			
		}
	}
}

void draw_weapon_info(int weapon_type,int weapon_num)
{
	//added/edited 2/8/99 by Victor Rachels for hudinfo stuff
	int x,y,w;
	
#ifdef SHAREWARE
	if (Newdemo_state==ND_STATE_RECORDING )
		newdemo_record_player_weapon(weapon_type, weapon_num);
#endif
	
	if (weapon_type == 0)
	{
		if (Cockpit_mode == CM_STATUS_BAR)
		{
			draw_weapon_info_sub(Primary_weapon_to_weapon_info[weapon_num],&gauge_boxes[2],SB_PRIMARY_W_PIC_X,SB_PRIMARY_W_PIC_Y,Primary_weapon_names_short[weapon_num],SB_PRIMARY_W_TEXT_X,SB_PRIMARY_W_TEXT_Y);
			x=SB_PRIMARY_AMMO_X;
			y=SB_PRIMARY_AMMO_Y;
		}
		else
		{
			draw_weapon_info_sub(Primary_weapon_to_weapon_info[weapon_num],&gauge_boxes[0],PRIMARY_W_PIC_X,PRIMARY_W_PIC_Y,	Primary_weapon_names_short[weapon_num],PRIMARY_W_TEXT_X,PRIMARY_W_TEXT_Y);
			x=PRIMARY_AMMO_X;
			y=PRIMARY_AMMO_Y;
		}
		w = 23;
	}
	else
	{
		if (Cockpit_mode == CM_STATUS_BAR)
		{
			draw_weapon_info_sub(Secondary_weapon_to_weapon_info[weapon_num],&gauge_boxes[3],SB_SECONDARY_W_PIC_X,SB_SECONDARY_W_PIC_Y,SECONDARY_WEAPON_NAMES_SHORT(weapon_num),SB_SECONDARY_W_TEXT_X,SB_SECONDARY_W_TEXT_Y);
			x=SB_SECONDARY_AMMO_X;
			y=SB_SECONDARY_AMMO_Y;
		}
		else
		{
			draw_weapon_info_sub(Secondary_weapon_to_weapon_info[weapon_num],&gauge_boxes[1],SECONDARY_W_PIC_X,SECONDARY_W_PIC_Y,SECONDARY_WEAPON_NAMES_SHORT(weapon_num),SECONDARY_W_TEXT_X,SECONDARY_W_TEXT_Y);
			x=SECONDARY_AMMO_X;
			y=SECONDARY_AMMO_Y;
		}
	w = 8;
	}
	
	if (Gauge_hud_mode!=0)
		hud_show_weapons_mode1(weapon_type,1,1,x,y);
}

void draw_ammo_info(int x,int y,int ammo_count,int primary)
{
	if (Gauge_hud_mode!=0)
		hud_show_weapons_mode1(!primary,1,1,x,y);
	else
	{
		gr_setcolor(BM_XRGB(0,0,0));
		gr_set_fontcolor(gr_getcolor(20,0,0),-1 );
		gr_printf(x,y,"%03d",ammo_count);
		//added on 10/5/98 by Victor Rachels to add proxbomb count
		if(!primary)
		{
			gr_setcolor(BM_XRGB(0,0,0));
			if(Cockpit_mode == CM_STATUS_BAR)
			{
				gr_set_fontcolor(gr_getcolor(20,0,0),-1 );
				gr_printf(x-8,y+FONTSCALE_Y(18),"PX: %u",Players[Player_num].secondary_ammo[2]);
			}
			else
			{
				gr_set_fontcolor(gr_getcolor(20,0,0),-1 );
				gr_printf(x-4,y+FONTSCALE_Y(14),"PX: %u",Players[Player_num].secondary_ammo[2]);
			}
		}
	}
}

void draw_primary_ammo_info(int ammo_count)
{
	if (Cockpit_mode == CM_STATUS_BAR)
		draw_ammo_info(SB_PRIMARY_AMMO_X,SB_PRIMARY_AMMO_Y,ammo_count,1);
	else
		draw_ammo_info(PRIMARY_AMMO_X,PRIMARY_AMMO_Y,ammo_count,1);
}

void draw_secondary_ammo_info(int ammo_count)
{
	if (Cockpit_mode == CM_STATUS_BAR)
		draw_ammo_info(SB_SECONDARY_AMMO_X,SB_SECONDARY_AMMO_Y,ammo_count,0);
	else
                draw_ammo_info(SECONDARY_AMMO_X,SECONDARY_AMMO_Y,ammo_count,0);
}

//returns true if drew picture
int draw_weapon_box(int weapon_type,int weapon_num)
{
	int drew_flag=0;

#ifndef OGL
	gr_set_current_canvas(&VR_render_buffer[0]);
#endif

	gr_set_curfont( GAME_FONT );

	if (weapon_num != old_weapon[weapon_type][VR_current_page] && weapon_box_states[weapon_type] == WS_SET && (old_weapon[weapon_type][VR_current_page] != -1) && !Gauge_hud_mode)
	{
		weapon_box_states[weapon_type] = WS_FADING_OUT;
		weapon_box_fade_values[weapon_type]=i2f(GR_FADE_LEVELS-1);
	}
			
	if ((old_weapon[weapon_type][VR_current_page] == -1) || gauge_update_hud_mode==2)
	{
		draw_weapon_info(weapon_type,weapon_num);
		old_weapon[weapon_type][VR_current_page] = weapon_num;
		old_ammo_count[weapon_type][VR_current_page]=-1;
		drew_flag=1;
		weapon_box_states[weapon_type] = WS_SET;
	}
	
	if (weapon_box_states[weapon_type] == WS_FADING_OUT) {
		draw_weapon_info(weapon_type,old_weapon[weapon_type][VR_current_page]);
		old_ammo_count[weapon_type][VR_current_page]=-1;
		drew_flag=1;
		weapon_box_fade_values[weapon_type] -= FrameTime * FADE_SCALE;
		if (weapon_box_fade_values[weapon_type] <= 0)
		{
			weapon_box_states[weapon_type] = WS_FADING_IN;
			old_weapon[weapon_type][VR_current_page] = weapon_num;
			old_weapon[weapon_type][!VR_current_page] = weapon_num;
			weapon_box_fade_values[weapon_type] = 0;
		}
	}
	else if (weapon_box_states[weapon_type] == WS_FADING_IN)
	{
		if (weapon_num != old_weapon[weapon_type][VR_current_page])
		{
			weapon_box_states[weapon_type] = WS_FADING_OUT;
		}
		else
		{
			draw_weapon_info(weapon_type,weapon_num);
			old_ammo_count[weapon_type][VR_current_page]=-1;
			drew_flag=1;
			weapon_box_fade_values[weapon_type] += FrameTime * FADE_SCALE;
			if (weapon_box_fade_values[weapon_type] >= i2f(GR_FADE_LEVELS-1)) {
				weapon_box_states[weapon_type] = WS_SET;
				old_weapon[weapon_type][!VR_current_page] = -1;	//force redraw (at full fade-in) of other page
			}
		}
	}
	else
	{
		draw_weapon_info(weapon_type, weapon_num);
		old_weapon[weapon_type][VR_current_page] = weapon_num;
		old_ammo_count[weapon_type][VR_current_page] = -1;
		drew_flag = 1;
	}

	
	if (weapon_box_states[weapon_type] != WS_SET)         //fade gauge
	{
		int fade_value = f2i(weapon_box_fade_values[weapon_type]);
		int boxofs = (Cockpit_mode==CM_STATUS_BAR)?2:0;
		
		Gr_scanline_darkening_level = fade_value;
		gr_rect(COCKPITSCALE_X*gauge_boxes[boxofs+weapon_type].left,COCKPITSCALE_Y*gauge_boxes[boxofs+weapon_type].top,COCKPITSCALE_X*gauge_boxes[boxofs+weapon_type].right,COCKPITSCALE_Y*gauge_boxes[boxofs+weapon_type].bot);
		Gr_scanline_darkening_level = GR_FADE_LEVELS;
	}
	
	gr_set_current_canvas(get_current_game_screen());
	
	return drew_flag;
}

int gauge_update_hud_mode=0;

void draw_weapon_boxes()
{
	int boxofs = (Cockpit_mode==CM_STATUS_BAR)?2:0;
	int drew;
	
	drew = draw_weapon_box(0,Primary_weapon);
	if (drew)
		copy_gauge_box(&gauge_boxes[boxofs+0],&VR_render_buffer[0].cv_bitmap);
	
	if (weapon_box_states[0] == WS_SET)
		if (Players[Player_num].primary_ammo[Primary_weapon] != old_ammo_count[0][VR_current_page])
			if (Primary_weapon == VULCAN_INDEX)
			{
	#ifndef SHAREWARE
				if (Newdemo_state == ND_STATE_RECORDING)
					newdemo_record_primary_ammo(old_ammo_count[0][VR_current_page], Players[Player_num].primary_ammo[Primary_weapon]);
	#endif
				draw_primary_ammo_info(f2i(VULCAN_AMMO_SCALE * Players[Player_num].primary_ammo[Primary_weapon]));
				old_ammo_count[0][VR_current_page] = Players[Player_num].primary_ammo[Primary_weapon];
			}
	
	drew = draw_weapon_box(1,Secondary_weapon);
	if (drew)
		copy_gauge_box(&gauge_boxes[boxofs+1],&VR_render_buffer[0].cv_bitmap);
	
	if (weapon_box_states[1] == WS_SET)
		if (Players[Player_num].secondary_ammo[Secondary_weapon] != old_ammo_count[1][VR_current_page] || Players[Player_num].secondary_ammo[2] != old_prox)
		{
			old_prox=Players[Player_num].secondary_ammo[2];
	
#ifndef SHAREWARE
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_secondary_ammo(old_ammo_count[1][VR_current_page], Players[Player_num].secondary_ammo[Secondary_weapon]);
#endif
			draw_secondary_ammo_info(Players[Player_num].secondary_ammo[Secondary_weapon]);
			old_ammo_count[1][VR_current_page] = Players[Player_num].secondary_ammo[Secondary_weapon];
		}
	
	if(gauge_update_hud_mode)
	{
		if(Gauge_hud_mode!=0)
		{
			draw_primary_ammo_info(f2i(VULCAN_AMMO_SCALE * Players[Player_num].primary_ammo[Primary_weapon]));
			draw_secondary_ammo_info(Players[Player_num].secondary_ammo[Secondary_weapon]);
			gauge_update_hud_mode=0;
		}
	}
}


void sb_draw_energy_bar(int energy)
{
	int erase_height;
	int ew,eh,eaw;

	PIGGY_PAGE_IN(Gauges[SB_GAUGE_ENERGY]);
	hud_bitblt( SB_ENERGY_GAUGE_X, SB_ENERGY_GAUGE_Y, &GameBitmaps[Gauges[SB_GAUGE_ENERGY].index], F1_0, 0 );
	erase_height = (100 - energy) * SB_ENERGY_GAUGE_H / 100;

	if (erase_height > 0) {
		gr_setcolor( 0 );
		gr_rect(COCKPITSCALE_X*SB_ENERGY_GAUGE_X,COCKPITSCALE_Y*SB_ENERGY_GAUGE_Y,COCKPITSCALE_X*(SB_ENERGY_GAUGE_X+SB_ENERGY_GAUGE_W)-1,COCKPITSCALE_Y*(SB_ENERGY_GAUGE_Y+(erase_height-1)));
	}

	gr_set_current_canvas( get_current_game_screen() );

	//draw numbers
	gr_set_fontcolor(gr_getcolor(25,18,6),-1 );
	gr_get_string_size((energy>99)?"100":(energy>9)?"00":"0",&ew,&eh,&eaw);
	gr_printf((grd_curscreen->sc_w/3)-(ew/2),COCKPITSCALE_Y*SB_ENERGY_NUM_Y,"%d",energy);
}

void sb_draw_shield_num(int shield)
{
	grs_bitmap *bm = &GameBitmaps[cockpit_bitmap[Cockpit_mode].index];
	int sw,sh,saw;

	//draw numbers
	gr_set_curfont( GAME_FONT );
	gr_set_fontcolor(gr_getcolor(14,14,23),-1 );

	//erase old one
	PIGGY_PAGE_IN( cockpit_bitmap[Cockpit_mode] );
	gr_setcolor(gr_gpixel(bm,SB_SHIELD_NUM_X,SB_SHIELD_NUM_Y-(VR_render_height-bm->bm_h)));
	gr_get_string_size((shield>99)?"100":(shield>9)?"00":"0",&sw,&sh,&saw);
	gr_printf((grd_curscreen->sc_w/2.267)-(sw/2),COCKPITSCALE_Y*SB_SHIELD_NUM_Y,"%d",shield);
}

void sb_draw_shield_bar(int shield)
{
	int bm_num = shield>=100?9:(shield / 10);

	gr_set_current_canvas( get_current_game_screen() );

	PIGGY_PAGE_IN( Gauges[GAUGE_SHIELDS+9-bm_num] );
	hud_bitblt( SB_SHIELD_GAUGE_X, SB_SHIELD_GAUGE_Y, &GameBitmaps[Gauges[GAUGE_SHIELDS+9-bm_num].index], F1_0, 0 );
}

void sb_draw_keys()
{
	grs_bitmap * bm;
	int flags = Players[Player_num].flags;

	gr_set_current_canvas( get_current_game_screen() );

	bm = &GameBitmaps[Gauges[(flags&PLAYER_FLAGS_BLUE_KEY)?SB_GAUGE_BLUE_KEY:SB_GAUGE_BLUE_KEY_OFF].index];
	PIGGY_PAGE_IN(Gauges[(flags&PLAYER_FLAGS_BLUE_KEY)?SB_GAUGE_BLUE_KEY:SB_GAUGE_BLUE_KEY_OFF]);
	hud_bitblt( SB_GAUGE_KEYS_X, SB_GAUGE_BLUE_KEY_Y, bm, F1_0, 0 );
	bm = &GameBitmaps[Gauges[(flags&PLAYER_FLAGS_GOLD_KEY)?SB_GAUGE_GOLD_KEY:SB_GAUGE_GOLD_KEY_OFF].index];
	PIGGY_PAGE_IN(Gauges[(flags&PLAYER_FLAGS_GOLD_KEY)?SB_GAUGE_GOLD_KEY:SB_GAUGE_GOLD_KEY_OFF]);
	hud_bitblt( SB_GAUGE_KEYS_X, SB_GAUGE_GOLD_KEY_Y, bm, F1_0, 0 );
	bm = &GameBitmaps[Gauges[(flags&PLAYER_FLAGS_RED_KEY)?SB_GAUGE_RED_KEY:SB_GAUGE_RED_KEY_OFF].index];
	PIGGY_PAGE_IN(Gauges[(flags&PLAYER_FLAGS_RED_KEY)?SB_GAUGE_RED_KEY:SB_GAUGE_RED_KEY_OFF]);
	hud_bitblt( SB_GAUGE_KEYS_X, SB_GAUGE_RED_KEY_Y, bm, F1_0, 0  );
}

// Draws invulnerable ship, or maybe the flashing ship, depending on invulnerability time left.
void draw_invulnerable_ship()
{
	static fix time=0;

	gr_set_current_canvas( get_current_game_screen() );

	if (((Players[Player_num].invulnerable_time + INVULNERABLE_TIME_MAX - GameTime) > F1_0*4) || (GameTime & 0x8000)) {

		if (Cockpit_mode == CM_STATUS_BAR)	{
			PIGGY_PAGE_IN(Gauges[GAUGE_INVULNERABLE+invulnerable_frame]);
			hud_bitblt( SB_SHIELD_GAUGE_X, SB_SHIELD_GAUGE_Y, &GameBitmaps[Gauges[GAUGE_INVULNERABLE+invulnerable_frame].index], F1_0, 0 );
		} else {
			PIGGY_PAGE_IN(Gauges[GAUGE_INVULNERABLE+invulnerable_frame]);
			hud_bitblt( SHIELD_GAUGE_X, SHIELD_GAUGE_Y, &GameBitmaps[Gauges[GAUGE_INVULNERABLE+invulnerable_frame].index], F1_0, 0 );
		}

		time += FrameTime;

		while (time > INV_FRAME_TIME) {
			time -= INV_FRAME_TIME;
			if (++invulnerable_frame == N_INVULNERABLE_FRAMES)
				invulnerable_frame=0;
		}
	} else if (Cockpit_mode == CM_STATUS_BAR)
		sb_draw_shield_bar(f2ir(Players[Player_num].shields));
	else
		draw_shield_bar(f2ir(Players[Player_num].shields));
}

#ifdef HOSTAGE_FACES
void draw_hostage_gauge()
{
	int drew;

//        gr_set_current_canvas(Canv_game_offscrn);
        gr_set_current_canvas(get_current_game_screen());

	drew = do_hostage_effects();

	if (drew) {
		int boxofs = (Cockpit_mode==CM_STATUS_BAR)?2:0;

//                gr_set_current_canvas(Canv_game);
//                copy_gauge_box(&gauge_boxes[boxofs+1],&Canv_game_offscrn->cv_bitmap);

		old_weapon[1][VR_current_page] = old_ammo_count[1][VR_current_page] = -1;
	}

}
#endif

extern int Missile_gun;
extern int allowed_to_fire_laser(void);
extern int allowed_to_fire_missile(void);

rgb player_rgb[] = {	{15,15,23},
			{27,0,0},
			{0,23,0},
			{30,11,31},
			{31,16,0},
			{24,17,6},
			{14,21,12},
			{29,29,0}, };

extern int hud_display_all;

//draw the reticle
void show_reticle(int force_big_one)
{
	int x,y;
	int laser_ready,missile_ready,laser_ammo,missile_ammo;
	int cross_bm_num,primary_bm_num,secondary_bm_num;


	if (hud_display_all)
		return;

	x = grd_curcanv->cv_w/2;
	y = grd_curcanv->cv_h/2;

	laser_ready = allowed_to_fire_laser();
	missile_ready = allowed_to_fire_missile();

	laser_ammo = player_has_weapon(Primary_weapon,0);
	missile_ammo = player_has_weapon(Secondary_weapon,1);

	primary_bm_num = (laser_ready && laser_ammo==HAS_ALL);
	secondary_bm_num = (missile_ready && missile_ammo==HAS_ALL);

	if (primary_bm_num && Primary_weapon==LASER_INDEX && (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
		primary_bm_num++;

	if (Secondary_weapon!=CONCUSSION_INDEX && Secondary_weapon!=HOMING_INDEX)
		secondary_bm_num += 3;
	else if (secondary_bm_num && !(Missile_gun&1))
			secondary_bm_num++;

	cross_bm_num = ((primary_bm_num > 0) || (secondary_bm_num > 0));

	Assert(primary_bm_num <= 2);
	Assert(secondary_bm_num <= 4);
	Assert(cross_bm_num <= 1);
#ifdef OGL
	if (gl_reticle==2 || (gl_reticle && grd_curcanv->cv_bitmap.bm_w > 320)){
		ogl_draw_reticle(cross_bm_num,primary_bm_num,secondary_bm_num);
	}else
#endif

	if (grd_curcanv->cv_bitmap.bm_w > 200 || force_big_one) {
#ifndef OGL
		PIGGY_PAGE_IN(Gauges[RETICLE_CROSS + cross_bm_num]);
		gr_ubitmapm(x-4 ,y-2,&GameBitmaps[Gauges[RETICLE_CROSS + cross_bm_num].index]);
		PIGGY_PAGE_IN(Gauges[RETICLE_PRIMARY + primary_bm_num]);
		gr_ubitmapm(x-15,y+6,&GameBitmaps[Gauges[RETICLE_PRIMARY + primary_bm_num].index]);
		PIGGY_PAGE_IN(Gauges[RETICLE_SECONDARY + secondary_bm_num]);
		gr_ubitmapm(x-12,y+1,&GameBitmaps[Gauges[RETICLE_SECONDARY + secondary_bm_num].index]);
#else
		PIGGY_PAGE_IN(Gauges[RETICLE_CROSS + cross_bm_num]);
		ogl_ubitmapm_cf(x-(4*COCKPITSCALE_X) ,y-(2*COCKPITSCALE_Y),9*COCKPITSCALE_X,7*COCKPITSCALE_Y,&GameBitmaps[Gauges[RETICLE_CROSS + cross_bm_num].index],255,F1_0);
		PIGGY_PAGE_IN(Gauges[RETICLE_PRIMARY + primary_bm_num]);
		ogl_ubitmapm_cf(x-(15*COCKPITSCALE_X),y+(6*COCKPITSCALE_Y),31*COCKPITSCALE_X,5*COCKPITSCALE_Y,&GameBitmaps[Gauges[RETICLE_PRIMARY + primary_bm_num].index],255,F1_0);
		PIGGY_PAGE_IN(Gauges[RETICLE_SECONDARY + secondary_bm_num]);
		ogl_ubitmapm_cf(x-(12*COCKPITSCALE_X),y+(1*COCKPITSCALE_Y),25*COCKPITSCALE_X,10*COCKPITSCALE_Y,&GameBitmaps[Gauges[RETICLE_SECONDARY + secondary_bm_num].index],255,F1_0);
#endif
	}
	else {
		PIGGY_PAGE_IN(Gauges[SML_RETICLE_CROSS + cross_bm_num]);
		gr_ubitmapm(x-2,y-1,&GameBitmaps[Gauges[SML_RETICLE_CROSS + cross_bm_num].index]);
		PIGGY_PAGE_IN(Gauges[SML_RETICLE_PRIMARY + primary_bm_num]);
		gr_ubitmapm(x-8,y+2,&GameBitmaps[Gauges[SML_RETICLE_PRIMARY + primary_bm_num].index]);
		PIGGY_PAGE_IN(Gauges[SML_RETICLE_SECONDARY + secondary_bm_num]);
		gr_ubitmapm(x-6,y-2,&GameBitmaps[Gauges[SML_RETICLE_SECONDARY + secondary_bm_num].index]);
	}

#ifndef SHAREWARE
#ifdef NETWORK
	if ((Newdemo_state == ND_STATE_PLAYBACK) || (((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_TEAM)) && Show_reticle_name))
	{
		// Draw player callsign for player in sights 
		fvi_query fq;
		vms_vector orient;
		int Hit_type;
		fvi_info Hit_data;

		fq.p0 		= &ConsoleObject->pos;
		orient 		= ConsoleObject->orient.fvec;
		vm_vec_scale(&orient, F1_0*1024);
		vm_vec_add2(&orient, fq.p0);
		fq.p1 		= &orient;
		fq.rad 		= 0;
		fq.thisobjnum = ConsoleObject - Objects;
		fq.flags 	= FQ_TRANSWALL | FQ_CHECK_OBJS;
		fq.startseg	= ConsoleObject->segnum;
		fq.ignore_obj_list = NULL;

		Hit_type = find_vector_intersection(&fq, &Hit_data);
		if ((Hit_type == HIT_OBJECT) && (Objects[Hit_data.hit_object].type == OBJ_PLAYER))
		{
			// Draw callsign on HUD
			char s[CALLSIGN_LEN+1];
			int w, h, aw;
			int x1, y1;
			int pnum;
			int color_num;

			pnum = Objects[Hit_data.hit_object].id;

			if ((Game_mode & GM_TEAM) && (get_team(pnum) != get_team(Player_num)) && (Newdemo_state != ND_STATE_PLAYBACK))
				return;

			if (Game_mode & GM_TEAM)
				color_num = get_team(pnum);
			else
				color_num = pnum;
			sprintf(s, "%s", Players[pnum].callsign);
			gr_get_string_size(s, &w, &h, &aw);
			gr_set_fontcolor(gr_getcolor(player_rgb[color_num].r,player_rgb[color_num].g,player_rgb[color_num].b),-1 );
			x1 = x-(w/2);
			y1 = y+12;
			gr_string (x1, y1, s);
		}
#ifndef NDEBUG
		else if ((Hit_type == HIT_OBJECT) && (Objects[Hit_data.hit_object].type == OBJ_ROBOT))
		{
			char s[CALLSIGN_LEN+1];
			int w, h, aw;
			int x1, y1;
			int color_num = 0;

			sprintf(s, "%d", Hit_data.hit_object);
			gr_get_string_size(s, &w, &h, &aw);
			gr_set_fontcolor(gr_getcolor(player_rgb[color_num].r,player_rgb[color_num].g,player_rgb[color_num].b),-1 );
			x1 = x-(w/2);
			y1 = y+12;
			gr_string (x1, y1, s);
		}
#endif
	}
#endif
#endif
}

#ifdef NETWORK
void hud_show_kill_list()
{
	int n_players,player_list[MAX_NUM_NET_PLAYERS];
	int n_left,i,x0,x1,y,save_y,fth;

	if (Show_kill_list_timer > 0)
	{
		Show_kill_list_timer -= FrameTime;
		if (Show_kill_list_timer < 0)
			Show_kill_list = 0;
	}
	
#ifdef SHAREWARE
	if (Game_mode & GM_MULTI_COOP)
	{
		Show_kill_list = 0;
		return;
	}
#endif

	gr_set_curfont( GAME_FONT );

	n_players = multi_get_kill_list(player_list);

        if (Show_kill_list == 3)
		n_players = 2;

	if (n_players <= 4)
		n_left = n_players;
	else
		n_left = (n_players+1)/2;

	fth = FONTSCALE_Y(GAME_FONT->ft_h);

	x0 = 1; x1 = GAME_FONT->ft_aw*9;

#ifndef SHAREWARE
	if (Game_mode & GM_MULTI_COOP)
		x1 = GAME_FONT->ft_aw*7;
#endif

	save_y = y = grd_curcanv->cv_h - n_left*(fth+(COCKPITSCALE_Y*1));

	if (Cockpit_mode == CM_FULL_COCKPIT) {
		save_y = y -= 6*(SHEIGHT/200);
	}

	for (i=0;i<n_players;i++) {
		int player_num;
		char name[9];
		int sw,sh,aw;

		if (i==n_left) {
			if (Cockpit_mode == CM_FULL_COCKPIT)
				x0 = grd_curcanv->cv_w - GAME_FONT->ft_aw*12;
			else
				x0 = grd_curcanv->cv_w - GAME_FONT->ft_aw*13;
#ifndef SHAREWARE
			if (Game_mode & GM_MULTI_COOP)
				x1 = grd_curcanv->cv_w - GAME_FONT->ft_aw*6;
			else
#endif
			if(Show_kill_list == 2)
				x1 = grd_curcanv->cv_w - GAME_FONT->ft_aw*6;
			else
				x1 = grd_curcanv->cv_w - GAME_FONT->ft_aw*3;
			y = save_y;
		}
	
                if (Show_kill_list == 3)
			player_num = i;
		else
			player_num = player_list[i];

		if (Show_kill_list == 1 || Show_kill_list ==2)
		{
			int color;

			if (Players[player_num].connected != 1)
				gr_set_fontcolor(gr_getcolor(12, 12, 12), -1);
			else if (Game_mode & GM_TEAM) {
				color = get_team(player_num);
				gr_set_fontcolor(gr_getcolor(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );
			}
			else {
				color = player_num;
				gr_set_fontcolor(gr_getcolor(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );
			}
		}
		else
		{
			gr_set_fontcolor(gr_getcolor(player_rgb[player_num].r,player_rgb[player_num].g,player_rgb[player_num].b),-1 );
		}

                if (Show_kill_list == 3)
			strcpy(name, Netgame.team_name[i]);
		else
			strcpy(name,Players[player_num].callsign);	// Note link to above if!!
		gr_get_string_size(name,&sw,&sh,&aw);
		while (sw > (x1-x0-3)) {
			name[strlen(name)-1]=0;
                        gr_get_string_size(name,&sw,&sh,&aw);
		}
		gr_printf(x0,y,"%s",name);
		if (Show_kill_list == 3)
			gr_printf(x1,y,"%3d",team_kills[i]);
#ifndef SHAREWARE
		else if (Game_mode & GM_MULTI_COOP)
			gr_printf(x1,y,"%-6d",Players[player_num].score);
#endif
		else if (Show_kill_list == 2)
			gr_printf(x1,y,"%i/%i(%i%%)",Players[player_num].net_kills_total,Players[player_num].net_killed_total,(Players[player_num].net_killed_total+Players[player_num].net_kills_total)?(Players[player_num].net_kills_total*100)/(Players[player_num].net_killed_total+Players[player_num].net_kills_total):0);
		else
			gr_printf(x1,y,"%3d",Players[player_num].net_kills_total);
		if(ping_stats_on && ping_stats_getping(player_num))
		{
			int x2;
			if(i<n_left)
			{
				x2=x1 + GAME_FONT->ft_aw*4; //was +20;
				if (Game_mode & GM_MULTI_COOP)
				x2=x1 + GAME_FONT->ft_aw*7;//was +30;
				if(Show_kill_list == 2)
				x2=x1 + GAME_FONT->ft_aw*12;//was +25;
			}
			else
				x2=x0 - GAME_FONT->ft_aw*9;//was -40;
			gr_printf(x2,y,"%lu %i%%",
			fixmuldiv(ping_stats_getping(player_num),1000,F1_0),(100-((ping_stats_getgot(player_num)*100)/ping_stats_getsent(player_num))) );
		}
		y += fth+1;
	}
}
#endif

//draw all the things on the HUD

extern int last_drawn_cockpit[2];

void draw_hud()
{
	// Show score so long as not in rearview
	if ( !Rear_view && Cockpit_mode!=CM_REAR_VIEW && Cockpit_mode!=CM_STATUS_BAR) {
		hud_show_score();
		if (score_time)
			hud_show_score_added();
	}

	// Show other stuff if not in rearview or letterbox.
	if (!Rear_view && Cockpit_mode!=CM_REAR_VIEW) {
		if (Cockpit_mode==CM_STATUS_BAR || Cockpit_mode==CM_FULL_SCREEN)
			hud_show_homing_warning();

		if (Cockpit_mode==CM_FULL_SCREEN) {
			hud_show_energy();
			hud_show_shield();
			hud_show_weapons();
			hud_show_keys();
			hud_show_cloak_invuln();

			if ( ( Newdemo_state==ND_STATE_RECORDING ) && ( Players[Player_num].flags != old_flags[VR_current_page] )) {
				newdemo_record_player_flags(old_flags[VR_current_page], Players[Player_num].flags);
				old_flags[VR_current_page] = Players[Player_num].flags;
			}
		}

#ifdef NETWORK
#ifndef RELEASE
		if (!(Game_mode&GM_MULTI && Show_kill_list))
			show_time();
#endif
#endif
		if (Cockpit_mode != CM_LETTERBOX && (!Use_player_head_angles))
			show_reticle(0);
		HUD_render_message_frame();

		if (Cockpit_mode!=CM_STATUS_BAR)
			hud_show_lives();
#ifdef NETWORK
		if (Game_mode&GM_MULTI && Show_kill_list)
			hud_show_kill_list();
#endif
	}

	if (Rear_view && Cockpit_mode!=CM_REAR_VIEW) {
		HUD_render_message_frame();
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(gr_getcolor(0,31,0),-1 );
		if (Newdemo_state == ND_STATE_PLAYBACK)
			gr_printf(0x8000,grd_curcanv->cv_h-COCKPITSCALE_Y*14,TXT_REAR_VIEW);
		else
			gr_printf(0x8000,grd_curcanv->cv_h-COCKPITSCALE_Y*10,TXT_REAR_VIEW);
	}

}

//print out some player statistics
void render_gauges()
{
	int energy = f2ir(Players[Player_num].energy);
	int shields = f2ir(Players[Player_num].shields);
	int cloak = ((Players[Player_num].flags&PLAYER_FLAGS_CLOAKED) != 0);
 
	Assert(Cockpit_mode==CM_FULL_COCKPIT || Cockpit_mode==CM_STATUS_BAR);

// check to see if our display mode has changed since last render time --
// if so, then we need to make new gauge canvases.

	if (shields < 0 ) shields = 0;

	gr_set_current_canvas(get_current_game_screen());
	gr_set_curfont( GAME_FONT );

	if (Newdemo_state == ND_STATE_RECORDING)
		if (Players[Player_num].homing_object_dist >= 0)
			newdemo_record_homing_distance(Players[Player_num].homing_object_dist);

	if (Cockpit_mode == CM_FULL_COCKPIT)
		draw_player_ship(cloak, old_cloak[VR_current_page], SHIP_GAUGE_X, SHIP_GAUGE_Y);
	else
		draw_player_ship(cloak, old_cloak[VR_current_page], SB_SHIP_GAUGE_X, SB_SHIP_GAUGE_Y);

	old_cloak[VR_current_page] = cloak;

	if (Cockpit_mode == CM_FULL_COCKPIT) {
		if (Newdemo_state == ND_STATE_RECORDING && (energy != old_energy[VR_current_page]))
		{
			newdemo_record_player_energy(old_energy[VR_current_page], energy);
			old_energy[VR_current_page] = energy;
		}
		draw_energy_bar(energy);
		draw_numerical_display(shields, energy);

		if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) {
			draw_invulnerable_ship();
			old_shields[VR_current_page] = shields ^ 1;
		} else {		// Draw the shield gauge
			if (Newdemo_state == ND_STATE_RECORDING && (shields != old_shields[VR_current_page]))
			{
				newdemo_record_player_shields(old_shields[VR_current_page], shields);
				old_shields[VR_current_page] = shields;
			}
			draw_shield_bar(shields);
		}
		draw_numerical_display(shields, energy);
	
		if (Newdemo_state == ND_STATE_RECORDING && (Players[Player_num].flags != old_flags[VR_current_page]))
		{
			newdemo_record_player_flags(old_flags[VR_current_page], Players[Player_num].flags);
			old_flags[VR_current_page] = Players[Player_num].flags;
		}
		draw_keys();

		show_homing_warning();

	} else if (Cockpit_mode == CM_STATUS_BAR) {

		if (Newdemo_state == ND_STATE_RECORDING && (energy != old_energy[VR_current_page]))
		{
			newdemo_record_player_energy(old_energy[VR_current_page], energy);
			old_energy[VR_current_page] = energy;
		}
		sb_draw_energy_bar(energy);

		if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE)
		{
			draw_invulnerable_ship();
			old_shields[VR_current_page] = shields ^ 1;
		} 
		else
		{	// Draw the shield gauge
			if (Newdemo_state == ND_STATE_RECORDING && (shields != old_shields[VR_current_page]))
			{
				newdemo_record_player_shields(old_shields[VR_current_page], shields);
				old_shields[VR_current_page] = shields;
			}
			sb_draw_shield_bar(shields);
		}
		sb_draw_shield_num(shields);

		if (Newdemo_state == ND_STATE_RECORDING && (Players[Player_num].flags != old_flags[VR_current_page]))
		{
			newdemo_record_player_flags(old_flags[VR_current_page], Players[Player_num].flags);
			old_flags[VR_current_page] = Players[Player_num].flags;
		}
		sb_draw_keys();
	

		// May want to record this in a demo...
		if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
		{
			sb_show_lives();
			old_lives[VR_current_page] = Players[Player_num].net_killed_total;
		}
		else
		{
			sb_show_lives();
			old_lives[VR_current_page] = Players[Player_num].lives;
		}

		if ((Game_mode&GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
		{
			sb_show_score();
			old_score[VR_current_page] = Players[Player_num].net_kills_total;
		}
		else
		{
			sb_show_score();
			old_score[VR_current_page] = Players[Player_num].score;
			sb_show_score_added();
		}
	}
	draw_weapon_boxes();
}

//	---------------------------------------------------------------------------------------------------------
//	Call when picked up a laser powerup.
//	If laser is active, set old_weapon[0] to -1 to force redraw.
void update_laser_weapon_info(void)
{
	if (old_weapon[0][VR_current_page] == 0)
		old_weapon[0][VR_current_page] = -1;
}

