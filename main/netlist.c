/*
 * $Source: /cvs/cvsroot/d2x/main/netlist.c,v $
 * $Revision: 1.2 $
 * $Author: bradleyb $
 * $Date: 2002-02-14 09:24:19 $
 *
 * Descent II-a-like network game join menu
 * Arne de Bruijn, 1998
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2002/02/14 09:05:33  bradleyb
 * Lotsa networking stuff from d1x
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pstypes.h"
#include "timer.h"
#include "gr.h"
#include "palette.h"
#include "inferno.h"
#include "mono.h"
#include "key.h"
#include "error.h"
#include "netmisc.h"
#include "network.h"
#include "ipx.h"
#include "game.h"
#include "multi.h"
#include "text.h"
//added 4/18/99 Matt Mueller - show radar in game info
#include "multipow.h"
//end addition -MM

#include "gamefont.h"
#include "u_mem.h"

#include "string.h"

//added on 1/5/99 by Victor Rachels for missiondir
#include "cfile.h"
//end this section addition

#define LINE_ITEMS 8
#define MAX_TEXT_LEN 25
// from network.c
extern int Network_games_changed;
extern netgame_info Active_games[MAX_ACTIVE_NETGAMES];
extern AllNetPlayers_info ActiveNetPlayers[MAX_ACTIVE_NETGAMES];
extern int num_active_games;
extern int Network_socket;
void network_listen();
void network_send_game_list_request();
// bitblt.c
void gr_bm_bitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);

typedef struct bkg {
  grs_canvas * menu_canvas;
  grs_bitmap * saved;			// The background under the menu.
  grs_bitmap * background;
  int background_is_sub;
} bkg;

extern grs_bitmap nm_background;

struct line_item {
	int x, y;
	int width;
	char *value;
};

void network_draw_game(const netgame_info *game) {
}

char *network_mode_text(const netgame_info *game) {
	//edit 4/18/99 Matt Mueller - GTEAM?  heh.  Should be "TEAM" I think.
	static char *names[4]={"ANRCHY", "TEAM", "ROBO", "COOP"};
	//end edit -MM
#ifndef SHAREWARE
	if (game->gamemode >= 4 ||
	    (game->protocol_version != MULTI_PROTO_VERSION &&
	     (game->protocol_version != MULTI_PROTO_D2X_VER ||
	      game->required_subprotocol > MULTI_PROTO_D2X_MINOR)))
	    return "UNSUP";
#endif
	return names[game->gamemode];
}

char *network_status_text(const netgame_info *game, int activeplayers) {
	switch (game->game_status) {
		case NETSTAT_STARTING:
			return "Forming";
		case NETSTAT_PLAYING:
			if (game->game_flags & NETGAME_FLAG_CLOSED)
				return "Closed";
			else if (activeplayers == game->max_numplayers)
				return "Full";
			else
				return "Open";
		default:
			return "Between";
	}
}

static void network_update_item(const netgame_info *game, const AllNetPlayers_info *players, struct line_item li[]) {
	int i, activeplayers = 0;
	// get number of active players
	for (i = 0; i < game->numplayers; i++)
		if (players->players[i].connected)
			activeplayers++;
	strcpy(li[1].value, game->protocol_version == MULTI_PROTO_D2X_VER ? "+" : "");
	strcpy(li[2].value, game->game_name);
	strcpy(li[3].value, network_mode_text(game));

	sprintf(li[4].value, "%d/%d", activeplayers, game->max_numplayers);
#ifndef SHAREWARE
	strcpy(li[5].value, game->mission_title);
#else
	strcpy(li[5].value, "Descent: First Strike");
#endif
	if (game->levelnum < 0)
		sprintf(li[6].value, "S%d", -game->levelnum);
	else	
		sprintf(li[6].value, "%d", game->levelnum);
	strcpy(li[7].value, network_status_text(game, activeplayers));
}

static void update_items(struct line_item lis[MAX_ACTIVE_NETGAMES][LINE_ITEMS]) {
	int i, j;

	for (i = 0; i < MAX_ACTIVE_NETGAMES; i++) {
		if (i >= num_active_games) {
			for (j = 1; j < LINE_ITEMS; j++)
				lis[i][j].value[0] = 0;
		} else
			network_update_item(&Active_games[i], &ActiveNetPlayers[i], lis[i]);
	}
}

int network_menu_hskip=4;
int network_menu_width[LINE_ITEMS] = { 10, 6, 72, 37, 38, 66, 25, 40 };
int ref_network_menu_width[LINE_ITEMS] = { 10, 6, 72, 37, 38, 66, 25, 40 };
char *network_menu_title[LINE_ITEMS] = { "", "", "Game", "Mode", "#Plrs", "Mission", "Lev", "Status" };

static int selected_game;

static void draw_back(bkg *b, int x, int y, int w, int h) {
       gr_bm_bitblt(b->background->bm_w-15, h, 5, y, 5, y, b->background, &(grd_curcanv->cv_bitmap) );
}

static void draw_item(bkg *b, struct line_item *li, int is_current) {
	int i, w, h, aw, max_w, pad_w, y;
	char str[MAX_TEXT_LEN], *p;

	y = li[0].y;
	if (is_current)
		gr_set_fontcolor(BM_XRGB(31, 27, 6), -1);
	else
		gr_set_fontcolor(BM_XRGB(17, 17, 26), -1);
	gr_get_string_size(" ...", &pad_w, &h, &aw);
	draw_back(b, li[0].x, y, 
	 li[LINE_ITEMS-1].x + li[LINE_ITEMS-1].width - li[0].x, h);
	for (i = 0; i < LINE_ITEMS; i++) {
		strcpy(str, li[i].value);
		gr_get_string_size(str, &w, &h, &aw);
		if (w > li[i].width) {
			max_w = li[i].width - pad_w;
			p = str + strlen(str);
			while (p > str && w > max_w) {
				*(--p) = 0;
				gr_get_string_size(str, &w, &h, &aw);
			}
			strcpy(p, " ...");
		}
		gr_ustring(li[i].x, y, str);
	}
}

static void draw_list(bkg *bg,
 struct line_item lis[MAX_ACTIVE_NETGAMES][LINE_ITEMS]) {
	int i;
	update_items(lis);
	for (i = 0; i < MAX_ACTIVE_NETGAMES; i++) {
		draw_item(bg, lis[i], i == selected_game);
	}
}
static void init_background(bkg *bg, int x, int y, int w, int h) {
	bg->menu_canvas = gr_create_sub_canvas( &grd_curscreen->sc_canvas, x, y, w, h );
	gr_set_current_canvas( bg->menu_canvas );

	// Save the background under the menu...
	bg->saved = gr_create_bitmap( w, h );
	Assert( bg->saved != NULL );
	gr_bm_bitblt(w, h, 0, 0, 0, 0, &grd_curcanv->cv_bitmap, bg->saved );
	gr_set_current_canvas( NULL );
	nm_draw_background(x,y,x+w-1,y+h-1);
	if (w > nm_background.bm_w || h > nm_background.bm_h){
		bg->background=gr_create_bitmap(w,h);
		gr_bitmap_scale_to(&nm_background,bg->background);
		bg->background_is_sub=0;
	}else{
		bg->background = gr_create_sub_bitmap(&nm_background,0,0,w,h);
		bg->background_is_sub=1;
	}
	gr_set_current_canvas( bg->menu_canvas );
}

static void done_background(bkg *bg) {
	gr_set_current_canvas(bg->menu_canvas);
	gr_bitmap(0, 0, bg->saved); 	
	gr_free_bitmap(bg->saved);
	if (bg->background_is_sub)
		gr_free_sub_bitmap( bg->background );
	else
		gr_free_bitmap( bg->background );
	gr_free_sub_canvas( bg->menu_canvas );
}


//added on 9/16-18/98 by Victor Rachels to add info boxes to netgameslist
void show_game_players(netgame_info game, AllNetPlayers_info players)
{
 char pilots[(CALLSIGN_LEN+2)*MAX_PLAYERS];
 int i;

  memset(pilots,0,sizeof(char)*(CALLSIGN_LEN+2)*MAX_PLAYERS);

   for(i=0;i<game.numplayers;i++)
    if(players.players[i].connected)
     {
      strcat(pilots,players.players[i].callsign);
      strcat(pilots,"\n");
     }

  nm_messagebox("Players", 1, TXT_OK, pilots);
}

void show_game_score(netgame_info game, AllNetPlayers_info players);

int show_game_stats(netgame_info game, AllNetPlayers_info players, int awesomeflag)
{
 //edited 4/18/99 Matt Mueller - show radar flag and banned stuff too
 //switched to the info/rinfo mode to 1)allow sprintf easily 2) save some
 //cpu cycles not seeking to the end of the string each time.
 char rinfo[512],*info=rinfo;

  memset(info,0,sizeof(char)*256);

#ifndef SHAREWARE
   if(!game.mission_name)
#endif
    info+=sprintf(info,"Level: Descent: First Strike");
#ifndef SHAREWARE
   else
    info+=sprintf(info,game.mission_name);
#endif

   switch(game.gamemode)
    {
     case 0 : info+=sprintf(info,"\nMode:  Anarchy"); break;
     case 1 : info+=sprintf(info,"\nMode:  Team"); break;
     case 2 : info+=sprintf(info,"\nMode:  Robo"); break;
     case 3 : info+=sprintf(info,"\nMode:  Coop"); break;
     default: info+=sprintf(info,"\nMode:  Unknown"); break;
    }

   switch(game.difficulty)
    {
     case 0 : info+=sprintf(info,"\nDiff:  Trainee"); break;
     case 1 : info+=sprintf(info,"\nDiff:  Rookie"); break;
     case 2 : info+=sprintf(info,"\nDiff:  Hotshot"); break;
     case 3 : info+=sprintf(info,"\nDiff:  Ace"); break;
     case 4 : info+=sprintf(info,"\nDiff:  Insane"); break;
     default: info+=sprintf(info,"\nDiff:  Unknown"); break;
    }

   switch(game.protocol_version)
    {
     case 1 : info+=sprintf(info,"\nPrtcl: Shareware"); break;
     case 2 : info+=sprintf(info,"\nPrtcl: Normal"); break;
     case 3 : info+=sprintf(info,"\nPrtcl: D2X only"); break;
     case 0 : info+=sprintf(info,"\nPrtcl: D2X hybrid"); break;
     default: info+=sprintf(info,"\nPrtcl: Unknown"); break;
    }

   if(game.game_flags & NETGAME_FLAG_SHOW_MAP)
	   info+=sprintf(info,"\n Players on Map");
   //edited 04/19/99 Matt Mueller - check protocol_ver too.
   if (game.protocol_version==MULTI_PROTO_D2X_VER 
#ifndef SHAREWARE
&& game.subprotocol>=1
#endif
){
#ifndef SHAREWARE
           int i=0,b=0;
           char sep0=':';
	   if(game.flags & NETFLAG_ENABLE_RADAR)
		   info+=sprintf(info,"\n Radar");
           if(game.flags & NETFLAG_ENABLE_ALT_VULCAN)
                   info+=sprintf(info,"\n Alt Vulcan");
	   info+=sprintf(info,"\n Banned");
#define NETFLAG_SHOW_BANNED(D,V) if (!(game.flags&NETFLAG_DO ## D)) {if (i>3) i=0; else i++; info+=sprintf(info,"%c %s",i?sep0:'\n',#V);b++;sep0=',';}
	   NETFLAG_SHOW_BANNED(LASER,laser);
	   NETFLAG_SHOW_BANNED(QUAD,quad);
	   NETFLAG_SHOW_BANNED(VULCAN,vulcan);
	   NETFLAG_SHOW_BANNED(SPREAD,spread);
	   NETFLAG_SHOW_BANNED(PLASMA,plasma);
	   NETFLAG_SHOW_BANNED(FUSION,fusion);
	   NETFLAG_SHOW_BANNED(HOMING,homing);
	   NETFLAG_SHOW_BANNED(SMART,smart);
	   NETFLAG_SHOW_BANNED(MEGA,mega);
	   NETFLAG_SHOW_BANNED(PROXIM,proxim);
	   NETFLAG_SHOW_BANNED(CLOAK,cloak);
	   NETFLAG_SHOW_BANNED(INVUL,invuln);
	   if (b==0) info+=sprintf(info,": none");
#endif /* ! SHAREWARE */
   }
   //end edit -MM
 //end edit -MM
	if (awesomeflag){
		int c;
		while (1){
			c=nm_messagebox("Stats", 3, "Players","Scores","Join Game", rinfo);
			if (c==0)
				show_game_players(game, players);
			else if (c==1)
				show_game_score(game, players);
			else if (c==2)
				return 1;
			else
				return 0;
		}
	}else{
	   nm_messagebox("Stats", 1, TXT_OK, rinfo);
	   return 0;
	}
}
//end this section addition - Victor Rachels

