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


#pragma off (unreferenced)
static char rcsid[] = "$Id: winferno.c,v 1.1.1.1 2001-01-19 03:30:14 bradleyb Exp $";
#pragma on (unreferenced)

#include "desw.h"
#include <mmsystem.h>

#include <stdlib.h>
#include <stdio.h>

#include "gr.h"
#include "gamepal.h"
#include "palette.h"
#include "key.h"
#include "mono.h"
#include "timer.h"
#include "3d.h"
#include "bm.h"
#include "inferno.h"
#include "error.h"
#include "game.h"
#include "mem.h"
#include "screens.h"
#include "texmap.h"
#include "texmerge.h"
#include "menu.h"
#include "polyobj.h"
#include "pcx.h"
#include "args.h"
#include "player.h"
#include "text.h"
#include "gamefont.h"
#include "kconfig.h"
#include "newmenu.h"
#include "desc_id.h"
#include "config.h"
#include "cfile.h"
#include "mission.h"
#include "network.h"
#include "newdemo.h"
#include "digi.h"
#include "songs.h"
#include "ipx.h"
#include "modem.h"
#include "joy.h"
#include "movie.h"

#ifdef _3DFX
#include "3dfx_des.h"
#endif



//	Globals --------------------------------------------------------------------
extern int Network_allow_socket_changes;
extern char gr_palette[3*256];
extern int grd_fades_disabled;
extern int MenuHiresAvailable;
extern int piggy_low_memory;

int Function_mode=FMODE_MENU;		//game or editor?
int Screen_mode=-1;					//game screen or editor screen?
int descent_critical_error = 0;
unsigned descent_critical_deverror = 0;
unsigned descent_critical_errcode = 0;

/// CHANGE THIS.  PUT THIS WHERE IT SHOULD ME.

int WVIDEO_running=1;		//debugger can set to 1 if running


//	Version ID Info. -----------------------------------------------------------

#include "vers_id.h"

//Current version number

ubyte Version_major = 1;		//FULL VERSION
ubyte Version_minor = 2;

int registered_copy=0;
char name_copy[DESC_ID_LEN];

static const char desc_id_checksum_str[] = DESC_ID_CHKSUM_TAG "0000";	//4-byte checksum
char desc_id_exit_num = 0;
time_t t_current_time, t_saved_time;

byte CybermouseActive = 0;

static 	char 	title_pal[768];


//	THIS IS A HACK UNTIL WE HAVE MOVIES
int MovieHires = 1;
char CDROM_dir[30] = "";
int Hogfile_on_CD = 0;


// Function Prototypes --------------------------------------------------------
int init_cdrom();
int InitGraphics(void);
void DisplayDescentTitle(void);
void TestSounds();
void check_joystick_calibration();

void check_id_checksum_and_date();

HANDLE clipboard_renderformat(int cf);
HPALETTE SetPalette(HDC hdc, HPALETTE hPal);
void DestroyPalette(HPALETTE hPal);

#define HOGNAME "descent2.hog"

extern int CD_audio_desktop_dev;
extern DWORD CD_audio_desktop_vol;


//	Functions ------------------------------------------------------------------

void InitCD(char *arg)
{
	int i;

	logentry("Initializing CD-ROM...\n");

	if (!init_cdrom()) {		//look for Cd, etc.
		#ifdef RELEASE
			Error("Sorry, the Descent II CD must be present to run.");
		#endif
	}
	else {		//check to make sure not running off CD
//		if (toupper(arg[0]) == toupper(CDROM_dir[0]))
//			Error("You cannot run Descent II directly from your CD-ROM drive.\n"
//					"       To run Descent II, CD to the directory you installed to, then type D2.\n"
//					"       To install Descent II, type %c:\\INSTALL.",toupper(CDROM_dir[0]));
	}

//	Let's save the current volume of CD-audio-out
//	Find CD-AUDIO device if there is one.
	for (i = 0; i < auxGetNumDevs(); i++)
	{
		AUXCAPS caps;
		auxGetDevCaps(i, &caps, sizeof(caps));
		if (caps.wTechnology == AUXCAPS_CDAUDIO) {
			CD_audio_desktop_dev = i;
			mprintf((0, "CD audio operating on AUX %d.\n", CD_audio_desktop_dev));
			break;
		}
	}
					
	if (CD_audio_desktop_dev != -1) {
	//	 get the volume! 
		auxGetVolume(CD_audio_desktop_dev, &CD_audio_desktop_vol);
		mprintf((1, "CD vol: 0x%x\n", CD_audio_desktop_vol));
	}
}


