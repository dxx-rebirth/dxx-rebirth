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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/gamefont.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:43:52 $
 * 
 * Fonts for the game.
 * 
 * $Log: gamefont.c,v $
 * Revision 1.1.1.1  2006/03/17 19:43:52  zicodxx
 * initial import
 *
 * Revision 1.5  1999/11/20 10:05:17  donut
 * variable size menu patch from Jan Bobrowski.  Variable menu font size support and a bunch of fixes for menus that didn't work quite right, by me (MPM).
 *
 * Revision 1.4  1999/10/08 09:00:47  donut
 * fixed undefined error on mingw
 *
 * Revision 1.3  1999/10/08 04:14:02  donut
 * added default usage of larger fonts at highres, added checks for font existance before trying to load
 *
 * Revision 1.2  1999/10/08 00:57:03  donut
 * variable GAME_FONT
 *
 * Revision 1.1.1.1  1999/06/14 22:06:59  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:30:14  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.8  1994/11/18  16:41:39  adam
 * trimmed some meat
 * 
 * Revision 1.7  1994/11/17  13:07:11  adam
 * removed unused font
 * 
 * Revision 1.6  1994/11/03  21:36:12  john
 * Added code for credit fonts.
 * 
 * Revision 1.5  1994/08/17  20:20:02  matt
 * Took out alternate-color versions of font3, since this is a mono font
 * 
 * Revision 1.4  1994/08/12  12:03:44  adam
 * tweaked fonts.
 * 
 * Revision 1.3  1994/08/11  12:43:40  adam
 * changed font filenames
 * 
 * Revision 1.2  1994/08/10  19:57:15  john
 * Changed font stuff; Took out old menu; messed up lots of
 * other stuff like game sequencing messages, etc.
 * 
 * Revision 1.1  1994/08/10  17:20:09  john
 * Initial revision
 * 
 * 
 */


#ifdef RCS
#pragma off (unreferenced)
static char rcsid[] = "$Id: gamefont.c,v 1.1.1.1 2006/03/17 19:43:52 zicodxx Exp $";
#pragma on (unreferenced)
#endif

#include <stdlib.h>
#include <stdio.h>

#include "gr.h"
#include "error.h"
#include <string.h>
#include "strutil.h"
#include "mono.h"
#include "args.h"
#include "cfile.h"
#include "hudmsg.h"
#include "gamefont.h"

char * Gamefont_filenames_l[] = { 	"font1-1.fnt",			// Font 0
											"font2-1.fnt",			// Font 1
											"font2-2.fnt",			// Font 2
											"font2-3.fnt",			// Font 3
											"font3-1.fnt"			// Font 4
										};

//large fonts from d2:
char * Gamefont_filenames_h[] = { 	"font1-1h.fnt",			// Font 0
											"font2-1h.fnt",			// Font 1
											"font2-2h.fnt",			// Font 2
											"font2-3h.fnt",			// Font 3
											"font3-1h.fnt"			// Font 4
										};

grs_font *Gamefonts[MAX_FONTS];

int Gamefont_installed=0;

//code to allow variable GAME_FONT, added 10/7/99 Matt Mueller - updated 11/18/99 to handle all fonts, not just GFONT_SMALL
//	take scry into account? how/when?
typedef struct _a_gamefont_conf{
	int x;
	union{
		char name[64];//hrm.
		grs_font *ptr;
	} f;
}a_gamefont_conf;
typedef struct _gamefont_conf{
	a_gamefont_conf font[10];
	int num,cur;
}gamefont_conf;

gamefont_conf font_conf[MAX_FONTS];

char *gamefont_curfontname(int gf){
	if (font_conf[gf].cur<0)
		return Gamefont_filenames_l[gf];
	else
		return font_conf[gf].font[font_conf[gf].cur].f.name;
}