//added on 11/12/98 by Victor Rachels to maybe fix some bugs
void alphanumize(char *string, int length);

void alphanumize(char *string, int length)
{
 int i=0;
  while(i<length && string[i])
   {
     if(!((string[i]<=122&&string[i]>=97) ||
          (string[i]<=90 &&string[i]>=65) ||
          (string[i]<=57 &&string[i]>=48) ||
          (string[i]=='\n') ||
          (string[i]==' ')  ||
          (string[i]=='-')
          ) )
      string[i] = '_';
    i++;
   }
}

//added on 10/12/98 by Victor Rachels to show netgamelist scores
void show_game_score(netgame_info game, AllNetPlayers_info players)
{
 char scores[2176]="",info[2048]="",deaths[128]="",tmp[10];
 int i,j,k;

  strcat(scores,"Pilots ");
  strcat(deaths,"    ");

   for(i=0;i<game.numplayers;i++)
    if(players.players[i].connected)
     {
       strcat(scores,"     ");
       sprintf(tmp,"%c",players.players[i].callsign[0]);
       strcat(scores,tmp);
       strcat(info,players.players[i].callsign);
        for(k=strlen(players.players[i].callsign);k<9;k++)
         strcat(info," ");
        for(j=0;j<game.numplayers;j++)
         if(players.players[j].connected)
          {
           if(i==j&&game.kills[i][j]>0)
            {
             strcat(info,"  ");
             sprintf(tmp,"-%i",game.kills[i][j]);
            }
           else
            {
             strcat(info,"   ");
             sprintf(tmp,"%i",game.kills[i][j]);
            }
           strcat(info,tmp);
            for(k=strlen(tmp);k<3;k++)
             strcat(info," ");
          }
       sprintf(tmp,"     %i",game.player_kills[i]);
       strcat(info,tmp);
       strcat(info,"\n\n");
       sprintf(tmp,"     %i",game.killed[i]);
       strcat(deaths,tmp);
     }
  strcat(scores,"   Total\n\n");
  strcat(scores,info);
  strcat(deaths,"   \n\n");
  strcat(scores,deaths);
  alphanumize(scores,sizeof(scores));
  nm_messagebox_fixedfont("Score\n", 1, TXT_OK, scores);
}
//end this section addition - Victor Rachels

