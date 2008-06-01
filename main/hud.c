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
 * Routines for displaying HUD messages...
 * 
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "hudmsg.h"
#include "inferno.h"
#include "game.h"
#include "screens.h"
#include "gauges.h"
#include "physics.h"
#include "error.h"
#include "menu.h"
#include "collide.h"
#include "newdemo.h"
#include "player.h"
#include "gamefont.h"
#include "wall.h"
#include "screens.h"
#include "text.h"
#include "args.h"
#include "strutil.h"
#include "console.h"
#include "playsave.h"

int hud_first = 0;
int hud_last = 0;
int HUD_nmessages = 0;
fix HUD_message_timer = 0;      // Time, relative to Players[Player_num].time (int.frac seconds.frac), at which to erase gauge message
char  HUD_messages[HUD_MAX_NUM][HUD_MESSAGE_LENGTH+5];

char Displayed_background_message[HUD_MESSAGE_LENGTH] = "";
int Last_msg_ycrd = -1;
int Last_msg_height = 6;
int HUD_color = -1;

int     MSG_Playermessages = 0;
int     MSG_Noredundancy = 0;

// ----------------------------------------------------------------------------
void clear_background_messages(void)
{
	if (((PlayerCfg.CockpitMode == CM_STATUS_BAR) || (PlayerCfg.CockpitMode == CM_FULL_SCREEN)) && (Last_msg_ycrd != -1) && (Screen_3d_window.cv_bitmap.bm_y >= 6)) {
		grs_canvas	*canv_save = grd_curcanv;

		gr_set_current_canvas(NULL);

		gr_set_current_canvas(canv_save);

		Last_msg_ycrd = -1;
	}

	Displayed_background_message[0] = 0;

}

void HUD_clear_messages()
{
	int i;
	HUD_nmessages = 0;
	hud_first = hud_last = 0;
	HUD_message_timer = 0;
	clear_background_messages();
	for (i = 0; i < HUD_MAX_NUM; i++)
		sprintf(HUD_messages[i], "SlagelSlagel!!");
}


extern int max_window_h, max_window_w;

// ----------------------------------------------------------------------------
//	Writes a message on the HUD and checks its timer.
void HUD_render_message_frame()
{
	int i,y,n;

	if (( HUD_nmessages < 0 ) || (HUD_nmessages > HUD_MAX_NUM))
		Int3(); // Get Rob!

	if (HUD_nmessages < 1 )
		return;

	HUD_message_timer -= FrameTime;

	if ( HUD_message_timer < 0 )	{
		// Timer expired... get rid of oldest message...
		if (hud_last!=hud_first)	{
			int	temp;

			//&HUD_messages[hud_first][0] is deing deleted...;
			hud_first = (hud_first+1) % HUD_MAX_NUM;
			HUD_message_timer = F1_0*2;
			HUD_nmessages--;
			temp = Last_msg_ycrd;
			clear_background_messages();			//	If in status bar mode and no messages, then erase.
		}
	}

	if (HUD_nmessages > 0 )	{
		if (HUD_color == -1)
			HUD_color = BM_XRGB(0,28,0);

		gr_set_curfont( GAME_FONT );
		y = FSPACY(1);

		for (i=0; i<HUD_nmessages; i++ )	{
			n = (hud_first+i) % HUD_MAX_NUM;
			if ((n < 0) || (n >= HUD_MAX_NUM))
				Int3(); // Get Rob!!
			if (!strcmp(HUD_messages[n], "This is a bug."))
				Int3(); // Get Rob!!
			gr_set_fontcolor( HUD_color, -1);

			gr_printf(0x8000,y, &HUD_messages[n][0] );
			y += LINE_SPACING;
		}
	}

	gr_set_curfont( GAME_FONT );
}

int PlayerMessage=1;