void gamefont_unloadfont(int gf){
	if (Gamefonts[gf]){
		font_conf[gf].cur=-1;
		gr_close_font(Gamefonts[gf]);
		Gamefonts[gf]=NULL;
	}
}
void gamefont_loadfont(int gf,int fi){
	if (cfexist(font_conf[gf].font[fi].f.name)){
		gamefont_unloadfont(gf);
		Gamefonts[gf]=gr_init_font(font_conf[gf].font[fi].f.name);
	}else {
		hud_message(MSGC_GAME_FEEDBACK,"Couldn't find font file %s!",font_conf[gf].font[fi].f.name);
		if (Gamefonts[gf]==NULL){
			Gamefonts[gf]=gr_init_font(Gamefont_filenames_l[gf]);
			font_conf[gf].cur=-1;
		}
		return;
	}
	font_conf[gf].cur=fi;
}
void gamefont_choose_game_font(int scrx,int scry){
	int gf,i,close=-1,m=-1;
	mprintf ((0,"gamefont_choose_game_font(%i,%i) %i\n",scrx,scry,Gamefont_installed));
	if (!Gamefont_installed) return;

	for (gf=0;gf<MAX_FONTS;gf++){
		for (i=0;i<font_conf[gf].num;i++)
			if (scrx>=font_conf[gf].font[i].x && close<font_conf[gf].font[i].x){
				close=font_conf[gf].font[i].x;
				m=i;
			}
		if (m<0)
			Error("no gamefont found for %ix%i\n",scrx,scry);

		gamefont_loadfont(gf,m);
	}

	for (i=0; i<MAX_FONTS; i++ )
		mprintf ((0,"font %i: %ix%i\n",i,Gamefonts[i]->ft_w,Gamefonts[i]->ft_h));
}
	
void addfontconf(int gf, int x,char * fn){
	int i;
	mprintf((0,"adding font %s at %i %i: ",fn,gf,x));	
	for (i=0;i<font_conf[gf].num;i++){
		if (font_conf[gf].font[i].x==x){
			mprintf((0,"replaced %i\n",i));
			if (i==font_conf[gf].cur)
				gamefont_unloadfont(gf);
			strcpy(font_conf[gf].font[i].f.name,fn);
			if (i==font_conf[gf].cur)
				gamefont_loadfont(gf,i);
			return;
		}
	}
	mprintf((0,"added %i\n",font_conf[gf].num));
	font_conf[gf].font[font_conf[gf].num].x=x;
	strcpy(font_conf[gf].font[font_conf[gf].num].f.name,fn);
	font_conf[gf].num++;
}

void gamefont_init()
{
	int i;

	if (Gamefont_installed) return;
	Gamefont_installed = 1;

	for (i=0;i<MAX_FONTS;i++){
		Gamefonts[i]=NULL;


		// HDG: commented, these files are not available in the normal
		// d1data files since they are d2 files
		// addfontconf(i,640,Gamefont_filenames_h[i]);
		addfontconf(i,640,Gamefont_filenames_h[i]); // ZICO - addition to use D2 fonts
		addfontconf(i,0,Gamefont_filenames_l[i]); // ZICO - but not in small resolutions...
	}
	// HDG: uncommented, these addon files can be easily added
//	addfontconf(4, 640,"pc6x8.fnt");
//	addfontconf(4, 1024,"pc8x16.fnt");
	if ((i=FindArg("-font320")))
		addfontconf(4,320,Args[i+1]);
	if ((i=FindArg("-font640")))
		addfontconf(4,640,Args[i+1]);
	if ((i=FindArg("-font800")))
		addfontconf(4,800,Args[i+1]);
	if ((i=FindArg("-font1024")))
		addfontconf(4,1024,Args[i+1]);
	
	gamefont_choose_game_font(grd_curscreen->sc_canvas.cv_bitmap.bm_w,grd_curscreen->sc_canvas.cv_bitmap.bm_h);
	
	atexit( gamefont_close );
}


void gamefont_close()
{
	int i;

	if (!Gamefont_installed) return;
	Gamefont_installed = 0;

	for (i=0; i<MAX_FONTS; i++ )	{
		gamefont_unloadfont(i);
//		gr_close_font( Gamefonts[i] );
//		Gamefonts[i] = NULL;
	}

}
