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

char copyright[] = "DESCENT II  COPYRIGHT (C) 1994-1996 PARALLAX SOFTWARE CORPORATION";

#include <conf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pstypes.h"
#include "strutil.h"
#include "console.h"
#include "pa_enabl.h"       //$$POLY_ACC
#include "gr.h"
#include "fix.h"
#include "vecmat.h"
#include "mono.h"
#include "key.h"
#include "timer.h"
#include "3d.h"
#include "bm.h"
#include "inferno.h"
#include "error.h"
#include "game.h"
#include "segment.h"		//for Side_to_verts
#include "u_mem.h"
#include "segpoint.h"
#include "screens.h"
#include "texmap.h"
#include "texmerge.h"
#include "menu.h"
#include "wall.h"
#include "polyobj.h"
#include "effects.h"
#include "digi.h"
#include "iff.h"
#include "pcx.h"
#include "palette.h"
#include "args.h"
#include "sounds.h"
#include "titles.h"
#include "player.h"
#include "text.h"
#include "ipx.h"
#include "newdemo.h"
#include "network.h"
#include "modem.h"
#include "gamefont.h"
#include "kconfig.h"
#include "mouse.h"
#include "joy.h"
#include "newmenu.h"
#include "desc_id.h"
#include "config.h"
#include "joydefs.h"
#include "multi.h"
#include "songs.h"
#include "cfile.h"
#include "gameseq.h"
#include "gamepal.h"
#include "mission.h"
#include "movie.h"
#include "compbit.h"

// #  include "3dfx_des.h"

#if defined(POLY_ACC)
#include "poly_acc.h"
extern int Current_display_mode;        //$$ there's got to be a better way than hacking this.
#endif

#ifdef EDITOR
#include "editor\editor.h"
#include "editor\kdefs.h"
#include "ui.h"
#endif

#include "vers_id.h"

void mem_init(void);
void arch_init(void);
void arch_init_start(void);

//Current version number

ubyte Version_major = 1;		//FULL VERSION
ubyte Version_minor = 2;

static const char desc_id_checksum_str[] = DESC_ID_CHKSUM_TAG "0000";	//4-byte checksum
char desc_id_exit_num = 0;

int Function_mode=FMODE_MENU;		//game or editor?
int Screen_mode=-1;					//game screen or editor screen?

//--unused-- grs_bitmap Inferno_bitmap_title;

int WVIDEO_running=0;		//debugger can set to 1 if running

#ifdef EDITOR
int Inferno_is_800x600_available = 0;
#endif

//--unused-- int Cyberman_installed=0;			// SWIFT device present
ubyte CybermouseActive=0;

int __far descent_critical_error_handler( unsigned deverr, unsigned errcode, unsigned __far * devhdr );

void check_joystick_calibration(void);


//--------------------------------------------------------------------------

extern int piggy_low_memory;


int descent_critical_error = 0;
unsigned descent_critical_deverror = 0;
unsigned descent_critical_errcode = 0;

extern int Network_allow_socket_changes;

extern void vfx_set_palette_sub(ubyte *);

extern int VR_low_res;

extern int Config_vr_type;
extern int Config_vr_resolution;
extern int Config_vr_tracking;
int grd_fades_disabled=1;

#define LINE_LEN	100

void do_joystick_init()
{
 

	if (!args_find( "-nojoystick" ))	{
		con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_6);
		joy_init();
		if ( args_find( "-joyslow" ))	{
			con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_7);
			joy_set_slow_reading(JOY_SLOW_READINGS);
		}
		if ( args_find( "-joypolled" ))	{
			con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_8);
			joy_set_slow_reading(JOY_POLLED_READINGS);
		}
		if ( args_find( "-joybios" ))	{
			con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_9);
			joy_set_slow_reading(JOY_BIOS_READINGS);
		}

	//	Added from Descent v1.5 by John.  Adapted by Samir.
	} else {
		con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_10);
	}
}

//set this to force game to run in low res
int disable_high_res=0;