void InitVideo()
{
	int flag;

	logentry("Initializing Video System...\n");

	if (FindArg("-hw_3dacc")) flag = 1;
	else flag = 0;

	if (!gfxInit(flag)) 
		Error("Unable to initialize GFX system.");

//@@   #ifdef _3DFX
//@@   _3dfx_available = _3dfx_Init();
//@@   #endif
}

void InitIO()
{
	logentry("Initializing IO System...\n");

	key_init();

	if (!win95_controls_init()) 
		logentry("\tUnable to detect an auxillary control.\n");
	mouse_set_window (_hAppWnd);
	mouse_set_mode(0);  				// Non-centering mode
}


void InitData()
{
	logentry("Initializing Data...\n");

	ReadConfigFile();

	if (! cfile_init(HOGNAME)) {		//didn't find HOG.  Check on CD
		#ifdef RELEASE
			Error("Could not find required file <%s>",HOGNAME);
		#endif
	}

	load_text();

//print out the banner title
#ifndef RELEASE
	cprintf("\nDESCENT II %s v%d.%d",VERSION_TYPE,Version_major,Version_minor);
	#ifdef VERSION_NAME
	cprintf("  %s", VERSION_NAME);
	#endif
	cprintf("  %s %s\n", __DATE__,__TIME__);
	cprintf("%s\n%s\n",TXT_COPYRIGHT,TXT_TRADEMARK);	
	cprintf("\n\n");
#endif

	check_id_checksum_and_date();
}


void InitSound()
{
	logentry("Initializing Sound System...\n");

	if (digi_init()) {
		logentry("Unable to initialize sound system.\n");
	}
}


void InitNetwork()
{
	int socket=0, showaddress=0;
	int ipx_error;
	int t;

	logentry("Initializing Networking system...\n");

	if ((t=FindArg("-socket")))
		socket = atoi( Args[t+1] );
		
	if ( FindArg("-showaddress") ) showaddress=1;
		
	if ((ipx_error=ipx_init(IPX_DEFAULT_SOCKET+socket,showaddress))==0)	{
  		mprintf((0, "IPX protocol on channel %x.\n", IPX_DEFAULT_SOCKET+socket ));
		Network_active = 1;
	} 
	else {
		switch( ipx_error )	{
			case 3: 	mprintf((1, "%s\n", TXT_NO_NETWORK)); break;
			case -2: mprintf((1, "%s 0x%x.\n", TXT_SOCKET_ERROR, IPX_DEFAULT_SOCKET+socket)); break;
			case -4: mprintf((1, "%s\n", TXT_MEMORY_IPX )); break;
			default:
				mprintf((1, "\t%s %d\n", TXT_ERROR_IPX, ipx_error ));
		}
			
		logentry("%s\n",TXT_NETWORK_DISABLED);
		Network_active = 0;		// Assume no network
	}
		
	mprintf((0, "Here?"));

	ipx_read_user_file( "descent.usr" );
	ipx_read_network_file( "descent.net" );
}


void InitDescent()
{
	Lighting_on = 1;							//	Turn on lighting for now.

//	Set Display Mode
	
	if (!dd_VR_offscreen_buffer)	
		set_display_mode(0);					

//	Create and Set Initial Palette.
	logentry("Initializing palette system...\n" );
	gr_use_palette_table(DEFAULT_PALETTE);
	gr_palette_load(gr_palette);

	logentry( "Initializing font system...\n" );
	gamefont_init();

	if (FindArg("-lowresmovies")) 
		MovieHires = 0;

	MenuHires = MenuHiresAvailable = 1;

//	Set up movies and title
	logentry("Initializing movie libraries...\n" );
	init_movies();		//init movie libraries

	if (!init_subtitles("intro.tex")) 
		mprintf((1, "Unable to open subtitles.\n"));
	PlayMovie("intro.mve",MOVIE_ABORT_ON);
	HideCursorW();

	close_subtitles();
	songs_play_song( SONG_TITLE, 1);

	FontHires = MenuHires;

	//LoadCursorWin(MOUSE_WAIT_CURSOR);

	DEFINE_SCREEN("DESCENTB.PCX");
	DisplayDescentTitle();	

//	SendMessage(_hAppWnd, WM_SETREDRAW, FALSE, 0);

//	Doing some 3d initialization
	logentry( "Doing bm_init...\n" );
	bm_init();

	DDGRRESTORE

	logentry( "Initializing 3d system...\n" );
	g3_init();

	logentry( "Initializing texture caching system...\n" );
	texmerge_init( 10 );		// 10 cache bitmaps

	logentry("Final initialization...\n" );

	memcpy(gr_palette,title_pal,sizeof(gr_palette));

	set_screen_mode(SCREEN_MENU);

	init_game();

	set_detail_level_parameters(Detail_level);
	
//	SendMessage(_hAppWnd, WM_SETREDRAW, TRUE, 0);
	//LoadCursorWin(MOUSE_DEFAULT_CURSOR);
}


