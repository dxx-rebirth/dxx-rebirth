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

#ifdef SHAREWARE
	#ifdef RELEASE
		#ifdef MACINTOSH
			#define Menu_pcx_name "\x01menub.pcx"	//read only from hog file
		#else
			#define Menu_pcx_name "\x01menud.pcx"	//read only from hog file
		#endif
	#else
//		#define Menu_pcx_name (MenuHires?"menub.pcx":"menud.pcx")	//name of background bitmap
		#define Menu_pcx_name "menud.pcx"
	#endif
#else
	#ifdef D2_OEM
		#ifdef RELEASE
			#define Menu_pcx_name (MenuHires?"\x01menuob.pcx":"\x01menuo.pcx")	//read only from hog file
		#else
			#define Menu_pcx_name (MenuHires?"menuob.pcx":"menuo.pcx")		//name of background bitmap
		#endif
	#else	//Full version
		#ifdef RELEASE
			#define Menu_pcx_name (MenuHires?"\x01menub.pcx":"\x01menu.pcx")	//read only from hog file
		#else
			#define Menu_pcx_name (MenuHires?"menub.pcx":"menu.pcx")		//name of background bitmap
		#endif
	#endif
#endif

extern void set_detail_level_parameters(int detail_level);

extern char *menu_difficulty_text[];
extern int Player_default_difficulty;
extern int Max_debris_objects;
extern int Auto_leveling_on;
extern int Missile_view_enabled;
extern int Escort_view_enabled;
extern int Cockpit_rear_view;

#endif
