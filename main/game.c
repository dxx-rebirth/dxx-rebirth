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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
char game_rcsid[] = "$Id: game.c,v 1.7 2001-01-31 16:31:13 bradleyb Exp $";
#endif

#ifdef WINDOWS
#include "desw.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

#ifdef MACINTOSH
#include <Files.h>
#include <StandardFile.h>
#include <Quickdraw.h>
#include <Script.h>
#include <Strings.h>
#endif

#ifdef OGL
#include "ogl_init.h"
#endif

#include "pstypes.h"
#include "console.h"
#include "pa_enabl.h"       //$$POLY_ACC
#include "gr.h"
#include "inferno.h"
#include "game.h"
#include "key.h"
#include "object.h"
#include "physics.h"
#include "error.h"
#include "joy.h"
#include "mono.h"
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
#include "digi.h"
#include "ibitblt.h"
#include "u_mem.h"
#include "palette.h"
#include "morph.h"
#include "lighting.h"
#include "newdemo.h"
#include "collide.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "gameseq.h"
#include "automap.h"
#include "text.h"
#include "powerup.h"
#include "fireball.h"
#include "newmenu.h"
#include "network.h"
#include "gamefont.h"
#include "endlevel.h"
#include "joydefs.h"
#include "kconfig.h"
#include "mouse.h"
#include "switch.h"
#include "controls.h"
#include "songs.h"
#include "gamepal.h"

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

#include "multi.h"
#include "desc_id.h"
#include "cntrlcen.h"
#include "pcx.h"
#include "state.h"
#include "piggy.h"
#include "multibot.h"
#include "ai.h"
#include "robot.h"
#include "playsave.h"
#include "fix.h"
#include "d_delay.h"

int VGA_current_mode;

#ifdef MWPROFILER
#include <profiler.h>
#endif

//#define TEST_TIMER	1		//if this is set, do checking on timer

#define	SHOW_EXIT_PATH	1

#ifdef EDITOR
#include "editor\editor.h"
#endif

//#define _MARK_ON 1
#ifdef __WATCOMC__
#if __WATCOMC__ < 1000
#include <wsample.h>            //should come after inferno.h to get mark setting
#endif
#endif


extern void ReadControls(void);		// located in gamecntl.c
extern int Current_display_mode;
extern void do_final_boss_frame(void);

int	Speedtest_on = 0;

#ifndef NDEBUG
int	Mark_count = 0;                 // number of debugging marks set
int	Speedtest_start_time;
int	Speedtest_segnum;
int	Speedtest_sidenum;
int	Speedtest_frame_start;
int	Speedtest_count=0;				//	number of times to do the debug test.
#endif

static fix last_timer_value=0;
fix ThisLevelTime=0;

#if defined(TIMER_TEST) && !defined(NDEBUG)
fix _timer_value,actual_last_timer_value,_last_frametime;
int stop_count,start_count;
int time_stopped,time_started;
#endif

#ifndef MACINTOSH
ubyte * Game_cockpit_copy_code = NULL;
#else
ubyte Game_cockpit_copy_code = 0;
ubyte Scanline_double = 1;
#endif

int			VR_screen_mode			= 0;

ubyte			VR_screen_flags	= 0;		//see values in screens.h
ubyte			VR_current_page	= 0;
fix			VR_eye_width		= F1_0;
int			VR_render_mode		= VR_NONE;
int			VR_low_res 			= 3;				// Default to low res
int 			VR_show_hud = 1;
int			VR_sensitivity     = 1;		// 0 - 2

//NEWVR
int			VR_eye_offset		 = 0;
int			VR_eye_switch		 = 0;
int			VR_eye_offset_changed = 0;
int			VR_use_reg_code 	= 0;

grs_canvas  *VR_offscreen_buffer	= NULL;		// The offscreen data buffer
grs_canvas	VR_render_buffer[2];					//  Two offscreen buffers for left/right eyes.
grs_canvas	VR_render_sub_buffer[2];			//  Two sub buffers for left/right eyes.
grs_canvas	VR_screen_pages[2];					//  Two pages of VRAM if paging is available
grs_canvas	VR_editor_canvas;						//  The canvas that the editor writes to.

#ifdef WINDOWS
//@@ LPDIRECTDRAWSURFACE _lpDDSMask = NULL;
dd_grs_canvas *dd_VR_offscreen_buffer = NULL;
dd_grs_canvas dd_VR_screen_pages[2];
dd_grs_canvas dd_VR_render_buffer[2];
dd_grs_canvas dd_VR_render_sub_buffer[2];

void game_win_init_cockpit_mask(int sram);
#endif

//do menus work in 640x480 or 320x200?
//PC version sets this in main().  Mac versios is always high-res, so set to 1 here
int MenuHiresAvailable = 1;		//can we do highres menus?
int MenuHires = 1;				//are we currently in highres menus?

int Debug_pause=0;				//John's debugging pause system

int Cockpit_mode=CM_FULL_COCKPIT;		//set game.h for values

int Cockpit_mode_save=-1;					//set while in letterbox or rear view, or -1
int force_cockpit_redraw=0;

cvar_t r_framerate = {"r_framerate","0"};

int PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd;

//	Toggle_var points at a variable which gets !ed on ctrl-alt-T press.
int	Dummy_var;
int	*Toggle_var = &Dummy_var;

#ifdef EDITOR
//flag for whether initial fade-in has been done
char faded_in;
#endif

#ifndef NDEBUG                          //these only exist if debugging

int Game_double_buffer = 1;     //double buffer by default
fix fixed_frametime=0;          //if non-zero, set frametime to this

#endif

int Game_suspended=0;           //if non-zero, nothing moves but player

fix 	RealFrameTime;
fix	Auto_fire_fusion_cannon_time = 0;
fix	Fusion_charge = 0;
fix	Fusion_next_sound_time = 0;
fix	Fusion_last_sound_time = 0;

int Debug_spew = 1;
int Game_turbo_mode = 0;

int Game_mode = GM_GAME_OVER;

int	Global_laser_firing_count = 0;
int	Global_missile_firing_count = 0;

grs_bitmap background_bitmap;

int Game_aborted;

#define BACKGROUND_NAME "statback.pcx"

//	Function prototypes for GAME.C exclusively.

void GameLoop(int RenderFlag, int ReadControlsFlag);
void FireLaser(void);
void slide_textures(void);
void powerup_grab_cheat_all(void);

//	Other functions
extern void multi_check_for_killgoal_winner();
extern void RestoreGameSurfaces();

// window functions

void grow_window(void);
void shrink_window(void);

// text functions

void fill_background();

#ifndef RELEASE
void show_framerate(void);
void ftoa(char *string, fix f);
#endif

extern ubyte DefiningMarkerMessage;
extern char Marker_input[];

//	==============================================================================================

extern char john_head_on;

void load_background_bitmap()
{
	ubyte pal[256*3];
	int pcx_error;

	if (background_bitmap.bm_data)
		d_free(background_bitmap.bm_data);

	background_bitmap.bm_data=NULL;
	pcx_error = pcx_read_bitmap(john_head_on?"johnhead.pcx":BACKGROUND_NAME,&background_bitmap,BM_LINEAR,pal);
	if (pcx_error != PCX_ERROR_NONE)
		Error("File %s - PCX error: %s",BACKGROUND_NAME,pcx_errormsg(pcx_error));
	gr_remap_bitmap_good( &background_bitmap, pal, -1, -1 );
}


//this is called once per game
void init_game()
{
	atexit(close_game);             //for cleanup

	init_objects();

	init_special_effects();

	init_ai_system();

	init_gauge_canvases();

	init_exploding_walls();

	load_background_bitmap();

	Clear_window = 2;		//	do portal only window clear.

	set_detail_level_parameters(Detail_level);

	build_mission_list(0);		// This also loads mission 0.

	/* Register cvars */
	cvar_registervariable(&r_framerate);

}


void reset_palette_add()
{
	PaletteRedAdd 		= 0;
	PaletteGreenAdd	= 0;
	PaletteBlueAdd		= 0;
	//gr_palette_step_up( PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd );
}


#ifdef WINDOWS
void win_get_span_list(grs_bitmap *bm, int miny, int maxy)
{
	int x,y;
	int mode = 0;
	ubyte *data;
	int offset;
	int lspan=0, rspan=0, span=0;
//@@FILE *fp;

	data = bm->bm_data;
//@@	fp = fopen("cockspan.dat", "w");

	for (y = 0; y < miny; y++)
		win_cockpit_mask[y].num = 0;

	for (y = miny; y <= maxy; y++) 
	{
		span = 0;
		//@@ fprintf(fp, "line %d: ", y);		
		for (x = 0; x < bm->bm_w; x++)
		{
			offset = y*bm->bm_rowsize + x;

			if (data[offset] == 255) {
				switch (mode)
				{
					case 0:				// Start Mode
						lspan	= x;
						win_cockpit_mask[y].span[span].xmin = x;
						mode = 1;
					//@@	fprintf(fp, "<%d,", lspan);
						break;

					case 1:				// Transparency mode
						rspan = x;
						win_cockpit_mask[y].span[span].xmax = x;
						break;

					case 2:				// Switch from Draw mode to transparent
						lspan = x;
						win_cockpit_mask[y].span[span].xmin = x;
					//@@	fprintf(fp, "<%d,", lspan);
						mode = 1;
						break;
				}
			}
			else {
				switch(mode) 
				{
					case 0:				// Start mode
						mode = 2;
						break;
					
					case 1:				// Switching from transparent to Draw
						rspan = x;
						mode = 2;
						win_cockpit_mask[y].span[span].xmax = x;
						span++;
					//@@	fprintf(fp, "%d> ", rspan);
						break;

					case 2:
						break;
				}
			}
		}
		if (mode == 1) {
		//@@	fprintf(fp, "%d> ", rspan);
			win_cockpit_mask[y].span[span].xmax = rspan;
			span++;
		}
		win_cockpit_mask[y].num = span;
	//@@	fprintf(fp, "\n");
		mode = 0;
	}
	win_cockpit_mask[y].num = 255;
}
#endif //WINDOWS
	

void game_show_warning(char *s)
{

	if (!((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME)))
		stop_time();

	nm_messagebox( TXT_WARNING, 1, TXT_OK, s );

	if (!((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME)))
		start_time();
}


//these should be in gr.h
#define cv_w  cv_bitmap.bm_w
#define cv_h  cv_bitmap.bm_h

int Game_window_x = 0;
int Game_window_y = 0;
int Game_window_w = 0;
int Game_window_h = 0;
int max_window_w = 0;
int max_window_h = 0;

extern void newdemo_record_cockpit_change(int);

//initialize the various canvases on the game screen
//called every time the screen mode or cockpit changes
void init_cockpit()
{
	int minx, maxx, miny, maxy;

#if defined(POLY_ACC)
    pa_flush();                 // get rid of undrawn polys.
    pa_clear_buffer(1, 0);
#endif

	//Initialize the on-screen canvases

   if (Newdemo_state==ND_STATE_RECORDING)
	 {

		newdemo_record_cockpit_change(Cockpit_mode);
	 }

	if ( VR_render_mode != VR_NONE )
		Cockpit_mode = CM_FULL_SCREEN;

	if (!(VR_screen_flags & VRF_ALLOW_COCKPIT) && (Cockpit_mode==CM_FULL_COCKPIT || Cockpit_mode==CM_STATUS_BAR || Cockpit_mode==CM_REAR_VIEW) )
		Cockpit_mode = CM_FULL_SCREEN;

	if ( Screen_mode == SCREEN_EDITOR )
		Cockpit_mode = CM_FULL_SCREEN;

 WINDOS(
	dd_gr_set_current_canvas(NULL),
	gr_set_current_canvas(NULL)
 );
	gr_set_curfont( GAME_FONT );

#if !defined(MACINTOSH) && !defined(WINDOWS)
	if (Game_cockpit_copy_code)
		d_free(Game_cockpit_copy_code);
	Game_cockpit_copy_code  = NULL;
#else
	if (Game_cockpit_copy_code)
		Game_cockpit_copy_code = 0;
#endif

//@@	#ifdef WINDOWS
//@@		if (_lpDDSMask) { DDFreeSurface(_lpDDSMask); _lpDDSMask = NULL; }
//@@	#endif

#ifdef WINDOWS
	game_win_init_cockpit_mask(0);
#endif

	switch( Cockpit_mode )	
	{
	case CM_FULL_COCKPIT:
	case CM_REAR_VIEW:		{
		grs_bitmap *bm = &GameBitmaps[cockpit_bitmap[Cockpit_mode+(Current_display_mode?(Num_cockpits/2):0)].index];

		PIGGY_PAGE_IN(cockpit_bitmap[Cockpit_mode+(Current_display_mode?(Num_cockpits/2):0)]);

	#ifdef WINDOWS
		dd_gr_set_current_canvas(NULL);
		game_win_init_cockpit_mask(1);
		dd_gr_set_current_canvas(dd_VR_offscreen_buffer);
	#else
		gr_set_current_canvas(VR_offscreen_buffer)
	#endif		

	WIN(DDGRLOCK(dd_grd_curcanv));
		gr_bitmap( 0, 0, bm );
		bm = &VR_offscreen_buffer->cv_bitmap;
		bm->bm_flags = BM_FLAG_TRANSPARENT;
		gr_ibitblt_find_hole_size ( bm, &minx, &miny, &maxx, &maxy );
	WIN(	win_get_span_list(bm, miny, maxy);
			DDGRUNLOCK(dd_grd_curcanv)
	);

#ifndef WINDOWS
	#if 1 // def MACINTOSH
			gr_ibitblt_create_mask( bm, minx, miny, maxx-minx+1, maxy-miny+1, VR_offscreen_buffer->cv_bitmap.bm_rowsize);
	#else
		if ( Current_display_mode )
      {
		#if defined(POLY_ACC)
			Game_cockpit_copy_code  = gr_ibitblt_create_mask_pa( bm, minx, miny, maxx-minx+1, maxy-miny+1, VR_offscreen_buffer->cv_bitmap.bm_rowsize );
			pa_clear_buffer(1, 0);      // clear offscreen to reduce white flash.
		#else
			Game_cockpit_copy_code  = gr_ibitblt_create_mask_svga( bm, minx, miny, maxx-minx+1, maxy-miny+1, VR_offscreen_buffer->cv_bitmap.bm_rowsize );
		#endif
		} else
			Game_cockpit_copy_code  = gr_ibitblt_create_mask( bm, minx, miny, maxx-minx+1, maxy-miny+1, VR_offscreen_buffer->cv_bitmap.bm_rowsize );
	#endif
		bm->bm_flags = 0;		// Clear all flags for offscreen canvas
#else
		Game_cockpit_copy_code  = (ubyte *)(1); 
		bm->bm_flags = 0;		// Clear all flags for offscreen canvas
#endif
		game_init_render_sub_buffers( 0, 0, maxx-minx+1, maxy-miny+1 );
		break;
		}

	case CM_FULL_SCREEN:

		max_window_h = grd_curscreen->sc_h;

		if (Game_window_h > max_window_h || VR_screen_flags&VRF_ALLOW_COCKPIT)
			Game_window_h = max_window_h;

		if (Game_window_w > max_window_w || VR_screen_flags&VRF_ALLOW_COCKPIT)
			Game_window_w = max_window_w;

		Game_window_x = (max_window_w - Game_window_w)/2;
		Game_window_y = (max_window_h - Game_window_h)/2;

		game_init_render_sub_buffers( Game_window_x, Game_window_y, Game_window_w, Game_window_h );
		break;

	case CM_STATUS_BAR:

     	max_window_h = grd_curscreen->sc_h - GameBitmaps[cockpit_bitmap[CM_STATUS_BAR+(Current_display_mode?(Num_cockpits/2):0)].index].bm_h;

		if (Game_window_h > max_window_h)
			Game_window_h = max_window_h;

		if (Game_window_w > max_window_w)
			Game_window_w = max_window_w;

		Game_window_x = (max_window_w - Game_window_w)/2;
		Game_window_y = (max_window_h - Game_window_h)/2;

		game_init_render_sub_buffers( Game_window_x, Game_window_y, Game_window_w, Game_window_h );
		break;

	case CM_LETTERBOX:	{
		int x,y,w,h;

		x = 0; w = VR_render_buffer[0].cv_bitmap.bm_w;		//VR_render_width;
		h = (VR_render_buffer[0].cv_bitmap.bm_h * 7) / 10;
		y = (VR_render_buffer[0].cv_bitmap.bm_h-h)/2;

		game_init_render_sub_buffers( x, y, w, h );
		break;
		}

	}

WINDOS(
	dd_gr_set_current_canvas(NULL),
	gr_set_current_canvas(NULL)
);
}

