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
 * Fonts for the game.
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
char * Gamefont_filenames_h[] = { 	HIRES_DIR "font1-1h.fnt",			// Font 0
											HIRES_DIR "font2-1h.fnt",			// Font 1
											HIRES_DIR "font2-2h.fnt",			// Font 2
											HIRES_DIR "font2-3h.fnt",			// Font 3
											HIRES_DIR "font3-1h.fnt"			// Font 4
										};

grs_font *Gamefonts[MAX_FONTS];

int Gamefont_installed=0;

//code to allow variable GAME_FONT, added 10/7/99 Matt Mueller - updated 11/18/99 to handle all fonts, not just GFONT_SMALL
//	take scry into account? how/when?
typedef struct _a_gamefont_conf{
	int x;
	int y;
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
			if ((scrx>=font_conf[gf].font[i].x && close<font_conf[gf].font[i].x)&&(scry>=font_conf[gf].font[i].y && close<font_conf[gf].font[i].y)){
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
	
void addfontconf(int gf, int x, int y, char * fn){
	int i;
	mprintf((0,"adding font %s at %i %i %i: ",fn,gf,x,y));	
	for (i=0;i<font_conf[gf].num;i++){
		if (font_conf[gf].font[i].x==x && font_conf[gf].font[i].y==y){
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
	font_conf[gf].font[font_conf[gf].num].y=y;
	strcpy(font_conf[gf].font[font_conf[gf].num].f.name,fn);
	font_conf[gf].num++;
}

void gamefont_init()
{
	int i;

	if (Gamefont_installed)
		return;

	Gamefont_installed = 1;

	for (i=0;i<MAX_FONTS;i++){
		Gamefonts[i]=NULL;

		// HDG: commented, these files are not available in the normal
		// d1data files since they are d2 files
		// addfontconf(i,640,Gamefont_filenames_h[i]);

		if (GameArg.GfxUseHiresFont
			&& cfexist(DESCENT_DATA_PATH HIRES_DIR "font1-1h.fnt")
			&& cfexist(DESCENT_DATA_PATH HIRES_DIR "font2-1h.fnt")
			&& cfexist(DESCENT_DATA_PATH HIRES_DIR "font2-2h.fnt")
			&& cfexist(DESCENT_DATA_PATH HIRES_DIR "font2-3h.fnt")
			&& cfexist(DESCENT_DATA_PATH HIRES_DIR "font3-1h.fnt"))
			addfontconf(i,640,480,Gamefont_filenames_h[i]); // ZICO - addition to use D2 fonts if available
		else
			GameArg.GfxUseHiresFont = 0;

		addfontconf(i,0,0,Gamefont_filenames_l[i]);
	}
	// HDG: uncommented, these addon files can be easily added
//	addfontconf(4, 640,"pc6x8.fnt");
//	addfontconf(4, 1024,"pc8x16.fnt");

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