//added on 2000/01/29 by Matt Mueller for direct ip join.
#include "netpkt.h"
int network_do_join_game(netgame_info *jgame);
int get_and_show_netgame_info(ubyte *server, ubyte *node, ubyte *net_address){
	sequence_packet me;
	fix nextsend;
	int numsent;
	fix curtime;

	if (setjmp(LeaveGame))
		return 0;
    num_active_games = 0;
	Network_games_changed = 0;
	Network_status = NETSTAT_BROWSING;
    memset(Active_games, 0, sizeof(netgame_info)*MAX_ACTIVE_NETGAMES);

	nextsend=0;numsent=0;

	while (1){
		curtime=timer_get_fixed_seconds();
		if (nextsend<curtime){
			if (numsent>=5)
				return 0;//timeout
			nextsend=curtime+F1_0*3;
			numsent++;
			mprintf((0, "Sending game_list request.\n"));
			memcpy( me.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1 );
			memcpy( me.player.network.ipx.node, ipx_get_my_local_address(), 6 );
			memcpy( me.player.network.ipx.server, ipx_get_my_server_address(), 4 );
			me.type = PID_D2X_GAME_INFO_REQ;//get full info.

			send_sequence_packet( me, server,node,net_address);
		}

		network_listen();

		if (Network_games_changed){
			if (num_active_games<1){
				Network_games_changed=0;
				continue;
			}
			if (show_game_stats(Active_games[0], ActiveNetPlayers[0], 1))
				return (network_do_join_game(&Active_games[0]));
			else
				return 0;
		}
		if (key_inkey()==KEY_ESC)
			return 0;

	}
	return 0;
}
//end addition -MM