//selects a given cockpit (or lack of one).  See types in game.h
void select_cockpit(int mode)
{
	if (mode != Cockpit_mode) {		//new mode
		Cockpit_mode=mode;
		init_cockpit();
	}
}

extern int last_drawn_cockpit[2];

//force cockpit redraw next time. call this if you've trashed the screen
void reset_cockpit()
{
	force_cockpit_redraw=1;
	last_drawn_cockpit[0] = -1;
	last_drawn_cockpit[1] = -1;
}

// void HUD_clear_messages();				//Already declared in gauges.h

//NEWVR
void VR_reset_params()
{
	VR_eye_width = VR_SEPARATION;
	VR_eye_offset = VR_PIXEL_SHIFT;
	VR_eye_offset_changed = 2;
}

void game_init_render_sub_buffers( int x, int y, int w, int h )
{
#ifdef WINDOWS
	dd_gr_init_sub_canvas( &dd_VR_render_sub_buffer[0], &dd_VR_render_buffer[0], x, y, w, h );
	dd_gr_init_sub_canvas( &dd_VR_render_sub_buffer[1], &dd_VR_render_buffer[1], x, y, w, h );

	dd_VR_render_sub_buffer[0].canvas.cv_bitmap.bm_x = 0;
	dd_VR_render_sub_buffer[0].canvas.cv_bitmap.bm_y = 0;
	dd_VR_render_sub_buffer[0].xoff = x;
	dd_VR_render_sub_buffer[0].yoff = y;
	dd_VR_render_sub_buffer[1].canvas.cv_bitmap.bm_x = 0;
	dd_VR_render_sub_buffer[1].canvas.cv_bitmap.bm_y = 0;
	dd_VR_render_sub_buffer[1].xoff = x;
	dd_VR_render_sub_buffer[1].yoff = y;

#endif
	if (Scanline_double) {
		#ifdef MACINTOSH
		if ( w & 0x3 )
			w &= ~0x3;
		gr_init_sub_canvas( &VR_render_sub_buffer[0], &VR_render_buffer[0], x, y, w/2, (h/2)+1);
		gr_init_sub_canvas( &VR_render_sub_buffer[1], &VR_render_buffer[1], x, y, w/2, (h/2)+1);
		#endif
	} else {
		gr_init_sub_canvas( &VR_render_sub_buffer[0], &VR_render_buffer[0], x, y, w, h );
		gr_init_sub_canvas( &VR_render_sub_buffer[1], &VR_render_buffer[1], x, y, w, h );
	}

	#ifdef MACINTOSH
		#ifdef POLY_ACC
			if ( PAEnabled )
			{
				TQARect	newBounds;
				
				newBounds.left = x;
				newBounds.right = x + w;
				newBounds.top = y;
				newBounds.bottom = y + h;
				
				pa_set_context(kGamePlayDrawContextID, &newBounds);		// must resize/create new context
			}
		#endif
	#endif

#ifdef WINDOWS
	VR_render_sub_buffer[0].cv_bitmap.bm_x = 0;
	VR_render_sub_buffer[0].cv_bitmap.bm_y = 0;
	VR_render_sub_buffer[1].cv_bitmap.bm_x = 0;
	VR_render_sub_buffer[1].cv_bitmap.bm_y = 0;
#endif
}


#ifdef WINDOWS
// Sets up the canvases we will be rendering to (WIN95)
void game_init_render_buffers(int screen_mode, int render_w, int render_h, int render_method, int flags )
{
//	Hack for switching to higher that 640x480 modes (DDraw doesn't allow
//	 creating surfaces greater than the current resolution

	if (GRMODEINFO(rw) < render_w || GRMODEINFO(rh) < render_h) {
		render_w = GRMODEINFO(rw);
		render_h = GRMODEINFO(rh);
	}

	VR_screen_mode		= screen_mode;

	VR_screen_flags	=  flags;

	VR_reset_params();
	VR_render_mode 	= render_method;

	Game_window_w 		= render_w;
	Game_window_h		= render_h;

	if (dd_VR_offscreen_buffer && dd_VR_offscreen_buffer != dd_grd_backcanv) {
		dd_gr_free_canvas(dd_VR_offscreen_buffer);
	}

	if ( (VR_render_mode==VR_AREA_DET) || (VR_render_mode==VR_INTERLACED ) )	{
		if ( render_h*2 < 200 )	{
		 	Int3();		// Not Supported yet!!!
//			VR_offscreen_buffer = gr_create_canvas( render_w, 200 );
		}
		else {
			Int3();		// Not Supported yet!!!
//			VR_offscreen_buffer = gr_create_canvas( render_w, render_h*2 );
		}

		Int3();			// Not Supported yet!!!
//		gr_init_sub_canvas( &VR_render_buffer[0], VR_offscreen_buffer, 0, 0, render_w, render_h );
//		gr_init_sub_canvas( &VR_render_buffer[1], VR_offscreen_buffer, 0, render_h, render_w, render_h );
	}
	else if (GRMODEINFO(paged) && !GRMODEINFO(dbuf)) {
	//	Here we will make the VR_offscreen_buffer the 2nd page and hopefully
	//	we can just flip it, saving a blt.  

		dd_VR_offscreen_buffer = dd_grd_backcanv;
		VR_offscreen_buffer = & dd_grd_backcanv->canvas;
	}
	else if (GRMODEINFO(dbuf)||GRMODEINFO(emul)) {
	//	The offscreen buffer will be created.  We will just blt this
	//	to the screen (which may be blted to the primary surface)
		if ( render_h < 200 ) {
			dd_VR_offscreen_buffer = dd_gr_create_canvas(render_w, 200);
			VR_offscreen_buffer = &dd_VR_offscreen_buffer->canvas;
		}
		else {
			dd_VR_offscreen_buffer = dd_gr_create_canvas(render_w, render_h);
			VR_offscreen_buffer = &dd_VR_offscreen_buffer->canvas;
		}
	}

	dd_gr_init_sub_canvas( &dd_VR_render_buffer[0], dd_VR_offscreen_buffer, 0, 0, render_w, render_h );
	dd_gr_init_sub_canvas( &dd_VR_render_buffer[1], dd_VR_offscreen_buffer, 0, 0, render_w, render_h );
	gr_init_sub_canvas( &VR_render_buffer[0], VR_offscreen_buffer, 0, 0, render_w, render_h );
	gr_init_sub_canvas( &VR_render_buffer[1], VR_offscreen_buffer, 0, 0, render_w, render_h );

	game_init_render_sub_buffers( 0, 0, render_w, render_h );
}

#else

// Sets up the canvases we will be rendering to (NORMAL VERSION)
void game_init_render_buffers(int screen_mode, int render_w, int render_h, int render_method, int flags )
{
//	if (vga_check_mode(screen_mode) != 0) 
//		Error("Cannot set requested video mode");

	VR_screen_mode		=	screen_mode;

	VR_screen_flags	=  flags;

//NEWVR
	VR_reset_params();
	VR_render_mode 	= render_method;

	Game_window_w 		= render_w;
	Game_window_h		= render_h;

	if (VR_offscreen_buffer) {
		gr_free_canvas(VR_offscreen_buffer);
	}

	if ( (VR_render_mode==VR_AREA_DET) || (VR_render_mode==VR_INTERLACED ) )	{
		if ( render_h*2 < 200 )	{
			VR_offscreen_buffer = gr_create_canvas( render_w, 200 );
		}
		else {
			VR_offscreen_buffer = gr_create_canvas( render_w, render_h*2 );
		}

		gr_init_sub_canvas( &VR_render_buffer[0], VR_offscreen_buffer, 0, 0, render_w, render_h );
		gr_init_sub_canvas( &VR_render_buffer[1], VR_offscreen_buffer, 0, render_h, render_w, render_h );
	}
	else {
		if ( render_h < 200 ) {
			VR_offscreen_buffer = gr_create_canvas( render_w, 200 );
		}
		else {
#if defined(POLY_ACC)
			#ifndef MACINTOSH
            VR_offscreen_buffer = gr_create_canvas( render_w, render_h );
            d_free(VR_offscreen_buffer->cv_bitmap.bm_data);
            gr_init_canvas(VR_offscreen_buffer, pa_get_buffer_address(1), BM_LINEAR15, render_w, render_h);
			#else
				if ( PAEnabled || gConfigInfo.mAcceleration ) {
					Cockpit_mode=CM_FULL_SCREEN;		// HACK HACK HACK HACK HACK!!!!
					VR_offscreen_buffer = gr_create_canvas2(render_w, render_h, BM_LINEAR15);
				} else
	            VR_offscreen_buffer = gr_create_canvas( render_w, render_h );
			#endif
#else
            VR_offscreen_buffer = gr_create_canvas( render_w, render_h );
#endif
        }

#ifdef OGL
		VR_offscreen_buffer->cv_bitmap.bm_type = BM_OGL;
#endif

		gr_init_sub_canvas( &VR_render_buffer[0], VR_offscreen_buffer, 0, 0, render_w, render_h );
		gr_init_sub_canvas( &VR_render_buffer[1], VR_offscreen_buffer, 0, 0, render_w, render_h );
	}

	game_init_render_sub_buffers( 0, 0, render_w, render_h );
}
#endif

//called to get the screen in a mode compatible with popup menus.
//if we can't have popups over the game screen, switch to menu mode.
void set_popup_screen(void)
{
	//WIN(LoadCursorWin(MOUSE_DEFAULT_CURSOR));

	if (! (VR_screen_flags & VRF_COMPATIBLE_MENUS))
		set_screen_mode(SCREEN_MENU);		//must switch to menu mode
}


