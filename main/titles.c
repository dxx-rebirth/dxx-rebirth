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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#define ROBOT_MOVIES

#ifdef WINDOWS
#include "desw.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef MACINTOSH
#include <Events.h>
#endif

#include "pa_enabl.h"                   //$$POLY_ACC
#include "pstypes.h"
#include "timer.h"
#include "key.h"
#include "gr.h"
#include "palette.h"
#include "iff.h"
#include "pcx.h"
#include "u_mem.h"
#include "joy.h"
#include "mono.h"
#include "gamefont.h"
#include "cfile.h"
#include "error.h"
#include "polyobj.h"
#include "textures.h"
#include "screens.h"
#include "multi.h"
#include "player.h"
#include "digi.h"
#include "compbit.h"
#include "text.h"
#include "kmatrix.h"
#include "piggy.h"
#include "songs.h"
#include "newmenu.h"
#include "state.h"
#ifdef ROBOT_MOVIES
#include "movie.h"
#endif
#include "menu.h"

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

#ifdef ROBOT_MOVIES
extern void RotateRobot();
#endif

void DoBriefingColorStuff ();
int get_new_message_num(char **message);
int DefineBriefingBox (char **buf);

extern unsigned RobSX,RobSY,RobDX,RobDY; // Robot movie coords

extern int MVEPaletteCalls;

ubyte New_pal[768];
int	New_pal_254_bash;

char CurBriefScreenName[15]="brief03.pcx";
char	* Briefing_text;
#ifdef ROBOT_MOVIES
char RobotPlaying=0;
#endif

#define	MAX_BRIEFING_COLORS	3

#define	SHAREWARE_ENDING_FILENAME	"ending.tex"

//	Can be set by -noscreens command line option.  Causes bypassing of all briefing screens.
int	Skip_briefing_screens=0;
int	Briefing_foreground_colors[MAX_BRIEFING_COLORS], Briefing_background_colors[MAX_BRIEFING_COLORS];
int	Current_color = 0;
int	Erase_color;

extern int check_button_press();

#ifdef MACINTOSH
extern void macintosh_quit(void);
#endif

#ifndef ROBOT_MOVIES
static int rescale_x(int x)
{
	return x * GWIDTH / 320;
}

static int rescale_y(int y)
{
	return y * GHEIGHT / 200;
}
#endif

#ifndef MACINTOSH
int local_key_inkey(void)
{
	int	rval;

#ifdef WINDOWS
	MSG msg;
	
	DoMessageStuff(&msg);
#endif

	rval = key_inkey();

	if (rval == KEY_PRINT_SCREEN) {
		#ifdef POLY_ACC
#ifdef ROBOT_MOVIES
		if (RobotPlaying) {
			gr_palette_read(gr_palette);
			gr_copy_palette(gr_palette,gr_palette,0);	//reset color lookup cache
		}
#endif
		#endif
		save_screen_shot(0);
		return 0;				//say no key pressed
	}

	if (check_button_press())		//joystick or mouse button pressed?
		rval = KEY_SPACEBAR;

	#ifdef MACINTOSH
	if ( rval == KEY_Q+KEY_COMMAND )
		macintosh_quit();
	#endif

	return rval;
}
#else
int local_key_inkey(void)
{
	EventRecord event;
	int	rval;

	if (!GetOSEvent(everyEvent, &event))
		return 0;

	if (event.what != keyDown)
		return 0;
		
	rval = (int)((event.message & keyCodeMask) >> 8);

	if (rval == KEY_PRINT_SCREEN) {
		save_screen_shot(0);
		return 0;				//say no key pressed
	}

	if (check_button_press())		//joystick or mouse button pressed?
		rval = KEY_SPACEBAR;

	#ifdef MACINTOSH
	if ( rval == KEY_Q+KEY_COMMAND )
		macintosh_quit();
	#endif

	return rval;
}
#endif

int show_title_screen( char * filename, int allow_keys, int from_hog_only )
{
	fix timer;
	int pcx_error;
	grs_bitmap title_bm;
	ubyte	palette_save[768];
	char new_filename[FILENAME_LEN+1] = "";

	#ifdef RELEASE
	if (from_hog_only)
		strcpy(new_filename,"\x01");	//only read from hog file
	#endif

	strcat(new_filename,filename);
	filename = new_filename;

	title_bm.bm_data=NULL;
	if ((pcx_error=pcx_read_bitmap( filename, &title_bm, BM_LINEAR, New_pal ))!=PCX_ERROR_NONE)	{
		printf( "File '%s', PCX load error: %s (%i)\n  (No big deal, just no title screen.)\n",filename, pcx_errormsg(pcx_error), pcx_error);
		mprintf((0, "File '%s', PCX load error: %s (%i)\n  (No big deal, just no title screen.)\n",filename, pcx_errormsg(pcx_error), pcx_error));
		Error( "Error loading briefing screen <%s>, PCX load error: %s (%i)\n",filename, pcx_errormsg(pcx_error), pcx_error);
	}

	memcpy(palette_save,gr_palette,sizeof(palette_save));

#if defined(POLY_ACC)
    pa_save_clut();
    pa_update_clut(New_pal, 0, 256, 0);
#endif

	//vfx_set_palette_sub( New_pal );
#ifdef OGL
	gr_palette_load( New_pal );
#else
	gr_palette_clear();	
#endif

	WINDOS(	
		dd_gr_set_current_canvas(NULL),
		gr_set_current_canvas( NULL )
	);
	WIN(DDGRLOCK(dd_grd_curcanv));
	show_fullscr(&title_bm);
	WIN(DDGRUNLOCK(dd_grd_curcanv));

	WIN(DDGRRESTORE);

#if defined(POLY_ACC)
    pa_restore_clut();
#endif

	if (gr_palette_fade_in( New_pal, 32, allow_keys ))	
		return 1;
	gr_copy_palette(gr_palette, New_pal, sizeof(gr_palette));

	gr_palette_load( New_pal );
	timer	= timer_get_fixed_seconds() + i2f(3);
	while (1)	{
		if ( local_key_inkey() && allow_keys ) break;
		if ( timer_get_fixed_seconds() > timer ) break;
	}			
	if (gr_palette_fade_out( New_pal, 32, allow_keys ))
		return 1;
	gr_copy_palette(gr_palette, palette_save, sizeof(palette_save));
	d_free(title_bm.bm_data);
	return 0;
}

