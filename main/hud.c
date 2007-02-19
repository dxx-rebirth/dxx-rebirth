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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/hud.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:42:04 $
 * 
 * Routines for displaying HUD messages...
 * 
 * $Log: hud.c,v $
 * Revision 1.1.1.1  2006/03/17 19:42:04  zicodxx
 * initial import
 *
 * Revision 1.6  2000/01/17 07:41:43  donut
 * added -hudlog_multi
 *
 * Revision 1.5  1999/10/07 20:59:18  donut
 * support for variable game font sizes
 *
 * Revision 1.4  1999/08/31 22:18:21  donut
 * cleaned up a few &HUD_messages[n][0] where HUD_messages[n] is much cleaner, also fixed a possible cause of message log going off the screen
 *
 * Revision 1.3  1999/08/31 07:47:03  donut
 * added user configurable number of hud message lines (and moved some defines to hudmsg.h to remove redundancy in gauges.c)
 *
 * Revision 1.2  1999/08/30 02:25:41  donut
 * fixed hud message log going a few pixels off the bottom sometimes
 *
 * Revision 1.1.1.1  1999/06/14 22:07:50  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.2  1995/03/30  16:36:40  mike
 * text localization.
 * 
 * Revision 2.1  1995/03/06  15:23:50  john
 * New screen techniques.
 * 
 * Revision 2.0  1995/02/27  11:30:41  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.27  1995/01/23  16:51:30  mike
 * Show hud messages on 3d if window in three largest sizes.
 * 
 * Revision 1.26  1995/01/17  17:42:45  rob
 * Made message timeout for HUD messages longer.
 * 
 * Revision 1.25  1995/01/04  11:39:03  rob
 * Made HUD text get out of the way of large HUD messages.
 * 
 * Revision 1.24  1995/01/01  14:20:32  rob
 * longer timer for hud messages.
 * 
 * 
 * Revision 1.23  1994/12/15  13:04:34  mike
 * Replace Players[Player_num].time_total references with GameTime.
 * 
 * Revision 1.22  1994/12/13  12:55:12  mike
 * move press any key to continue message when you are dead to bottom of window.
 * 
 * Revision 1.21  1994/12/07  17:08:01  rob
 * removed unnecessary debug info.
 * 
 * Revision 1.20  1994/12/07  16:24:16  john
 * Took out code that kept track of messages differently for different
 * screen modes... I made it so they just draw differently depending on screen mode.
 * 
 * Revision 1.19  1994/12/07  15:42:57  rob
 * Added a bunch of debug stuff to look for HUD message problems in net games...
 * 
 * Revision 1.18  1994/12/06  16:30:35  yuan
 * Localization
 * 
 * Revision 1.17  1994/12/05  00:32:36  mike
 * fix randomness of color on status bar hud messages.
 * 
 * Revision 1.16  1994/11/19  17:05:53  rob
 * Moved dead_player_message down to avoid overwriting HUD messages.
 * 
 * Revision 1.15  1994/11/18  23:37:56  john
 * Changed some shorts to ints.
 * 
 * Revision 1.14  1994/11/12  16:38:25  mike
 * clear some annoying debug messages.
 * 
 * Revision 1.13  1994/11/11  15:36:39  mike
 * write hud messages on background if 3d window small enough
 * 
 * Revision 1.12  1994/10/20  09:49:31  mike
 * Reduce number of messages.
 * 
 * Revision 1.11  1994/10/17  10:49:15  john
 * Took out some warnings.
 * 
 * Revision 1.10  1994/10/17  10:45:13  john
 * Made the player able to abort death by pressing any button or key.
 * 
 * Revision 1.9  1994/10/13  15:17:33  mike
 * Remove afterburner references.
 * 
 * Revision 1.8  1994/10/11  12:06:32  mike
 * Only show message advertising death sequence abort after player exploded.
 * 
 * Revision 1.7  1994/10/10  17:21:53  john
 * Made so instead of saying too many messages, it scrolls off the
 * oldest message.
 * 
 * Revision 1.6  1994/10/07  23:05:39  john
 * Fixed bug with HUD not drawing stuff sometimes...
 * ( I had a circular buffer that I was stepping thru
 * to draw text that went: for (i=first;i<last;i++)...
 * duh!! last could be less than first.)
 * /
 * 
 * Revision 1.5  1994/09/16  13:08:20  mike
 * Arcade stuff, afterburner stuff.
 * 
 * Revision 1.4  1994/09/14  16:26:57  mike
 * player_dead_message.
 * 
 * Revision 1.3  1994/08/18  16:35:45  john
 * Made gauges messages stay up a bit longer.
 * 
 * Revision 1.2  1994/08/18  12:10:21  john
 * Made HUD messages scroll.
 * 
 * Revision 1.1  1994/08/18  11:22:09  john
 * Initial revision
 * 
 * 
 */