//called to change the screen mode. Parameter sm is the new mode, one of
//SMODE_GAME or SMODE_EDITOR. returns mode acutally set (could be other
//mode if cannot init requested mode)
int set_screen_mode(int sm)
{
WIN(static int force_mode_change=0);
WIN(static int saved_window_w);
WIN(static int saved_window_h);

#ifdef EDITOR
	if ( (sm==SCREEN_MENU) && (Screen_mode==SCREEN_EDITOR) )	{
		gr_set_current_canvas( Canv_editor );
		return 1;
	}
#endif

#ifdef WINDOWS
	if ( Screen_mode == sm && W95DisplayMode == VR_screen_mode) {
		dd_gr_set_current_canvas( &dd_VR_screen_pages[VR_current_page] );
		return 1;
	}
#else
	if ( Screen_mode == sm && VGA_current_mode == VR_screen_mode) {
		gr_set_current_canvas( &VR_screen_pages[VR_current_page] );
		return 1;
	}
#endif

#ifdef OGL
	if ((Screen_mode == sm) && !((sm==SCREEN_GAME) && (grd_curscreen->sc_mode != VR_screen_mode) && (Screen_mode == SCREEN_GAME))) {
             
		gr_set_current_canvas( &VR_screen_pages[VR_current_page] );
		ogl_set_screen_mode();
		return 1;
	}
#endif

#ifdef EDITOR
	Canv_editor = NULL;
#endif

	Screen_mode = sm;

	switch( Screen_mode )
	{
		case SCREEN_MENU:
		#ifdef WINDOWS
			//mouse_set_mode(0);
			//ShowCursorW();
			if (!(VR_screen_flags & VRF_COMPATIBLE_MENUS)) {
			// HACK!!!  Meant to save window size when switching from
			// non-compat menu mode to menu mode.
				saved_window_w = Game_window_w;
				saved_window_h = Game_window_h; 
				force_mode_change = 1;
			}
			if (W95DisplayMode != SM95_640x480x8) {
//@@				piggy_bitmap_page_out_all_w();		// 2D GFX Flush cache.
				DDSETDISPLAYMODE(SM95_640x480x8);
				dd_gr_init_screen();
				if (!gr_palette_faded_out) gr_palette_load(gr_palette);
			}

			dd_gr_init_sub_canvas(&dd_VR_screen_pages[0], dd_grd_screencanv,
									0,0,
									dd_grd_screencanv->canvas.cv_bitmap.bm_w,
									dd_grd_screencanv->canvas.cv_bitmap.bm_h);
			dd_gr_init_sub_canvas(&dd_VR_screen_pages[1], dd_grd_screencanv,
									0,0,
									dd_grd_screencanv->canvas.cv_bitmap.bm_w,
									dd_grd_screencanv->canvas.cv_bitmap.bm_h);
			MenuHires = 1;
			FontHires = 1;

		#else
		{
			int menu_mode;

			MenuHires = MenuHiresAvailable;		//do highres if we can

#if defined(POLY_ACC)
				#ifndef MACINTOSH
	            menu_mode = MenuHires?SM(640,480):SM(320,200);
				#else
					menu_mode = PAEnabled?SM_640x480x15xPA:SM_640x480V;
				#endif
#else
            menu_mode = MenuHires?SM(640,480):SM(320,200);
#endif

			if (VGA_current_mode != menu_mode) {
				if (gr_set_mode(menu_mode))
					Error("Cannot set screen mode for menu");
				if (!gr_palette_faded_out)
					gr_palette_load(gr_palette);
			}

			gr_init_sub_canvas( &VR_screen_pages[0], &grd_curscreen->sc_canvas, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h );
			gr_init_sub_canvas( &VR_screen_pages[1], &grd_curscreen->sc_canvas, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h );

			FontHires = MenuHires;

		}
		#endif
		break;

	case SCREEN_GAME:
	#ifdef WINDOWS
		//mouse_set_mode(1);
		HideCursorW();
		if (force_mode_change || (W95DisplayMode != VR_screen_mode)) {

			DDSETDISPLAYMODE(VR_screen_mode);
//@@			piggy_bitmap_page_out_all_w();		// 2D GFX Flush cache.
			dd_gr_init_screen();
			mprintf((0, "Reinitializing render buffers due to display mode change.\n"));
			game_init_render_buffers(W95DisplayMode,
					GRMODEINFO(rw), GRMODEINFO(rh),
					VR_render_mode, VR_screen_flags);

			reset_cockpit();
		}
	#else
		if (VGA_current_mode != VR_screen_mode) {
			if (gr_set_mode(VR_screen_mode))	{
				Error("Cannot set desired screen mode for game!");
				//we probably should do something else here, like select a standard mode
			}
			#ifdef MACINTOSH
			if ( (Config_control_type == 1) && (Function_mode == FMODE_GAME) )
				joydefs_calibrate();
			#endif
			reset_cockpit();
		}
	#endif

		if ( VR_render_mode == VR_NONE )
		{
			max_window_w = grd_curscreen->sc_w;
			max_window_h = grd_curscreen->sc_h;

			if (VR_screen_flags & VRF_ALLOW_COCKPIT) {
				if (Cockpit_mode == CM_STATUS_BAR)
		      	max_window_h = grd_curscreen->sc_h - GameBitmaps[cockpit_bitmap[CM_STATUS_BAR+(Current_display_mode?(Num_cockpits/2):0)].index].bm_h;
			}
			else if (Cockpit_mode != CM_LETTERBOX) 
				Cockpit_mode = CM_FULL_SCREEN;

	      if (Game_window_h==0 || Game_window_h > max_window_h || Game_window_w==0 || Game_window_w > max_window_w) {
				Game_window_w = max_window_w;
				Game_window_h = max_window_h;
	      }

		}
		else
			Cockpit_mode = CM_FULL_SCREEN;

		#ifdef WINDOWS
		//	Super hack.   If we are switching from a 320x200 game to 640x480.
		//						and we were in a menumode when switching, we don't
		//						restore Game_window vals
			if (force_mode_change && (W95DisplayMode == SM95_320x200x8X)) {
				Game_window_w = saved_window_w;
				Game_window_h = saved_window_h;
				force_mode_change = 0;
			}
		#endif


	//	Define screen pages for game mode
	// If we designate through screen_flags to use paging, then do so.
		WINDOS(
			dd_gr_init_sub_canvas( &dd_VR_screen_pages[0], dd_grd_screencanv, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h ),
			gr_init_sub_canvas( &VR_screen_pages[0], &grd_curscreen->sc_canvas, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h )
		);

		if ( VR_screen_flags&VRF_USE_PAGING ) {
		WINDOS(
			dd_gr_init_sub_canvas( &dd_VR_screen_pages[1], dd_grd_backcanv, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h ),
			gr_init_sub_canvas( &VR_screen_pages[1], &grd_curscreen->sc_canvas, 0, grd_curscreen->sc_h, grd_curscreen->sc_w, grd_curscreen->sc_h )
		);
		}
		else {
		WINDOS (
			dd_gr_init_sub_canvas( &dd_VR_screen_pages[1], dd_grd_screencanv, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h ),
			gr_init_sub_canvas( &VR_screen_pages[1], &grd_curscreen->sc_canvas, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h )
		);
		}

		init_cockpit();

	#ifdef WINDOWS
		FontHires = (Current_display_mode != 0);
		MenuHires = 1;
	#else
		FontHires = MenuHires = ((Current_display_mode != 0) && (Current_display_mode != 2));
	#endif

		if ( VR_render_mode != VR_NONE )	{
			// for 640x480 or higher, use hires font.
			if ( grd_curscreen->sc_h > 400 )
				FontHires = 1;
			else
				FontHires = 0;
		}

		break;
	#ifdef EDITOR
	case SCREEN_EDITOR:
		if (VGA_current_mode != SM_800x600V)	{
			int gr_error;
			if ((gr_error=gr_set_mode(SM_800x600V))!=0) { //force into game scrren
				Warning("Cannot init editor screen (error=%d)",gr_error);
				return 0;
			}
		}
		gr_palette_load( gr_palette );

		gr_init_sub_canvas( &VR_editor_canvas, &grd_curscreen->sc_canvas, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h );
		Canv_editor = &VR_editor_canvas;
		gr_init_sub_canvas( &VR_screen_pages[0], Canv_editor, 0, 0, Canv_editor->cv_w, Canv_editor->cv_h );
		gr_init_sub_canvas( &VR_screen_pages[1], Canv_editor, 0, 0, Canv_editor->cv_w, Canv_editor->cv_h );
		gr_set_current_canvas( Canv_editor );
		init_editor_screen();   //setup other editor stuff
		break;
	#endif
	default:
		Error("Invalid screen mode %d",sm);
	}

	VR_current_page = 0;

	WINDOS(
		dd_gr_set_current_canvas(&dd_VR_screen_pages[VR_current_page]),
		gr_set_current_canvas( &VR_screen_pages[VR_current_page] )
	);

	if ( VR_screen_flags&VRF_USE_PAGING )	{
	WINDOS(
		dd_gr_flip(),
		gr_show_canvas( &VR_screen_pages[VR_current_page] )
	);
	}
#ifdef OGL
	ogl_set_screen_mode();
#endif

	return 1;
}

static int timer_paused=0;

void stop_time()
{
	if (timer_paused==0) {
		fix time;
		time = timer_get_fixed_seconds();
		last_timer_value = time - last_timer_value;
		if (last_timer_value < 0) {
			#if defined(TIMER_TEST) && !defined(NDEBUG)
			Int3();		//get Matt!!!!
			#endif
			last_timer_value = 0;
		}
		#if defined(TIMER_TEST) && !defined(NDEBUG)
		time_stopped = time;
		#endif
	}
	timer_paused++;

	#if defined(TIMER_TEST) && !defined(NDEBUG)
	stop_count++;
	#endif
}

void start_time()
{
	timer_paused--;
	Assert(timer_paused >= 0);
	if (timer_paused==0) {
		fix time;
		time = timer_get_fixed_seconds();
		#if defined(TIMER_TEST) && !defined(NDEBUG)
		if (last_timer_value < 0)
			Int3();		//get Matt!!!!
		}
		#endif
		last_timer_value = time - last_timer_value;
		#if defined(TIMER_TEST) && !defined(NDEBUG)
		time_started = time;
		#endif
	}

	#if defined(TIMER_TEST) && !defined(NDEBUG)
	start_count++;
	#endif
}

MAC(extern ubyte joydefs_calibrating;)

void game_flush_inputs()
{
	int dx,dy;
	key_flush();
	joy_flush();
	mouse_flush();
	#ifdef MACINTOSH
	if ( (Function_mode != FMODE_MENU) && !joydefs_calibrating )		// only reset mouse when not in menu or not calibrating
	#endif
		mouse_get_delta( &dx, &dy );	// Read mouse
	memset(&Controls,0,sizeof(control_info));
}

void reset_time()
{
	last_timer_value = timer_get_fixed_seconds();

}

#ifndef RELEASE
extern int Saving_movie_frames;
int Movie_fixed_frametime;
#else
#define Saving_movie_frames	0
#define Movie_fixed_frametime	0
#endif

static const int max_fps = 80;

void calc_frame_time()
{
	fix timer_value,last_frametime = FrameTime;

	#if defined(TIMER_TEST) && !defined(NDEBUG)
	_last_frametime = last_frametime;
	#endif

	timer_value = timer_get_fixed_seconds();
	FrameTime = timer_value - last_timer_value;

	do {
	    timer_value = timer_get_fixed_seconds();
	    FrameTime = timer_value - last_timer_value;
	    if (FrameTime < f1_0/max_fps);
	    {
		d_delay(1);
	    }
	} while (FrameTime < f1_0/max_fps);

	#if defined(TIMER_TEST) && !defined(NDEBUG)
	_timer_value = timer_value;
	#endif

	#ifndef NDEBUG
	if (!(((FrameTime > 0) && (FrameTime <= F1_0)) || (Function_mode == FMODE_EDITOR) || (Newdemo_state == ND_STATE_PLAYBACK))) {
		mprintf((1,"Bad FrameTime - value = %x\n",FrameTime));
		if (FrameTime == 0)
			Int3();	//	Call Mike or Matt or John!  Your interrupts are probably trashed!
//		if ( !dpmi_virtual_memory )
//			Int3();		//Get MATT if hit this!
	}
	#endif

	#if defined(TIMER_TEST) && !defined(NDEBUG)
	actual_last_timer_value = last_timer_value;
	#endif

	if ( Game_turbo_mode )
		FrameTime *= 2;

	// Limit frametime to be between 5 and 150 fps.
	RealFrameTime = FrameTime;
	if ( FrameTime < F1_0/150 ) FrameTime = F1_0/150;
	if ( FrameTime > F1_0/5 ) FrameTime = F1_0/5;

	last_timer_value = timer_value;

	if (FrameTime < 0)						//if bogus frametime...
		FrameTime = last_frametime;		//...then use time from last frame

	#ifndef NDEBUG
	if (fixed_frametime) FrameTime = fixed_frametime;
	#endif

	#ifndef NDEBUG
	// Pause here!!!
	if ( Debug_pause )      {
		int c;
		c = 0;
		while( c==0 )
			c = key_peekkey();

		if ( c == KEY_P )       {
			Debug_pause = 0;
			c = key_inkey();
		}
		last_timer_value = timer_get_fixed_seconds();
	}
	#endif

	#if Arcade_mode
		FrameTime /= 2;
	#endif

	#if defined(TIMER_TEST) && !defined(NDEBUG)
	stop_count = start_count = 0;
	#endif

	//	Set value to determine whether homing missile can see target.
	//	The lower frametime is, the more likely that it can see its target.
	if (FrameTime <= F1_0/64)
		Min_trackable_dot = MIN_TRACKABLE_DOT;	// -- 3*(F1_0 - MIN_TRACKABLE_DOT)/4 + MIN_TRACKABLE_DOT;
	else if (FrameTime < F1_0/32)
		Min_trackable_dot = MIN_TRACKABLE_DOT + F1_0/64 - 2*FrameTime;	// -- fixmul(F1_0 - MIN_TRACKABLE_DOT, F1_0-4*FrameTime) + MIN_TRACKABLE_DOT;
	else if (FrameTime < F1_0/4)
		Min_trackable_dot = MIN_TRACKABLE_DOT + F1_0/64 - F1_0/16 - FrameTime;	// -- fixmul(F1_0 - MIN_TRACKABLE_DOT, F1_0-4*FrameTime) + MIN_TRACKABLE_DOT;
	else
		Min_trackable_dot = MIN_TRACKABLE_DOT + F1_0/64 - F1_0/8;

}

//--unused-- int Auto_flythrough=0;  //if set, start flythough automatically

void move_player_2_segment(segment *seg,int side)
{
	vms_vector vp;

	compute_segment_center(&ConsoleObject->pos,seg);
	compute_center_point_on_side(&vp,seg,side);
	vm_vec_sub2(&vp,&ConsoleObject->pos);
	vm_vector_2_matrix(&ConsoleObject->orient,&vp,NULL,NULL);

	obj_relink( ConsoleObject-Objects, SEG_PTR_2_NUM(seg) );

}

