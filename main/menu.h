/* $Id: menu.h,v 1.4 2002-08-22 19:18:13 btb Exp $ */
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


#ifndef _MENU_H
#define _MENU_H

//returns number of item chosen
extern int DoMenu();
extern void do_options_menu();

extern int MenuHires;

#ifdef RELEASE  //read only from hog file
#define MENU_PCX_SHAREWARE ("\x01menud.pcx")
#define MENU_PCX_OEM (MenuHires?"\x01menuob.pcx":"\x01menuo.pcx")
#define MENU_PCX_FULL (MenuHires?"\x01menub.pcx":"\x01menu.pcx")
#else
#define MENU_PCX_SHAREWARE ("menud.pcx")
#define MENU_PCX_OEM (MenuHires?"menuob.pcx":"menuo.pcx")
#define MENU_PCX_FULL (MenuHires?"menub.pcx":"menu.pcx")
#endif

//name of background bitmap
#define Menu_pcx_name (cfexist(MENU_PCX_FULL)?MENU_PCX_FULL:(cfexist(MENU_PCX_OEM)?MENU_PCX_OEM:MENU_PCX_SHAREWARE))

extern void set_detail_level_parameters(int detail_level);

extern char *menu_difficulty_text[];
extern int Player_default_difficulty;
extern int Max_debris_objects;
extern int Auto_leveling_on;
extern int Missile_view_enabled;
extern int Escort_view_enabled;
extern int Cockpit_rear_view;

#endif
