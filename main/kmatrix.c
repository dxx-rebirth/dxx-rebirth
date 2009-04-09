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
 * Kill matrix displayed at end of level.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "error.h"
#include "pstypes.h"
#include "gr.h"
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
#include "rbaudio.h"
#include "multi.h"
#include "kmatrix.h"
#include "gauges.h"
#include "pcx.h"

#ifdef OGL
#include "ogl_init.h"
#endif

#define CENTERING_OFFSET(x) ((300 - (70 + (x)*25 ))/2)
#define CENTERSCREEN (SWIDTH/2)

int kmatrix_kills_changed = 0;
char ConditionLetters[]={' ','P','E','D','E','E','V','W'};
char WaitingForOthers=0;

int Kmatrix_nomovie_message=0;

extern char MaxPowerupsAllowed[],PowerupsInMine[];
extern void newmenu_close();

void kmatrix_reactor (char *message);
void kmatrix_phallic ();
void kmatrix_redraw_coop();

void kmatrix_draw_item( int  i, int *sorted )
{
  int j, x, y;
  char temp[10];

  y = FSPACY(50+i*9);

  // Print player name.

  gr_printf( FSPACX(CENTERING_OFFSET(N_players)), y, "%s", Players[sorted[i]].callsign );

  gr_printf (FSPACX(CENTERING_OFFSET(N_players)-15),y,"%c",ConditionLetters[Players[sorted[i]].connected]);

  for (j=0; j<N_players; j++) {

    x = FSPACX(70 + CENTERING_OFFSET(N_players) + j*25);

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

  x = FSPACX(60 + CENTERING_OFFSET(N_players) + N_players*25);
  gr_set_fontcolor( BM_XRGB(25,25,25),-1 );
  gr_printf( x ,y,"%4d/%s",Players[sorted[i]].net_kills_total,temp);
}

void kmatrix_draw_coop_item( int  i, int *sorted )
{
  int  x, y;

  y = FSPACY(50+i*9);

  // Print player name.

  gr_printf( FSPACX(CENTERING_OFFSET(N_players)), y, "%s", Players[sorted[i]].callsign );
  gr_printf (FSPACX(CENTERING_OFFSET(N_players)-15),y,"%c",ConditionLetters[Players[sorted[i]].connected]);

  x = CENTERSCREEN;

  gr_set_fontcolor( BM_XRGB(60,40,10),-1 );
  gr_printf( x, y, "%d", Players[sorted[i]].score );

  x = CENTERSCREEN+FSPACX(50);

  gr_set_fontcolor( BM_XRGB(60,40,10),-1 );
  gr_printf( x, y, "%d", Players[sorted[i]].net_killed_total);
}


void kmatrix_draw_names(int *sorted)
{
  int j, x, color;

  if (Kmatrix_nomovie_message)
  {
    gr_set_fontcolor( BM_XRGB(63,0,0),-1 );
    gr_printf( CENTERSCREEN-FSPACX(40), FSPACY(20), "(Movie not played)");
  }

  for (j=0; j<N_players; j++) {
    if (Game_mode & GM_TEAM)
      color = get_team(sorted[j]);
    else
      color = sorted[j];

    x = FSPACX (70 + CENTERING_OFFSET(N_players) + j*25);

    if (Players[sorted[j]].connected==0)
      gr_set_fontcolor(gr_find_closest_color(31,31,31),-1);
    else
      gr_set_fontcolor(BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );

    gr_printf( x, FSPACY(40), "%c", Players[sorted[j]].callsign[0] );
  }

  x = FSPACX(72 + CENTERING_OFFSET(N_players) + N_players*25);
  gr_set_fontcolor( BM_XRGB(31,31,31),-1 );
  gr_printf( x, FSPACY(40), "K/E");
}

void kmatrix_draw_coop_names(int *sorted)
{
  sorted=sorted;

  if (Kmatrix_nomovie_message)
  {
    gr_set_fontcolor( BM_XRGB(63,0,0),-1 );
    gr_printf( CENTERSCREEN-FSPACX(40), FSPACY(20), "(Movie not played)");
  }

  gr_set_fontcolor( BM_XRGB(63,31,31),-1 );
  gr_printf( CENTERSCREEN, FSPACY(40), "SCORE");
  gr_set_fontcolor( BM_XRGB(63,31,31),-1 );
  gr_printf( CENTERSCREEN+FSPACX(50), FSPACY(40), "DEATHS");
}


void kmatrix_draw_deaths(int *sorted)
{
  int y,x;
  char reactor_message[50];

  sorted=sorted;

  y = FSPACY(55 + 72 + 35);
  x = FSPACX(35);

  {
    int sw, sh, aw;

    gr_set_fontcolor(gr_find_closest_color(63,20,0),-1);
    gr_get_string_size("P-Playing E-Escaped D-Died", &sw, &sh, &aw);

    gr_printf( CENTERSCREEN-(sw/2), y,"P-Playing E-Escaped D-Died");

    y+=(sh+5);
    gr_get_string_size("V-Viewing scores W-Waiting", &sw, &sh, &aw);

    gr_printf( CENTERSCREEN-(sw/2), y,"V-Viewing scores W-Waiting");

  }

  y+=FSPACY(20);

  {
    int sw, sh, aw;
    gr_set_fontcolor(gr_find_closest_color(63,63,63),-1);

    if (Players[Player_num].connected==CONNECT_KMATRIX_WAITING)
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
    sprintf((char *)&reactor_message, "%s: %d %s  ", TXT_TIME_REMAINING, Countdown_seconds_left, TXT_SECONDS);
    kmatrix_reactor ((char *)&reactor_message);
  }

  if (Game_mode & GM_HOARD) 
    kmatrix_phallic();
}

void kmatrix_draw_coop_deaths(int *sorted)
{
  int j, x, y;
  char reactor_message[50];

  y = FSPACY(55 + N_players * 9);

  gr_set_fontcolor( BM_XRGB(31,31,31),-1 );

  x = CENTERSCREEN+FSPACX(50);
  gr_printf( x, y, TXT_DEATHS );

  for (j=0; j<N_players; j++) {
    x = CENTERSCREEN+FSPACX(50);
    gr_printf( x, y, "%d", Players[sorted[j]].net_killed_total );
  }

  y = FSPACY(55 + 72 + 35);
  x = FSPACX(35);

  {
    int sw, sh, aw;

    gr_set_fontcolor(gr_find_closest_color(63,20,0),-1);
    gr_get_string_size("P-Playing E-Escaped D-Died", &sw, &sh, &aw);

    gr_printf( CENTERSCREEN-(sw/2), y,"P-Playing E-Escaped D-Died");

    y+=(sh+5);
    gr_get_string_size("V-Viewing scores W-Waiting", &sw, &sh, &aw);

    gr_printf( CENTERSCREEN-(sw/2), y,"V-Viewing scores W-Waiting");

  }

  y+=FSPACY(20);

  {
    int sw, sh, aw;

    gr_set_fontcolor(gr_find_closest_color(63,63,63),-1);

    if (Players[Player_num].connected==CONNECT_KMATRIX_WAITING)
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
    sprintf((char *)&reactor_message, "%s: %d %s  ", TXT_TIME_REMAINING, Countdown_seconds_left, TXT_SECONDS);
    kmatrix_reactor ((char *)&reactor_message);
  }
}

void kmatrix_reactor (char *message)
{
  static char oldmessage[50]={0};
  int sw, sh, aw;

  grd_curcanv->cv_font = GAME_FONT;

  if (oldmessage[0]!=0)
  {
    gr_set_fontcolor(gr_find_closest_color(0,0,0),-1);
    gr_get_string_size(oldmessage, &sw, &sh, &aw);
  }

  gr_set_fontcolor(gr_find_closest_color(0,32,63),-1);
  gr_get_string_size(message, &sw, &sh, &aw);
  gr_printf( CENTERSCREEN-(sw/2), FSPACY(55+72+12), message);
  strcpy ((char *)&oldmessage,message);
}

extern int PhallicLimit,PhallicMan;

void kmatrix_phallic ()
{
  int sw, sh, aw;
  char message[80];

  if (!(Game_mode & GM_HOARD))
    return;

  if (PhallicMan==-1)
    strcpy (message,"There was no record set for this level.");
  else
    sprintf (message,"%s had the best record at %d points.",Players[PhallicMan].callsign,PhallicLimit);

  grd_curcanv->cv_font = GAME_FONT;
  gr_set_fontcolor(gr_find_closest_color(63,63,63),-1);
  gr_get_string_size(message, &sw, &sh, &aw);
  gr_printf( CENTERSCREEN-(sw/2), FSPACY(55+72+3), message);
}

void load_stars(void);

void kmatrix_redraw()
{
  int i, pcx_error, color;
  int sorted[MAX_NUM_NET_PLAYERS];

  pcx_error = pcx_read_fullscr(STARS_BACKGROUND, gr_palette);
  Assert(pcx_error == PCX_ERROR_NONE);

  if (Game_mode & GM_MULTI_COOP)
  {
    kmatrix_redraw_coop();
    return;
  }

  multi_sort_kill_list();

  gr_set_current_canvas(NULL);

  grd_curcanv->cv_font = MEDIUM3_FONT;

  if (Game_mode & GM_CAPTURE)
    gr_string( 0x8000, FSPACY(10), "CAPTURE THE FLAG SUMMARY");
  else if (Game_mode & GM_HOARD)
    gr_string( 0x8000, FSPACY(10), "HOARD SUMMARY");
  else
    gr_string( 0x8000, FSPACY(10), TXT_KILL_MATRIX_TITLE);

  grd_curcanv->cv_font = GAME_FONT;

  multi_get_kill_list(sorted);

  kmatrix_draw_names(sorted);

  for (i=0; i<N_players; i++ )  {
    if (Game_mode & GM_TEAM)
      color = get_team(sorted[i]);
    else
      color = sorted[i];

    if (Players[sorted[i]].connected==0)
      gr_set_fontcolor(gr_find_closest_color(31,31,31),-1);
    else
      gr_set_fontcolor(BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );

    kmatrix_draw_item( i, sorted );
  }

  kmatrix_draw_deaths(sorted);

  gr_palette_load(gr_palette);
}

void kmatrix_redraw_coop()
{
  int i, color;
  int sorted[MAX_NUM_NET_PLAYERS];

  multi_sort_kill_list();

  gr_set_current_canvas(NULL);

  grd_curcanv->cv_font = MEDIUM3_FONT;
  gr_string( 0x8000, FSPACY(10), "COOPERATIVE SUMMARY");

  grd_curcanv->cv_font = GAME_FONT;

  multi_get_kill_list(sorted);

  kmatrix_draw_coop_names(sorted);

  for (i=0; i<N_players; i++ )  {

    color = sorted[i];

    if (Players[sorted[i]].connected==0)
      gr_set_fontcolor(gr_find_closest_color(31,31,31),-1);
    else
      gr_set_fontcolor(BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );

    kmatrix_draw_coop_item( i, sorted );
  }

  kmatrix_draw_deaths(sorted);

  gr_palette_load(gr_palette);
}

#define MAX_VIEW_TIME       F1_0*15
#define ENDLEVEL_IDLE_TIME  F1_0*10

fix StartAbortMenuTime;

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

  WaitingForOthers=0;

  game_flush_inputs();

  done = 0;

  for (i=0;i<N_players;i++)
    oldstates[i]=Players[i].connected;

  if (network)
      multi_endlevel(&key);

  while(!done)
  {
		timer_delay2(50);
		kmatrix_redraw();
      kmatrix_kills_changed = 0;

      //see if redbook song needs to be restarted
      RBACheckFinishedHook();

      k = key_inkey();
      switch( k ) {
        case KEY_ENTER:
        case KEY_SPACEBAR:
          if (is_D2_OEM)
          {
            if (Current_level_num==8)
            {
              Players[Player_num].connected=0;
              if (network)
                multi_send_endlevel_packet();
              multi_leave_game();
              Kmatrix_nomovie_message=0;
              longjmp(LeaveGame, 0);
              return;
            }
          }

          Players[Player_num].connected=CONNECT_KMATRIX_WAITING;
          if (network)	
            multi_send_endlevel_packet();
          break;

        case KEY_ESC:
          if (Game_mode & GM_NETWORK)
          {
            StartAbortMenuTime=timer_get_approx_seconds();
            choice=nm_messagebox1( NULL,multi_endlevel_poll3, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME );
          }
          else
            choice=nm_messagebox( NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME );
          if (choice==0)
          {
            Players[Player_num].connected=0;
            if (network)
              multi_send_endlevel_packet();
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

      if (timer_get_approx_seconds() >= (entry_time+MAX_VIEW_TIME) && Players[Player_num].connected!=CONNECT_KMATRIX_WAITING)
      {
        if (is_D2_OEM)
        {
          if (Current_level_num==8)
          {
            Players[Player_num].connected=0;
            if (network)
              multi_send_endlevel_packet();
            multi_leave_game();
            Kmatrix_nomovie_message=0;
            longjmp(LeaveGame, 0);
            return;
          }
        }

        Players[Player_num].connected=CONNECT_KMATRIX_WAITING;
        if (network)
          multi_send_endlevel_packet();
      }

      if (network && (Game_mode & GM_NETWORK))
      {
        multi_endlevel_poll2(0, NULL, &key, 0);

        for (num_escaped=0,num_ready=0,i=0;i<N_players;i++)
        {
          if (Players[i].connected && i!=Player_num)
          {
            // Check timeout for idle players
            if (timer_get_approx_seconds() > Netgame.players[i].LastPacketTime+ENDLEVEL_IDLE_TIME)
            {
              Players[i].connected = 0;
              multi_send_endlevel_sub(i);
            }
          }

		  // Important: Make sure we keep connected state CONNECT_KMATRIX_WAITING even if player exits kmatrix loop which will change to CONNECT_WAITING! If we don't get all palyer packets in sync and order this condition is very handy to keep all connections alive!
		  if ((oldstates[i]==CONNECT_END_MENU || oldstates[i]==CONNECT_KMATRIX_WAITING) && (Players[i].connected!=0 || Players[i].connected!=CONNECT_END_MENU || Players[i].connected!=CONNECT_KMATRIX_WAITING))
			Players[i].connected=CONNECT_KMATRIX_WAITING;

          if (Players[i].connected!=oldstates[i])
          {
            if (ConditionLetters[Players[i].connected]!=ConditionLetters[oldstates[i]])
              kmatrix_kills_changed=1;
            oldstates[i]=Players[i].connected;
            multi_send_endlevel_packet();
          }

          if (Players[i].connected==0 || Players[i].connected==CONNECT_KMATRIX_WAITING)
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
		gr_flip();
  }

  Players[Player_num].connected=CONNECT_KMATRIX_WAITING;

  if (network)
    multi_send_endlevel_packet();  // make sure

  game_flush_inputs();

  Kmatrix_nomovie_message=0;

	newmenu_close();
}
