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


#ifdef RCS
static char rcsid[] = "$Id: kmatrix.c,v 1.1.1.1 2001-01-19 03:29:59 bradleyb Exp $";
#endif

#include <conf.h>

#ifdef NETWORK

#ifdef WINDOWS
#include "desw.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "pa_enabl.h"       //$$POLY_ACC
#include "error.h"
#include "pstypes.h"
#include "gr.h"
#include "mono.h"
#include "key.h"
#include "palette.h"
#include "game.h"
#include "gamefont.h"
#include "u_mem.h"
#include "newmenu.h"
#include "menu.h"
#include "player.h"
#include "screens.h"
#include "gamefont.h"
#include "cntrlcen.h"
#include "mouse.h"
#include "joy.h"
#include "timer.h"
#include "text.h"
#include "songs.h"
#include "multi.h"
#include "kmatrix.h"
#include "gauges.h"
#include "pcx.h"
#include "network.h"

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

#define CENTERING_OFFSET(x) ((300 - (70 + (x)*25 ))/2)
#define CENTERSCREEN (MenuHires?320:160)

int kmatrix_kills_changed = 0;
char ConditionLetters[]={' ','P','E','D','E','E','V','W'};
char WaitingForOthers=0;

int Kmatrix_nomovie_message=0;

extern char MaxPowerupsAllowed[],PowerupsInMine[];
extern void network_send_endlevel_sub(int);

#define LHX(x)		((x)*(MenuHires?2:1))
#define LHY(y)		((y)*(MenuHires?2.4:1))
 
void kmatrix_draw_item( int  i, int *sorted )
{
	int j, x, y;
	char temp[10];

	y = LHY(50+i*9);

	// Print player name.

	gr_printf( LHX(CENTERING_OFFSET(N_players)), y, "%s", Players[sorted[i]].callsign );

   if (!((Game_mode & GM_MODEM) || (Game_mode & GM_SERIAL)))
	   gr_printf (LHX(CENTERING_OFFSET(N_players)-15),y,"%c",ConditionLetters[Players[sorted[i]].connected]);
    
	for (j=0; j<N_players; j++) {

		x = LHX(70 + CENTERING_OFFSET(N_players) + j*25);

		if (sorted[i]==sorted[j]) {
			if (kill_matrix[sorted[i]][sorted[j]] == 0) {
				gr_set_fontcolor( BM_XRGB(10,10,10),-1 );
				gr_printf( x, y, "%d", kill_matrix[sorted[i]][sorted[j]] );
			} else {
				gr_set_fontcolor( BM_XRGB(25,25,25),-1 );
				gr_printf( x, y, "-%d", kill_matrix[sorted[i]][sorted[j]] );
			}
		} else {
			if (kill_matrix[sorted[i]][sorted[j]] <= 0) {
				gr_set_fontcolor( BM_XRGB(10,10,10),-1 );
				gr_printf( x, y, "%d", kill_matrix[sorted[i]][sorted[j]] );
			} else {
				gr_set_fontcolor( BM_XRGB(25,25,25),-1 );
				gr_printf( x, y, "%d", kill_matrix[sorted[i]][sorted[j]] );
			}
		}

	}

  if (Players[sorted[i]].net_killed_total+Players[sorted[i]].net_kills_total==0)
 	sprintf (temp,"NA");
  else
   sprintf (temp,"%d%%",(int)((float)((float)Players[sorted[i]].net_kills_total/((float)Players[sorted[i]].net_killed_total+(float)Players[sorted[i]].net_kills_total))*100.0));		
		
	x = LHX(60 + CENTERING_OFFSET(N_players) + N_players*25);
	gr_set_fontcolor( BM_XRGB(25,25,25),-1 );
	gr_printf( x ,y,"%4d/%s",Players[sorted[i]].net_kills_total,temp);
}

void kmatrix_draw_coop_item( int  i, int *sorted )
{
	int  x, y;

	y = LHY(50+i*9);

	// Print player name.

	gr_printf( LHX(CENTERING_OFFSET(N_players)), y, "%s", Players[sorted[i]].callsign );
   gr_printf (LHX(CENTERING_OFFSET(N_players)-15),y,"%c",ConditionLetters[Players[sorted[i]].connected]);
    

	x = CENTERSCREEN;

	gr_set_fontcolor( BM_XRGB(60,40,10),-1 );
	gr_printf( x, y, "%d", Players[sorted[i]].score );

	x = CENTERSCREEN+LHX(50);

	gr_set_fontcolor( BM_XRGB(60,40,10),-1 );
	gr_printf( x, y, "%d", Players[sorted[i]].net_killed_total);
}