typedef struct {
	char	bs_name[14];						//	filename, eg merc01.  Assumes .lbm suffix.
	byte	level_num;
	byte	message_num;
	short	text_ulx, text_uly;		 	//	upper left x,y of text window
	short	text_width, text_height; 	//	width and height of text window
} briefing_screen;

#define BRIEFING_SECRET_NUM	31			//	This must correspond to the first secret level which must come at the end of the list.
#define BRIEFING_OFFSET_NUM	4			// This must correspond to the first level screen (ie, past the bald guy briefing screens)

#define	SHAREWARE_ENDING_LEVEL_NUM		0x7f
#define	REGISTERED_ENDING_LEVEL_NUM	0x7e

#ifdef SHAREWARE
#define ENDING_LEVEL_NUM 	SHAREWARE_ENDING_LEVEL_NUM
#else
#define ENDING_LEVEL_NUM 	REGISTERED_ENDING_LEVEL_NUM
#endif

#define MAX_BRIEFING_SCREENS 60

briefing_screen Briefing_screens[MAX_BRIEFING_SCREENS]=
 {{"brief03.pcx",0,3,8,8,257,177}}; // default=0!!!

int	Briefing_text_x, Briefing_text_y;

void init_char_pos(int x, int y)
{
	Briefing_text_x = x;
	Briefing_text_y = y;
   mprintf ((0,"Setting init x=%d y=%d\n",x,y));
}

grs_canvas	*Robot_canv = NULL;
vms_angvec	Robot_angles;

char	Bitmap_name[32] = "";
#define	EXIT_DOOR_MAX	14
#define	OTHER_THING_MAX	10		//	Adam: This is the number of frames in your new animating thing.
#define	DOOR_DIV_INIT	6
byte	Door_dir=1, Door_div_count=0, Animating_bitmap_type=0;

//	-----------------------------------------------------------------------------
void show_bitmap_frame(void)
{
#ifdef WINDOWS
	dd_grs_canvas *curcanv_save, *bitmap_canv=0;
#else
	grs_canvas	*curcanv_save, *bitmap_canv=0;
#endif

	grs_bitmap	*bitmap_ptr;

	//	Only plot every nth frame.
	if (Door_div_count) {
		Door_div_count--;
		return;
	}

	Door_div_count = DOOR_DIV_INIT;

	if (Bitmap_name[0] != 0) {
		char		*pound_signp;
		int		num, dig1, dig2;

		//	Set supertransparency color to black
		if (!New_pal_254_bash) {
			New_pal_254_bash = 1;
			New_pal[254*3] = 0;
			New_pal[254*3+1] = 0;
			New_pal[254*3+2] = 0;
			gr_palette_load( New_pal );
		}

		switch (Animating_bitmap_type) {
			case 0:
			WINDOS(	
				bitmap_canv = dd_gr_create_sub_canvas(dd_grd_curcanv, 220, 45, 64, 64);	break,
				bitmap_canv = gr_create_sub_canvas(grd_curcanv, 220, 45, 64, 64);	break
			);
			case 1:	
			WINDOS(
				bitmap_canv = dd_gr_create_sub_canvas(dd_grd_curcanv, 220, 45, 94, 94);	break,
				bitmap_canv = gr_create_sub_canvas(grd_curcanv, 220, 45, 94, 94);	break
			);	
			
			//	Adam: Change here for your new animating bitmap thing. 94, 94 are bitmap size.
			default:	Int3();	//	Impossible, illegal value for Animating_bitmap_type
		}

		WINDOS(
			curcanv_save = dd_grd_curcanv; dd_grd_curcanv = bitmap_canv,
			curcanv_save = grd_curcanv; grd_curcanv = bitmap_canv
		);

		pound_signp = strchr(Bitmap_name, '#');
		Assert(pound_signp != NULL);

		dig1 = *(pound_signp+1);
		dig2 = *(pound_signp+2);
		if (dig2 == 0)
			num = dig1-'0';
		else
			num = (dig1-'0')*10 + (dig2-'0');

		switch (Animating_bitmap_type) {
			case 0:
				num += Door_dir;
				if (num > EXIT_DOOR_MAX) {
					num = EXIT_DOOR_MAX;
					Door_dir = -1;
				} else if (num < 0) {
					num = 0;
					Door_dir = 1;
				}
				break;
			case 1:
				num++;
				if (num > OTHER_THING_MAX)
					num = 0;
				break;
		}

		Assert(num < 100);
		if (num >= 10) {
			*(pound_signp+1) = (num / 10) + '0';
			*(pound_signp+2) = (num % 10) + '0';
			*(pound_signp+3) = 0;
		} else {
			*(pound_signp+1) = (num % 10) + '0';
			*(pound_signp+2) = 0;
		}

		{
			bitmap_index bi;
			bi = piggy_find_bitmap(Bitmap_name);
			bitmap_ptr = &GameBitmaps[bi.index];
			PIGGY_PAGE_IN( bi );
		}

		WIN(DDGRLOCK(dd_grd_curcanv));
			gr_bitmapm(0, 0, bitmap_ptr);
		WIN(DDGRUNLOCK(dd_grd_curcanv));

		WINDOS(
			dd_grd_curcanv = curcanv_save,
			grd_curcanv = curcanv_save
		);
		d_free(bitmap_canv);

		switch (Animating_bitmap_type) {
			case 0:
				if (num == EXIT_DOOR_MAX) {
					Door_dir = -1;
					Door_div_count = 64;
				} else if (num == 0) {
					Door_dir = 1;
					Door_div_count = 64;
				}
				break;
			case 1:
				break;
		}
	}

}