void InitPilot()
{
//	Title Page gone, now start main program
//	Players[Player_num].callsign[0] = '\0';

	do_register_player(title_pal);

	gr_palette_fade_out( title_pal, 32, 0 );

	//check for special stamped version
	if (registered_copy) {
		char time_str[32];	
		char time_str2[32];
		
		_ctime(&t_saved_time, time_str);
		_ctime(&t_current_time, time_str2);

		nm_messagebox("EVALUATION COPY",1,"Continue",
			"This special evaluation copy\n"
			"of DESCENT II has been issued to:\n\n"
			"%s\n"
			"\nEXPIRES %s\nYOUR TIME: %s\nNOT FOR DISTRIBUTION",
			name_copy, time_str,time_str2);

		gr_palette_fade_out( gr_palette, 32, 0 );
	}

	Game_mode = GM_GAME_OVER;
}


//	Winferno Internal Routines

void DisplayDescentTitle(void)
{
//	Create Descent Title Window and Load Title bitmap, and display it.
	int			pcx_error;
	int			width, height;

	logentry("Displaying title...\n");

//	Display Title Bitmap, using title palette.
	pcx_error = pcx_get_dimensions("DESCENTB.PCX", &width, &height);	
	if (pcx_error != PCX_ERROR_NONE)	{
		gr_close();
		Error( "Couldn't load pcx file 'DESCENTB.PCX', PCX load error: %s\n", pcx_errormsg(pcx_error));
	}

//	pcx_canvas = dd_gr_create_canvas(width, height );

	gr_palette_clear();

	dd_gr_set_current_canvas(NULL);
	DDGRLOCK(dd_grd_curcanv)
	{
		pcx_error=pcx_read_bitmap( "DESCENTB.PCX", &grd_curcanv->cv_bitmap, 
				grd_curcanv->cv_bitmap.bm_type, 
				title_pal );
	}
	DDGRUNLOCK(dd_grd_curcanv)

	if (pcx_error == PCX_ERROR_NONE)	{
	//	Blt to screen, then restore display
		DDGRRESTORE;
		gr_palette_fade_in( title_pal, 32, 0 );
	} else {
		gr_close();
		Error( "Couldn't load pcx file 'DESCENT.PCX', PCX load error: %s\n", pcx_errormsg(pcx_error));
	}
}


do_register_player(ubyte *title_pal)
{
	Players[Player_num].callsign[0] = '\0';

	if (!Auto_demo) 	{

		key_flush();

		memcpy(gr_palette,title_pal,sizeof(gr_palette));
		remap_fonts_and_menus(1);
		RegisterPlayer();		//get player's name
	}
}


//@@void check_joystick_calibration()	
//@@{
//@@	int x1, y1, x2, y2, c;
//@@	fix t1;
//@@
//@@	if ( (Config_control_type!=CONTROL_JOYSTICK) &&
//@@		  (Config_control_type!=CONTROL_FLIGHTSTICK_PRO) &&
//@@		  (Config_control_type!=CONTROL_THRUSTMASTER_FCS) &&
//@@		  (Config_control_type!=CONTROL_GRAVIS_GAMEPAD)
//@@		) return;
//@@
//@@	joy_get_pos( &x1, &y1 );
//@@
//@@	t1 = timer_get_fixed_seconds();
//@@	while( timer_get_fixed_seconds() < t1 + F1_0/100 )
//@@		;
//@@
//@@	joy_get_pos( &x2, &y2 );
//@@
//@@	// If joystick hasn't moved...
//@@	if ( (abs(x2-x1)<30) &&  (abs(y2-y1)<30) )	{
//@@		if ( (abs(x1)>30) || (abs(x2)>30) ||  (abs(y1)>30) || (abs(y2)>30) )	{
//@@			c = nm_messagebox( NULL, 2, TXT_CALIBRATE, TXT_SKIP, TXT_JOYSTICK_NOT_CEN );
//@@			if ( c==0 )	{
//@@				joydefs_calibrate();
//@@			}
//@@		}
//@@	}
//@@
//@@}