#ifdef RCS
static char rcsid[] = "$Id: hud.c,v 1.1.1.1 2006/03/17 19:42:04 zicodxx Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

//edited 11/01/98 Matt Mueller - moved to hudlog.c
#include "hudlog.h"
//added on 10/04/98 by Matt Mueller to allow timestamps in hud log
//#include <time.h>
//end addition -MM			 
//end edit -MM

#include "hudmsg.h"

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

#include "args.h"

int hud_first = 0;
int hud_last = 0;

//edited/added on 02/05/99 by Matt Mueller
//#define HUD_MAX_NUM_DISP 4
int HUD_max_num_disp = 4; //max to display normally
int hudlog_first = 0;
int hudlog_num = 0;
int hud_display_all = 0;
//end addition -MM

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

//killed 11/01/98 -MM
//added on 10/04/98 by Matt Mueller to allow hud message logging
//int HUD_log_messages = 0;
//end addition -MM
//end kill -MM

//	-----------------------------------------------------------------------------
void clear_background_messages(void)
{
	if ((Cockpit_mode == CM_STATUS_BAR) && (Last_msg_ycrd != -1) && (VR_render_sub_buffer[0].cv_bitmap.bm_y >= 6)) {
		grs_canvas	*canv_save = grd_curcanv;
		gr_set_current_canvas(get_current_game_screen());
		copy_background_rect(0, Last_msg_ycrd, grd_curcanv->cv_bitmap.bm_w, Last_msg_ycrd+Last_msg_height-1);
		gr_set_current_canvas(canv_save);
		Displayed_background_message[0] = 0;
		Last_msg_ycrd = -1;
	}

}
//edited 02/05/99 Matt Mueller - message log shouldn't be destroyed be resizing screen/death animation
void HUD_clear_messages()
{
//        int i;
///
//	hudlog_first = 0;
//	hudlog_num = 0;
///
	HUD_nmessages = 0;
	hud_first = hud_last;// = 0;
	HUD_message_timer = 0;
	clear_background_messages();
//	for (i = 0; i < HUD_MAX_NUM; i++)
//		sprintf(HUD_messages[i], "SlagelSlagel!!");
}
//end edit -MM

//	-----------------------------------------------------------------------------
//	Writes a message on the HUD and checks its timer.
//edited 02/05/99 Matthew Mueller - added big message scrollback thingy
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

//	gr_printf(1,40, "n:%i m:%i lf:%i f:%i l:%i",hudlog_num, HUD_nmessages, hudlog_first, hud_first, hud_last);
	if ( num < 1 ) return;

	if (HUD_nmessages > 0 )	{
		HUD_message_timer -= FrameTime;

		if ( HUD_message_timer < 0 )	{
			// Timer expired... get rid of oldest message...
			if (hud_last!=hud_first)	{
				//&HUD_messages[hud_first][0] is deing deleted...;
				hud_first = (hud_first+1) % HUD_MAX_NUM;
				HUD_message_timer = F1_0*2;
				HUD_nmessages--;
				clear_background_messages();			//	If in status bar mode and no messages, then erase.
			}
		}
	}

	if (num > 0 )	{

		gr_set_curfont( GAME_FONT );    
//		gr_set_curfont( Gamefonts[GFONT_SMALL] );    
		if (HUD_color == -1)
			HUD_color = BM_XRGB(0,28,0);

		if ((Cockpit_mode == CM_STATUS_BAR) && (VR_render_sub_buffer[0].cv_bitmap.bm_y >= 19)) {
			// Only display the most recent message in this mode
			char	*message = HUD_messages[(hud_first+HUD_nmessages-1) % HUD_MAX_NUM];

			if (strcmp(Displayed_background_message, message)) {
				int	ycrd;
				grs_canvas	*canv_save = grd_curcanv;

				ycrd = grd_curcanv->cv_bitmap.bm_y - 9;

				if (ycrd < 0)
					ycrd = 0;

				gr_set_current_canvas(get_current_game_screen());

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
				y += h+1;
			}
		}
	}