//	-----------------------------------------------------------------------------
void show_briefing_bitmap(grs_bitmap *bmp)
{
#ifdef WINDOWS
  	dd_grs_canvas *bitmap_canv, *curcanv_save;
  
	bitmap_canv = dd_gr_create_sub_canvas(dd_grd_curcanv, 220, 45, bmp->bm_w, bmp->bm_h); 
	curcanv_save = dd_grd_curcanv;
	dd_gr_set_current_canvas(bitmap_canv);
	DDGRLOCK(dd_grd_curcanv);
	gr_bitmapm(0,0,bmp);
	DDGRUNLOCK(dd_grd_curcanv);
	dd_gr_set_current_canvas(curcanv_save);
#else
	grs_canvas	*curcanv_save, *bitmap_canv;

	bitmap_canv = gr_create_sub_canvas(grd_curcanv, 220, 45, bmp->bm_w, bmp->bm_h);
	curcanv_save = grd_curcanv;
	gr_set_current_canvas(bitmap_canv);
	gr_bitmapm(0, 0, bmp);
	gr_set_current_canvas(curcanv_save);
#endif

	d_free(bitmap_canv);
}

#ifndef ROBOT_MOVIES //WINDOWS
//	-----------------------------------------------------------------------------
void show_spinning_robot_frame(int robot_num)
{
	grs_canvas	*curcanv_save;

	if (robot_num != -1) {
		Robot_angles.h += 150;

		curcanv_save = grd_curcanv;
		grd_curcanv = Robot_canv;
		Assert(Robot_info[robot_num].model_num != -1);
		draw_model_picture(Robot_info[robot_num].model_num, &Robot_angles);
		grd_curcanv = curcanv_save;
	}

}

//  -----------------------------------------------------------------------------
void init_spinning_robot(void) //(int x,int y,int w,int h)
 {
#if 0
	Robot_angles.p += 0;
	Robot_angles.b += 0;
	Robot_angles.h += 0;

#else
	int x = rescale_x(138);
	int y = rescale_y(55);
	int w = rescale_x(166);
	int h = rescale_y(138);
#endif

        Robot_canv = gr_create_sub_canvas(grd_curcanv, x, y, w, h);
        // 138, 55, 166, 138
 }
#endif

//	-----------------------------------------------------------------------------
//	Returns char width.
//	If show_robot_flag set, then show a frame of the spinning robot.
int show_char_delay(char the_char, int delay, int robot_num, int cursor_flag)
{
	int	w, h, aw;
	char	message[2];
	static fix	start_time=0;

   robot_num=0;
	message[0] = the_char;
	message[1] = 0;

   if (start_time==0 && timer_get_fixed_seconds()<0)
    start_time=timer_get_fixed_seconds(); 
 
	gr_get_string_size(message, &w, &h, &aw );

	Assert((Current_color >= 0) && (Current_color < MAX_BRIEFING_COLORS));

	//	Draw cursor if there is some delay and caller says to draw cursor
	if (cursor_flag && delay) {
	WIN(DDGRLOCK(dd_grd_curcanv));
		gr_set_fontcolor(Briefing_foreground_colors[Current_color], -1);
		gr_printf(Briefing_text_x+1, Briefing_text_y, "_" );
	WIN(DDGRUNLOCK(dd_grd_curcanv));
	}

		if (delay)
		 delay=fixdiv (F1_0,i2f(15));

		if (delay != 0)
			show_bitmap_frame();

#ifdef ROBOT_MOVIES
		if (RobotPlaying && (delay != 0))
			RotateRobot();
      
      
		while (timer_get_fixed_seconds() < (start_time + delay)) {
			if (RobotPlaying && delay != 0)
				RotateRobot();
		}
#else
                if (robot_num != -1)
			show_spinning_robot_frame(robot_num);
#endif
		start_time = timer_get_fixed_seconds();

WIN(DDGRLOCK(dd_grd_curcanv));
	//	Erase cursor
	if (cursor_flag && delay) {
		gr_set_fontcolor(Erase_color, -1);
		gr_printf(Briefing_text_x+1, Briefing_text_y, "_" );
	}

	//	Draw the character
	gr_set_fontcolor(Briefing_background_colors[Current_color], -1);
	gr_printf(Briefing_text_x, Briefing_text_y, message );

	gr_set_fontcolor(Briefing_foreground_colors[Current_color], -1);
	gr_printf(Briefing_text_x+1, Briefing_text_y, message );
WIN(DDGRUNLOCK(dd_grd_curcanv));

//	if (the_char != ' ')
//		if (!digi_is_sound_playing(SOUND_MARKER_HIT))
//			digi_play_sample( SOUND_MARKER_HIT, F1_0 );

	return w;
}