int find_descent_cd()
{
	char path[4];
	char oldpath[MAX_PATH];
	char volume[256];
	int i;
	int cdrom_drive=-1;

	path[1] = ':';
	path[2] = '\\';
	path[3] = 0;

	GetCurrentDirectory(MAX_PATH, oldpath);

	for (i = 0; i < 26; i++) 
	{
		path[0] = 'A'+i;
		if (GetDriveType(path) == DRIVE_CDROM) {
			cdrom_drive = -3;
			GetVolumeInformation(path, volume, 256, NULL, NULL, NULL, NULL, 0);
			mprintf((0, "CD volume: %s\n", volume));
			if (!strcmpi(volume, "DESCENT_II") || !strcmpi(volume, "DESCENT.II")) {
				if (!chdir(path)) 
					if (!chdir("\\d2data")) {
						cdrom_drive = i;
						break;
					}
			}
		}
	}

	SetCurrentDirectory(oldpath);
	return cdrom_drive;
}


int init_cdrom()
{
	int i;

	//scan for CD, etc.

	i = find_descent_cd();

	if (i < 0) {			//no CD

		#ifndef RELEASE
		if ((i=FindArg("-cdproxy")) != 0) {
			strcpy(CDROM_dir,"d:\\d2data\\");	//set directory
			CDROM_dir[0] = Args[i+1][0];
			return 1;
		}
		#endif

		return 0;
	}
	else {
		strcpy(CDROM_dir,"d:\\d2data\\");	//set directory
		CDROM_dir[0] = 'A'+i;					//set correct drive leter
		mprintf((0, "Descent 2 CD-ROM: %s\n", CDROM_dir));
		return 1;
	}
}


//	----------------------------------------------------------------------------

void
check_id_checksum_and_date()
{
	const char name[sizeof(DESC_ID_TAG)-1+DESC_ID_LEN] = DESC_ID_TAG;
	char time_str[] = DESC_DEAD_TIME_TAG "00000000";	//second part gets overwritten
	int i, found;
	unsigned long *checksum, test_checksum;
	time_t current_time, saved_time;

	saved_time = (time_t)strtol(time_str + strlen(DESC_DEAD_TIME_TAG), NULL, 16);
	
	t_saved_time = saved_time;

	if (saved_time == (time_t)0)
		return;

	mprintf((1, "TIME STAMPED: %d\n", saved_time));

	strcpy(name_copy,name+strlen(DESC_ID_TAG));
	registered_copy = 1;

	t_current_time = current_time = time(NULL);

	mprintf((1, "CURTIME: %d\n", current_time));

	if (current_time >= saved_time)
		desc_id_exit_num = 1;

	test_checksum = 0;
	for (i = 0; i < strlen(name_copy); i++) {
		found = 0;	  
		test_checksum += name_copy[i];
		if (((test_checksum / 2) * 2) != test_checksum)
			found = 1;
		test_checksum = test_checksum >> 1;
		if (found)
			test_checksum |= 0x80000000;
	}
	checksum = (unsigned long *)(desc_id_checksum_str + strlen(DESC_ID_CHKSUM_TAG));
	if (test_checksum != *checksum)
		desc_id_exit_num = 2;

	cprintf ("%s %s\n", TXT_REGISTRATION, name_copy);
}


//	----------------------------------------------------------------------------
//	Nifty clipboard functions for placing a screen shot
//	----------------------------------------------------------------------------

extern HANDLE win95_dib_from_bitmap(HBITMAP hbm);