void do_register_player(ubyte *title_pal)
{
	Players[Player_num].callsign[0] = '\0';

	if (!Auto_demo) 	{

		key_flush();

		//now, before we bring up the register player menu, we need to 
		//do some stuff to make sure the palette is ok.  First, we need to
		//get our current palette into the 2d's array, so the remapping will
		//work.  Second, we need to remap the fonts.  Third, we need to fill 
		//in part of the fade tables so the darkening of the menu edges works

		memcpy(gr_palette,title_pal,sizeof(gr_palette));
		remap_fonts_and_menus(1);
		RegisterPlayer();		//get player's name
	}

}

#ifdef NETWORK
void do_network_init()
{
	if (!args_find( "-nonetwork" ))	{
		int socket=0, showaddress=0, t;
		int ipx_error;

		con_printf(CON_VERBOSE, "\n%s ", TXT_INITIALIZING_NETWORK);
		if ((t=args_find("-socket")))
			socket = atoi( Args[t+1] );
		//@@if ( args_find("-showaddress") ) showaddress=1;
		if ((ipx_error=ipx_init(IPX_DEFAULT_SOCKET+socket,showaddress))==0)	{
  			con_printf(CON_VERBOSE, "%s %d.\n", TXT_IPX_CHANNEL, socket );
			Network_active = 1;
		} else {
			switch( ipx_error )	{
			case 3: 	con_printf(CON_VERBOSE, "%s\n", TXT_NO_NETWORK); break;
			case -2: con_printf(CON_VERBOSE, "%s 0x%x.\n", TXT_SOCKET_ERROR, IPX_DEFAULT_SOCKET+socket); break;
			case -4: con_printf(CON_VERBOSE, "%s\n", TXT_MEMORY_IPX ); break;
			default:
				con_printf(CON_VERBOSE, "%s %d", TXT_ERROR_IPX, ipx_error );
			}
			con_printf(CON_VERBOSE, "%s\n",TXT_NETWORK_DISABLED);
			Network_active = 0;		// Assume no network
		}
		ipx_read_user_file( "descent.usr" );
		ipx_read_network_file( "descent.net" );
		//@@if ( args_find( "-dynamicsockets" ))
		//@@	Network_allow_socket_changes = 1;
		//@@else
		//@@	Network_allow_socket_changes = 0;
	} else {
		con_printf(CON_VERBOSE, "%s\n", TXT_NETWORK_DISABLED);
		Network_active = 0;		// Assume no network
	}
}
#endif

#ifdef SHAREARE
#define PROGNAME "d2demo"
#else
#define PROGNAME "d2"
#endif

extern char Language[];

//can we do highres menus?
extern int MenuHiresAvailable;

#ifdef D2_OEM
int intro_played = 0;
#endif

int open_movie_file(char *filename,int must_have);

#if defined(POLY_ACC)
#define MENU_HIRES_MODE SM_640x480x15xPA
#else
#define MENU_HIRES_MODE SM(640,480)
#endif

//	DESCENT II by Parallax Software
//		Descent Main

//extern ubyte gr_current_pal[];

#ifdef	EDITOR
int	Auto_exit = 0;
char	Auto_file[128] = "";
#endif