//	-----------------------------------------------------------------------------
int load_briefing_screen( int screen_num )
{
	int	pcx_error;

	WIN(DDGRLOCK(dd_grd_curcanv));
      if ((pcx_error=pcx_read_fullscr( CurBriefScreenName, New_pal ))!=PCX_ERROR_NONE)     {
		printf( "File '%s', PCX load error: %s\n  (It's a briefing screen.  Does this cause you pain?)\n",Briefing_screens[screen_num].bs_name, pcx_errormsg(pcx_error));
		printf( "File '%s', PCX load error: %s (%i)\n  (It's a briefing screen.  Does this cause you pain?)\n",Briefing_screens[screen_num].bs_name, pcx_errormsg(pcx_error), pcx_error);
		WIN(DDGRUNLOCK(dd_grd_curcanv));
		Error( "Error loading briefing screen <%s>, PCX load error: %s (%i)\n",CurBriefScreenName, pcx_errormsg(pcx_error), pcx_error);
	}
	WIN(DDGRUNLOCK(dd_grd_curcanv));

	WIN(DDGRRESTORE);

	return 0;
}

int load_new_briefing_screen( char *fname )
{
	int	pcx_error;

   mprintf ((0,"Loading new briefing %s!\n",fname));
   strcpy (CurBriefScreenName,fname);

// WIN(DEFINE_SCREEN(CurBriefScreenName));

	if (gr_palette_fade_out( New_pal, 32, 0 ))	
		return 0;

WIN(DDGRLOCK(dd_grd_curcanv));
   if ((pcx_error=pcx_read_fullscr( fname, New_pal ))!=PCX_ERROR_NONE)     {
	   	printf( "File '%s', PCX load error: %s (%i)\n  (It's a briefing screen.  Does this cause you pain?)\n",fname, pcx_errormsg(pcx_error), pcx_error);
			WIN(DDGRUNLOCK(dd_grd_curcanv));
		Error( "Error loading briefing screen <%s>, PCX load error: %s (%i)\n",fname, pcx_errormsg(pcx_error), pcx_error);
	}
WIN(DDGRUNLOCK(dd_grd_curcanv));

WIN(DDGRRESTORE);

	gr_copy_palette(gr_palette, New_pal, sizeof(gr_palette));

	if (gr_palette_fade_in( New_pal, 32, 0 ))	
		return 0;
   DoBriefingColorStuff();

	return 1;
}



#define KEY_DELAY_DEFAULT       ((F1_0*20)/1000)

//	-----------------------------------------------------------------------------
int get_message_num(char **message)
{
	int	num=0;

	while (**message == ' ')
		(*message)++;

	while ((**message >= '0') && (**message <= '9')) {
		num = 10*num + **message-'0';
		(*message)++;
	}

	while (*(*message)++ != 10)		//	Get and drop eoln
		;

	return num;
}

//	-----------------------------------------------------------------------------
void get_message_name(char **message, char *result)
{
	while (**message == ' ')
		(*message)++;

	while ((**message != ' ') && (**message != 10)) {
		if (**message != '\n')
			*result++ = **message;
		(*message)++;
	}

	if (**message != 10)
		while (*(*message)++ != 10)		//	Get and drop eoln
			;

	*result = 0;
}

//	-----------------------------------------------------------------------------
void flash_cursor(int cursor_flag)
{
	if (cursor_flag == 0)
		return;

WIN(DDGRLOCK(dd_grd_curcanv));
	if ((timer_get_fixed_seconds() % (F1_0/2) ) > (F1_0/4))
		gr_set_fontcolor(Briefing_foreground_colors[Current_color], -1);
	else
		gr_set_fontcolor(Erase_color, -1);

	gr_printf(Briefing_text_x+1, Briefing_text_y, "_" );
WIN(DDGRUNLOCK(dd_grd_curcanv));
}

extern int InitMovieBriefing();