#ifdef NETWORK
void game_draw_time_left()
{
        char temp_string[30];
        fix timevar;
        int i;

        gr_set_curfont( GAME_FONT );    //GAME_FONT
        gr_set_fontcolor(gr_getcolor(0,63,0), -1 );

        timevar=i2f (Netgame.PlayTimeAllowed*5*60);
        i=f2i(timevar-ThisLevelTime);
        i++;

        sprintf( temp_string, "Time left: %d secs", i );

        if (i>=0)
         gr_string(0, 32, temp_string );
}
#endif


extern int Game_pause;

void do_photos();
void level_with_floor();

void modex_clear_box(int x,int y,int w,int h)
{
	grs_canvas *temp_canv,*save_canv;

	save_canv = grd_curcanv;
	temp_canv = gr_create_canvas(w,h);
	gr_set_current_canvas(temp_canv);
	gr_clear_canvas(BM_XRGB(0,0,0));
	gr_set_current_canvas(save_canv);
	gr_bitmapm(x,y,&temp_canv->cv_bitmap);
	gr_free_canvas(temp_canv);

}

extern void modex_printf(int x,int y,char *s,grs_font *font,int color);

// mac routine to drop contents of screen to a pict file using copybits
// save a PICT to a file
#ifdef MACINTOSH

void SavePictScreen(int multiplayer)
{
	OSErr err;
	int parid, i, count;
	char *pfilename, filename[50], buf[512], cwd[FILENAME_MAX];
	short fd;
	FSSpec spec;
	PicHandle pict_handle;
	static int multi_count = 0;
	StandardFileReply sf_reply;
	
// dump the contents of the GameWindow into a picture using copybits

	pict_handle = OpenPicture(&GameWindow->portRect);
	if (pict_handle == NULL)
		return;
		
	CopyBits(&GameWindow->portBits, &GameWindow->portBits, &GameWindow->portRect, &GameWindow->portRect, srcBic, NULL);
	ClosePicture();

// get the cwd to restore with chdir when done -- this keeps the mac world sane
	if (!getcwd(cwd, FILENAME_MAX))
		Int3();
// create the fsspec

	sprintf(filename, "screen%d", multi_count++);
	pfilename = c2pstr(filename);
	if (!multiplayer) {
		show_cursor();
		StandardPutFile("\pSave PICT as:", pfilename, &sf_reply);
		if (!sf_reply.sfGood) 
			goto end;
		memcpy( &spec, &(sf_reply.sfFile), sizeof(FSSpec) );
		if (sf_reply.sfReplacing)
			FSpDelete(&spec);
		err = FSpCreate( &spec, 'ttxt', 'PICT', smSystemScript );
		if (err)
			goto end;
	} else {
//		parid = GetAppDirId();
		err = FSMakeFSSpec(0, 0, pfilename, &spec);
		if (err == nsvErr)
			goto end;
		if (err != fnfErr)
			FSpDelete(&spec);
		err = FSpCreate(&spec, 'ttxt', 'PICT', smSystemScript);
		if (err != 0)
			goto end;
	}

// write the PICT file
	if ( FSpOpenDF(&spec, fsRdWrPerm, &fd) )
		goto end;
	memset(buf, 0, sizeof(buf));
	count = 512;
	if ( FSWrite(fd, &count, buf) )
		goto end;
	count = GetHandleSize((Handle)pict_handle);
	HLock((Handle)pict_handle);
	if ( FSWrite(fd, &count, *pict_handle) ) {
		FSClose(fd);
		FSpDelete(&spec);
	}

end:
	HUnlock((Handle)pict_handle);
	DisposeHandle((Handle)pict_handle);
	FSClose(fd);
	hide_cursor();
	chdir(cwd);
}

#endif

//automap_flag is now unused, since we just check if the screen we're
//writing to is modex
//if called from automap, current canvas is set to visible screen
#ifndef OGL
void save_screen_shot(int automap_flag)
{
#if defined(WINDOWS)
	mprintf((0, "Doing screen shot thing.\n"));
	win95_save_pcx_shot();

#elif !defined(MACINTOSH)
	fix t1;
	char message[100];
	grs_canvas *screen_canv=&grd_curscreen->sc_canvas;
	grs_font *save_font;
	static int savenum=0;
	static int stereo_savenum=0;
	grs_canvas *temp_canv,*temp_canv2,*save_canv;
        char savename[FILENAME_LEN],savename2[FILENAME_LEN];
	ubyte pal[768];
	int w,h,aw,x,y;
	int modex_flag;
	int stereo=0;

	temp_canv2=NULL;

//	// Can't do screen shots in VR modes.
//	if ( VR_render_mode != VR_NONE )
//		return;

	stop_time();

	save_canv = grd_curcanv;

	if ( VR_render_mode != VR_NONE && !automap_flag && Function_mode==FMODE_GAME && Screen_mode==SCREEN_GAME)
		stereo = 1;

	if ( stereo ) {
		temp_canv = gr_create_canvas(VR_render_buffer[0].cv_bitmap.bm_w,VR_render_buffer[0].cv_bitmap.bm_h);
		gr_set_current_canvas(temp_canv);
		gr_ubitmap(0,0,&VR_render_buffer[0].cv_bitmap);

		temp_canv2 = gr_create_canvas(VR_render_buffer[1].cv_bitmap.bm_w,VR_render_buffer[1].cv_bitmap.bm_h);
		gr_set_current_canvas(temp_canv2);
		gr_ubitmap(0,0,&VR_render_buffer[1].cv_bitmap);
	}
	else {
		temp_canv = gr_create_canvas(screen_canv->cv_bitmap.bm_w,screen_canv->cv_bitmap.bm_h);
		gr_set_current_canvas(temp_canv);
		gr_ubitmap(0,0,&screen_canv->cv_bitmap);
	}

	gr_set_current_canvas(save_canv);

	if ( savenum > 99 ) savenum = 0;
	if ( stereo_savenum > 99 ) stereo_savenum = 0;

	if ( stereo ) {
		sprintf(savename,"left%02d.pcx",stereo_savenum);
		sprintf(savename2,"right%02d.pcx",stereo_savenum);
		if (VR_eye_switch) {char t[FILENAME_LEN]; strcpy(t,savename); strcpy(savename,savename2); strcpy(savename2,t);}
		stereo_savenum++;
		sprintf( message, "%s '%s' & '%s'", TXT_DUMPING_SCREEN, savename, savename2 );
	}
	else {
		sprintf(savename,"screen%02d.pcx",savenum++);
		sprintf( message, "%s '%s'", TXT_DUMPING_SCREEN, savename );
	}

	if (!automap_flag)		//if from automap, curcanv is already visible canv
		gr_set_current_canvas(NULL);
	modex_flag = (grd_curcanv->cv_bitmap.bm_type==BM_MODEX);
	if (!automap_flag && modex_flag)
		gr_set_current_canvas(&VR_screen_pages[VR_current_page]);

	save_font = grd_curcanv->cv_font;
	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(gr_find_closest_color_current(0,31,0),-1);
	gr_get_string_size(message,&w,&h,&aw);

	if (modex_flag)
		h *= 2;

	//I changed how these coords were calculated for the high-res automap. -MT 
	//x = (VR_screen_pages[VR_current_page].cv_w-w)/2;
	//y = (VR_screen_pages[VR_current_page].cv_h-h)/2;
	x = (grd_curcanv->cv_w-w)/2;
	y = (grd_curcanv->cv_h-h)/2;

	if (modex_flag) {
		modex_clear_box(x-2,y-2,w+4,h+4);
		modex_printf(x, y, message,GAME_FONT,gr_find_closest_color_current(0,31,0));
	} else {
		gr_setcolor(gr_find_closest_color_current(0,0,0));
		gr_rect(x-2,y-2,x+w+2,y+h+2);
		gr_printf(x,y,message);
		gr_set_curfont(save_font);
	}
	t1 = timer_get_fixed_seconds() + F1_0;

	gr_palette_read(pal);		//get actual palette from the hardware
	pcx_write_bitmap(savename,&temp_canv->cv_bitmap,pal);
	if ( stereo )
		pcx_write_bitmap(savename2,&temp_canv2->cv_bitmap,pal);

	while ( timer_get_fixed_seconds() < t1 );		// Wait so that messag stays up at least 1 second.

	gr_set_current_canvas(screen_canv);

	if (grd_curcanv->cv_bitmap.bm_type!=BM_MODEX && !stereo)
		gr_ubitmap(0,0,&temp_canv->cv_bitmap);

	gr_free_canvas(temp_canv);
	if ( stereo )
		gr_free_canvas(temp_canv2);

	gr_set_current_canvas(save_canv);
	key_flush();
	start_time();
	
#else

	grs_canvas *screen_canv = &grd_curscreen->sc_canvas;
	grs_canvas *temp_canv, *save_canv;
	
	// Can't do screen shots in VR modes.
	if ( VR_render_mode != VR_NONE )
		return;

	stop_time();

	save_canv = grd_curcanv;	
	temp_canv = gr_create_canvas( screen_canv->cv_bitmap.bm_w, screen_canv->cv_bitmap.bm_h );
	if (!temp_canv)
		goto shot_done;
	gr_set_current_canvas( temp_canv );
	gr_ubitmap( 0, 0, &screen_canv->cv_bitmap );
	gr_set_current_canvas( &VR_screen_pages[VR_current_page] );

	show_cursor();
	key_close();
	if (Game_mode & GM_MULTI)
		SavePictScreen(1);
	else
		SavePictScreen(0);
	key_init();
	hide_cursor();

	gr_set_current_canvas(screen_canv);
	
//	if (!automap_flag)
		gr_ubitmap( 0, 0, &temp_canv->cv_bitmap);

	gr_free_canvas(temp_canv);
shot_done:
	gr_set_current_canvas(save_canv);
	key_flush();
	start_time();
	#endif
}

#endif

//initialize flying
void fly_init(object *obj)
{
	obj->control_type = CT_FLYING;
	obj->movement_type = MT_PHYSICS;

	vm_vec_zero(&obj->mtype.phys_info.velocity);
	vm_vec_zero(&obj->mtype.phys_info.thrust);
	vm_vec_zero(&obj->mtype.phys_info.rotvel);
	vm_vec_zero(&obj->mtype.phys_info.rotthrust);
}

//void morph_test(), morph_step();


//	------------------------------------------------------------------------------------

void test_anim_states();

#include "fvi.h"

//put up the help message
void do_show_help()
{
	show_help();
}


extern int been_in_editor;

//	------------------------------------------------------------------------------------
void do_cloak_stuff(void)
{
	int i;
	for (i = 0; i < N_players; i++)
		if (Players[i].flags & PLAYER_FLAGS_CLOAKED) {
			// mprintf(0, "Cloak time left: %7.3f\n", f2fl(CLOAK_TIME_MAX - (GameTime - Players[Player_num].cloak_time)));
			if (GameTime - Players[i].cloak_time > CLOAK_TIME_MAX) {
				Players[i].flags &= ~PLAYER_FLAGS_CLOAKED;
				if (i == Player_num) {
					digi_play_sample( SOUND_CLOAK_OFF, F1_0);
					#ifdef NETWORK
					if (Game_mode & GM_MULTI)
						multi_send_play_sound(SOUND_CLOAK_OFF, F1_0);
					maybe_drop_net_powerup(POW_CLOAK);
					multi_send_decloak(); // For demo recording
					#endif
//					mprintf((0, " --- You have been DE-CLOAKED! ---\n"));
				}
			}
		}
}

int FakingInvul=0;

//	------------------------------------------------------------------------------------
void do_invulnerable_stuff(void)
{
	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) {
		if (GameTime - Players[Player_num].invulnerable_time > INVULNERABLE_TIME_MAX) {
			Players[Player_num].flags ^= PLAYER_FLAGS_INVULNERABLE;
			if (FakingInvul==0)
			{
				digi_play_sample( SOUND_INVULNERABILITY_OFF, F1_0);
				#ifdef NETWORK
				if (Game_mode & GM_MULTI)
				{
					multi_send_play_sound(SOUND_INVULNERABILITY_OFF, F1_0);
					maybe_drop_net_powerup(POW_INVULNERABILITY);
				}
				#endif
				mprintf((0, " --- You have been DE-INVULNERABLEIZED! ---\n"));
			}
			FakingInvul=0;
		}
	}
}

ubyte	Last_afterburner_state = 0;
fix Last_afterburner_charge = 0;

#define AFTERBURNER_LOOP_START	((digi_sample_rate==SAMPLE_RATE_22K)?32027:(32027/2))		//20098
#define AFTERBURNER_LOOP_END		((digi_sample_rate==SAMPLE_RATE_22K)?48452:(48452/2))		//25776

int	Ab_scale = 4;

//@@//	------------------------------------------------------------------------------------
//@@void afterburner_shake(void)
//@@{
//@@	int	rx, rz;
//@@
//@@	rx = (Ab_scale * fixmul(d_rand() - 16384, F1_0/8 + (((GameTime + 0x4000)*4) & 0x3fff)))/16;
//@@	rz = (Ab_scale * fixmul(d_rand() - 16384, F1_0/2 + ((GameTime*4) & 0xffff)))/16;
//@@
//@@	// -- mprintf((0, "AB: %8x %8x\n", rx, rz));
//@@	ConsoleObject->mtype.phys_info.rotvel.x += rx;
//@@	ConsoleObject->mtype.phys_info.rotvel.z += rz;
//@@
//@@}

//	------------------------------------------------------------------------------------
#ifdef NETWORK
extern void multi_send_sound_function (char,char);
#endif