// Call to flash a message on the HUD.  Returns true if message drawn.
// (message might not be drawn if previous message was same)
int HUD_init_message_va(char * format, va_list args)
{
	int temp, i;
	char *message = NULL;
	char *last_message=NULL;

	if ( (hud_last < 0) || (hud_last >= HUD_MAX_NUM))
		Int3(); // Get Rob!!

	message = &HUD_messages[hud_last][0];
	vsprintf(message,format,args);

	// clean message if necessary.
	// using placeholders may mess up message string and crash game(s).
	// block them also to prevent attacks from other clients.
	for (i = 0; i <= strlen(message); i++)
		if (message[i] == '%')
			message [i] = ' ';

	// Added by Leighton

   if (GameArg.SysNoRedundancy)
	 if (!strnicmp ("You already",message,11) || !stricmp("your laser is maxed out!",message))
		return 0;

   if ((Game_mode & GM_MULTI) && GameArg.MplPlayerMessages && PlayerMessage==0)
		return 0;

	if (HUD_nmessages > 0)	{
		if (hud_last==0)
			last_message = &HUD_messages[HUD_MAX_NUM-1][0];
		else
			last_message = &HUD_messages[hud_last-1][0];
	}

	temp = (hud_last+1) % HUD_MAX_NUM;

	if ( temp==hud_first )	{
		// If too many messages, remove oldest message to make room
		hud_first = (hud_first+1) % HUD_MAX_NUM;
		HUD_nmessages--;
	}

	if (last_message && (!strcmp(last_message, message))) {
		HUD_message_timer = F1_0*3;		// 1 second per 5 characters
		return 0;	// ignore since it is the same as the last one
	}

	// if many redundant messages at the same time, just block them all together
	// we probably want at least all visible once, but this isn't worth the work
	for (i=0; i<=HUD_MAX_NUM; i++)
		if (last_message && !strcmp(&HUD_messages[i][0],message) && (!strnicmp("You already",message,11) || !stricmp("your laser is maxed out!",message)))
			return 0;

	if (HUD_color == -1)
		HUD_color = BM_XRGB(0,28,0);
	con_printf(CON_HUD, "%s\n", message);

	hud_last = temp;
	// Check if memory has been overwritten at this point.
	if (strlen(message) >= HUD_MESSAGE_LENGTH)
		Error( "Your message to HUD is too long.  Limit is %i characters.\n", HUD_MESSAGE_LENGTH);
	if (Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_hud_message( message );
	HUD_message_timer = F1_0*3;		// 1 second per 5 characters
	HUD_nmessages++;

	return 1;
}


int HUD_init_message(char * format, ... )
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = HUD_init_message_va(format, args);
	va_end(args);

	return ret;
}

void player_dead_message(void)
{
	if (Player_exploded) {
		if ( Players[Player_num].lives < 2 )    {
			int x, y, w, h, aw;
			gr_set_curfont( HUGE_FONT );
			gr_get_string_size( TXT_GAME_OVER, &w, &h, &aw );
			w += 20;
			h += 8;
			x = (grd_curcanv->cv_bitmap.bm_w - w ) / 2;
			y = (grd_curcanv->cv_bitmap.bm_h - h ) / 2;
		
			Gr_scanline_darkening_level = 2*7;
			gr_setcolor( BM_XRGB(0,0,0) );
			gr_rect( x, y, x+w, y+h );
			Gr_scanline_darkening_level = GR_FADE_LEVELS;
		
			gr_printf(0x8000, (GHEIGHT - h)/2 + h/8, TXT_GAME_OVER );
		}
	
		gr_set_curfont( GAME_FONT );
		if (HUD_color == -1)
			HUD_color = BM_XRGB(0,28,0);
		gr_set_fontcolor( HUD_color, -1);
		gr_printf(0x8000, GHEIGHT-LINE_SPACING, TXT_PRESS_ANY_KEY);
	}
}

void hud_message(int class, char *format, ...)
{
	va_list vp;

	va_start(vp, format);
	if ((!MSG_Noredundancy || (class & MSGC_NOREDUNDANCY)) &&
	    (!MSG_Playermessages || !(Game_mode & GM_MULTI) ||
	     (class & MSGC_PLAYERMESSAGES)))
		HUD_init_message_va(format, vp);
	va_end(vp);
}