//	-----------------------------------------------------------------------------
//	Return true if message got aborted by user (pressed ESC), else return false.
int show_briefing_message(int screen_num, char *message)
{
	int	prev_ch=-1;
	int	ch, done=0,i;
	briefing_screen	*bsp = &Briefing_screens[screen_num];
	int	delay_count = KEY_DELAY_DEFAULT;
	int	key_check;
	int	robot_num=-1;
	int	rval=0;
	static int tab_stop=0;
	int	flashing_cursor=0;
	int	new_page=0,GotZ=0;
#ifdef ROBOT_MOVIES
	char *spinRobotName="rba.mve",kludge;  // matt don't change this!  
#endif
	char fname[15];
   char DumbAdjust=0;
	char chattering=0;
	int hum_channel=-1,printing_channel=-1;
	int LineAdjustment=0;	
	WIN(int wpage_done=0);

	Bitmap_name[0] = 0;
	Current_color = 0;
#ifdef ROBOT_MOVIES
	RobotPlaying=0;

	InitMovieBriefing();
#endif

	#ifndef SHAREWARE
	hum_channel  = digi_start_sound( digi_xlat_sound(SOUND_BRIEFING_HUM), F1_0/2, 0xFFFF/2, 1, -1, -1, -1 );
	#endif
 
	// mprintf((0, "Going to print message [%s] at x=%i, y=%i\n", message, x, y));
	gr_set_curfont( GAME_FONT );

   bsp=&Briefing_screens[0];
   init_char_pos(bsp->text_ulx, bsp->text_uly-(8*(1+MenuHires)));

	while (!done) {
		ch = *message++;
		if (ch == '$') {
			ch = *message++;
	      if (ch=='D')
	       {
	  	      screen_num=DefineBriefingBox (&message);
		    	//load_new_briefing_screen (Briefing_screens[screen_num].bs_name);
				
       	   bsp = &Briefing_screens[screen_num];
            init_char_pos(bsp->text_ulx, bsp->text_uly);
				LineAdjustment=0;
            prev_ch = 10;                                     //      read to eoln
          }
         else if (ch=='U')
          {
	        screen_num=get_message_num(&message);
           bsp = &Briefing_screens[screen_num];
           init_char_pos(bsp->text_ulx, bsp->text_uly);
           prev_ch = 10;                                                   //      read to eoln
          }
                       
         else if (ch == 'C') {
				Current_color = get_message_num(&message)-1;
				Assert((Current_color >= 0) && (Current_color < MAX_BRIEFING_COLORS));
				prev_ch = 10;
			} else if (ch == 'F') {		//	toggle flashing cursor
				flashing_cursor = !flashing_cursor;
				prev_ch = 10;
				while (*message++ != 10)
					;
			} else if (ch == 'T') {
				tab_stop = get_message_num(&message);
				tab_stop*=(1+MenuHires);
				prev_ch = 10;							//	read to eoln
			} else if (ch == 'R') {
				if (Robot_canv != NULL) {
					d_free(Robot_canv); 
					Robot_canv=NULL;
				}
#ifdef ROBOT_MOVIES
				if (RobotPlaying) {
					DeInitRobotMovie();
					RobotPlaying=0;
				}
				
				kludge=*message++;
				spinRobotName[2]=kludge; // ugly but proud

				RobotPlaying=InitRobotMovie(spinRobotName);

				// gr_remap_bitmap_good( &grd_curcanv->cv_bitmap, pal, -1, -1 );

				if (RobotPlaying) { 			
					DoBriefingColorStuff ();
					mprintf ((0,"Robot playing is %d!!!",RobotPlaying));
				}
#else
				init_spinning_robot();
				robot_num = get_message_num(&message);
#endif
            prev_ch = 10;							//	read to eoln
			} else if (ch == 'N') {
				//--grs_bitmap	*bitmap_ptr;
				if (Robot_canv != NULL)
					{d_free(Robot_canv); Robot_canv=NULL;}

				get_message_name(&message, Bitmap_name);
				strcat(Bitmap_name, "#0");
				Animating_bitmap_type = 0;
				prev_ch = 10;
			} else if (ch == 'O') {
				if (Robot_canv != NULL)
					{d_free(Robot_canv); Robot_canv=NULL;}

				get_message_name(&message, Bitmap_name);
				strcat(Bitmap_name, "#0");
				Animating_bitmap_type = 1;
				prev_ch = 10;
			} else if (ch=='A') {
				LineAdjustment=1-LineAdjustment;
			} else if (ch=='Z') {
				mprintf ((0,"Got a Z!\n"));
				GotZ=1;
				#if defined (D2_OEM) || defined(COMPILATION) || (defined(MACINTOSH) && defined(SHAREWARE))
					DumbAdjust=1;
				#else
					if (LineAdjustment==1)
						DumbAdjust=1;
					else
						DumbAdjust=2;
				#endif
	
				i=0;
				while ((fname[i]=*message) != '\n') {
					i++;
					message++;
				}
				fname[i]=0;
  
				if (MenuHires)
				 {
				   i=0; 
				   while (fname[i]!='.')
				   	i++;
#ifndef SHAREWARE
				   fname[i++]='b';
#endif
				   fname[i++]='.';	
				   fname[i++]='p';	
				   fname[i++]='c';	
				   fname[i++]='x';	
				   fname[i++]=0;	

				   load_new_briefing_screen (fname);
				 }
				else
				   load_new_briefing_screen (fname);

				//load_new_briefing_screen (MenuHires?"end01b.pcx":"end01.pcx");	

			} else if (ch == 'B') {
				char			bitmap_name[32];
				grs_bitmap	guy_bitmap;
				ubyte			temp_palette[768];
				int			iff_error;

				if (Robot_canv != NULL)
					{d_free(Robot_canv); Robot_canv=NULL;}

				get_message_name(&message, bitmap_name);
				strcat(bitmap_name, ".bbm");
				guy_bitmap.bm_data = NULL;
				iff_error = iff_read_bitmap(bitmap_name, &guy_bitmap, BM_LINEAR, temp_palette);
				Assert(iff_error == IFF_NO_ERROR);
				gr_remap_bitmap_good( &guy_bitmap, temp_palette, -1, -1 );

				show_briefing_bitmap(&guy_bitmap);
				d_free(guy_bitmap.bm_data);
				prev_ch = 10;
//		   } else if (ch==EOF) {
//				done=1;
//			} else if (ch == 'B') {
//				if (Robot_canv != NULL)
//					{d_free(Robot_canv); Robot_canv=NULL;}
//
//				bitmap_num = get_message_num(&message);
//				if (bitmap_num != -1)
//					show_briefing_bitmap(Textures[bitmap_num]);
//				prev_ch = 10;							//	read to eoln
			} else if (ch == 'S') {
				int	keypress;
				fix	start_time;
			
				chattering=0;
				if (printing_channel>-1)
				   digi_stop_sound( printing_channel );
				printing_channel=-1;

			#ifdef WINDOWS
				if (!wpage_done) {
					DDGRRESTORE;
					wpage_done =1;
				}
			#endif

				start_time = timer_get_fixed_seconds();
				while ( (keypress = local_key_inkey()) == 0 ) {		//	Wait for a key
				#ifdef WINDOWS
					if (_RedrawScreen) {
						_RedrawScreen = FALSE;
						hum_channel  = digi_start_sound( digi_xlat_sound(SOUND_BRIEFING_HUM), F1_0/2, 0xFFFF/2, 1, -1, -1, -1 );
						keypress = KEY_ESC;
						break;
					}
				#endif

					while (timer_get_fixed_seconds() < start_time + KEY_DELAY_DEFAULT/2)
						;
					flash_cursor(flashing_cursor);

#ifdef ROBOT_MOVIES
					if (RobotPlaying)	RotateRobot ();
#else
					show_spinning_robot_frame(robot_num);
#endif

					show_bitmap_frame();
					start_time += KEY_DELAY_DEFAULT/2;
				}

#ifndef NDEBUG
				if (keypress == KEY_BACKSP)
					Int3();
#endif
				if (keypress == KEY_ESC)
					rval = 1;

				flashing_cursor = 0;
				done = 1;
				WIN(wpage_done = 0);
			} else if (ch == 'P') {		//	New page.
				if (!GotZ)
				 {
			  		 Int3(); // Hey ryan!!!! You gotta load a screen before you start 
								// printing to it! You know, $Z !!!
		  		    load_new_briefing_screen (MenuHires?"end01b.pcx":"end01.pcx");
				 }
					 				
		
				new_page = 1;
				while (*message != 10) {
					message++;	//	drop carriage return after special escape sequence
				}
				message++;
				prev_ch = 10;
			}
		} else if (ch == '\t') {		//	Tab
			if (Briefing_text_x - bsp->text_ulx < tab_stop)
				Briefing_text_x = bsp->text_ulx + tab_stop;
		} else if ((ch == ';') && (prev_ch == 10)) {
			while (*message++ != 10)
				;
			prev_ch = 10;
		} else if (ch == '\\') {
			prev_ch = ch;
		} else if (ch == 10) {
			if (prev_ch != '\\') {
				prev_ch = ch;
				if (DumbAdjust==0)
					Briefing_text_y += (8*(MenuHires+1));
				else
					DumbAdjust--;
				Briefing_text_x = bsp->text_ulx;
				if (Briefing_text_y > bsp->text_uly + bsp->text_height) {
					load_briefing_screen(screen_num);
					Briefing_text_x = bsp->text_ulx;
					Briefing_text_y = bsp->text_uly;
				}
			} else {
				if (ch == 13)		//Can this happen? Above says ch==10
					Int3();
				prev_ch = ch;
			}

		} else {
			if (!GotZ)
			 {
		  		 Int3(); // Hey ryan!!!! You gotta load a screen before you start 
							// printing to it! You know, $Z !!!
		  		 load_new_briefing_screen (MenuHires?"end01b.pcx":"end01.pcx");
			 }
				 
			prev_ch = ch;

         if (!chattering)
			 {
		 		printing_channel  = digi_start_sound( digi_xlat_sound(SOUND_BRIEFING_PRINTING), F1_0, 0xFFFF/2, 1, -1, -1, -1 );
	  		   chattering=1;
			 }
		
			WIN(if (GRMODEINFO(emul)) delay_count = 0);

			Briefing_text_x += show_char_delay(ch, delay_count, robot_num, flashing_cursor);

		}

		//	Check for Esc -> abort.
		key_check = local_key_inkey();

	#ifdef WINDOWS
		if (_RedrawScreen) {
			_RedrawScreen = FALSE;
			hum_channel  = digi_start_sound( digi_xlat_sound(SOUND_BRIEFING_HUM), F1_0/2, 0xFFFF/2, 1, -1, -1, -1 );
			key_check = KEY_ESC;
		}
	#endif
		if ( key_check == KEY_ESC ) {
			rval = 1;
			done = 1;
		}

		if ((key_check == KEY_SPACEBAR) || (key_check == KEY_ENTER))
			delay_count = 0;

		if (Briefing_text_x > bsp->text_ulx + bsp->text_width) {
			Briefing_text_x = bsp->text_ulx;
			Briefing_text_y += bsp->text_uly;
		}

		if ((new_page) || (Briefing_text_y > bsp->text_uly + bsp->text_height)) {
			fix	start_time = 0;
			int	keypress;

			new_page = 0;

			if (printing_channel>-1)
			   digi_stop_sound( printing_channel );
			printing_channel=-1;
			
			chattering=0;

			#ifdef WINDOWS
				if (!wpage_done) {
					DDGRRESTORE;
					wpage_done =1;
				}
			#endif

			start_time = timer_get_fixed_seconds();
			while ( (keypress = local_key_inkey()) == 0 ) {		//	Wait for a key
			#ifdef WINDOWS
				if (_RedrawScreen) {
					_RedrawScreen = FALSE;
					hum_channel  = digi_start_sound( digi_xlat_sound(SOUND_BRIEFING_HUM), F1_0/2, 0xFFFF/2, 1, -1, -1, -1 );
					keypress = KEY_ESC;
					break;
				}
			#endif

				while (timer_get_fixed_seconds() < start_time + KEY_DELAY_DEFAULT/2)
					;
				flash_cursor(flashing_cursor);
#ifdef ROBOT_MOVIES
				if (RobotPlaying) RotateRobot();
#else
				show_spinning_robot_frame(robot_num);
#endif
				show_bitmap_frame();
				start_time += KEY_DELAY_DEFAULT/2;
			}

#ifdef ROBOT_MOVIES
			if (RobotPlaying) DeInitRobotMovie();
			RobotPlaying=0;
#endif
			robot_num = -1;

#ifndef NDEBUG
			if (keypress == KEY_BACKSP)
				Int3();
#endif
			if (keypress == KEY_ESC) {
				rval = 1;
				done = 1;
			}

			load_briefing_screen(screen_num);
			Briefing_text_x = bsp->text_ulx;
			Briefing_text_y = bsp->text_uly;
         delay_count = KEY_DELAY_DEFAULT;

			WIN(wpage_done = 0);
		}
	}

#ifdef ROBOT_MOVIES
  	if (RobotPlaying) {
		DeInitRobotMovie();
		RobotPlaying=0;
	}
#endif

	if (Robot_canv != NULL)
		{d_free(Robot_canv); Robot_canv=NULL;}

	if (hum_channel>-1)
		digi_stop_sound( hum_channel );
	if (printing_channel>-1)
	   digi_stop_sound( printing_channel );

	return rval;
}

