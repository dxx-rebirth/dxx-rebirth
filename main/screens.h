/* $Id: screens.h,v 1.3 2004-08-28 23:17:45 schaffner Exp $ */
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
 * Info on canvases, screens, etc.
 *
 */


#ifndef _SCREEN_H
#define _SCREEN_H

#include "gr.h"

// What graphics modes the game & editor open

// for Screen_mode variable
#define SCREEN_MENU		0	// viewing the menu screen
#define SCREEN_GAME		1	// viewing the menu screen
#define SCREEN_EDITOR	2	// viewing the editor screen

//from editor.c
extern grs_canvas *Canv_editor;			// the full on-scrren editor canvas
extern grs_canvas *Canv_editor_game;	// the game window on the editor screen

//from game.c
extern int set_screen_mode(int sm);		// True = editor screen

//About the screen
extern ubyte		VR_screen_flags;
extern ubyte		VR_current_page;
extern fix			VR_eye_width;
extern int 			VR_eye_switch;

//NEWVR (Add a bunch off lines)
#define VR_SEPARATION F1_0*7/10
#define VR_PIXEL_SHIFT -6
#define VR_WHITE_INDEX 255
#define VR_BLACK_INDEX 0
extern int			VR_eye_offset;
extern int 			VR_eye_offset_changed;
extern void 		VR_reset_params();
extern int			VR_use_reg_code;


extern int			VR_screen_mode;
extern int			VR_render_mode;
extern int			VR_low_res;
extern int 			VR_show_hud;
extern int			VR_sensitivity;

extern grs_canvas	*VR_offscreen_buffer;		// The offscreen data buffer
extern grs_canvas	VR_render_buffer[2];		// Two offscreen buffers for left/right eyes.
extern grs_canvas	VR_render_sub_buffer[2];	// Two sub buffers for left/right eyes.
extern grs_canvas	VR_screen_pages[2];			// Two pages of VRAM if paging is available
extern grs_canvas	VR_screen_sub_pages[2];		// Two sub pages of VRAM if paging is available

//values for VR_screen_flags

#define VRF_USE_PAGING			1		// means page flips
#define VRF_SWITCH_EYES			2
#define VRF_COMPATIBLE_MENUS	4
#define VRF_ALLOW_COCKPIT		8

//values for VR_render_mode

#define VR_NONE			0	// viewing the game screen
#define VR_AREA_DET		1	// viewing with the stereo area determined method
#define VR_INTERLACED	2	// viewing with the stereo interlaced method

void game_init_render_buffers(int screen_mode, int render_max_w, int render_max_h, int render_method, int flags);
void game_init_render_sub_buffers(int x, int y, int w, int h);

void set_display_mode(int mode);
extern int Default_display_mode;		// cannot be -1
// values are:
// -1: some special VR mode.
//  0: 320 x 200
//  1: 640 x 480
//  2: 320 x 400
//  3: 640 x 400
//  4: 800 x 600
//  5: 102 x 768

// called to get the screen in a mode compatible with popup menus.
// if we can't have popups over the game screen, switch to menu mode.
void set_popup_screen(void);

#endif /* _SCREENS_H */