//added on 1/5/99 by Victor Rachels for missiondir
void change_missiondir()
{
 newmenu_item m[1];
 int i=1;
 char thogdir[64];

  sprintf(thogdir,AltHogDir);

  m[0].type = NM_TYPE_INPUT; m[0].text = thogdir; m[0].text_len = 64;

  i=newmenu_do1(NULL, "Mission Directory", 1, m, NULL, 0);

   if(i==0)
    {
     cfile_use_alternate_hogdir(thogdir);
     sprintf(thogdir,AltHogDir);
     m[0].text=thogdir;
    }
}
//end this section addition - VR

//added/changed on 9/17/98 by Victor Rachels for netgame info screen redraw
//this was mostly a bunch of random moves and copies from the main function
//if you can figure this out more elegantly, go for it.
void netlist_redraw(bkg bg,
                    char menu_text[MAX_ACTIVE_NETGAMES][LINE_ITEMS][MAX_TEXT_LEN],
                    struct line_item lis[MAX_ACTIVE_NETGAMES][LINE_ITEMS])
{
 int i,j,k,yp;

        gr_set_current_canvas( NULL );

        init_background(&bg, 0, 7, grd_curcanv->cv_bitmap.bm_w,
         grd_curcanv->cv_bitmap.bm_h - 14);

