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


#ifdef RCS
static char rcsid[] = "$Id: hud.c,v 1.1.1.1 2006/03/17 19:42:04 zicodxx Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "hudlog.h"
#include "hudmsg.h"
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
#include "wall.h"
#include "screens.h"
#include "text.h"
#include "args.h"

int hud_first = 0;
int hud_last = 0;
int HUD_max_num_disp = 4; //max to display normally
int hudlog_first = 0;
int hudlog_num = 0;
int hud_display_all = 0;
int 	HUD_nmessages = 0;
fix	HUD_message_timer = 0;		// Time, relative to Players[Player_num].time (int.frac seconds.frac), at which to erase gauge message
char    HUD_messages[HUD_MAX_NUM][HUD_MESSAGE_LENGTH+5];
extern void copy_background_rect(int left,int top,int right,int bot);
char Displayed_background_message[HUD_MESSAGE_LENGTH] = "";
int	Last_msg_ycrd = -1;
int	Last_msg_height = 6;
int	HUD_color = -1;
int     MSG_Playermessages = 0;
int     MSG_Noredundancy = 0;

//	-----------------------------------------------------------------------------
void clear_background_messages(void)
{
	if ((Cockpit_mode == CM_STATUS_BAR) && (Last_msg_ycrd != -1) && (Screen_3d_window.cv_bitmap.bm_y >= 6)) {
		grs_canvas	*canv_save = grd_curcanv;
		gr_set_current_canvas(NULL);
		copy_background_rect(0, Last_msg_ycrd, grd_curcanv->cv_bitmap.bm_w, Last_msg_ycrd+Last_msg_height-1);
		gr_set_current_canvas(canv_save);
		Displayed_background_message[0] = 0;
		Last_msg_ycrd = -1;
	}

}

void HUD_clear_messages()
{
	HUD_nmessages = 0;
	hud_first = hud_last;
	HUD_message_timer = 0;
	clear_background_messages();
}

//	-----------------------------------------------------------------------------
//	Writes a message on the HUD and checks its timer.
void HUD_render_message_frame()
{
	int i, y,n;
	int h,w,aw;
	int first,num;
	if (hud_display_all){
		first=hudlog_first;
		num=hudlog_num;
	}else{
		first=hud_first;
		num=HUD_nmessages;
	}

	if (( HUD_nmessages < 0 ) || (HUD_nmessages > HUD_MAX_NUM))
		Int3(); // Get Rob!

	if ( num < 1 ) return;

	if (HUD_nmessages > 0 )	{
		HUD_message_timer -= FrameTime;

		if ( HUD_message_timer < 0 )	{
			// Timer expired... get rid of oldest message...
			if (hud_last!=hud_first)	{
				hud_first = (hud_first+1) % HUD_MAX_NUM;
				HUD_message_timer = F1_0*2;
				HUD_nmessages--;
				clear_background_messages(); // If in status bar mode and no messages, then erase.
			}
		}
	}

	if (num > 0 )	{

		gr_set_curfont( GAME_FONT );    
		if (HUD_color == -1)
			HUD_color = BM_XRGB(0,28,0);

		if ((Cockpit_mode == CM_STATUS_BAR) && (Screen_3d_window.cv_bitmap.bm_y >= (max_window_h/8))) {
			// Only display the most recent message in this mode
			char	*message = HUD_messages[(hud_first+HUD_nmessages-1) % HUD_MAX_NUM];

			if (strcmp(Displayed_background_message, message)) {
				int	ycrd;
				grs_canvas	*canv_save = grd_curcanv;

				ycrd = grd_curcanv->cv_bitmap.bm_y - 9;

				if (ycrd < 0)
					ycrd = 0;

				gr_set_current_canvas(NULL);

				gr_get_string_size(message, &w, &h, &aw );
				clear_background_messages();
				gr_set_fontcolor( HUD_color, -1);
				gr_printf((grd_curcanv->cv_bitmap.bm_w-w)/2, ycrd, message );

				strcpy(Displayed_background_message, message);
				gr_set_current_canvas(canv_save);
				Last_msg_ycrd = ycrd;
				Last_msg_height = h;
			}
		} else {
			y = 3;
			gr_get_string_size("0", &w, &h, &aw );
			i= num - (grd_curcanv->cv_bitmap.bm_h-y)/(h+1);//fit as many as possible
			if (i<0) i=0;
		  	for (; i<num; i++ )	{	
				n = (first+i) % HUD_MAX_NUM;
				if ((n < 0) || (n >= HUD_MAX_NUM))
					Int3(); // Get Rob!!
				if (!strcmp(HUD_messages[n], "This is a bug."))
					Int3(); // Get Rob!!
				gr_get_string_size(HUD_messages[n], &w, &h, &aw );
				gr_set_fontcolor( HUD_color, -1);
				gr_printf((grd_curcanv->cv_bitmap.bm_w-w)/2,y, HUD_messages[n] );
				y += h+FONTSCALE_Y(1);
			}
		}
	}
}