//	-----------------------------------------------------------------------------
//	Return a pointer to the start of text for screen #screen_num.
char * get_briefing_message(int screen_num)
{
	char	*tptr = Briefing_text;
	int	cur_screen=0;
	int	ch;

	Assert(screen_num >= 0);

	while ( (*tptr != 0 ) && (screen_num != cur_screen)) {
		ch = *tptr++;
		if (ch == '$') {
			ch = *tptr++;
			if (ch == 'S')
				cur_screen = get_message_num(&tptr);
		}
	}

   if (screen_num!=cur_screen)
	 return (NULL);
  
	return tptr;
}

// -----------------------------------------------------------------------------
//	Load Descent briefing text.
int load_screen_text(char *filename, char **buf)
{
	CFILE	*tfile;
	CFILE *ifile;
	int	len, i,x;
	int	have_binary = 0;

	if ((tfile = cfopen(filename,"rb")) == NULL) {
		char nfilename[30], *ptr;

		strcpy(nfilename, filename);
		ptr = strrchr(nfilename, '.');
		*ptr = '\0';
		strcat(nfilename, ".txb");
		if ((ifile = cfopen(nfilename, "rb")) == NULL)
	        { 
			mprintf ((0,"can't open %s!\n",nfilename));
         		return (0); 
			//Error("Cannot open file %s or %s", filename, nfilename); 
		}	
   
		mprintf ((0,"reading...\n"));
		have_binary = 1;

		len = cfilelength(ifile);
		MALLOC(*buf, char, len+500); 
      mprintf ((0,"len=%d\n",len));
		for (x=0,i=0;i<len;i++,x++)
		 {
	     cfread (*buf+x,1,1,ifile);
		//  mprintf ((0,"%c",*(*buf+x)));
		  if (*(*buf+x)==13)
 			x--;
       }
      
		cfclose(ifile);
	} else {
		len = cfilelength(tfile);
		MALLOC(*buf, char, len+500); 
		for (x=0,i=0;i<len;i++,x++)
		 {
	     cfread (*buf+x,1,1,tfile);
		 // mprintf ((0,"%c",*(*buf+x)));
		  if (*(*buf+x)==13)
 			x--;
       }
	  
      
		//cfread(*buf, 1, len, tfile);
		cfclose(tfile);
	}

	if (have_binary) {
		char *ptr;

		for (i = 0, ptr = *buf; i < len; i++, ptr++) {
			if (*ptr != '\n') {
				encode_rotate_left(ptr);
				*ptr = *ptr ^ BITMAP_TBL_XOR;
				encode_rotate_left(ptr);
			}
		}
	}

 return (1);
}