	yp=22;
	gr_set_curfont(Gamefonts[GFONT_BIG_1]);
	gr_string(0x8000, yp, "Netgames");//yp was 22
	yp+=grd_curcanv->cv_font->ft_h+network_menu_hskip*3+Gamefonts[GFONT_SMALL]->ft_h;//need to account for size of current socket, drawn elsewhere
	// draw titles
	gr_set_curfont(Gamefonts[GFONT_SMALL]);
	gr_set_fontcolor(BM_XRGB(27, 27, 27), -1);
	k = 15;
	for (j = 0; j < LINE_ITEMS; j++) {
		gr_ustring(k, yp, network_menu_title[j]);//yp was 61
		k += network_menu_width[j];
	}
	

	for (i = 0; i < MAX_ACTIVE_NETGAMES; i++) {
		struct line_item *li = lis[i];
		k=15;

		yp+=grd_curcanv->cv_font->ft_h+network_menu_hskip;
		for (j = 0; j < LINE_ITEMS; j++) {
			li[j].x = k;
			li[j].y = yp;
			//			li[j].y = 70 + i * 9;
			li[j].width = network_menu_width[j] - (j > 1 ? 4 : 0); // HACK!
			k += network_menu_width[j];
			li[j].value = menu_text[i][j];
		}
		sprintf(li[0].value, "%d.", i + 1); 
	}

}
//end this section addition - Victor Rachels