void win95_save_pcx_shot()
{
	char message[100];
	static int savenum = 0;
	fix t1;

	dd_grs_canvas *screen_canv = dd_grd_screencanv;
	dd_grs_canvas *temp_canv_1, *save_canv; 	//*temp_canv_2, 
	grs_font *save_font;
	ubyte pal[768];
        
	char savename[FILENAME_LEN];	//NO STEREO savename2[FILENAME_LEN];
	
	int w,h,aw,x,y;

	stop_time();

	mprintf((0, "Screen shot!\n"));

/* Start */
	save_canv = dd_grd_curcanv;
	temp_canv_1 = dd_gr_create_canvas(screen_canv->canvas.cv_bitmap.bm_w,
													 screen_canv->canvas.cv_bitmap.bm_h);
		
	dd_gr_blt_notrans(screen_canv, 0, 0, 0, 0, temp_canv_1, 0, 0, 0, 0);

/* Saved screen shot */
	dd_gr_set_current_canvas(save_canv);

	if ( savenum > 99 ) savenum = 0;
	sprintf(savename,"screen%02d.pcx",savenum++);
	sprintf( message, "%s '%s'", TXT_DUMPING_SCREEN, savename );

	dd_gr_set_current_canvas(NULL);

	DDGRLOCK(dd_grd_curcanv);
	{
		save_font = grd_curcanv->cv_font;
		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(gr_find_closest_color_current(0,31,0),-1);
		gr_get_string_size(message,&w,&h,&aw);

		x = (grd_curcanv->cv_w-w)/2;
		y = (grd_curcanv->cv_h-h)/2;

		gr_setcolor(gr_find_closest_color_current(0,0,0));
		gr_rect(x-2,y-2,x+w+2,y+h+2);
		gr_printf(x,y,message);
		gr_set_curfont(save_font);
	}
	DDGRUNLOCK(dd_grd_curcanv);

	if (GRMODEINFO(modex)) dd_gr_flip();

	t1 = timer_get_fixed_seconds() + F1_0;

	gr_palette_read(pal);		//get actual palette from the hardware

	DDGRLOCK(temp_canv_1);
		pcx_write_bitmap(savename,&temp_canv_1->canvas.cv_bitmap,pal);
	DDGRUNLOCK(temp_canv_1);

	while ( timer_get_fixed_seconds() < t1 );		// Wait so that messag stays up at least 1 second.

	dd_gr_set_current_canvas(screen_canv);

	dd_gr_blt_notrans(temp_canv_1, 0,0,0,0, dd_grd_curcanv, 0,0,0,0);

	if (GRMODEINFO(modex)) dd_gr_flip();

	dd_gr_free_canvas(temp_canv_1);

	dd_gr_set_current_canvas(save_canv);

	key_flush();
	start_time();
}


HANDLE hClipData = NULL;

BOOL clipboard_screenshot()
{
	if (OpenClipboard(_hAppWnd)) {
		EmptyClipboard();
		SetClipboardData (CF_DIB     ,clipboard_renderformat(CF_DIB)); 
		SetClipboardData (CF_BITMAP  ,clipboard_renderformat(CF_BITMAP)); 
		SetClipboardData (CF_PALETTE ,clipboard_renderformat(CF_PALETTE)); 
		CloseClipboard (); 
		return TRUE;
	}
	else return FALSE;
}

 
HANDLE clipboard_renderformat(int cf)
{
	HANDLE hMem = NULL;
	HBITMAP hbm;

	switch(cf)
	{
		case CF_BITMAP:
		{
		// Create screen shot BBM
			mprintf((1, "Creating DDB for clipboard.\n"));
			hMem = win95_screen_shot();
			if (!hMem) mprintf((1, "Unable to create clipboard object: CF_BITMAP\n"));
			break;
		}
		
		case CF_DIB:
		{
		// Create DIB and all memory associated with it.
			mprintf((1, "Creating DIB for clipboard.\n"));
			hbm = win95_screen_shot();
			hMem = win95_dib_from_bitmap(hbm);
			if (!hMem) mprintf((1, "Unable to create clipboard object: CF_DIB\n"));
			DeleteObject(hbm);
			break;
		}
		
		case CF_PALETTE:
		{
		//	 Get current palette! (create a GDI palette from grpal)
			PALETTE lpal;
			char grpal[768];
			int i;
 
			mprintf((1, "Creating Palette for clipboard.\n"));
			gr_palette_read(grpal);
			lpal.version = 0x300;
			lpal.num_entries = 256;
			for (i = 0; i < 256; i++) 
			{
				lpal.entries[i].peRed = grpal[i*3] << 2;
				lpal.entries[i].peGreen = grpal[i*3+1] << 2;
				lpal.entries[i].peBlue = grpal[i*3+2] << 2;
				lpal.entries[i].peFlags = 0;
			}				

			hMem = CreatePalette((LOGPALETTE*)&lpal);
			if (!hMem) mprintf((1, "Unable to create clipboard object: CF_PALETTE\n"));
			break;
		}
	}
		
	return hMem;
}
		