//	-----------------------------------------------------------------------------
//	Return true if message got aborted, else return false.
int show_briefing_text(int screen_num)
{
	char	*message_ptr;

   message_ptr = get_briefing_message(screen_num);
   if (message_ptr==NULL)
	 return (0);
    
   DoBriefingColorStuff();

	return show_briefing_message(screen_num, message_ptr);
}
void DoBriefingColorStuff ()
 {
   Briefing_foreground_colors[0] = gr_find_closest_color_current( 0, 40, 0);
   Briefing_background_colors[0] = gr_find_closest_color_current( 0, 6, 0);

   Briefing_foreground_colors[1] = gr_find_closest_color_current( 40, 33, 35);
   Briefing_background_colors[1] = gr_find_closest_color_current( 5, 5, 5);

   Briefing_foreground_colors[2] = gr_find_closest_color_current( 8, 31, 54);
   Briefing_background_colors[2] = gr_find_closest_color_current( 1, 4, 7);

   Erase_color = gr_find_closest_color_current(0, 0, 0);
 } 

//	-----------------------------------------------------------------------------
//	Return true if screen got aborted by user, else return false.
int show_briefing_screen( int screen_num, int allow_keys)
{
	int	rval=0;
	ubyte	palette_save[768];

	New_pal_254_bash = 0;

	if (Skip_briefing_screens) {
   	mprintf((0, "Skipping briefing screen [brief03.pcx]\n"));
		return 0;
	}

//	briefing_bm.bm_data=NULL;	
//   if ((pcx_error=pcx_read_bitmap( "brief03.pcx", &briefing_bm, BM_LINEAR, New_pal ))!=PCX_ERROR_NONE)     {
//        mprintf((0, "File '%s', PCX load error: %s (%i)\n  (It's a briefing screen.  Does this cause you pain?)\n","Brief03.pcx", pcx_errormsg(pcx_error), pcx_error));
//		Int3();
//		return 0;
//	}

	memcpy(palette_save,gr_palette,sizeof(palette_save));
	memcpy(New_pal,gr_palette,sizeof(gr_palette));
   

//	vfx_set_palette_sub( New_pal );
//	gr_palette_clear();
//	gr_bitmap( 0, 0, &briefing_bm );
#ifdef OGL
	gr_palette_load(New_pal);
#endif

//	if (gr_palette_fade_in( New_pal, 32, allow_keys ))	
//		return 1;
//	memcpy(gr_palette,New_pal,sizeof(gr_palette));

	#ifdef MACINTOSH
	key_close();		// kill the keyboard handler during briefing screens for movies
	#endif
	rval = show_briefing_text(screen_num);
	#ifdef MACINTOSH
	key_init();
	#endif
	
	#if defined (MACINTOSH) || defined(WINDOWS)
	memcpy(New_pal,gr_palette,sizeof(gr_palette));		// attempt to get fades after briefing screens done correctly.
	#endif


	#ifndef WINDOWS 
	if (gr_palette_fade_out( New_pal, 32, allow_keys ))
		return 1;
   #else
		DEFINE_SCREEN(NULL);
		WIN(DDGRLOCK(dd_grd_curcanv));
		gr_clear_canvas (0);
		WIN(DDGRUNLOCK(dd_grd_curcanv));
		if (gr_palette_fade_out( New_pal, 32, allow_keys ))
			return 1;
	#endif

	gr_copy_palette(gr_palette, palette_save, sizeof(palette_save));

//	d_free(briefing_bm.bm_data);

	return rval;
}