//	gr_set_curfont( GAME_FONT );    
}
//end edit -MM

//killed 02/05/99 Matthew Mueller - commented out the old message recall stuff, and hijacked the key combo

//edited 8/13/98 by Geoff Coovert to save last 4 messages for hotkey recall
//used with companion code below and in game.c
//char mekh_hud_last_msg[3][HUD_MESSAGE_LENGTH+5]; //Array of 3 strings of msg l
       //Three because that's how many are displayed at one time.
       //Not HUD_MAX_NUM which is four.
//int  mekh_ignore_msg = 0;                        //HUD_init_message doesn't store any msgs when this is != 0;

void mekh_hud_recall_msgs()
{
	hud_display_all = !hud_display_all;
//        int temp;
//        mekh_ignore_msg = 1; //Hack to stop it from saving its own saved as it shows em.
//        for(temp=0; temp < (HUD_MAX_NUM-1); temp++)
//edited on 10/04/98 by Matt Mueller to remove compile warning (killed "!= NULL")
//	    if (mekh_hud_last_msg[temp][0]) //Don't display msg if empty
 //end edit -MM
//	      hud_message(MSGC_GAME_FEEDBACK,"*%s",&mekh_hud_last_msg[temp][0]); //Use star to denote old msg
  //      mekh_ignore_msg = 0;
};
//end edit -GC 8/13

void HUD_init_message(char * format, va_list args)
{
	int temp;
	char *message = NULL;
	char *last_message=NULL;

	if ( (hud_last < 0) || (hud_last >= HUD_MAX_NUM))
		Int3(); // Get Rob!!

		message = HUD_messages[hud_last];
		vsprintf(message,format,args);

//              mprintf((/0, "Hud_message: [%s]\n", message));

//edited 8/13/98 by Geoff Coovert to save last 4 messages for hotkey recall
//used with companion code above and in game.c
//        if ( mekh_ignore_msg == 0)
//           {
//                for (temp=0; temp < (HUD_MAX_NUM_DISP-2); temp++)
//                 strcpy(&mekh_hud_last_msg[temp][0], &mekh_hud_last_msg[temp+1][0]);
//              strcpy(&mekh_hud_last_msg[HUD_MAX_NUM_DISP-2][0], message);
//edited 11/01/98 - moved to hudlog.c
//			 hud_log_message(message);
//added on 10/04/98 by Matt Mueller to allow hud message logging (placed here so we don't log recalled msgs)
//			 if (HUD_log_messages){
//				 time_t t;
//				 struct tm *lt;
//				 t=time(NULL);
//				 lt=localtime(&t);
//edited on 11/01/98 Matthew Mueller - zero pad time 
//				 printf("%2i:%02i:%02i %s\n",lt->tm_hour,lt->tm_min,lt->tm_sec,message);
//end edit -MM
//			 }
//end addition -MM
//end kill/edit -MM
//             }
//Move each message back in history, then save the new msg at the front
//end edit -GC 8/13
//end kill -MM

		if (HUD_nmessages > 0)	{
			if (hud_last==0)
				last_message = HUD_messages[HUD_MAX_NUM-1];
			else
				last_message = HUD_messages[hud_last-1];
		}

		temp = (hud_last+1) % HUD_MAX_NUM;
//added/edited 02/05/99 Matthew Mueller - message scrollback stuff
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
//end edit
//moved 02/09/99 Matt Mueller - moved from above to fix hudlog flooding with repeated messages - moved again 2000/01/16 to fix problem with recursive calling.
		hud_log_message(message);
//end move -MM
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

