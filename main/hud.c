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
 * $Source: /cvs/cvsroot/d2x/main/hud.c,v $
 * $Revision: 1.4 $
 * $Author: btb $
 * $Date: 2002-10-10 19:08:15 $
 *
 * Routines for displaying HUD messages...
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2001/11/04 09:00:25  bradleyb
 * Enable d1x-style hud_message
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "hudmsg.h"

#include "pstypes.h"
#include "u_mem.h"
#include "strutil.h"
#include "console.h"
#include "inferno.h"
#include "game.h"
#include "screens.h"
#include "gauges.h"
#include "physics.h"
#include "error.h"

#include "menu.h"			// For the font.
#include "mono.h"
#include "collide.h"
#include "newdemo.h"
#include "player.h"
#include "gamefont.h"

#include "wall.h"
#include "screens.h"
#include "text.h"
#include "laser.h"
#include "args.h"
#include "pa_enabl.h"

int hud_first = 0;
int hud_last = 0;

int 	HUD_nmessages = 0;
fix	HUD_message_timer = 0;		// Time, relative to Players[Player_num].time (int.frac seconds.frac), at which to erase gauge message
char  HUD_messages[HUD_MAX_NUM][HUD_MESSAGE_LENGTH+5];

extern void copy_background_rect(int left,int top,int right,int bot);
char Displayed_background_message[2][HUD_MESSAGE_LENGTH] = {"",""};
int	Last_msg_ycrd = -1;
int	Last_msg_height = 6;
int	HUD_color = -1;

int     MSG_Playermessages = 0;
int     MSG_Noredundancy = 0;

int	Modex_hud_msg_count;

#define LHX(x)		((x)*(FontHires?2:1))
#define LHY(y)		((y)*(FontHires?2.4:1))

#ifdef WINDOWS
int extra_clear=0;
#endif