//	-----------------------------------------------------------------------------
void do_briefing_screens(char *filename,int level_num)
{

   MVEPaletteCalls=0;

	if (Skip_briefing_screens) {
		mprintf((0, "Skipping all briefing screens.\n"));
		return;
	}
	
	#ifdef APPLE_DEMO
	return;			// no briefing screens at all for demo
	#endif

	mprintf ((0,"Trying briefing screen! %s\n",filename));

	if (!filename)
		return;

	if (!load_screen_text(filename, &Briefing_text))
	 return;

	#ifdef SHAREWARE
	songs_play_song( SONG_BRIEFING, 1 );
	#else
	songs_stop_all();
	#endif

	set_screen_mode( SCREEN_MENU );
	
	WINDOS(
		dd_gr_set_current_canvas(NULL),
		gr_set_current_canvas(NULL)
	);

	mprintf ((0,"Playing briefing screen! %s %d\n",filename,level_num));

	key_flush();
	
	show_briefing_screen(level_num,0);

	d_free (Briefing_text);
	key_flush();

	return;

}

int DefineBriefingBox (char **buf)
 {
  int n,i=0;
  char name[20];

  n=get_new_message_num (buf);

	Assert(n < MAX_BRIEFING_SCREENS);

  while (**buf!=' ')
   {
    name[i++]=**buf;
    (*buf)++;
   }

  name[i]='\0';   // slap a delimiter on this guy

  strcpy (Briefing_screens[n].bs_name,name);
  Briefing_screens[n].level_num=get_new_message_num (buf);
  Briefing_screens[n].message_num=get_new_message_num (buf);
  Briefing_screens[n].text_ulx=get_new_message_num (buf);
  Briefing_screens[n].text_uly=get_new_message_num (buf);
  Briefing_screens[n].text_width=get_new_message_num (buf);
  Briefing_screens[n].text_height=get_message_num (buf);  // NOTICE!!!

  if (MenuHires)
	{
	 Briefing_screens[n].text_ulx*=2;
	 Briefing_screens[n].text_uly*=2.4;
	 Briefing_screens[n].text_width*=2;
	 Briefing_screens[n].text_height*=2.4;
	}
 
  return (n);
 }

int get_new_message_num(char **message)
{
	int	num=0;

	while (**message == ' ')
		(*message)++;

	while ((**message >= '0') && (**message <= '9')) {
		num = 10*num + **message-'0';
		(*message)++;
	}

       (*message)++;

	return num;
}