void do_afterburner_stuff(void)
{
   if (!(Players[Player_num].flags & PLAYER_FLAGS_AFTERBURNER))
		Afterburner_charge=0;

	if (Endlevel_sequence || Player_is_dead)
		{
		 digi_kill_sound_linked_to_object( Players[Player_num].objnum);
#ifdef NETWORK
		 multi_send_sound_function (0,0);  
#endif
		}

	if ((Controls.afterburner_state != Last_afterburner_state && Last_afterburner_charge) || (Last_afterburner_state && Last_afterburner_charge && !Afterburner_charge)) {

		if (Afterburner_charge && Controls.afterburner_state && (Players[Player_num].flags & PLAYER_FLAGS_AFTERBURNER)) {
			digi_link_sound_to_object3( SOUND_AFTERBURNER_IGNITE, Players[Player_num].objnum, 1, F1_0, i2f(256), AFTERBURNER_LOOP_START, AFTERBURNER_LOOP_END );
#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_sound_function (3,SOUND_AFTERBURNER_IGNITE);
#endif
		} else {
			digi_kill_sound_linked_to_object( Players[Player_num].objnum);
			digi_link_sound_to_object2( SOUND_AFTERBURNER_PLAY, Players[Player_num].objnum, 0, F1_0, i2f(256));
#ifdef NETWORK
			if (Game_mode & GM_MULTI)
			 	multi_send_sound_function (0,0);  
#endif
			mprintf((0,"Killing afterburner sound\n"));
		}
	}

	//@@if (Controls.afterburner_state && Afterburner_charge)
	//@@	afterburner_shake();

	Last_afterburner_state = Controls.afterburner_state;
	Last_afterburner_charge = Afterburner_charge;
}

// -- //	------------------------------------------------------------------------------------
// -- //	if energy < F1_0/2, recharge up to F1_0/2
// -- void recharge_energy_frame(void)
// -- {
// -- 	if (Players[Player_num].energy < Weapon_info[0].energy_usage) {
// -- 		Players[Player_num].energy += FrameTime/4;
// --
// -- 		if (Players[Player_num].energy > Weapon_info[0].energy_usage)
// -- 			Players[Player_num].energy = Weapon_info[0].energy_usage;
// -- 	}
// -- }

//	Amount to diminish guns towards normal, per second.
#define	DIMINISH_RATE	16		//	gots to be a power of 2, else change the code in diminish_palette_towards_normal

extern fix Flash_effect;

 //adds to rgb values for palette flash
void PALETTE_FLASH_ADD(int _dr,int _dg,int _db)
{
	int	maxval;

	PaletteRedAdd += _dr;
	PaletteGreenAdd += _dg;
	PaletteBlueAdd += _db;

	// -- mprintf((0, "Palette add: %3i %3i %3i\n", PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd));

	if (Flash_effect)
		maxval = 60;
	else
		maxval = MAX_PALETTE_ADD;

	if (PaletteRedAdd > maxval)
		PaletteRedAdd = maxval;

	if (PaletteGreenAdd > maxval)
		PaletteGreenAdd = maxval;

	if (PaletteBlueAdd > maxval)
		PaletteBlueAdd = maxval;

	if (PaletteRedAdd < -maxval)
		PaletteRedAdd = -maxval;

	if (PaletteGreenAdd < -maxval)
		PaletteGreenAdd = -maxval;

	if (PaletteBlueAdd < -maxval)
		PaletteBlueAdd = -maxval;
}

fix	Time_flash_last_played;


void game_palette_step_up( int r, int g, int b );
//	------------------------------------------------------------------------------------
//	Diminish palette effects towards normal.
void diminish_palette_towards_normal(void)
{
	int	dec_amount = 0;

	//	Diminish at DIMINISH_RATE units/second.
	//	For frame rates > DIMINISH_RATE Hz, use randomness to achieve this.
	if (FrameTime < F1_0/DIMINISH_RATE) {
		if (d_rand() < FrameTime*DIMINISH_RATE/2)	//	Note: d_rand() is in 0..32767, and 8 Hz means decrement every frame
			dec_amount = 1;
	} else {
		dec_amount = f2i(FrameTime*DIMINISH_RATE);		// one second = DIMINISH_RATE counts
		if (dec_amount == 0)
			dec_amount++;						// make sure we decrement by something
	}

	if (Flash_effect) {
		int	force_do = 0;

		//	Part of hack system to force update of palette after exiting a menu.
		if (Time_flash_last_played) {
			force_do = 1;
			PaletteRedAdd ^= 1;	//	Very Tricky!  In gr_palette_step_up, if all stepups same as last time, won't do anything!
		}

		if ((Time_flash_last_played + F1_0/8 < GameTime) || (Time_flash_last_played > GameTime)) {
			digi_play_sample( SOUND_CLOAK_OFF, Flash_effect/4);
			Time_flash_last_played = GameTime;
		}

		Flash_effect -= FrameTime;
		if (Flash_effect < 0)
			Flash_effect = 0;

		if (force_do || (d_rand() > 4096 )) {
      	if ( (Newdemo_state==ND_STATE_RECORDING) && (PaletteRedAdd || PaletteGreenAdd || PaletteBlueAdd) )
	      	newdemo_record_palette_effect(PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd);

			game_palette_step_up( PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd );

			return;
		}

	}

	if (PaletteRedAdd > 0 ) { PaletteRedAdd -= dec_amount; if (PaletteRedAdd < 0 ) PaletteRedAdd = 0; }
	if (PaletteRedAdd < 0 ) { PaletteRedAdd += dec_amount; if (PaletteRedAdd > 0 ) PaletteRedAdd = 0; }

	if (PaletteGreenAdd > 0 ) { PaletteGreenAdd -= dec_amount; if (PaletteGreenAdd < 0 ) PaletteGreenAdd = 0; }
	if (PaletteGreenAdd < 0 ) { PaletteGreenAdd += dec_amount; if (PaletteGreenAdd > 0 ) PaletteGreenAdd = 0; }

	if (PaletteBlueAdd > 0 ) { PaletteBlueAdd -= dec_amount; if (PaletteBlueAdd < 0 ) PaletteBlueAdd = 0; }
	if (PaletteBlueAdd < 0 ) { PaletteBlueAdd += dec_amount; if (PaletteBlueAdd > 0 ) PaletteBlueAdd = 0; }

	if ( (Newdemo_state==ND_STATE_RECORDING) && (PaletteRedAdd || PaletteGreenAdd || PaletteBlueAdd) )
		newdemo_record_palette_effect(PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd);

	game_palette_step_up( PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd );

	//mprintf(0, "%2i %2i %2i\n", PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd);
}

int	Redsave, Bluesave, Greensave;

void palette_save(void)
{
	Redsave = PaletteRedAdd; Bluesave = PaletteBlueAdd; Greensave = PaletteGreenAdd;
}

extern void gr_palette_step_up_vr( int r, int g, int b, int white, int black );

void game_palette_step_up( int r, int g, int b )
{
	if ( VR_use_reg_code )	{
	#ifndef WINDOWS
//		gr_palette_step_up_vr( r, g, b, VR_WHITE_INDEX, VR_BLACK_INDEX );
	#endif
	} else {
		gr_palette_step_up( r, g, b );
	}
}

void palette_restore(void)
{
	PaletteRedAdd = Redsave; PaletteBlueAdd = Bluesave; PaletteGreenAdd = Greensave;
	game_palette_step_up( PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd );

	//	Forces flash effect to fixup palette next frame.
	Time_flash_last_played = 0;
}

extern void dead_player_frame(void);


//	--------------------------------------------------------------------------------------------------
int allowed_to_fire_laser(void)
{
	if (Player_is_dead) {
		Global_missile_firing_count = 0;
		return 0;
	}

	//	Make sure enough time has elapsed to fire laser, but if it looks like it will
	//	be a long while before laser can be fired, then there must be some mistake!
	if (Next_laser_fire_time > GameTime)
		if (Next_laser_fire_time < GameTime + 2*F1_0)
			return 0;

	return 1;
}

fix	Next_flare_fire_time = 0;
#define	FLARE_BIG_DELAY	(F1_0*2)

int allowed_to_fire_flare(void)
{
	if (Next_flare_fire_time > GameTime)
		if (Next_flare_fire_time < GameTime + FLARE_BIG_DELAY)	//	In case time is bogus, never wait > 1 second.
			return 0;

	if (Players[Player_num].energy >= Weapon_info[FLARE_ID].energy_usage)
		Next_flare_fire_time = GameTime + F1_0/4;
	else
		Next_flare_fire_time = GameTime + FLARE_BIG_DELAY;

	return 1;
}

int allowed_to_fire_missile(void)
{
// mprintf(0, "Next fire = %7.3f, Cur time = %7.3f\n", f2fl(Next_missile_fire_time), f2fl(GameTime));
	//	Make sure enough time has elapsed to fire missile, but if it looks like it will
	//	be a long while before missile can be fired, then there must be some mistake!
	if (Next_missile_fire_time > GameTime)
		if (Next_missile_fire_time < GameTime + 5*F1_0)
			return 0;

	return 1;
}

void full_palette_save(void)
{
	palette_save();
	apply_modified_palette();
	reset_palette_add();
	gr_palette_load( gr_palette );
}

extern int Death_sequence_aborted;
extern int newmenu_dotiny2( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem) );

void show_help()
{
	int nitems;
	newmenu_item m[25];
	#ifdef MACINTOSH
	char command_help[64], pixel_double_help[64], save_help[64], restore_help[64];
	#endif

	m[ 0].type = NM_TYPE_TEXT; m[ 0].text = TXT_HELP_ESC;
	#ifndef MACINTOSH
	m[ 1].type = NM_TYPE_TEXT; m[ 1].text = TXT_HELP_ALT_F2;
	m[ 2].type = NM_TYPE_TEXT; m[ 2].text = TXT_HELP_ALT_F3;
	#else
	sprintf(save_help, "OPT-F2 (%c-s)\t Save Game", 133);
	sprintf(restore_help, "OPT-F3 (%c-o)\t Load Game", 133);
	m[ 1].type = NM_TYPE_TEXT; m[1].text = save_help;
	m[ 2].type = NM_TYPE_TEXT; m[2].text = restore_help;
	#endif
	m[ 3].type = NM_TYPE_TEXT; m[ 3].text = TXT_HELP_F2;
	m[ 4].type = NM_TYPE_TEXT; m[ 4].text = TXT_HELP_F3;
	m[ 5].type = NM_TYPE_TEXT; m[ 5].text = TXT_HELP_F4;
	m[ 6].type = NM_TYPE_TEXT; m[ 6].text = TXT_HELP_F5;
	#ifndef MACINTOSH
	m[ 7].type = NM_TYPE_TEXT; m[ 7].text = TXT_HELP_PAUSE;
	#else
	m[ 7].type = NM_TYPE_TEXT; m[ 7].text = "Pause (F15)\t  Pause";
	#endif
	m[ 8].type = NM_TYPE_TEXT; m[ 8].text = TXT_HELP_MINUSPLUS;
	#ifndef MACINTOSH
	m[ 9].type = NM_TYPE_TEXT; m[ 9].text = TXT_HELP_PRTSCN;
	#else
	m[ 9].type = NM_TYPE_TEXT; m[ 9].text = "printscrn (F13)\t  save screen shot";
	#endif
	m[10].type = NM_TYPE_TEXT; m[10].text = TXT_HELP_1TO5;
	m[11].type = NM_TYPE_TEXT; m[11].text = TXT_HELP_6TO10;
	m[12].type = NM_TYPE_TEXT; m[12].text = "Shift-F1\t  Cycle left window";
	m[13].type = NM_TYPE_TEXT; m[13].text = "Shift-F2\t  Cycle right window";
	m[14].type = NM_TYPE_TEXT; m[14].text = "Shift-F4\t  GuideBot menu";
	#ifndef MACINTOSH
	m[15].type = NM_TYPE_TEXT; m[15].text = "Alt-Shift-F4\t  Rename GuideBot";
	#else
	m[15].type = NM_TYPE_TEXT; m[15].text = "Opt-Shift-F4\t  Rename GuideBot";
	#endif
	m[16].type = NM_TYPE_TEXT; m[16].text = "Shift-F5\t  Drop primary";
	m[17].type = NM_TYPE_TEXT; m[17].text = "Shift-F6\t  Drop secondary";
	m[18].type = NM_TYPE_TEXT; m[18].text = "Shift-F7\t  Calibrate joystick";
	m[19].type = NM_TYPE_TEXT; m[19].text = "Shift-number\t  GuideBot commands";
	nitems = 20;
	#ifdef MACINTOSH
	sprintf(pixel_double_help, "%c-D\t  Toggle Pixel Double Mode", 133);
	m[20].type = NM_TYPE_TEXT; m[20].text = pixel_double_help;
	m[21].type = NM_TYPE_TEXT; m[21].text = "";
	sprintf(command_help, "(Use %c-# for F#. i.e. %c-1 for F1)", 133, 133);
	m[22].type = NM_TYPE_TEXT; m[22].text = command_help;
	nitems = 23;
	#endif

	full_palette_save();

	newmenu_dotiny2( NULL, TXT_KEYS, nitems, m, NULL );

	palette_restore();
}

//temp function until Matt cleans up game sequencing
extern void temp_reset_stuff_on_level();

//deal with rear view - switch it on, or off, or whatever
void check_rear_view()
{

	#define LEAVE_TIME 0x1000		//how long until we decide key is down	(Used to be 0x4000)

	static int leave_mode;
	static fix entry_time;

	if ( Controls.rear_view_down_count )	{		//key/button has gone down

		if (Rear_view) {
			Rear_view = 0;
			if (Cockpit_mode==CM_REAR_VIEW) {
				select_cockpit(Cockpit_mode_save);
				Cockpit_mode_save = -1;
			}
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_restore_rearview();
		}
		else {
			Rear_view = 1;
			leave_mode = 0;		//means wait for another key
			entry_time = timer_get_fixed_seconds();
			if (Cockpit_mode == CM_FULL_COCKPIT) {
				Cockpit_mode_save = Cockpit_mode;
				select_cockpit(CM_REAR_VIEW);
			}
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_rearview();
		}
	}
	else
		if (Controls.rear_view_down_state) {

			if (leave_mode==0 && (timer_get_fixed_seconds()-entry_time)>LEAVE_TIME)
				leave_mode = 1;
		}
		else {

			//@@if (leave_mode==1 && Cockpit_mode==CM_REAR_VIEW) {

			if (leave_mode==1 && Rear_view) {
				Rear_view = 0;
				if (Cockpit_mode==CM_REAR_VIEW) {
					select_cockpit(Cockpit_mode_save);
					Cockpit_mode_save = -1;
				}
				if (Newdemo_state == ND_STATE_RECORDING)
					newdemo_record_restore_rearview();
			}
		}
}