void mekh_hud_recall_msgs()
{
	hud_display_all = !hud_display_all;
}

void HUD_init_message(char * format, va_list args)
{
	int temp, i;
	char *message = NULL;
	char *last_message=NULL;

	if ( (hud_last < 0) || (hud_last >= HUD_MAX_NUM))
		Int3(); // Get Rob!!

	message = HUD_messages[hud_last];
	vsprintf(message,format,args);

	// clean message if necessary.
	// using placeholders may mess up message string and crash game(s).
	// block them also to prevent attacks from other clients.
	for (i = 0; i <= strlen(message); i++)
		if (message[i] == '%')
			message [i] = ' ';

	if (HUD_nmessages > 0)	{
		if (hud_last==0)
			last_message = HUD_messages[HUD_MAX_NUM-1];
		else
			last_message = HUD_messages[hud_last-1];
	}

	temp = (hud_last+1) % HUD_MAX_NUM;
	if ( temp==hudlog_first )	{
		hudlog_first= (hudlog_first+1) % HUD_MAX_NUM;
		hudlog_num--;
	}
	if ( HUD_nmessages>=HUD_max_num_disp){
		// If too many messages, remove oldest message to make room
		hud_first = (hud_first+1) % HUD_MAX_NUM;
		HUD_nmessages--;
	}

	if (last_message && (!strcmp(last_message, message))) {
		HUD_message_timer = F1_0*3;		// 1 second per 5 characters
		return;	// ignore since it is the same as the last one
	}

	hud_last = temp;
	// Check if memory has been overwritten at this point.
	if (strlen(message) >= HUD_MESSAGE_LENGTH)
		Error( "Your message to HUD is too long.  Limit is %i characters.\n", HUD_MESSAGE_LENGTH);
	#ifdef NEWDEMO
	if (Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_hud_message( message );
	#endif
	HUD_message_timer = F1_0*3;		// 1 second per 5 characters
	HUD_nmessages++;
	hudlog_num++;

	hud_log_message(message);
}


void player_dead_message(void)
{
	if (Player_exploded) {
		if ( Players[Player_num].lives < 2 )    {
			int x, y, w, h, aw;
			gr_set_curfont(Gamefonts[GFONT_BIG_1]);
			gr_get_string_size( TXT_GAME_OVER, &w, &h, &aw );
			w += 20;
			h += 8;
			x = (GWIDTH - w ) / 2;
			y = (GHEIGHT - h ) / 2;
			Gr_scanline_darkening_level = 2*7;
			gr_setcolor( BM_XRGB(0,0,0) );
			gr_rect( x, y, x+w, y+h );
			Gr_scanline_darkening_level = GR_FADE_LEVELS;
		
			gr_string(0x8000, (GHEIGHT - FONTSCALE_Y(grd_curcanv->cv_font->ft_h))/2 + h/8, TXT_GAME_OVER );
        	}
		gr_set_curfont( GAME_FONT );
		if (HUD_color == -1)
		HUD_color = BM_XRGB(0,28,0);
		gr_set_fontcolor( HUD_color, -1);
		gr_string(0x8000, GHEIGHT-FONTSCALE_Y(grd_curcanv->cv_font->ft_h+3)*((Newdemo_state == ND_STATE_RECORDING)?2:1), TXT_PRESS_ANY_KEY);
	}
}

void hud_message(int class, char *format, ...)
{
 va_list vp;
 va_start(vp, format);
  if ((!MSG_Noredundancy || (class & MSGC_NOREDUNDANCY)) &&
      (!MSG_Playermessages || !(Game_mode & GM_MULTI) ||
      (class & MSGC_PLAYERMESSAGES)))
   HUD_init_message(format, vp);
 va_end(vp);
}
