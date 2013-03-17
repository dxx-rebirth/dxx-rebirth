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
 * Menu prototypes and variables
 *
 */

#ifndef _MENU_H
#define _MENU_H

extern int hide_menus(void);
extern void show_menus(void);

// called once at program startup to get the player's name
extern int RegisterPlayer();

// returns number of item chosen
extern int DoMenu();
extern void do_options_menu();
extern void d2x_options_menu();
extern int select_demo(void);

#define MENU_PCX_MAC_SHARE ("menub.pcx")
#define MENU_PCX_SHAREWARE ("menud.pcx")
#define MENU_PCX_OEM (HIRESMODE?"menuob.pcx":"menuo.pcx")
#define MENU_PCX_FULL (HIRESMODE?"menub.pcx":"menu.pcx")

// name of background bitmap
#define Menu_pcx_name (PHYSFSX_exists(MENU_PCX_FULL,1)?MENU_PCX_FULL:(PHYSFSX_exists(MENU_PCX_OEM,1)?MENU_PCX_OEM:PHYSFSX_exists(MENU_PCX_SHAREWARE,1)?MENU_PCX_SHAREWARE:MENU_PCX_MAC_SHARE))
#define STARS_BACKGROUND ((HIRESMODE && PHYSFSX_exists("starsb.pcx",1))?"starsb.pcx":PHYSFSX_exists("stars.pcx",1)?"stars.pcx":"starsb.pcx")

extern char *menu_difficulty_text[];
extern int Escort_view_enabled;
extern int Cockpit_rear_view;

#endif /* _MENU_H */