void reset_rear_view(void)
{
	if (Rear_view) {
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_restore_rearview();
	}

	Rear_view = 0;

	if (!(Cockpit_mode == CM_FULL_COCKPIT || Cockpit_mode == CM_STATUS_BAR || Cockpit_mode == CM_FULL_SCREEN)) {
		if (!(Cockpit_mode_save == CM_FULL_COCKPIT || Cockpit_mode_save == CM_STATUS_BAR || Cockpit_mode_save == CM_FULL_SCREEN))
			Cockpit_mode_save = CM_FULL_COCKPIT;
		select_cockpit(Cockpit_mode_save);
		Cockpit_mode_save	= -1;
	}

}

int Automap_flag;
int Config_menu_flag;

jmp_buf LeaveGame;

int Cheats_enabled=0;

extern int Laser_rapid_fire;
extern void do_lunacy_on(), do_lunacy_off();

extern int Physics_cheat_flag,Robots_kill_robots_cheat;
extern char BounceCheat,HomingCheat,OldHomingState[20];
extern char AcidCheatOn,old_IntMethod, Monster_mode;
extern int Buddy_dude_cheat;

//turns off active cheats
void turn_cheats_off()
{
	int i;

	if (HomingCheat)
		for (i=0;i<20;i++)
			Weapon_info[i].homing_flag=OldHomingState[i];

	if (AcidCheatOn)
	{
		AcidCheatOn=0;
		Interpolation_method=old_IntMethod;
	}

	Buddy_dude_cheat = 0;
	BounceCheat=0;
   HomingCheat=0;
	do_lunacy_off();
	Laser_rapid_fire = 0;
	Physics_cheat_flag = 0;
	Monster_mode = 0;
	Robots_kill_robots_cheat=0;
	Robot_firing_enabled = 1;
}

//turns off all cheats & resets cheater flag	
void game_disable_cheats()
{
	turn_cheats_off();
	Cheats_enabled=0;
}


//	game_setup()
// ----------------------------------------------------------------------------

void game_setup(void)
{
	//@@int demo_playing=0;
	//@@int multi_game=0;

	do_lunacy_on();		//	Copy values for insane into copy buffer in ai.c
	do_lunacy_off();		//	Restore true insane mode.

	Game_aborted = 0;
	last_drawn_cockpit[0] = -1;				// Force cockpit to redraw next time a frame renders.
	last_drawn_cockpit[1] = -1;				// Force cockpit to redraw next time a frame renders.
	Endlevel_sequence = 0;

	//@@if ( Newdemo_state == ND_STATE_PLAYBACK )
	//@@	demo_playing = 1;
	//@@if ( Game_mode & GM_MULTI )
	//@@	multi_game = 1;

	set_screen_mode(SCREEN_GAME);
	reset_palette_add();

	set_warn_func(game_show_warning);

	init_cockpit();
	init_gauges();
	//digi_init_sounds();

	//keyd_repeat = 0;                // Don't allow repeat in game
	keyd_repeat = 1;                // Do allow repeat in game

#if !defined(WINDOWS) && !defined(MACINTOSH)
	//_MARK_("start of game");
#endif

	#ifdef EDITOR
		if (Segments[ConsoleObject->segnum].segnum == -1)      //segment no longer exists
			obj_relink( ConsoleObject-Objects, SEG_PTR_2_NUM(Cursegp) );

		if (!check_obj_seg(ConsoleObject))
			move_player_2_segment(Cursegp,Curside);
	#endif

	Viewer = ConsoleObject;
	fly_init(ConsoleObject);

	Game_suspended = 0;

	reset_time();
	FrameTime = 0;			//make first frame zero

	#ifdef EDITOR
	if (Current_level_num == 0) {			//not a real level
		init_player_stats_game();
		init_ai_objects();
	}
	#endif

	fix_object_segs();

	game_flush_inputs();

}


#ifdef NETWORK
extern char IWasKicked;
#endif


//	------------------------------------------------------------------------------------
//this function is the game.  called when game mode selected.  runs until
//editor mode or exit selected
void game()
{
	game_setup();								// Replaces what was here earlier.
													// Good for Windows Sake.  

#ifdef MWPROFILE
	ProfilerSetStatus(1);
#endif

	if ( setjmp(LeaveGame)==0 )	{
		while (1) {
			int player_shields;

			// GAME LOOP!
			Automap_flag = 0;
			Config_menu_flag = 0;

			if ( ConsoleObject != &Objects[Players[Player_num].objnum] )
			  {
			    mprintf ((0,"Player_num=%d objnum=%d",Player_num,Players[Player_num].objnum));
			    //Assert( ConsoleObject == &Objects[Players[Player_num].objnum] );
			  }

			player_shields = Players[Player_num].shields;

		#ifdef WINDOWS
		{
			MSG msg;
			DoMessageStuff(&msg);		// Do Windows event handling.
			if (_RedrawScreen) {
				_RedrawScreen = FALSE;
				load_palette(Current_level_palette,1,1);
				gr_palette_load(gr_palette);
			}
		}
		#endif		

			ExtGameStatus=GAMESTAT_RUNNING;
			GameLoop( 1, 1 );		// Do game loop with rendering and reading controls.

			//if the player is taking damage, give up guided missile control
			if (Players[Player_num].shields != player_shields)
				release_guided_missile(Player_num);

			//see if redbook song needs to be restarted
			songs_check_redbook_repeat();	// Handle RedBook Audio Repeating.

			if (Config_menu_flag) 	{
				int double_save = Scanline_double;

				//WIN(mouse_set_mode(0));
				if (!(Game_mode&GM_MULTI)) {palette_save(); reset_palette_add();	apply_modified_palette(); gr_palette_load( gr_palette ); }
				do_options_menu();
				if (Scanline_double != double_save)	init_cockpit();
				if (!(Game_mode&GM_MULTI)) palette_restore();
				//WIN(mouse_set_mode(1));
			}

			if (Automap_flag) {
				int save_w=Game_window_w,save_h=Game_window_h;
				do_automap(0);
				Screen_mode=-1; set_screen_mode(SCREEN_GAME);
				Game_window_w=save_w; Game_window_h=save_h;
				init_cockpit();
				last_drawn_cockpit[0] = -1;
				last_drawn_cockpit[1] = -1;
			}

			if ( (Function_mode != FMODE_GAME) && Auto_demo && (Newdemo_state != ND_STATE_NORMAL) )	{
				int choice, fmode;
				fmode = Function_mode;
				Function_mode = FMODE_GAME;
				palette_save();
				apply_modified_palette();
				reset_palette_add();
				gr_palette_load( gr_palette );
				choice=nm_messagebox( NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_AUTODEMO );
				palette_restore();
				Function_mode = fmode;
				if (choice==0)	{
					Auto_demo = 0;
					newdemo_stop_playback();
					Function_mode = FMODE_MENU;
				} else {
					Function_mode = FMODE_GAME;
				}
			}

			if ( (Function_mode != FMODE_GAME ) && (Newdemo_state != ND_STATE_PLAYBACK ) && (Function_mode!=FMODE_EDITOR)
#ifdef NETWORK
				       	&& !IWasKicked
#endif
			   )		{
				int choice, fmode;
				fmode = Function_mode;
				Function_mode = FMODE_GAME;
				palette_save();
				apply_modified_palette();
				reset_palette_add();
				gr_palette_load( gr_palette );
				ExtGameStatus=GAMESTAT_ABORT_GAME;
				choice=nm_messagebox( NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME );
				palette_restore();
				Function_mode = fmode;
				if (choice != 0)
					Function_mode = FMODE_GAME;
			}

#ifdef NETWORK
			IWasKicked=0;
#endif
			if (Function_mode != FMODE_GAME)
				longjmp(LeaveGame,0);

			#ifdef APPLE_DEMO
			if ( (keyd_time_when_last_pressed + (F1_0 * 60)) < timer_get_fixed_seconds() )		// idle in game for 1 minutes means exit
				longjmp(LeaveGame,0);
			#endif
		}
	}

#ifdef MWPROFILE
	ProfilerSetStatus(0);
#endif

	digi_stop_all();

	if ( (Newdemo_state == ND_STATE_RECORDING) || (Newdemo_state == ND_STATE_PAUSED) )
		newdemo_stop_recording();

	#ifdef NETWORK
	multi_leave_game();
	#endif

	if ( Newdemo_state == ND_STATE_PLAYBACK )
 		newdemo_stop_playback();

   if (Cockpit_mode_save!=-1)
	 {
		Cockpit_mode=Cockpit_mode_save;
		Cockpit_mode_save=-1;		
	 }

	if (Function_mode != FMODE_EDITOR)
		gr_palette_fade_out(gr_palette,32,0);			// Fade out before going to menu

//@@	if ( (!demo_playing) && (!multi_game) && (Function_mode != FMODE_EDITOR))	{
//@@		scores_maybe_add_player(Game_aborted);
//@@	}

#if !defined(WINDOWS) && !defined(MACINTOSH)
	//_MARK_("end of game");
#endif

	clear_warn_func(game_show_warning);     //don't use this func anymore

	game_disable_cheats();

	#ifdef APPLE_DEMO
	Function_mode = FMODE_EXIT;		// get out of game in Apple OEM version
	#endif
}

//called at the end of the program
void close_game()
{
#ifdef WINDOWS
	if (dd_VR_offscreen_buffer) {
		if (dd_grd_backcanv != dd_VR_offscreen_buffer) {
			dd_gr_free_canvas(dd_VR_offscreen_buffer);
		}
		dd_VR_offscreen_buffer = NULL;
		VR_offscreen_buffer = NULL;
	}
#else
	if (VR_offscreen_buffer)	{
		gr_free_canvas(VR_offscreen_buffer);
		VR_offscreen_buffer = NULL;
	}
#endif

	close_gauge_canvases();

	restore_effect_bitmap_icons();

#if !defined(MACINTOSH) && !defined(WINDOWS)
	if (Game_cockpit_copy_code)	{
		d_free(Game_cockpit_copy_code);
		Game_cockpit_copy_code = NULL;
	}
#else
	if (Game_cockpit_copy_code)
		Game_cockpit_copy_code = 0;
#endif

	if (background_bitmap.bm_data)
		d_free(background_bitmap.bm_data);

	clear_warn_func(game_show_warning);     //don't use this func anymore
}

#ifdef WINDOWS
dd_grs_canvas * get_current_game_screen()
{
	return &dd_VR_screen_pages[VR_current_page];
}

#else

grs_canvas * get_current_game_screen()
{
	return &VR_screen_pages[VR_current_page];
}
#endif


extern void kconfig_center_headset();


#ifndef	NDEBUG
void	speedtest_frame(void);
int	Debug_slowdown=0;
#endif

#ifdef EDITOR
extern void player_follow_path(object *objp);
extern void check_create_player_path(void);

#endif

extern	int	Do_appearance_effect;

object *Missile_viewer=NULL;

int Missile_view_enabled = 1;

int Marker_viewer_num[2]={-1,-1};
int Coop_view_player[2]={-1,-1};
int Cockpit_3d_view[2]={CV_NONE,CV_NONE};

//returns ptr to escort robot, or NULL
object *find_escort()
{
	int i;

	for (i=0; i<=Highest_object_index; i++)
		if (Objects[i].type == OBJ_ROBOT)
			if (Robot_info[Objects[i].id].companion)
				return &Objects[i];

	return NULL;
}

extern void process_super_mines_frame(void);
extern void do_seismic_stuff(void);

#ifndef RELEASE
int Saving_movie_frames=0;
int __Movie_frame_num=0;

#define MAX_MOVIE_BUFFER_FRAMES 250
#define MOVIE_FRAME_SIZE	(320 * 200)

ubyte *Movie_frame_buffer;
int Movie_frame_counter;
ubyte Movie_pal[768];
char movie_path[50] = ".\\";

grs_bitmap Movie_bm;

void flush_movie_buffer()
{
	char savename[128];
	int f;

	stop_time();

	mprintf((0,"Flushing movie buffer..."));

	Movie_bm.bm_data = Movie_frame_buffer;

	for (f=0;f<Movie_frame_counter;f++) {
		sprintf(savename, "%sfrm%04d.pcx",movie_path,__Movie_frame_num);
		__Movie_frame_num++;
		pcx_write_bitmap(savename,&Movie_bm,Movie_pal);
		Movie_bm.bm_data += MOVIE_FRAME_SIZE;

		if (f % 5 == 0)
			mprintf((0,"%3d/%3d\10\10\10\10\10\10\10",f,Movie_frame_counter));
	}

	Movie_frame_counter=0;

	mprintf((0,"done   \n"));

	start_time();
}