void kmatrix_draw_names(int *sorted)
{
	int j, x;
	
	int color;

   if (Kmatrix_nomovie_message)
    {
		gr_set_fontcolor( BM_XRGB(63,0,0),-1 );
		gr_printf( CENTERSCREEN-LHX(40), LHY(20), "(Movie not played)");
	 }

	for (j=0; j<N_players; j++) {
		if (Game_mode & GM_TEAM)
			color = get_team(sorted[j]);
		else
			color = sorted[j];

		x = LHX (70 + CENTERING_OFFSET(N_players) + j*25);

      if (Players[sorted[j]].connected==0)
        gr_set_fontcolor(gr_find_closest_color(31,31,31),-1);
      else
        gr_set_fontcolor(gr_getcolor(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );

		gr_printf( x, LHY(40), "%c", Players[sorted[j]].callsign[0] );

	}

	x = LHX(72 + CENTERING_OFFSET(N_players) + N_players*25);
	gr_set_fontcolor( BM_XRGB(31,31,31),-1 );
	gr_printf( x, LHY(40), "K/E");
		
}
void kmatrix_draw_coop_names(int *sorted)
{
	sorted=sorted;

   if (Kmatrix_nomovie_message)
    {
		gr_set_fontcolor( BM_XRGB(63,0,0),-1 );
		gr_printf( CENTERSCREEN-LHX(40), LHY(20), "(Movie not played)");
	 }

	gr_set_fontcolor( BM_XRGB(63,31,31),-1 );
	gr_printf( CENTERSCREEN, LHY(40), "SCORE");

	gr_set_fontcolor( BM_XRGB(63,31,31),-1 );
	gr_printf( CENTERSCREEN+LHX(50), LHY(40), "DEATHS");
}


void kmatrix_draw_deaths(int *sorted)
{
	int y,x;
	char reactor_message[50];

   sorted=sorted;

   y = LHY(55 + 72 + 35);
	x = LHX(35);

	{
          
		int sw, sh, aw;
	   				
      gr_set_fontcolor(gr_find_closest_color(63,20,0),-1);
      gr_get_string_size("P-Playing E-Escaped D-Died", &sw, &sh, &aw); 

 	   if (!((Game_mode & GM_MODEM) || (Game_mode & GM_SERIAL)))
		   gr_printf( CENTERSCREEN-(sw/2), y,"P-Playing E-Escaped D-Died");
   
      y+=(sh+5);
	   gr_get_string_size("V-Viewing scores W-Waiting", &sw, &sh, &aw); 

	   if (!((Game_mode & GM_MODEM) || (Game_mode & GM_SERIAL)))
		   gr_printf( CENTERSCREEN-(sw/2), y,"V-Viewing scores W-Waiting");

	}

   y+=LHY(20);

	{
		int sw, sh, aw;
      gr_set_fontcolor(gr_find_closest_color(63,63,63),-1);

      if (Players[Player_num].connected==7)
       {
        gr_get_string_size("Waiting for other players...",&sw, &sh, &aw); 
        gr_printf( CENTERSCREEN-(sw/2), y,"Waiting for other players...");
       }
      else
       {
        gr_get_string_size(TXT_PRESS_ANY_KEY2, &sw, &sh, &aw); 
        gr_printf( CENTERSCREEN-(sw/2), y, TXT_PRESS_ANY_KEY2);
       }
	}

  if (Countdown_seconds_left <=0)
   kmatrix_reactor(TXT_REACTOR_EXPLODED);
  else
   {
     sprintf(&reactor_message, "%s: %d %s  ", TXT_TIME_REMAINING, Countdown_seconds_left, TXT_SECONDS);
     kmatrix_reactor (&reactor_message);
   }
  
  if (Game_mode & GM_HOARD)  
	  kmatrix_phallic();

}
void kmatrix_draw_coop_deaths(int *sorted)
{
	int j, x, y;
	char reactor_message[50];
	
	y = LHY(55 + N_players * 9);

//	gr_set_fontcolor(gr_getcolor(player_rgb[j].r,player_rgb[j].g,player_rgb[j].b),-1 );
	gr_set_fontcolor( BM_XRGB(31,31,31),-1 );

	x = CENTERSCREEN+LHX(50);
	gr_printf( x, y, TXT_DEATHS );

	for (j=0; j<N_players; j++) {
		x = CENTERSCREEN+LHX(50);
		gr_printf( x, y, "%d", Players[sorted[j]].net_killed_total );
	}

   y = LHY(55 + 72 + 35);
	x = LHX(35);

	{
          
		int sw, sh, aw;

      gr_set_fontcolor(gr_find_closest_color(63,20,0),-1);
      gr_get_string_size("P-Playing E-Escaped D-Died", &sw, &sh, &aw); 

	  if (!((Game_mode & GM_MODEM) || (Game_mode & GM_SERIAL)))
	      gr_printf( CENTERSCREEN-(sw/2), y,"P-Playing E-Escaped D-Died");
  	   
   
      y+=(sh+5);
	   gr_get_string_size("V-Viewing scores W-Waiting", &sw, &sh, &aw); 

	   if (!((Game_mode & GM_MODEM) || (Game_mode & GM_SERIAL)))
	       gr_printf( CENTERSCREEN-(sw/2), y,"V-Viewing scores W-Waiting");

	}

   y+=LHY(20);

	{
		int sw, sh, aw;
      gr_set_fontcolor(gr_find_closest_color(63,63,63),-1);

      if (Players[Player_num].connected==7)
       {
        gr_get_string_size("Waiting for other players...",&sw, &sh, &aw); 
        gr_printf( CENTERSCREEN-(sw/2), y,"Waiting for other players...");
       }
      else
       {
        gr_get_string_size(TXT_PRESS_ANY_KEY2, &sw, &sh, &aw); 
        gr_printf( CENTERSCREEN-(sw/2), y, TXT_PRESS_ANY_KEY2);
       }
	}

  if (Countdown_seconds_left <=0)
   kmatrix_reactor(TXT_REACTOR_EXPLODED);
  else
   {
     sprintf(&reactor_message, "%s: %d %s  ", TXT_TIME_REMAINING, Countdown_seconds_left, TXT_SECONDS);
     kmatrix_reactor (&reactor_message);
   }

}

void kmatrix_reactor (char *message)
 {
  static char oldmessage[50]={0};
  int sw, sh, aw;

  if ((Game_mode & GM_MODEM) || (Game_mode & GM_SERIAL))
	return;

  grd_curcanv->cv_font = SMALL_FONT;

  if (oldmessage[0]!=0)
   {
    gr_set_fontcolor(gr_find_closest_color(0,0,0),-1);
    gr_get_string_size(oldmessage, &sw, &sh, &aw); 
   // gr_printf( CENTERSCREEN-(sw/2), LHY(55+72+12), oldmessage);
   }
  gr_set_fontcolor(gr_find_closest_color(0,32,63),-1);
  gr_get_string_size(message, &sw, &sh, &aw); 
  gr_printf( CENTERSCREEN-(sw/2), LHY(55+72+12), message);

  strcpy (&oldmessage,message);
 }

extern int PhallicLimit,PhallicMan;

void kmatrix_phallic ()
 {
  int sw, sh, aw;
  char message[80];

  if (!(Game_mode & GM_HOARD))
	return;

  if ((Game_mode & GM_MODEM) || (Game_mode & GM_SERIAL))
	return;
  
  if (PhallicMan==-1)
	strcpy (message,"There was no record set for this level.");
  else
	sprintf (message,"%s had the best record at %d points.",Players[PhallicMan].callsign,PhallicLimit);

  grd_curcanv->cv_font = SMALL_FONT;
  gr_set_fontcolor(gr_find_closest_color(63,63,63),-1);
  gr_get_string_size(message, &sw, &sh, &aw); 
  gr_printf( CENTERSCREEN-(sw/2), LHY(55+72+3), message);
 }

 
void load_stars(void);

WINDOS(dd_grs_canvas *StarBackCanvas,
			grs_canvas *StarBackCanvas
);		//,*SaveCanvas;


void kmatrix_redraw()
{
	int i, color;
	int sorted[MAX_NUM_NET_PLAYERS];
WINDOS(
	dd_grs_canvas *tempcanvas,
   grs_canvas *tempcanvas
);
   
   if (Game_mode & GM_MULTI_COOP)
	 {
	  kmatrix_redraw_coop();
	  return;
	 }
         
	multi_sort_kill_list();

	WINDOS(
	   tempcanvas=dd_gr_create_canvas( grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h ),
   	tempcanvas=gr_create_canvas( grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h )
	);

   WINDOS(
		dd_gr_set_current_canvas(tempcanvas),
		gr_set_current_canvas (tempcanvas)
	);

	WINDOS (
		dd_gr_blt_notrans(StarBackCanvas, 0,0,0,0, tempcanvas,0,0,0,0),
	  	gr_bitmap (0,0,&StarBackCanvas->cv_bitmap)
	);

WIN(DDGRLOCK(dd_grd_curcanv));
	grd_curcanv->cv_font = MEDIUM3_FONT;

	gr_string( 0x8000, LHY(10), TXT_KILL_MATRIX_TITLE	);

	grd_curcanv->cv_font = SMALL_FONT;

	multi_get_kill_list(sorted);

	kmatrix_draw_names(sorted);

	for (i=0; i<N_players; i++ )		{
//		mprintf((0, "Sorted kill list pos %d = %d.\n", i+1, sorted[i]));

		if (Game_mode & GM_TEAM)
			color = get_team(sorted[i]);
		else
			color = sorted[i];

     if (Players[sorted[i]].connected==0)
       gr_set_fontcolor(gr_find_closest_color(31,31,31),-1);
     else
       gr_set_fontcolor(gr_getcolor(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );

     kmatrix_draw_item( i, sorted );
	}

	kmatrix_draw_deaths(sorted);
WIN(DDGRUNLOCK(dd_grd_curcanv));

WINDOS(
	dd_gr_set_current_canvas(NULL),
	gr_set_current_canvas(NULL)
);

#if defined(POLY_ACC)
    pa_save_clut();
    pa_update_clut(gr_palette, 0, 256, 0);
#endif

PA_DFX (pa_set_frontbuffer_current());

WINDOS(
	dd_gr_blt_notrans(tempcanvas, 0,0,0,0, dd_grd_curcanv, 0,0,0,0),
  	gr_bitmap (0,0,&tempcanvas->cv_bitmap)
);

PA_DFX (pa_set_backbuffer_current());

#if defined(POLY_ACC)
    pa_restore_clut();
#endif

	gr_palette_load(gr_palette);
WINDOS(
	dd_gr_free_canvas(tempcanvas),
	gr_free_canvas (tempcanvas)
);
}

void kmatrix_redraw_coop()
{
	int i, color;
	int sorted[MAX_NUM_NET_PLAYERS];

WINDOS(
	dd_grs_canvas *tempcanvas,
	grs_canvas *tempcanvas
);
        
	multi_sort_kill_list();

WINDOS(
   tempcanvas=dd_gr_create_canvas( grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h ),
   tempcanvas=gr_create_canvas( grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h )
);
WINDOS(
	dd_gr_set_current_canvas(tempcanvas),
   gr_set_current_canvas (tempcanvas)
);

WINDOS (
	dd_gr_blt_notrans(StarBackCanvas, 0,0,0,0, tempcanvas,0,0,0,0),
  	gr_bitmap (0,0,&StarBackCanvas->cv_bitmap)
);



WIN(DDGRLOCK(dd_grd_curcanv));
	grd_curcanv->cv_font = MEDIUM3_FONT;
	gr_string( 0x8000, LHY(10), "COOPERATIVE SUMMARY"	);

	grd_curcanv->cv_font = SMALL_FONT;

	multi_get_kill_list(sorted);

	kmatrix_draw_coop_names(sorted);

	for (i=0; i<N_players; i++ )		{

     color = sorted[i];

     if (Players[sorted[i]].connected==0)
       gr_set_fontcolor(gr_find_closest_color(31,31,31),-1);
     else
       gr_set_fontcolor(gr_getcolor(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );

     kmatrix_draw_coop_item( i, sorted );
	}

	kmatrix_draw_deaths(sorted);
WIN(DDGRUNLOCK(dd_grd_curcanv));

WINDOS(
	dd_gr_set_current_canvas(NULL),
	gr_set_current_canvas(NULL)
);

#if defined(POLY_ACC)
    pa_save_clut();
    pa_update_clut(gr_palette, 0, 256, 0);
#endif

PA_DFX (pa_set_frontbuffer_current());

WINDOS(
	dd_gr_blt_notrans(tempcanvas, 0,0,0,0, dd_grd_curcanv, 0,0,0,0),
  	gr_bitmap (0,0,&tempcanvas->cv_bitmap)
);

PA_DFX (pa_set_backbuffer_current());

#if defined(POLY_ACC)
    pa_restore_clut();
#endif

	gr_palette_load(gr_palette);
WINDOS(
	dd_gr_free_canvas(tempcanvas),
	gr_free_canvas (tempcanvas)
);
}

#define MAX_VIEW_TIME   F1_0*15
#define ENDLEVEL_IDLE_TIME	F1_0*10

fix StartAbortMenuTime;


#ifdef MACINTOSH
extern void load_stars_palette();
#endif

extern void network_endlevel_poll3( int nitems, struct newmenu_item * menus, int * key, int citem );

void kmatrix_view(int network)
{											  
   int i, k, done,choice;
	fix entry_time = timer_get_approx_seconds();
	int key;
   int oldstates[MAX_PLAYERS];
   int previous_seconds_left=-1;
   int num_ready,num_escaped;

 	network=Game_mode & GM_NETWORK;
 
   for (i=0;i<MAX_NUM_NET_PLAYERS;i++)
   	digi_kill_sound_linked_to_object (Players[i].objnum);
 	
   set_screen_mode( SCREEN_MENU );

	WINDOS(
		StarBackCanvas=dd_gr_create_canvas( grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h ),
		StarBackCanvas=gr_create_canvas( grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h )
	);
	WINDOS(
		dd_gr_set_current_canvas(StarBackCanvas),
	   gr_set_current_canvas(StarBackCanvas)
	);
	#ifdef MACINTOSH
	if (virtual_memory_on) {
		load_stars_palette();		// horrible hack to prevent too much paging when doing endlevel syncing
		gr_clear_canvas( BM_XRGB(0, 0, 0) );
	} else
	#endif							// note link to above if/else pair
		load_stars();
   
   WaitingForOthers=0;
   kmatrix_redraw();

	//@@gr_palette_fade_in( gr_palette,32, 0);
	game_flush_inputs();

	done = 0;

   for (i=0;i<N_players;i++)
    oldstates[i]=Players[i].connected;

	if (network)
	   network_endlevel(&key);

	while(!done)	{
	#ifdef WINDOWS
		MSG msg;

		DoMessageStuff(&msg);

		DDGRRESTORE;

	#endif

      kmatrix_kills_changed = 0;
      for (i=0; i<4; i++ ) 
          if (joy_get_button_down_cnt(i)>0) 
			  {  
				  #if defined (D2_OEM)
					 if (Current_level_num==8)
						{
	                Players[Player_num].connected=0;
						 if (network)
		                network_send_endlevel_packet();
						WINDOS(
							dd_gr_free_canvas(StarBackCanvas),
						   gr_free_canvas (StarBackCanvas)
						);
					    multi_leave_game();
				       Kmatrix_nomovie_message=0;
				       longjmp(LeaveGame, 0); 
	                return;
						}
				  #endif
		
            Players[Player_num].connected=7;
				if (network)
   	         network_send_endlevel_packet();
				break;
			  } 			
    	for (i=0; i<3; i++ )	
          if (mouse_button_down_count(i)>0) 
				{
				  #if defined (D2_OEM)
					 if (Current_level_num==8)
						{
	                Players[Player_num].connected=0;
						 if (network)
		                network_send_endlevel_packet();
						WINDOS(
							dd_gr_free_canvas(StarBackCanvas),
						   gr_free_canvas (StarBackCanvas)
						);
					    multi_leave_game();
				       Kmatrix_nomovie_message=0;
				       longjmp(LeaveGame, 0); 
	                return;
						}
				  #endif
					Players[Player_num].connected=7;
				   if (network)
	               network_send_endlevel_packet();
					break;
		      }			

		//see if redbook song needs to be restarted
		songs_check_redbook_repeat();

		k = key_inkey();
		switch( k )	{
			case KEY_ENTER:
			case KEY_SPACEBAR:
 				  if ((Game_mode & GM_SERIAL) || (Game_mode & GM_MODEM))		
					{
						done=1;
						break;
					}
					
				  #if defined (D2_OEM)
					 if (Current_level_num==8)
						{
	                Players[Player_num].connected=0;
						 if (network)
		                network_send_endlevel_packet();
						WINDOS(
							dd_gr_free_canvas(StarBackCanvas),
						   gr_free_canvas (StarBackCanvas)
						);
					    multi_leave_game();
				       Kmatrix_nomovie_message=0;
				       longjmp(LeaveGame, 0); 
	                return;
						}
				  #endif
		
              Players[Player_num].connected=7;
				  if (network)	
               network_send_endlevel_packet();
              break;
			case KEY_ESC:
				  if (Game_mode & GM_NETWORK)
					{
					 StartAbortMenuTime=timer_get_approx_seconds();
	   		  	 choice=nm_messagebox1( NULL,network_endlevel_poll3, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME );
					}
				  else
		          choice=nm_messagebox( NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME );
              if (choice==0)
               {
                Players[Player_num].connected=0;
					 if (network)
	                network_send_endlevel_packet();
						WINDOS(
							dd_gr_free_canvas(StarBackCanvas),
						   gr_free_canvas (StarBackCanvas)
						);
				    multi_leave_game();
			       Kmatrix_nomovie_message=0;
			       longjmp(LeaveGame, 0); 
                return;
               }
              else
               kmatrix_kills_changed=1;
           
				  break;

			case KEY_PRINT_SCREEN:
				save_screen_shot(0);
				break;
			case KEY_BACKSP:
				Int3();
				break;
			default:
				break;
		}
		if (timer_get_approx_seconds() >= (entry_time+MAX_VIEW_TIME) && Players[Player_num].connected!=7)
			{
				  #if defined (D2_OEM)
					 if (Current_level_num==8)
						{
	                Players[Player_num].connected=0;
						 if (network)
		                network_send_endlevel_packet();
						WINDOS(
							dd_gr_free_canvas(StarBackCanvas),
						   gr_free_canvas (StarBackCanvas)
						);
					    multi_leave_game();
				       Kmatrix_nomovie_message=0;
				       longjmp(LeaveGame, 0); 
	                return;
						}
				  #endif
		
			 if ((Game_mode & GM_SERIAL) || (Game_mode & GM_MODEM))		
				{
					done=1;
					break;
				}
          Players[Player_num].connected=7;
			 if (network)
	          network_send_endlevel_packet();
		   }	 

		if (network && (Game_mode & GM_NETWORK))
		{
        network_endlevel_poll2(0, NULL, &key, 0);
                       
        for (num_escaped=0,num_ready=0,i=0;i<N_players;i++)
          {
            if (Players[i].connected && i!=Player_num)
               {
                // Check timeout for idle players
                if (timer_get_approx_seconds() > LastPacketTime[i]+ENDLEVEL_IDLE_TIME)
                   {
                    mprintf((0, "idle timeout for player %d.\n", i));
			      	  Players[i].connected = 0;
                    network_send_endlevel_sub(i);
                   }                        
                }

               if (Players[i].connected!=oldstates[i])
                  {
                   if (ConditionLetters[Players[i].connected]!=ConditionLetters[oldstates[i]])
                     kmatrix_kills_changed=1;
                   oldstates[i]=Players[i].connected;
						 network_send_endlevel_packet();
                  }
					if (Players[i].connected==0 || Players[i].connected==7)
					  num_ready++;

               if (Players[i].connected!=1)
                   num_escaped++;
          }

         if (num_ready>=N_players)
             done=1;
         if (num_escaped>=N_players)
             Countdown_seconds_left=-1;

         if (previous_seconds_left != Countdown_seconds_left)
            {
             previous_seconds_left=Countdown_seconds_left;
				 kmatrix_kills_changed=1;
				}
                                                 
         if ( kmatrix_kills_changed )    
            {
             kmatrix_redraw();
             kmatrix_kills_changed=0;
	   		}

		}
	}

  Players[Player_num].connected=7;

  if (network)	
     network_send_endlevel_packet();  // make sure

       
// Restore background and exit
	gr_palette_fade_out( gr_palette, 32, 0 );

	game_flush_inputs();

	WINDOS(
		dd_gr_free_canvas(StarBackCanvas),
		gr_free_canvas (StarBackCanvas)
	);

   Kmatrix_nomovie_message=0;

}

#endif

