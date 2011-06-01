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
extern int select_demo(void);
#define Menu_pcx_name (((SWIDTH>=640&&SHEIGHT>=480) && PHYSFSX_exists("menuh.pcx",1))?"menuh.pcx":"menu.pcx")
#define STARS_BACKGROUND (((SWIDTH>=640&&SHEIGHT>=480) && PHYSFSX_exists("starsb.pcx",1))?"starsb.pcx":"stars.pcx")

extern char *menu_difficulty_text[];

#endif