void toggle_movie_saving()
{
	int exit;

	Saving_movie_frames = !Saving_movie_frames;

	if (Saving_movie_frames) {
		newmenu_item m[1];

		m[0].type=NM_TYPE_INPUT; m[0].text_len = 50; m[0].text = movie_path;
		exit = newmenu_do( NULL, "Directory for movie frames?" , 1, &(m[0]), NULL );

		if (exit==-1) {
			Saving_movie_frames = 0;
			return;
		}

		while (isspace(movie_path[strlen(movie_path)-1]))
			movie_path[strlen(movie_path)-1] = 0;
		if (movie_path[strlen(movie_path)-1]!='\\' && movie_path[strlen(movie_path)-1]!=':')
			strcat(movie_path,"\\");


		if (!Movie_frame_buffer) {
			Movie_frame_buffer = d_malloc(MAX_MOVIE_BUFFER_FRAMES * MOVIE_FRAME_SIZE);
			if (!Movie_frame_buffer) {
				Int3();
				Saving_movie_frames=0;
			}

			Movie_frame_counter=0;

			Movie_bm.bm_x = Movie_bm.bm_y = 0;
			Movie_bm.bm_w = 320;
			Movie_bm.bm_h = 200;
			Movie_bm.bm_type = BM_LINEAR;
			Movie_bm.bm_flags = 0;
			Movie_bm.bm_rowsize = 320;
			Movie_bm.bm_handle = 0;

			gr_palette_read(Movie_pal);		//get actual palette from the hardware

			if (Newdemo_state == ND_STATE_PLAYBACK)
				Newdemo_do_interpolate = 0;
		}
	}
	else {
		flush_movie_buffer();

		if (Newdemo_state == ND_STATE_PLAYBACK)
			Newdemo_do_interpolate = 1;
	}

}

void save_movie_frame()
{
	memcpy(Movie_frame_buffer+Movie_frame_counter*MOVIE_FRAME_SIZE,grd_curscreen->sc_canvas.cv_bitmap.bm_data,MOVIE_FRAME_SIZE);

	Movie_frame_counter++;

	if (Movie_frame_counter == MAX_MOVIE_BUFFER_FRAMES)
		flush_movie_buffer();

}

#endif

extern int Level_shake_duration;

//if water or fire level, make occasional sound
void do_ambient_sounds()
{
	int has_water,has_lava;
	int sound;

	has_lava = (Segment2s[ConsoleObject->segnum].s2_flags & S2F_AMBIENT_LAVA);
	has_water = (Segment2s[ConsoleObject->segnum].s2_flags & S2F_AMBIENT_WATER);

	if (has_lava) {							//has lava
		sound = SOUND_AMBIENT_LAVA;
		if (has_water && (d_rand() & 1))	//both, pick one
			sound = SOUND_AMBIENT_WATER;
	}
	else if (has_water)						//just water
		sound = SOUND_AMBIENT_WATER;
	else
		return;

	if (((d_rand() << 3) < FrameTime)) {						//play the sound
		fix volume = d_rand() + f1_0/2;
		digi_play_sample(sound,volume);
	}
}

// -- extern void lightning_frame(void);

void game_render_frame();
extern void omega_charge_frame(void);

extern time_t t_current_time, t_saved_time;

void flicker_lights();

void GameLoop(int RenderFlag, int ReadControlsFlag )
{
	con_update();
	#ifndef	NDEBUG
	//	Used to slow down frame rate for testing things.
	//	RenderFlag = 1; // DEBUG
	if (Debug_slowdown) {
		int	h, i, j=0;

		for (h=0; h<Debug_slowdown; h++)
			for (i=0; i<1000; i++)
				j += i;
	}
	#endif

	#ifdef WINDOWS
	{
		static int desc_dead_countdown=100;   /*  used if player shouldn't be playing */

		if (desc_id_exit_num) {				 // are we supposed to be checking
			if (!(--desc_dead_countdown)) {// if so, at zero, then pull the plug
				char time_str[32], time_str2[32];
			
				_ctime(&t_saved_time, time_str);
				_ctime(&t_current_time, time_str2);

				Error ("EXPIRES %s.  YOUR TIME %s.\n", time_str, time_str2);
				Error ("Loading overlay -- error number: %d\n", (int)desc_id_exit_num);
			}
		}
	}
	#endif

		#ifndef RELEASE
		if (FindArg("-invulnerability"))
			Players[Player_num].flags |= PLAYER_FLAGS_INVULNERABLE;
		#endif

      
		update_player_stats();
		diminish_palette_towards_normal();		//	Should leave palette effect up for as long as possible by putting right before render.
		do_afterburner_stuff();
		do_cloak_stuff();
		do_invulnerable_stuff();
		remove_obsolete_stuck_objects();
		init_ai_frame();
		do_final_boss_frame();
		// -- lightning_frame();
		// -- recharge_energy_frame();

		if ((Players[Player_num].flags & PLAYER_FLAGS_HEADLIGHT) && (Players[Player_num].flags & PLAYER_FLAGS_HEADLIGHT_ON)) {
			static int turned_off=0;
			Players[Player_num].energy -= (FrameTime*3/8);
			if (Players[Player_num].energy < i2f(10)) {
				if (!turned_off) {
					Players[Player_num].flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
					turned_off = 1;
#ifdef NETWORK
					if (Game_mode & GM_MULTI)
						multi_send_flags(Player_num);		
#endif
				}
			}
			else
				turned_off = 0;

			if (Players[Player_num].energy <= 0) {
				Players[Player_num].energy = 0;
				Players[Player_num].flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
#ifdef NETWORK
				if (Game_mode & GM_MULTI)
					multi_send_flags(Player_num);		
#endif
			}
		}


		#ifdef EDITOR
		check_create_player_path();
		player_follow_path(ConsoleObject);
		#endif

		#ifdef NETWORK
		if (Game_mode & GM_MULTI)
        {
         multi_do_frame();
         if (Netgame.PlayTimeAllowed && ThisLevelTime>=i2f((Netgame.PlayTimeAllowed*5*60)))
             multi_check_for_killgoal_winner();
        }

		#endif

		if (RenderFlag) {
			if (force_cockpit_redraw) {			//screen need redrawing?
				init_cockpit();
				force_cockpit_redraw=0;
			}
			game_render_frame();
			//show_extra_views();		//missile view, buddy bot, etc.

			#ifndef RELEASE
			if (Saving_movie_frames)
				save_movie_frame();
			#endif

		}


		//mprintf(0,"Velocity %2.2f\n", f2fl(vm_vec_mag(&ConsoleObject->phys_info.velocity)));

		calc_frame_time();

		dead_player_frame();
		if (Newdemo_state != ND_STATE_PLAYBACK)
			do_controlcen_dead_frame();

		process_super_mines_frame();
		do_seismic_stuff();
		do_ambient_sounds();

		#ifndef NDEBUG
		if (Speedtest_on)
			speedtest_frame();
		#endif

		if (ReadControlsFlag)
			ReadControls();
		else
			memset(&Controls, 0, sizeof(Controls));

		GameTime += FrameTime;

		if (f2i(GameTime)/10 != f2i(GameTime-FrameTime)/10)
			mprintf((0,"Gametime = %d secs\n",f2i(GameTime)));

		if (GameTime < 0 || GameTime > i2f(0x7fff - 600)) {
			GameTime = FrameTime;	//wrap when goes negative, or gets within 10 minutes
			mprintf((0,"GameTime reset to 0\n"));
		}

		#ifndef NDEBUG
		if (FindArg("-checktime") != 0)
			if (GameTime >= i2f(600))		//wrap after 10 minutes
				GameTime = FrameTime;
		#endif

#ifdef NETWORK
      if ((Game_mode & GM_MULTI) && Netgame.PlayTimeAllowed)
          ThisLevelTime +=FrameTime;
#endif

		digi_sync_sounds();

		if (Endlevel_sequence) {
			do_endlevel_frame();
			powerup_grab_cheat_all();
			do_special_effects();
			return;					//skip everything else
		}

		if (Newdemo_state != ND_STATE_PLAYBACK)
			do_exploding_wall_frame();
		if ((Newdemo_state != ND_STATE_PLAYBACK) || (Newdemo_vcr_state != ND_STATE_PAUSED)) {
			do_special_effects();
			wall_frame_process();
			triggers_frame_process();
		}


		if (Control_center_destroyed)	{
			if (Newdemo_state==ND_STATE_RECORDING )
				newdemo_record_control_center_destroyed();
		}

		flash_frame();

		if ( Newdemo_state == ND_STATE_PLAYBACK )	{
			newdemo_playback_one_frame();
			if ( Newdemo_state != ND_STATE_PLAYBACK )		{
				longjmp( LeaveGame, 0 );		// Go back to menu
			}
		} else
		{ // Note the link to above!

			Players[Player_num].homing_object_dist = -1;		//	Assume not being tracked.  Laser_do_weapon_sequence modifies this.

			object_move_all();
			powerup_grab_cheat_all();

			if (Endlevel_sequence)	//might have been started during move
				return;

			fuelcen_update_all();

			do_ai_frame_all();

			if (allowed_to_fire_laser())
				FireLaser();				// Fire Laser!

			if (Auto_fire_fusion_cannon_time) {
				if (Primary_weapon != FUSION_INDEX)
					Auto_fire_fusion_cannon_time = 0;
				else if (GameTime + FrameTime/2 >= Auto_fire_fusion_cannon_time) {
					Auto_fire_fusion_cannon_time = 0;
					Global_laser_firing_count = 1;
				} else {
					vms_vector	rand_vec;
					fix			bump_amount;

					Global_laser_firing_count = 0;

					ConsoleObject->mtype.phys_info.rotvel.x += (d_rand() - 16384)/8;
					ConsoleObject->mtype.phys_info.rotvel.z += (d_rand() - 16384)/8;
					make_random_vector(&rand_vec);

					bump_amount = F1_0*4;

					if (Fusion_charge > F1_0*2)
						bump_amount = Fusion_charge*4;

					bump_one_object(ConsoleObject, &rand_vec, bump_amount);
				}
			}

			if (Global_laser_firing_count) {
				//	Don't cap here, gets capped in Laser_create_new and is based on whether in multiplayer mode, MK, 3/27/95
				// if (Fusion_charge > F1_0*2)
				// 	Fusion_charge = F1_0*2;
				Global_laser_firing_count -= do_laser_firing_player();	//do_laser_firing(Players[Player_num].objnum, Primary_weapon);
			}

			if (Global_laser_firing_count < 0)
				Global_laser_firing_count = 0;
		}

	if (Do_appearance_effect) {
		create_player_appearance_effect(ConsoleObject);
		Do_appearance_effect = 0;
#ifdef NETWORK
		if ((Game_mode & GM_MULTI) && Netgame.invul)
		{
			Players[Player_num].flags |= PLAYER_FLAGS_INVULNERABLE;
			Players[Player_num].invulnerable_time = GameTime-i2f(27);
			FakingInvul=1;
		}
#endif
			
	}

	omega_charge_frame();
	slide_textures();
	flicker_lights();

	//!!hoard_light_pulse();		//do cool hoard light pulsing

}

//!!extern int Goal_blue_segnum,Goal_red_segnum;
//!!extern int Hoard_goal_eclip;
//!!
//!!//do cool pulsing lights in hoard goals
//!!hoard_light_pulse()
//!!{
//!!	if (Game_mode & GM_HOARD) {
//!!		fix light;
//!!		int frame;
//!!
//!!		frame = Effects[Hoard_goal_eclip].frame_count;
//!!
//!!		frame++;
//!!
//!!		if (frame >= Effects[Hoard_goal_eclip].vc.num_frames)
//!!			frame = 0;
//!!
//!!		light = abs(frame - 5) * f1_0 / 5;
//!!
//!!		Segment2s[Goal_red_segnum].static_light = Segment2s[Goal_blue_segnum].static_light = light;
//!!	}
//!!}


ubyte	Slide_segs[MAX_SEGMENTS];
int	Slide_segs_computed;

void compute_slide_segs(void)
{
	int	segnum, sidenum;

	for (segnum=0;segnum<=Highest_segment_index;segnum++) {
		Slide_segs[segnum] = 0;
		for (sidenum=0;sidenum<6;sidenum++) {
			int tmn = Segments[segnum].sides[sidenum].tmap_num;
			if (TmapInfo[tmn].slide_u != 0 || TmapInfo[tmn].slide_v != 0)
				Slide_segs[segnum] |= 1 << sidenum;
		}
	}

	Slide_segs_computed = 1;
}

//	-----------------------------------------------------------------------------
void slide_textures(void)
{
	int segnum,sidenum,i;

	if (!Slide_segs_computed)
		compute_slide_segs();

	for (segnum=0;segnum<=Highest_segment_index;segnum++) {
		if (Slide_segs[segnum]) {
			for (sidenum=0;sidenum<6;sidenum++) {
				if (Slide_segs[segnum] & (1 << sidenum)) {
					int tmn = Segments[segnum].sides[sidenum].tmap_num;
					if (TmapInfo[tmn].slide_u != 0 || TmapInfo[tmn].slide_v != 0) {
						for (i=0;i<4;i++) {
							Segments[segnum].sides[sidenum].uvls[i].u += fixmul(FrameTime,TmapInfo[tmn].slide_u<<8);
							Segments[segnum].sides[sidenum].uvls[i].v += fixmul(FrameTime,TmapInfo[tmn].slide_v<<8);
							if (Segments[segnum].sides[sidenum].uvls[i].u > f2_0) {
								int j;
								for (j=0;j<4;j++)
									Segments[segnum].sides[sidenum].uvls[j].u -= f1_0;
							}
							if (Segments[segnum].sides[sidenum].uvls[i].v > f2_0) {
								int j;
								for (j=0;j<4;j++)
									Segments[segnum].sides[sidenum].uvls[j].v -= f1_0;
							}
							if (Segments[segnum].sides[sidenum].uvls[i].u < -f2_0) {
								int j;
								for (j=0;j<4;j++)
									Segments[segnum].sides[sidenum].uvls[j].u += f1_0;
							}
							if (Segments[segnum].sides[sidenum].uvls[i].v < -f2_0) {
								int j;
								for (j=0;j<4;j++)
									Segments[segnum].sides[sidenum].uvls[j].v += f1_0;
							}
						}
					}
				}
			}
		}
	}
}

