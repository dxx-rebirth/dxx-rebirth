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

#include <stdlib.h>
#include <stdio.h>

#include "gr.h"
#include "dxxerror.h"
#include <string.h>
#include "strutil.h"
#include "args.h"
#include "gamefont.h"
#include "config.h"

static const char Gamefont_filenames_l[][16] = {
	"font1-1.fnt", // Font 0
	"font2-1.fnt", // Font 1
	"font2-2.fnt", // Font 2
	"font2-3.fnt", // Font 3
	"font3-1.fnt"  // Font 4
};

static const char Gamefont_filenames_h[][16] = {
	"font1-1h.fnt", // Font 0
	"font2-1h.fnt", // Font 1
	"font2-2h.fnt", // Font 2
	"font2-3h.fnt", // Font 3
	"font3-1h.fnt"  // Font 4
};

grs_font *Gamefonts[MAX_FONTS];

int Gamefont_installed=0;
float FNTScaleX = 1, FNTScaleY = 1;

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

const char *gamefont_curfontname(int gf){
	if (font_conf[gf].cur<0)
		return Gamefont_filenames_l[gf];
	else
		return font_conf[gf].font[font_conf[gf].cur].f.name;
}

static void gamefont_unloadfont(int gf)
{
	if (Gamefonts[gf]){
		font_conf[gf].cur=-1;
		gr_close_font(Gamefonts[gf]);
		Gamefonts[gf]=NULL;
	}
}

static void gamefont_loadfont(int gf,int fi)
{
	if (PHYSFSX_exists(font_conf[gf].font[fi].f.name,1)){
		gamefont_unloadfont(gf);
		Gamefonts[gf]=gr_init_font(font_conf[gf].font[fi].f.name);
	}else {
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
	if (!Gamefont_installed) return;

	for (gf=0;gf<MAX_FONTS;gf++){
		for (i=0;i<font_conf[gf].num;i++)
			if ((scrx>=font_conf[gf].font[i].x && close<font_conf[gf].font[i].x)&&(scry>=font_conf[gf].font[i].y && close<font_conf[gf].font[i].y)){
				close=font_conf[gf].font[i].x;
				m=i;
			}
		if (m<0)
			Error("no gamefont found for %ix%i\n",scrx,scry);

#ifdef OGL
	if (!GameArg.OglFixedFont)
	{
		// if there's no texture filtering, scale by int
		if (!GameCfg.TexFilt)
		{
			FNTScaleX = (int)scrx/font_conf[gf].font[m].x;
			FNTScaleY = (int)scry/font_conf[gf].font[m].y;
		}
		else
		{
			FNTScaleX = (float)scrx/font_conf[gf].font[m].x;
			FNTScaleY = (float)scry/font_conf[gf].font[m].y;
		}

		// keep proportions
		if (FNTScaleY*100 < FNTScaleX*100)
			FNTScaleX = FNTScaleY;
		else if (FNTScaleX*100 < FNTScaleY*100)
			FNTScaleY = FNTScaleX;
	}
#endif
		gamefont_loadfont(gf,m);
	}
}
	
static void addfontconf(int gf, int x, int y, const char *const fn)
{
	int i;

	if (!PHYSFSX_exists(fn,1))
		return;

	for (i=0;i<font_conf[gf].num;i++){
		if (font_conf[gf].font[i].x==x && font_conf[gf].font[i].y==y){
			if (i==font_conf[gf].cur)
				gamefont_unloadfont(gf);
			strcpy(font_conf[gf].font[i].f.name,fn);
			if (i==font_conf[gf].cur)
				gamefont_loadfont(gf,i);
			return;
		}
	}
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

		if (GameArg.GfxHiresFNTAvailable)
			addfontconf(i,640,480,Gamefont_filenames_h[i]); // ZICO - addition to use D2 fonts if available
		addfontconf(i,320,200,Gamefont_filenames_l[i]);
	}

	gamefont_choose_game_font(grd_curscreen->sc_canvas.cv_bitmap.bm_w,grd_curscreen->sc_canvas.cv_bitmap.bm_h);
}


void gamefont_close()
{
	int i;

	if (!Gamefont_installed) return;
	Gamefont_installed = 0;

	for (i=0; i<MAX_FONTS; i++ )	{
		gamefont_unloadfont(i);
	}

}