//	-----------------------------------------------------------------------------
void clear_background_messages(void)
{
	#ifdef WINDOWS
	if (extra_clear == FrameCount) 		//don't do extra clear on same frame
		return;
	#endif

#ifdef WINDOWS
	if (((Cockpit_mode == CM_STATUS_BAR) || (Cockpit_mode == CM_FULL_SCREEN)) && (Last_msg_ycrd != -1) && (dd_VR_render_sub_buffer[0].yoff >= 6)) {
		dd_grs_canvas *canv_save = dd_grd_curcanv;
#else
	if (((Cockpit_mode == CM_STATUS_BAR) || (Cockpit_mode == CM_FULL_SCREEN)) && (Last_msg_ycrd != -1) && (VR_render_sub_buffer[0].cv_bitmap.bm_y >= 6)) {
		grs_canvas	*canv_save = grd_curcanv;
#endif	  

		WINDOS(	dd_gr_set_current_canvas(get_current_game_screen()),
				gr_set_current_canvas(get_current_game_screen())
		);

		PA_DFX (pa_set_frontbuffer_current());
		PA_DFX (copy_background_rect(0, Last_msg_ycrd, grd_curcanv->cv_bitmap.bm_w, Last_msg_ycrd+Last_msg_height-1));
		PA_DFX (pa_set_backbuffer_current());
		copy_background_rect(0, Last_msg_ycrd, grd_curcanv->cv_bitmap.bm_w, Last_msg_ycrd+Last_msg_height-1);
	  
		WINDOS(	
			dd_gr_set_current_canvas(canv_save),
			gr_set_current_canvas(canv_save)
		);

		#ifdef WINDOWS
		if (extra_clear || !GRMODEINFO(modex)) {
			extra_clear = 0;
			Last_msg_ycrd = -1;
		}
		else
			extra_clear = FrameCount;
		#else
		Last_msg_ycrd = -1;
		#endif
	}

	Displayed_background_message[VR_current_page][0] = 0;

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


extern int Guided_in_big_window;
extern int max_window_h;

extern grs_canvas *print_to_canvas(char *s,grs_font *font, int fc, int bc, int double_flag);

//	-----------------------------------------------------------------------------
//	print to buffer, double heights, and blit bitmap to screen
void modex_hud_message(int x, int y, char *s, grs_font *font, int color)
{
	grs_canvas *temp_canv;

	temp_canv = print_to_canvas(s, font, color, -1, 1);

	gr_bitmapm(x,y,&temp_canv->cv_bitmap);

	gr_free_canvas(temp_canv);
}

extern int max_window_w;

//	-----------------------------------------------------------------------------
//	Writes a message on the HUD and checks its timer.
void HUD_render_message_frame()
{
	int i, y,n;
	int h,w,aw;

	if (( HUD_nmessages < 0 ) || (HUD_nmessages > HUD_MAX_NUM))
		Int3(); // Get Rob!

	if ( (HUD_nmessages < 1 ) && (Modex_hud_msg_count == 0))
		return;

	HUD_message_timer -= FrameTime;

	#ifdef WINDOWS
	if (extra_clear)
		clear_background_messages();			//	If in status bar mode and no messages, then erase.
	#endif

	if ( HUD_message_timer < 0 )	{
		// Timer expired... get rid of oldest message...
		if (hud_last!=hud_first)	{
			int	temp;

			//&HUD_messages[hud_first][0] is deing deleted...;
			hud_first = (hud_first+1) % HUD_MAX_NUM;
			HUD_message_timer = F1_0*2;
			HUD_nmessages--;
			if (HUD_nmessages == 0)
				Modex_hud_msg_count = 2;
			temp = Last_msg_ycrd;
			clear_background_messages();			//	If in status bar mode and no messages, then erase.
			if (Modex_hud_msg_count)
				Last_msg_ycrd = temp;
		}
	}

	if (HUD_nmessages > 0 )	{

		if (HUD_color == -1)
			HUD_color = BM_XRGB(0,28,0);

	#ifdef WINDOWS
		if ( (VR_render_mode==VR_NONE) && ((Cockpit_mode == CM_STATUS_BAR) || (Cockpit_mode == CM_FULL_SCREEN)) && (dd_VR_render_sub_buffer[0].yoff >= (max_window_h/8))) {
	#else
		if ( (VR_render_mode==VR_NONE) && ((Cockpit_mode == CM_STATUS_BAR) || (Cockpit_mode == CM_FULL_SCREEN)) && (VR_render_sub_buffer[0].cv_bitmap.bm_y >= (max_window_h/8))) {
	#endif
			// Only display the most recent message in this mode
			char	*message = HUD_messages[(hud_first+HUD_nmessages-1) % HUD_MAX_NUM];

			if (strcmp(Displayed_background_message[VR_current_page], message)) {
				int	ycrd;
				WINDOS(
					dd_grs_canvas *canv_save = dd_grd_curcanv,
					grs_canvas	*canv_save = grd_curcanv
				);

				#ifdef MACINTOSH
				if (Scanline_double)
					FontHires=1;	// always display hires font outside of display
				#endif

				WINDOS(
					ycrd = dd_grd_curcanv->yoff - (SMALL_FONT->ft_h+2),
					ycrd = grd_curcanv->cv_bitmap.bm_y - (SMALL_FONT->ft_h+2)
				);

				if (ycrd < 0)
					ycrd = 0;

				WINDOS(
					dd_gr_set_current_canvas(get_current_game_screen()),
					gr_set_current_canvas(get_current_game_screen())
				);

				gr_set_curfont( SMALL_FONT );
				gr_get_string_size(message, &w, &h, &aw );
				clear_background_messages();


				if (grd_curcanv->cv_bitmap.bm_type == BM_MODEX) {
					WIN(Int3());    // No no no no ....
					ycrd -= h;
					h *= 2;
					modex_hud_message((grd_curcanv->cv_bitmap.bm_w-w)/2, ycrd, message, SMALL_FONT, HUD_color);
					if (Modex_hud_msg_count > 0) {
						Modex_hud_msg_count--;
						Displayed_background_message[VR_current_page][0] = '!';
					} else
						strcpy(Displayed_background_message[VR_current_page], message);
				} else {
				WIN(DDGRLOCK(dd_grd_curcanv));
					gr_set_fontcolor( HUD_color, -1);
					PA_DFX (pa_set_frontbuffer_current());
					PA_DFX (gr_printf((grd_curcanv->cv_bitmap.bm_w-w)/2, ycrd, message ));
					PA_DFX (pa_set_backbuffer_current());
					gr_printf((grd_curcanv->cv_bitmap.bm_w-w)/2, ycrd, message );
					strcpy(Displayed_background_message[VR_current_page], message);
				WIN(DDGRUNLOCK(dd_grd_curcanv));
				}

			WINDOS(
				dd_gr_set_current_canvas(canv_save),
				gr_set_current_canvas(canv_save)
			);

				Last_msg_ycrd = ycrd;
				Last_msg_height = h;

				#ifdef MACINTOSH
				if (Scanline_double)
					FontHires=0;	// always display hires font outside of display
				#endif

			}
		} else {

			gr_set_curfont( SMALL_FONT );

			if ( (Cockpit_mode == CM_FULL_SCREEN) || (Cockpit_mode == CM_LETTERBOX) ) {
				if (Game_window_w == max_window_w)
					y = SMALL_FONT->ft_h/2;
				else
				 	y= SMALL_FONT->ft_h * 2;
			} else
				y = SMALL_FONT->ft_h/2;

		  if (Guided_missile[Player_num] && Guided_missile[Player_num]->type==OBJ_WEAPON && Guided_missile[Player_num]->id==GUIDEDMISS_ID &&
		      Guided_missile[Player_num]->signature==Guided_missile_sig[Player_num] && Guided_in_big_window)
			      y+=SMALL_FONT->ft_h+3; 

		WIN(DDGRLOCK(dd_grd_curcanv));
		  	for (i=0; i<HUD_nmessages; i++ )	{	
				n = (hud_first+i) % HUD_MAX_NUM;
				if ((n < 0) || (n >= HUD_MAX_NUM))
					Int3(); // Get Rob!!
				if (!strcmp(HUD_messages[n], "This is a bug."))
					Int3(); // Get Rob!!
				gr_get_string_size(&HUD_messages[n][0], &w, &h, &aw );
				gr_set_fontcolor( HUD_color, -1);
			
				PA_DFX (pa_set_frontbuffer_current());
				PA_DFX(gr_string((grd_curcanv->cv_bitmap.bm_w-w)/2,y, &HUD_messages[n][0] ));
				PA_DFX (pa_set_backbuffer_current());
				gr_string((grd_curcanv->cv_bitmap.bm_w-w)/2,y, &HUD_messages[n][0] );
				y += h+1;
			}
		WIN(DDGRUNLOCK(dd_grd_curcanv));
		}
	}
	#ifndef WINDOWS
	else if (get_current_game_screen()->cv_bitmap.bm_type == BM_MODEX) {
		if (Modex_hud_msg_count) {
			int	temp = Last_msg_ycrd;
			Modex_hud_msg_count--;
			clear_background_messages();			//	If in status bar mode and no messages, then erase.
			Last_msg_ycrd = temp;
		}
	}
	#endif

	gr_set_curfont( GAME_FONT );
}

int PlayerMessage=1;

// Call to flash a message on the HUD.  Returns true if message drawn.
//  (message might not be drawn if previous message was same)
int HUD_init_message(char * format, ... )
{
	va_list args;
	int temp, temp2;
	char *message = NULL;
	char *last_message=NULL;
	char *cleanmessage;

	Modex_hud_msg_count = 2;

	if ( (hud_last < 0) || (hud_last >= HUD_MAX_NUM))
		Int3(); // Get Rob!!

	// -- mprintf((0, "message timer: %7.3f\n", f2fl(HUD_message_timer)));
	va_start(args, format );
	message = &HUD_messages[hud_last][0];
	vsprintf(message,format,args);
	va_end(args);
	
	/* Produce a sanitised version and send it to the console */
	cleanmessage = d_strdup(message);
	for (temp=0,temp2=0; message[temp]!=0; temp++)
	{
        	if (isprint(message[temp])) cleanmessage[temp2++] = message[temp];
		else temp++; /* Skip next character as well */
	}
	cleanmessage[temp2] = 0;
	con_printf(CON_NORMAL, "%s\n", message);
	d_free(cleanmessage);

	// Added by Leighton 
   
   if ((Game_mode & GM_MULTI) && FindArg("-noredundancy"))
	 if (!strnicmp ("You already",message,11))
		return 0;

   if ((Game_mode & GM_MULTI) && FindArg("-PlayerMessages") && PlayerMessage==0)
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

	return 1;
}


//@@void player_dead_message(void)
//@@{
//@@	if (!Arcade_mode && Player_exploded) { //(ConsoleObject->flags & OF_EXPLODING)) {
//@@		gr_set_curfont( SMALL_FONT );    
//@@		if (HUD_color == -1)
//@@			HUD_color = BM_XRGB(0,28,0);
//@@		gr_set_fontcolor( HUD_color, -1);
//@@
//@@		gr_printf(0x8000, grd_curcanv->cv_bitmap.bm_h-8, TXT_PRESS_ANY_KEY);
//@@		gr_set_curfont( GAME_FONT );    
//@@	}
//@@
//@@}

void player_dead_message(void)
{
    if (Player_exploded) {      
        if ( Players[Player_num].lives < 2 )    {
            int x, y, w, h, aw;
            gr_set_curfont( HUGE_FONT );    
            gr_get_string_size( TXT_GAME_OVER, &w, &h, &aw );
            w += 20;
            h += 8;
            x = (grd_curcanv->cv_w - w ) / 2;
            y = (grd_curcanv->cv_h - h ) / 2;
            
            NO_DFX (Gr_scanline_darkening_level = 2*7);
            NO_DFX (gr_setcolor( BM_XRGB(0,0,0) ));
            NO_DFX (gr_rect( x, y, x+w, y+h ));
            Gr_scanline_darkening_level = GR_FADE_LEVELS;

            gr_string(0x8000, (grd_curcanv->cv_h - grd_curcanv->cv_font->ft_h)/2 + h/8, TXT_GAME_OVER );

#if 0
            // Automatically exit death after 10 secs
            if ( GameTime > Player_time_of_death + F1_0*10 ) {
                    Function_mode = FMODE_MENU;
                    Game_mode = GM_GAME_OVER;
                    longjmp( LeaveGame, 1 );        // Exit out of game loop
            }
#endif

        } 
        gr_set_curfont( GAME_FONT );    
        if (HUD_color == -1)
            HUD_color = BM_XRGB(0,28,0);
        gr_set_fontcolor( HUD_color, -1);
        gr_string(0x8000, grd_curcanv->cv_h-(grd_curcanv->cv_font->ft_h+3), TXT_PRESS_ANY_KEY);
    }
}

// void say_afterburner_status(void)
// {
// 	if (Players[Player_num].flags & PLAYER_FLAGS_AFTERBURNER)
// 		HUD_init_message("Afterburner engaged.");
// 	else
// 		HUD_init_message("Afterburner disengaged.");
// }

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