int network_join_game_menu() {
        char menu_text[MAX_ACTIVE_NETGAMES][LINE_ITEMS][MAX_TEXT_LEN];
        struct line_item lis[MAX_ACTIVE_NETGAMES][LINE_ITEMS];
        int k;
	int old_select, old_socket, done, last_num_games;
	grs_canvas * save_canvas;
	grs_font * save_font;
	bkg bg;
	fix t, req_timer = 0;
	
	selected_game = 0;
	gr_palette_fade_in( gr_palette, 32, 0 );
	save_canvas = grd_curcanv;
        gr_set_current_canvas( NULL );
	save_font = grd_curcanv->cv_font;
	
	for (k=0;k<LINE_ITEMS;k++)//scale columns to fit screen res.
		network_menu_width[k]=ref_network_menu_width[k]*grd_curcanv->cv_bitmap.bm_w/320;
	network_menu_hskip=(grd_curcanv->cv_bitmap.bm_h-Gamefonts[GFONT_BIG_1]->ft_h-22-Gamefonts[GFONT_SMALL]->ft_h*17)/17;

	init_background(&bg, 0, 7, grd_curcanv->cv_bitmap.bm_w,
	 grd_curcanv->cv_bitmap.bm_h - 14);

	game_flush_inputs();

//added/changed on 9/17/98 by Victor Rachels for netgame info screen redraw
        netlist_redraw(bg,menu_text,lis);
//end this section addition - Victor Rachels

	Network_games_changed = 1;
        old_socket = -32768;
	old_select = -1;
	last_num_games = 0;
	if ( gr_palette_faded_out ) {
	    gr_palette_fade_in( gr_palette, 32, 0 );
	}

	done = 0;

	while (!done) {
		if (Network_socket != old_socket) {
			gr_set_fontcolor(BM_XRGB(27, 27, 27), -1);
			draw_back(&bg, 30, 22+Gamefonts[GFONT_BIG_1]->ft_h+network_menu_hskip*2, 250, Gamefonts[GFONT_SMALL]->ft_h+4);//was 52,250,9
			gr_printf(30, 22+Gamefonts[GFONT_BIG_1]->ft_h+network_menu_hskip*2, "Current IPX socket is %+d "
					"(PgUp/PgDn to change)", Network_socket);
			if (old_socket != -32768) { /* changed by user? */
				network_listen();
				ipx_change_default_socket( IPX_DEFAULT_SOCKET + Network_socket );
				num_active_games = 0;
			}
			req_timer -= F1_0 * 5; /* force send request */
			Network_games_changed = 1;
		}
		if (Network_games_changed) {
			if (num_active_games > last_num_games) /* new game? */
				digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
			last_num_games = num_active_games;
			Network_games_changed = 0;
			draw_list(&bg, lis);
                        //added on 9/13/98 by adb to make update-needing arch's work
                        gr_update();
                        //end addition - adb

		} else if (selected_game != old_select) {
			draw_item(&bg, lis[old_select], 0);
			draw_item(&bg, lis[selected_game], 1);
                        //added on 9/13/98 by adb to make update-needing arch's work
                        gr_update();
                        //end addition - adb
		}
		old_socket = Network_socket;
		old_select = selected_game;

		t = timer_get_approx_seconds();
		if (t < req_timer || t > req_timer + F1_0 * 5) {
			req_timer = timer_get_approx_seconds();
			network_send_game_list_request();
		}				
		network_listen();

		k = key_inkey();
		switch (k) {
//added 9/16-17/98 by Victor Rachels for netgamelist info
	//edit 4/18/99 Matt Mueller - use KEY_? defines so it actually works on non-dos
                        case KEY_U: //U = 0x16
                                if(selected_game<num_active_games)
                                 {
                                   show_game_players(Active_games[selected_game], ActiveNetPlayers[selected_game]);
                                   netlist_redraw(bg,menu_text,lis);
                                   old_socket = -32768;
                                   old_select = -1;
                                 }
                                break;
                        case KEY_I: //I = 0x17
                                if(selected_game<num_active_games)
                                 {
                                   show_game_stats(Active_games[selected_game], ActiveNetPlayers[selected_game], 0);
                                   netlist_redraw(bg,menu_text,lis);
                                   old_socket = -32768;
                                   old_select = -1;
                                 }
                                break;
//end this section addition - Victor
//added 10/12/98 by Victor Rachels for netgamelist scores
                        case  KEY_S: //S = 0x1f
                                if(selected_game<num_active_games)
                                 {
                                  show_game_score(Active_games[selected_game], ActiveNetPlayers[selected_game]);
                                  netlist_redraw(bg,menu_text,lis);
                                  old_socket = -32768;
                                  old_select = -1;
                                 }
                                break;
//end this section addition - Victor
                        case KEY_M: //M = 0x32
	//end edit -MM
                                 {
                                  change_missiondir();
                                  netlist_redraw(bg,menu_text,lis);
                                  old_socket = -32768;
                                  old_select = -1;
                                 }
			case KEY_PAGEUP:
			case KEY_PAD9:
				if (Network_socket < 99) Network_socket++;
				break;
			case KEY_PAGEDOWN:
			case KEY_PAD3:
				if (Network_socket > -99) Network_socket--;
				break;
			case KEY_PAD5:
				Network_socket = 0;
				break;
			case KEY_TAB + KEY_SHIFTED:
			case KEY_UP:
			case KEY_PAD8:
				if (selected_game-- == 0)
					selected_game = MAX_ACTIVE_NETGAMES - 1;
				break;
			case KEY_TAB:
			case KEY_DOWN:
			case KEY_PAD2:
				if (++selected_game == MAX_ACTIVE_NETGAMES)
					selected_game = 0;
				break;
			case KEY_ENTER:
			case KEY_PADENTER:
				done = 1;
				break;
#if 0
			//added on 11/20/99 by Victor Rachels to add observer mode
			case KEY_O:
				I_am_observer=1;
				done = 1;
				break;
			//end this section addition - VR
#endif
			case KEY_ESC:
				selected_game = -1;
				done = 1;
				break;
			case KEYS_GR_TOGGLE_FULLSCREEN:
				gr_toggle_fullscreen_menu();
				break;
		}
	}
    done_background(&bg);
	//edited 05/18/99 Matt Mueller - restore the font *after* the canvas, or else we are writing into the free'd memory
	gr_set_current_canvas( save_canvas );
	grd_curcanv->cv_font = save_font;
	//end edit -MM
	return selected_game;
}