flickering_light Flickering_lights[MAX_FLICKERING_LIGHTS];

int Num_flickering_lights=0;

void flicker_lights()
{
	int l;
	flickering_light *f;

	f = Flickering_lights;

	for (l=0;l<Num_flickering_lights;l++,f++) {
		segment *segp = &Segments[f->segnum];

		//make sure this is actually a light
		if (! (WALL_IS_DOORWAY(segp, f->sidenum) & WID_RENDER_FLAG))
			continue;
		if (! (TmapInfo[segp->sides[f->sidenum].tmap_num].lighting | TmapInfo[segp->sides[f->sidenum].tmap_num2 & 0x3fff].lighting))
			continue;

		if (f->timer == 0x80000000)		//disabled
			continue;

		if ((f->timer -= FrameTime) < 0) {

			while (f->timer < 0)
				f->timer += f->delay;

			f->mask = ((f->mask&0x80000000)?1:0) + (f->mask<<1);

			if (f->mask & 1)
				add_light(f->segnum,f->sidenum);
			else
				subtract_light(f->segnum,f->sidenum);
		}
	}
}

//returns ptr to flickering light structure, or NULL if can't find
flickering_light *find_flicker(int segnum,int sidenum)
{
	int l;
	flickering_light *f;

	//see if there's already an entry for this seg/side

	f = Flickering_lights;

	for (l=0;l<Num_flickering_lights;l++,f++)
		if (f->segnum == segnum && f->sidenum == sidenum)	//found it!
			return f;

	return NULL;
}

//turn flickering off (because light has been turned off)
void disable_flicker(int segnum,int sidenum)
{
	flickering_light *f;

	if ((f=find_flicker(segnum,sidenum)) != NULL)
		f->timer = 0x80000000;
}

//turn flickering off (because light has been turned on)
void enable_flicker(int segnum,int sidenum)
{
	flickering_light *f;

	if ((f=find_flicker(segnum,sidenum)) != NULL)
		f->timer = 0;
}


#ifdef EDITOR

//returns 1 if ok, 0 if error
int add_flicker(int segnum,int sidenum,fix delay,ulong mask)
{
	int l;
	flickering_light *f;

	mprintf((0,"add_flicker: %d:%d %x %x\n",segnum,sidenum,delay,mask));

	//see if there's already an entry for this seg/side

	f = Flickering_lights;

	for (l=0;l<Num_flickering_lights;l++,f++)
		if (f->segnum == segnum && f->sidenum == sidenum)	//found it!
			break;

	if (mask==0) {		//clearing entry
		if (l == Num_flickering_lights)
			return 0;
		else {
			int i;
			for (i=l;i<Num_flickering_lights-1;i++)
				Flickering_lights[i] = Flickering_lights[i+1];
			Num_flickering_lights--;
			return 1;
		}
	}

	if (l == Num_flickering_lights)
		if (Num_flickering_lights == MAX_FLICKERING_LIGHTS)
			return 0;
		else
			Num_flickering_lights++;

	f->segnum = segnum;
	f->sidenum = sidenum;
	f->delay = f->timer = delay;
	f->mask = mask;

	return 1;
}

#endif

//	-----------------------------------------------------------------------------
//	Fire Laser:  Registers a laser fire, and performs special stuff for the fusion
//				    cannon.
void FireLaser()
{

	Global_laser_firing_count += Weapon_info[Primary_weapon_to_weapon_info[Primary_weapon]].fire_count * (Controls.fire_primary_state || Controls.fire_primary_down_count);

	if ((Primary_weapon == FUSION_INDEX) && (Global_laser_firing_count)) {
		if ((Players[Player_num].energy < F1_0*2) && (Auto_fire_fusion_cannon_time == 0)) {
			Global_laser_firing_count = 0;
		} else {
			if (Fusion_charge == 0)
				Players[Player_num].energy -= F1_0*2;

			Fusion_charge += FrameTime;
			Players[Player_num].energy -= FrameTime;

			if (Players[Player_num].energy <= 0) {
				Players[Player_num].energy = 0;
				Auto_fire_fusion_cannon_time = GameTime -1;	//	Fire now!
			} else
				Auto_fire_fusion_cannon_time = GameTime + FrameTime/2 + 1;
												//	Fire the fusion cannon at this time in the future.

			if (Fusion_charge < F1_0*2)
				PALETTE_FLASH_ADD(Fusion_charge >> 11, 0, Fusion_charge >> 11);
			else
				PALETTE_FLASH_ADD(Fusion_charge >> 11, Fusion_charge >> 11, 0);

			if (GameTime < Fusion_last_sound_time)		//gametime has wrapped
				Fusion_next_sound_time = Fusion_last_sound_time = GameTime;

			if (Fusion_next_sound_time < GameTime) {
				if (Fusion_charge > F1_0*2) {
					digi_play_sample( 11, F1_0 );
					apply_damage_to_player(ConsoleObject, ConsoleObject, d_rand() * 4);
				} else {
					create_awareness_event(ConsoleObject, PA_WEAPON_ROBOT_COLLISION);
					digi_play_sample( SOUND_FUSION_WARMUP, F1_0 );
					#ifdef NETWORK
					if (Game_mode & GM_MULTI)
						multi_send_play_sound(SOUND_FUSION_WARMUP, F1_0);
					#endif
				}
				Fusion_last_sound_time = GameTime;
				Fusion_next_sound_time = GameTime + F1_0/8 + d_rand()/4;
			}
		}
	}

}


//	-------------------------------------------------------------------------------------------------------
//	If player is close enough to objnum, which ought to be a powerup, pick it up!
//	This could easily be made difficulty level dependent.
void powerup_grab_cheat(object *player, int objnum)
{
	fix	powerup_size;
	fix	player_size;
	fix	dist;

	Assert(Objects[objnum].type == OBJ_POWERUP);

	powerup_size = Objects[objnum].size;
	player_size = player->size;

	dist = vm_vec_dist_quick(&Objects[objnum].pos, &player->pos);

	if ((dist < 2*(powerup_size + player_size)) && !(Objects[objnum].flags & OF_SHOULD_BE_DEAD)) {
		vms_vector	collision_point;

		vm_vec_avg(&collision_point, &Objects[objnum].pos, &player->pos);
		collide_player_and_powerup(player, &Objects[objnum], &collision_point);
	}
}

//	-------------------------------------------------------------------------------------------------------
//	Make it easier to pick up powerups.
//	For all powerups in this segment, pick them up at up to twice pickuppable distance based on dot product
//	from player to powerup and player's forward vector.
//	This has the effect of picking them up more easily left/right and up/down, but not making them disappear
//	way before the player gets there.
void powerup_grab_cheat_all(void)
{
	segment	*segp;
	int		objnum;

	segp = &Segments[ConsoleObject->segnum];
	objnum = segp->objects;

	while (objnum != -1) {
		if (Objects[objnum].type == OBJ_POWERUP)
			powerup_grab_cheat(ConsoleObject, objnum);
		objnum = Objects[objnum].next;
	}

}

int	Last_level_path_created = -1;

#ifdef SHOW_EXIT_PATH

//	------------------------------------------------------------------------------------------------------------------
//	Create path for player from current segment to goal segment.
//	Return true if path created, else return false.
int mark_player_path_to_segment(int segnum)
{
	int		i;
	object	*objp = ConsoleObject;
	short		player_path_length=0;
	int		player_hide_index=-1;

	if (Last_level_path_created == Current_level_num) {
		return 0;
	}

	Last_level_path_created = Current_level_num;

	if (create_path_points(objp, objp->segnum, segnum, Point_segs_free_ptr, &player_path_length, 100, 0, 0, -1) == -1) {
		mprintf((0, "Unable to form path of length %i for myself\n", 100));
		return 0;
	}

	player_hide_index = Point_segs_free_ptr - Point_segs;
	Point_segs_free_ptr += player_path_length;

	if (Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
		mprintf((1, "Can't create path.  Not enough point_segs.\n"));
		ai_reset_all_paths();
		return 0;
	}

	for (i=1; i<player_path_length; i++) {
		int			segnum, objnum;
		vms_vector	seg_center;
		object		*obj;

		segnum = Point_segs[player_hide_index+i].segnum;
		mprintf((0, "%3i ", segnum));
		seg_center = Point_segs[player_hide_index+i].point;

		objnum = obj_create( OBJ_POWERUP, POW_ENERGY, segnum, &seg_center, &vmd_identity_matrix, Powerup_info[POW_ENERGY].size, CT_POWERUP, MT_NONE, RT_POWERUP);
		if (objnum == -1) {
			Int3();		//	Unable to drop energy powerup for path
			return 1;
		}

		obj = &Objects[objnum];
		obj->rtype.vclip_info.vclip_num = Powerup_info[obj->id].vclip_num;
		obj->rtype.vclip_info.frametime = Vclip[obj->rtype.vclip_info.vclip_num].frame_time;
		obj->rtype.vclip_info.framenum = 0;
		obj->lifeleft = F1_0*100 + d_rand() * 4;
	}

	mprintf((0, "\n"));
	return 1;
}

//	Return true if it happened, else return false.
int create_special_path(void)
{
	int	i,j;

	//	---------- Find exit doors ----------
	for (i=0; i<=Highest_segment_index; i++)
		for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
			if (Segments[i].children[j] == -2) {
				mprintf((0, "Exit at segment %i\n", i));
				return mark_player_path_to_segment(i);
			}

	return 0;
}

#endif


#ifndef RELEASE
int	Max_obj_count_mike = 0;

//	Shows current number of used objects.
void show_free_objects(void)
{
	if (!(FrameCount & 8)) {
		int	i;
		int	count=0;

		mprintf((0, "Highest_object_index = %3i, MAX_OBJECTS = %3i, now used = ", Highest_object_index, MAX_OBJECTS));

		for (i=0; i<=Highest_object_index; i++)
			if (Objects[i].type != OBJ_NONE)
				count++;

		mprintf((0, "%3i", count));

		if (count > Max_obj_count_mike) {
			Max_obj_count_mike = count;
			mprintf((0, " ***"));
		}

		mprintf((0, "\n"));
	}

}

#endif

#ifdef WINDOWS
void game_win_init_cockpit_mask(int sram)
{
	if (dd_VR_offscreen_buffer && dd_VR_offscreen_buffer != dd_grd_backcanv) {
		dd_gr_free_canvas(dd_VR_offscreen_buffer);
	}

	if (GRMODEINFO(paged) && !GRMODEINFO(dbuf)) {
	//	Here we will make the VR_offscreen_buffer the 2nd page and hopefully
	//	we can just flip it, saving a blt.  
		Int3();
	}
	else if (GRMODEINFO(dbuf)||GRMODEINFO(emul)) {
	//	The offscreen buffer will be created.  We will just blt this
	//	to the screen (which may be blted to the primary surface)
		if ( grd_curscreen->sc_h < 200 ) {
			dd_VR_offscreen_buffer = dd_gr_create_canvas(grd_curscreen->sc_w, 200);
			VR_offscreen_buffer = &dd_VR_offscreen_buffer->canvas;
			if (sram) {
				DDFreeSurface(dd_VR_offscreen_buffer->lpdds);
				dd_VR_offscreen_buffer->lpdds = DDCreateSysMemSurface(grd_curscreen->sc_w, 200);
				dd_VR_offscreen_buffer->sram = 1;
			}
		}
		else {
			dd_VR_offscreen_buffer = dd_gr_create_canvas(grd_curscreen->sc_w, grd_curscreen->sc_h);
			VR_offscreen_buffer = &dd_VR_offscreen_buffer->canvas;
			if (sram) {
				DDFreeSurface(dd_VR_offscreen_buffer->lpdds);
				dd_VR_offscreen_buffer->lpdds = DDCreateSysMemSurface(grd_curscreen->sc_w, grd_curscreen->sc_h);
				dd_VR_offscreen_buffer->sram = 1;
			}
		}
	}

	dd_gr_init_sub_canvas( &dd_VR_render_buffer[0], dd_VR_offscreen_buffer, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h );
	dd_gr_init_sub_canvas( &dd_VR_render_buffer[1], dd_VR_offscreen_buffer, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h );
	gr_init_sub_canvas( &VR_render_buffer[0], VR_offscreen_buffer, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h );
	gr_init_sub_canvas( &VR_render_buffer[1], VR_offscreen_buffer, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h );

	game_init_render_sub_buffers( 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h );
}

//@@void game_win_init_cockpit_mask()
//@@{
//@@	char title_pal[768];
//@@	dd_grs_canvas ccanv;
//@@	int pcx_error;
//@@	LPDIRECTDRAWSURFACE dds;
//@@
//@@	dds = DDCreateSurface(GRMODEINFO(w), GRMODEINFO(h), 1);
//@@	Assert(dds != NULL);
//@@	
//@@	_lpDDSMask = dds;
//@@	ccanv.lpdds = dds;
//@@	dd_gr_reinit_canvas(&ccanv);
//@@
//@@	dd_gr_set_current_canvas(&ccanv);
//@@	DDGRLOCK(dd_grd_curcanv)
//@@	{
//@@		if (W95DisplayMode == SM95_640x480x8) {
//@@			pcx_error=pcx_read_bitmap( "MASKB.PCX", &grd_curcanv->cv_bitmap, 
//@@				grd_curcanv->cv_bitmap.bm_type, 
//@@				title_pal );
//@@		}
//@@		else {
//@@			pcx_error=pcx_read_bitmap( "MASK.PCX", &grd_curcanv->cv_bitmap, 
//@@				grd_curcanv->cv_bitmap.bm_type, 
//@@				title_pal );
//@@		}
//@@	}
//@@	DDGRUNLOCK(dd_grd_curcanv);
//@@
//@@	Assert(pcx_error == PCX_ERROR_NONE);
//@@	Game_cockpit_copy_code = (ubyte *)(0xABADC0DE);
//@@}

#endif