int main(int argc,char **argv)
{
	int i,t;
	ubyte title_pal[768];

	
	con_init();  // Initialise the console
	mem_init();

	error_init(NULL, NULL);

	args_init( argc,argv );

	if ( args_find( "-debug") )
	{
		con_threshold.value = (float)2;

	} else
		if ( args_find( "-verbose" ) ) 
		{
			con_threshold.value = (float)1;
		}

	arch_init_start();

	arch_init();

	//tell cfile about our counter
	cfile_set_critical_error_counter_ptr(&descent_critical_error);

	#ifdef SHAREWARE
		cfile_init("d2demo.hog");			//specify name of hogfile
	#else
	#define HOGNAME "descent2.hog"
	if (! cfile_init(HOGNAME)) {		//didn't find HOG.  Check on CD
		#ifdef RELEASE
			Error("Could not find required file <%s>",HOGNAME);
		#endif
	}
	#endif
	
	load_text();

	//print out the banner title
	con_printf(CON_NORMAL, "\nDESCENT 2 %s v%d.%d",VERSION_TYPE,Version_major,Version_minor);
	con_printf(CON_NORMAL, "  %s %s\n", __DATE__,__TIME__);
	con_printf(CON_NORMAL, "%s\n%s\n",TXT_COPYRIGHT,TXT_TRADEMARK);	
	con_printf(CON_NORMAL, "This is a MODIFIED version of Descent 2. Copyright (c) 1999 Peter Hawkins\n");


	if (args_find( "-?" ) || args_find( "-help" ) || args_find( "?" ) ) {
		//print_commandline_help();
		set_exit_message("");
		return(0);
	}

	con_printf(CON_NORMAL, "\n");
	con_printf(CON_NORMAL, TXT_HELP, PROGNAME);		//help message has %s for program name
	con_printf(CON_NORMAL, "\n");

	con_printf(CON_VERBOSE, "\n%s...", "Checking for Descent 2 CD-ROM");

	if ( args_find( "-autodemo" ))
		Auto_demo = 1;

	if ( args_find( "-noscreens" ) )
		Skip_briefing_screens = 1;

	Lighting_on = 1;

//	if (init_graphics()) return 1;

	#ifdef EDITOR
	if (!Inferno_is_800x600_available)	{
		con_printf(CON_NORMAL "The editor will not be available, press any key to start game...\n" );
		Function_mode = FMODE_MENU;
	}
	#endif

	if (!WVIDEO_running)
		con_printf(CON_DEBUG,"WVIDEO_running = %d\n",WVIDEO_running);

	con_printf (CON_VERBOSE, "%s", TXT_VERBOSE_1);
	ReadConfigFile();

#ifdef NETWORK
	do_network_init();
#endif

#if defined(POLY_ACC)
    Current_display_mode = -1;
    game_init_render_buffers(SM_640x480x15xPA, 640, 480, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT );
#else

	if (!VR_offscreen_buffer)	//if hasn't been initialied (by headset init)
		set_display_mode(0);		//..then set default display mode
#endif

	i = args_find( "-xcontrol" );
	if ( i > 0 )	{
		kconfig_init_external_controls( strtol(Args[i+1], NULL, 0), strtol(Args[i+2], NULL, 0) );
	}

	con_printf(CON_VERBOSE, "\n%s\n\n", TXT_INITIALIZING_GRAPHICS);
	if (args_find("-nofade"))
		grd_fades_disabled=1;
	
	if ((t=gr_init())!=0)				//doesn't do much
		Error(TXT_CANT_INIT_GFX,t);

   #ifdef _3DFX
   _3dfx_Init();
   #endif

	// Load the palette stuff. Returns non-zero if error.
	con_printf(CON_DEBUG, "\nInitializing palette system..." );
   gr_use_palette_table(DEFAULT_PALETTE );

	con_printf(CON_DEBUG, "\nInitializing font system..." );
	gamefont_init();	// must load after palette data loaded.

	//determine whether we're using high-res menus & movies
#if !defined(POLY_ACC)
	if (args_find("-nohires") || args_find("-nohighres") || (gr_check_mode(MENU_HIRES_MODE) != 0) || disable_high_res)
		MovieHires = MenuHires = MenuHiresAvailable = 0;
	else
#endif
		//NOTE LINK TO ABOVE!
		MenuHires = MenuHiresAvailable = 1;

	con_printf( CON_DEBUG, "\nInitializing movie libraries..." );
	init_movies();		//init movie libraries

	con_printf(CON_VERBOSE, "\nGoing into graphics mode...\n");
#if defined(POLY_ACC)
	gr_set_mode(SM_640x480x15xPA);
#else
	gr_set_mode(MovieHires?SM(640,480):SM(320,200));
#endif

	#ifndef RELEASE
	if ( args_find( "-notitles" ) ) 
		songs_play_song( SONG_TITLE, 1);
	else
	#endif
	{	//NOTE LINK TO ABOVE!
		int played=MOVIE_NOT_PLAYED;	//default is not played
		int song_playing = 0;

		#ifdef D2_OEM
		#define MOVIE_REQUIRED 0
		#else
		#define MOVIE_REQUIRED 1
		#endif

#ifdef D2_OEM   //$$POLY_ACC, jay.
		{	//show bundler screens
			FILE *tfile;
			char filename[FILENAME_LEN];

			played=MOVIE_NOT_PLAYED;	//default is not played

            played = PlayMovie("pre_i.mve",0);

			if (!played) {
                strcpy(filename,MenuHires?"pre_i1b.pcx":"pre_i1.pcx");

				while ((tfile=fopen(filename,"rb")) != NULL) {
					fclose(tfile);
					show_title_screen( filename, 1, 0 );
                    filename[5]++;
				}
			}
		}
#endif

		#ifndef SHAREWARE
			init_subtitles("intro.tex");
			played = PlayMovie("intro.mve",MOVIE_REQUIRED);
			close_subtitles();
		#endif

		#ifdef D2_OEM
		if (played != MOVIE_NOT_PLAYED)
			intro_played = 1;
		else {						//didn't get intro movie, try titles

			played = PlayMovie("titles.mve",MOVIE_REQUIRED);

			if (played == MOVIE_NOT_PLAYED) {
#if defined(POLY_ACC)
            gr_set_mode(SM_640x480x15xPA);
#else
				gr_set_mode(MenuHires?SM_640x480V:SM_320x200C);
#endif
				con_printf( CON_DEBUG, "\nPlaying title song..." );
				songs_play_song( SONG_TITLE, 1);
				song_playing = 1;
				con_printf( CON_DEBUG, "\nShowing logo screens..." );
				show_title_screen( MenuHires?"iplogo1b.pcx":"iplogo1.pcx", 1, 1 );
				show_title_screen( MenuHires?"logob.pcx":"logo.pcx", 1, 1 );
			}
		}

		{	//show bundler movie or screens

			FILE *tfile;
			char filename[FILENAME_LEN];
			int movie_handle;

			played=MOVIE_NOT_PLAYED;	//default is not played

			//check if OEM movie exists, so we don't stop the music if it doesn't
			movie_handle = open_movie_file("oem.mve",0);
			if (movie_handle != -1) {
				close(movie_handle);
				played = PlayMovie("oem.mve",0);
				song_playing = 0;		//movie will kill sound
			}

			if (!played) {
				strcpy(filename,MenuHires?"oem1b.pcx":"oem1.pcx");

				while ((tfile=fopen(filename,"rb")) != NULL) {
					fclose(tfile);
					show_title_screen( filename, 1, 0 );
					filename[3]++;
				}
			}
		}
		#endif

		if (!song_playing)
			songs_play_song( SONG_TITLE, 1);
			
	}

   PA_DFX (pa_splash());

	con_printf( CON_DEBUG, "\nShowing loading screen..." );
	{
		//grs_bitmap title_bm;
		int pcx_error;
		char filename[14];

		#ifdef SHAREWARE
		strcpy(filename, "descentd.pcx");
		#else
		#ifdef D2_OEM
		strcpy(filename, MenuHires?"descntob.pcx":"descento.pcx");
		#else
		strcpy(filename, MenuHires?"descentb.pcx":"descent.pcx");
		#endif
		#endif

#if defined(POLY_ACC)
		gr_set_mode(SM_640x480x15xPA);
#else
		gr_set_mode(MenuHires?SM(640,480):SM(320,200));
#endif

		FontHires = MenuHires;

		if ((pcx_error=pcx_read_bitmap( filename, &grd_curcanv->cv_bitmap, grd_curcanv->cv_bitmap.bm_type, title_pal ))==PCX_ERROR_NONE)	{
			//vfx_set_palette_sub( title_pal );
			gr_palette_clear();
			gr_palette_fade_in( title_pal, 32, 0 );
		} else
			Error( "Couldn't load pcx file '%s', PCX load error: %s\n",filename, pcx_errormsg(pcx_error));
	}

	con_printf( CON_DEBUG , "\nDoing bm_init..." );
	#ifdef EDITOR
		bm_init_use_tbl();
	#else
		bm_init();
	#endif

	#ifdef EDITOR
	if (args_find("-hoarddata") != 0) {
		#define MAX_BITMAPS_PER_BRUSH 30
		grs_bitmap * bm[MAX_BITMAPS_PER_BRUSH];
		grs_bitmap icon;
		int nframes;
		short nframes_short;
		ubyte palette[256*3];
		FILE *ofile;
		int iff_error,i;
		char *sounds[] = {"selforb.raw","selforb.r22",		//SOUND_YOU_GOT_ORB			
								"teamorb.raw","teamorb.r22",		//SOUND_FRIEND_GOT_ORB			
								"enemyorb.raw","enemyorb.r22",	//SOUND_OPPONENT_GOT_ORB	
								"OPSCORE1.raw","OPSCORE1.r22"};	//SOUND_OPPONENT_HAS_SCORED

		ofile = fopen("hoard.ham","wb");

	   iff_error = iff_read_animbrush("orb.abm",bm,MAX_BITMAPS_PER_BRUSH,&nframes,palette);
		Assert(iff_error == IFF_NO_ERROR);
		nframes_short = nframes;
		fwrite(&nframes_short,sizeof(nframes_short),1,ofile);
		fwrite(&bm[0]->bm_w,sizeof(short),1,ofile);
		fwrite(&bm[0]->bm_h,sizeof(short),1,ofile);
		fwrite(palette,3,256,ofile);
		for (i=0;i<nframes;i++)
			fwrite(bm[i]->bm_data,1,bm[i]->bm_w*bm[i]->bm_h,ofile);

	   iff_error = iff_read_animbrush("orbgoal.abm",bm,MAX_BITMAPS_PER_BRUSH,&nframes,palette);
		Assert(iff_error == IFF_NO_ERROR);
		Assert(bm[0]->bm_w == 64 && bm[0]->bm_h == 64);
		nframes_short = nframes;
		fwrite(&nframes_short,sizeof(nframes_short),1,ofile);
		fwrite(palette,3,256,ofile);
		for (i=0;i<nframes;i++)
			fwrite(bm[i]->bm_data,1,bm[i]->bm_w*bm[i]->bm_h,ofile);

		for (i=0;i<2;i++) {
			iff_error = iff_read_bitmap(i?"orbb.bbm":"orb.bbm",&icon,BM_LINEAR,palette);
			Assert(iff_error == IFF_NO_ERROR);
			fwrite(&icon.bm_w,sizeof(short),1,ofile);
			fwrite(&icon.bm_h,sizeof(short),1,ofile);
			fwrite(palette,3,256,ofile);
			fwrite(icon.bm_data,1,icon.bm_w*icon.bm_h,ofile);
		}

		for (i=0;i<sizeof(sounds)/sizeof(*sounds);i++) {
			FILE *ifile;
			int size;
			ubyte *buf;
			ifile = fopen(sounds[i],"rb");
			Assert(ifile != NULL);
			size = filelength(ifile->_handle);
			buf = d_malloc(size);
			fread(buf,1,size,ifile);
			fwrite(&size,sizeof(size),1,ofile);
			fwrite(buf,1,size,ofile);
			d_free(buf);
			fclose(ifile);
		}

		fclose(ofile);

		exit(1);
	}
	#endif

	//the bitmap loading code changes gr_palette, so restore it
	memcpy(gr_palette,title_pal,sizeof(gr_palette));

	if ( args_find( "-norun" ) )
		return(0);

	con_printf( CON_DEBUG, "\nInitializing 3d system..." );
	g3_init();

	con_printf( CON_DEBUG, "\nInitializing texture caching system..." );
	texmerge_init( 10 );		// 10 cache bitmaps

	con_printf( CON_DEBUG, "\nRunning game...\n" );
	set_screen_mode(SCREEN_MENU);

	init_game();

	//	If built with editor, option to auto-load a level and quit game
	//	to write certain data.
	#ifdef	EDITOR
	{	int t;
	if ( t = args_find( "-autoload" ) ) {
		Auto_exit = 1;
		strcpy(Auto_file, Args[t+1]);
	}
		
	}

	if (Auto_exit) {
		strcpy(Players[0].callsign, "dummy");
	} else
	#endif
		do_register_player(title_pal);

	gr_palette_fade_out( title_pal, 32, 0 );

	Game_mode = GM_GAME_OVER;

	if (Auto_demo)	{
		newdemo_start_playback("descent.dem");		
		if (Newdemo_state == ND_STATE_PLAYBACK )
			Function_mode = FMODE_GAME;
	}

	//do this here because the demo code can do a longjmp when trying to
	//autostart a demo from the main menu, never having gone into the game
	setjmp(LeaveGame);

	while (Function_mode != FMODE_EXIT)
	{
		switch( Function_mode )	{
		case FMODE_MENU:
			set_screen_mode(SCREEN_MENU);
			if ( Auto_demo )	{
				newdemo_start_playback(NULL);		// Randomly pick a file
				if (Newdemo_state != ND_STATE_PLAYBACK)	
					Error("No demo files were found for autodemo mode!");
			} else {
				#ifdef EDITOR
				if (Auto_exit) {
					strcpy(&Level_names[0], Auto_file);
					LoadLevel(1, 1);
					Function_mode = FMODE_EXIT;
					break;
				}
				#endif

				check_joystick_calibration();
				gr_palette_clear();		//I'm not sure why we need this, but we do
				DoMenu();									 	
				#ifdef EDITOR
				if ( Function_mode == FMODE_EDITOR )	{
					create_new_mine();
					SetPlayerFromCurseg();
					load_palette(NULL,1,0);
				}
				#endif
			}
			break;
		case FMODE_GAME:
			#ifdef EDITOR
				keyd_editor_mode = 0;
			#endif
			game();
			if ( Function_mode == FMODE_MENU )
				songs_play_song( SONG_TITLE, 1 );
			break;
		#ifdef EDITOR
		case FMODE_EDITOR:
			keyd_editor_mode = 1;
			editor();
			_harderr( (void*)descent_critical_error_handler );		// Reinstall game error handler
			if ( Function_mode == FMODE_GAME ) {
				Game_mode = GM_EDITOR;
				editor_reset_stuff_on_level();
				N_players = 1;
			}
			break;
		#endif
		default:
			Error("Invalid function mode %d",Function_mode);
		}
	}

	WriteConfigFile();

	#ifndef RELEASE
	if (!args_find( "-notitles" ))
	#endif

	#ifndef NDEBUG
	if ( args_find( "-showmeminfo" ) )
		show_mem_info = 1;		// Make memory statistics show
	#endif

	return(0);		//presumably successful exit
}


void check_joystick_calibration()	{
	int x1, y1, x2, y2, c;
	fix t1;

	if ( (Config_control_type!=CONTROL_JOYSTICK) &&
		  (Config_control_type!=CONTROL_FLIGHTSTICK_PRO) &&
		  (Config_control_type!=CONTROL_THRUSTMASTER_FCS) &&
		  (Config_control_type!=CONTROL_GRAVIS_GAMEPAD)
		) return;

	joy_get_pos( &x1, &y1 );

	t1 = timer_get_fixed_seconds();
	while( timer_get_fixed_seconds() < t1 + F1_0/100 )
		;

	joy_get_pos( &x2, &y2 );

	// If joystick hasn't moved...
	if ( (abs(x2-x1)<30) &&  (abs(y2-y1)<30) )	{
		if ( (abs(x1)>30) || (abs(x2)>30) ||  (abs(y1)>30) || (abs(y2)>30) )	{
			c = nm_messagebox( NULL, 2, TXT_CALIBRATE, TXT_SKIP, TXT_JOYSTICK_NOT_CEN );
			if ( c==0 )	{
				joydefs_calibrate();
			}
		}
	}

}
