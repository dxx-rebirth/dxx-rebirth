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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/weapon.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:44:46 $
 * 
 * Functions for weapons...
 * 
 * $Log: weapon.c,v $
 * Revision 1.1.1.1  2006/03/17 19:44:46  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:12:04  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.1  1995/03/21  14:38:43  john
 * Ifdef'd out the NETWORK code.
 * 
 * Revision 2.0  1995/02/27  11:27:25  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.54  1995/02/15  15:21:48  mike
 * make smart missile select if mega missiles used up.
 * 
 * 
 * Revision 1.53  1995/02/12  02:12:30  john
 * Fixed bug with state restore making weapon beeps.
 * 
 * Revision 1.52  1995/02/09  20:42:15  mike
 * change weapon autoselect, always autoselect smart, mega.
 * 
 * Revision 1.51  1995/02/07  20:44:26  mike
 * autoselect mega, smart when you pick them up.
 * 
 * Revision 1.50  1995/02/07  13:32:25  rob
 * Added include of multi.h
 * 
 * Revision 1.49  1995/02/07  13:21:33  yuan
 * Fixed 2nd typo
 * 
 * Revision 1.48  1995/02/07  13:16:39  yuan
 * Fixed typo.
 * 
 * Revision 1.47  1995/02/07  12:53:12  rob
 * Added network sound prop. to weapon switch.
 * 
 * Revision 1.46  1995/02/06  15:53:17  mike
 * don't autoselect smart or mega missile when you pick it up.
 * 
 * Revision 1.45  1995/02/02  21:43:34  mike
 * make autoselection better.
 * 
 * Revision 1.44  1995/02/02  16:27:21  mike
 * make concussion missiles trade up.
 * 
 * Revision 1.43  1995/02/01  23:34:57  adam
 * messed with weapon change sounds
 * 
 * Revision 1.42  1995/02/01  17:12:47  mike
 * Make smart missile, mega missile not auto-select.
 * 
 * Revision 1.41  1995/02/01  15:50:54  mike
 * fix bogus weapon selection sound code.
 * 
 * Revision 1.40  1995/01/31  16:16:31  mike
 * Separate smart blobs for robot and player.
 * 
 * Revision 1.39  1995/01/30  21:12:11  mike
 * Use new weapon selection sounds, different for primary and secondary.
 * 
 * Revision 1.38  1995/01/29  13:46:52  mike
 * Don't auto-select fusion cannon when you run out of energy.
 * 
 * Revision 1.37  1995/01/20  11:11:13  allender
 * record weapon changes again.  (John somehow lost my 1.35 changes).
 * 
 * Revision 1.36  1995/01/19  17:00:46  john
 * Made save game work between levels.
 * 
 * Revision 1.34  1995/01/09  17:03:48  mike
 * fix autoselection of weapons.
 * 
 * Revision 1.33  1995/01/05  15:46:31  john
 * Made weapons not rearm when starting a saved game.
 * 
 * Revision 1.32  1995/01/03  12:34:23  mike
 * autoselect next lower weapon if run out of smart or mega missile.
 * 
 * Revision 1.31  1994/12/12  21:39:37  matt
 * Changed vulcan ammo: 10K max, 5K w/weapon, 1250 per powerup
 * 
 * Revision 1.30  1994/12/09  19:55:04  matt
 * Added weapon name in "not available in shareware" message
 * 
 * Revision 1.29  1994/12/06  13:50:24  adam
 * added shareware msg. when choosing 4 top weapons
 * 
 * Revision 1.28  1994/12/02  22:07:13  mike
 * if you gots 19 concussion missiles and you runs over 4, say you picks up 1, not 4, we do the math, see?
 * 
 * Revision 1.27  1994/12/02  20:06:24  matt
 * Made vulcan ammo print at approx 25 times actual
 * 
 * Revision 1.26  1994/12/02  15:05:03  matt
 * Fixed bogus weapon constants and arrays
 * 
 * Revision 1.25  1994/12/02  10:50:34  yuan
 * Localization
 * 
 * Revision 1.24  1994/11/29  15:48:28  matt
 * selecting weapon now makes sound
 * 
 * Revision 1.23  1994/11/28  11:26:58  matt
 * Cleaned up hud message printing for picking up weapons
 * 
 * Revision 1.22  1994/11/27  23:13:39  matt
 * Made changes for new mprintf calling convention
 * 
 * Revision 1.21  1994/11/12  16:38:34  mike
 * clean up default ammo stuff.
 * 
 * Revision 1.20  1994/11/07  17:41:18  mike
 * messages for when you try to fire a weapon you don't have or don't have ammo for.
 * 
 * Revision 1.19  1994/10/21  20:40:05  mike
 * fix double vulcan ammo.
 * 
 * Revision 1.18  1994/10/20  09:49:05  mike
 * kill messages no one liked...*sniff* *sniff*
 * 
 * Revision 1.17  1994/10/19  11:17:07  mike
 * Limit amount of player ammo.
 * 
 * Revision 1.16  1994/10/12  08:04:18  mike
 * Fix proximity/homing confusion.
 * 
 * Revision 1.15  1994/10/11  18:27:58  matt
 * Changed auto selection of secondary weapons
 * 
 * Revision 1.14  1994/10/08  23:37:54  matt
 * Don't pick up weapons you already have; also fixed auto_select bug
 * for seconary weapons
 * 
 * Revision 1.13  1994/10/08  14:55:47  matt
 * Fixed bug that selected vulcan cannon when picked up ammo, even though
 * you didn't have the weapon.
 * 
 * Revision 1.12  1994/10/08  12:50:32  matt
 * Fixed bug that let you select weapons you don't have
 * 
 * Revision 1.11  1994/10/07  23:37:56  matt
 * Made weapons select when pick up better one
 * 
 * Revision 1.10  1994/10/07  16:02:08  matt
 * Fixed problem with weapon auto-select
 * 
 * Revision 1.9  1994/10/05  17:00:20  matt
 * Made player_has_weapon() public and moved constants to header file
 * 
 * Revision 1.8  1994/09/26  11:27:13  mike
 * Fix auto selection of weapon when you run out of ammo.
 * 
 * Revision 1.7  1994/09/13  16:40:45  mike
 * Add rearm delay and missile firing delay.
 * 
 * Revision 1.6  1994/09/13  14:43:12  matt
 * Added cockpit weapon displays
 * 
 * Revision 1.5  1994/09/03  15:23:06  mike
 * Auto select next weaker weapon when one runs out, clean up code.
 * 
 * Revision 1.4  1994/09/02  16:38:19  mike
 * Eliminate a pile of arrays, associate weapon data with Weapon_info.
 * 
 * Revision 1.3  1994/09/02  11:57:10  mike
 * Add a bunch of stuff, I forget what.
 * 
 * Revision 1.2  1994/06/03  16:26:32  john
 * Initial version.
 * 
 * Revision 1.1  1994/06/03  14:40:43  john
 * Initial revision
 * 
 * 
 */


#ifdef RCS
#pragma off (unreferenced)
static char rcsid[] = "$Id: weapon.c,v 1.1.1.1 2006/03/17 19:44:46 zicodxx Exp $";
#pragma on (unreferenced)
#endif

#include "game.h"
#include "weapon.h"
#include "mono.h"
#include "player.h"
#include "gauges.h"
#include "error.h"
#include "sounds.h"
#include "text.h"
#include "powerup.h"
#include "newdemo.h"
#include "multi.h"
#include "reorder.h"

//#define FUSION_KEEPS_CHARGE

//	Note, only Vulcan cannon requires ammo.
//ubyte	Default_primary_ammo_level[MAX_PRIMARY_WEAPONS] = {255, 0, 255, 255, 255};
//ubyte	Default_secondary_ammo_level[MAX_SECONDARY_WEAPONS] = {3, 0, 0, 0, 0};

//	Convert primary weapons to indices in Weapon_info array.
ubyte Primary_weapon_to_weapon_info[MAX_PRIMARY_WEAPONS] = {0, 11, 12, 13, 14};
ubyte Secondary_weapon_to_weapon_info[MAX_SECONDARY_WEAPONS] = {8, 15, 16, 17, 18};

int Primary_ammo_max[MAX_PRIMARY_WEAPONS] = {0, VULCAN_AMMO_MAX, 0, 0, 0};
ubyte Secondary_ammo_max[MAX_SECONDARY_WEAPONS] = {20, 10, 10, 5, 5};

weapon_info Weapon_info[MAX_WEAPON_TYPES];
int	N_weapon_types=0;
byte	Primary_weapon, Secondary_weapon;

//char	*Primary_weapon_names[MAX_PRIMARY_WEAPONS] = {
//	"Laser Cannon",
//	"Vulcan Cannon",
//	"Spreadfire Cannon",
//	"Plasma Cannon",
//	"Fusion Cannon"
//};

//char	*Secondary_weapon_names[MAX_SECONDARY_WEAPONS] = {
//	"Concussion Missile",
//	"Homing Missile",
//	"Proximity Bomb",
//	"Smart Missile",
//	"Mega Missile"
//};

//char	*Primary_weapon_names_short[MAX_PRIMARY_WEAPONS] = {
//	"Laser",
//	"Vulcan",
//	"Spread",
//	"Plasma",
//	"Fusion"
//};

//char	*Secondary_weapon_names_short[MAX_SECONDARY_WEAPONS] = {
//	"Concsn\nMissile",
//	"Homing\nMissile",
//	"Proxim.\nBomb",
//	"Smart\nMissile",
//	"Mega\nMissile"
//};

//	------------------------------------------------------------------------------------
//	Return:
// Bits set:
//		HAS_WEAPON_FLAG
//		HAS_ENERGY_FLAG
//		HAS_AMMO_FLAG	
// See weapon.h for bit values
int player_has_weapon(int weapon_num, int secondary_flag)
{
	int	return_value = 0;
	int	weapon_index;

	//	Hack! If energy goes negative, you can't fire a weapon that doesn't require energy.
	//	But energy should not go negative (but it does), so find out why it does!
	if (Players[Player_num].energy < 0)
		Players[Player_num].energy = 0;

	if (!secondary_flag) {
                if(weapon_num >= MAX_PRIMARY_WEAPONS)
                 {
                   switch(weapon_num-MAX_PRIMARY_WEAPONS)
                    {
                     case 0 : if((Players[Player_num].laser_level != 0)||(Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
                               return 0;
                              break;
                     case 1 : if((Players[Player_num].laser_level != 1)||(Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
                               return 0;
                              break;
                     case 2 : if((Players[Player_num].laser_level != 2)||(Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
                               return 0;
                              break;
                     case 3 : if((Players[Player_num].laser_level != 3)||(Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
                               return 0;
                              break;
                     case 4 : if((Players[Player_num].laser_level != 0)||!(Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
                               return 0;
                              break;
                     case 5 : if((Players[Player_num].laser_level != 1)||!(Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
                               return 0;
                              break;
                     case 6 : if((Players[Player_num].laser_level != 2)||!(Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
                               return 0;
                              break;
                     case 7 : if((Players[Player_num].laser_level != 3)||!(Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
                               return 0;
                              break;
                    }
                  weapon_num = 0;
                 }

		weapon_index = Primary_weapon_to_weapon_info[weapon_num];

		if (Players[Player_num].primary_weapon_flags & (1 << weapon_num))
			return_value |= HAS_WEAPON_FLAG;

		if (Weapon_info[weapon_index].ammo_usage <= Players[Player_num].primary_ammo[weapon_num])
			return_value |= HAS_AMMO_FLAG;

                //added on 1/21/99 by Victor Rachels... yet another hack
                //fusion has 0 energy usage, HAS_ENERGY_FLAG was always true
                if(weapon_num==FUSION_INDEX)
                 {
                    if(Players[Player_num].energy >= F1_0*2)
                     return_value |= HAS_ENERGY_FLAG;
                 }
                else
                //end this section addition - VR
                 if (Weapon_info[weapon_index].energy_usage <= Players[Player_num].energy)
                  return_value |= HAS_ENERGY_FLAG;
  
	} else {
		weapon_index = Secondary_weapon_to_weapon_info[weapon_num];

		if (Players[Player_num].secondary_weapon_flags & (1 << weapon_num))
			return_value |= HAS_WEAPON_FLAG;

		if (Weapon_info[weapon_index].ammo_usage <= Players[Player_num].secondary_ammo[weapon_num])
			return_value |= HAS_AMMO_FLAG;

		if (Weapon_info[weapon_index].energy_usage <= Players[Player_num].energy)
			return_value |= HAS_ENERGY_FLAG;
	}


	return return_value;
}

//	------------------------------------------------------------------------------------
//if message flag set, print message saying selected
void select_weapon(int weapon_num, int secondary_flag, int print_message, int wait_for_rearm)
{
	char	*weapon_name;

#ifndef SHAREWARE
	if (Newdemo_state==ND_STATE_RECORDING )
		newdemo_record_player_weapon(secondary_flag, weapon_num);
#endif

	if (!secondary_flag) {

                 //added on 10/9/98 by Victor Rachels to add laser cycle
                 if (weapon_num >= MAX_PRIMARY_WEAPONS)
                  {
                    LaserPowSelected=weapon_num;
                    weapon_num = 0;
                     if(Primary_weapon==0)
                      return;
                  }
                 else if (weapon_num == 0)
                  LaserPowSelected=0;
                 //end this section addition

		if (Primary_weapon != weapon_num) {
			#ifndef FUSION_KEEPS_CHARGE
			//added 8/6/98 by Victor Rachels to fix fusion charge bug
                        Fusion_charge=0;
			//end edit - Victor Rachels
			#endif
                        if (wait_for_rearm) digi_play_sample_once( SOUND_GOOD_SELECTION_PRIMARY, F1_0 );
			#ifdef NETWORK
			if (Game_mode & GM_MULTI)	{
				if (wait_for_rearm) multi_send_play_sound(SOUND_GOOD_SELECTION_PRIMARY, F1_0);
			}
			#endif
			if (wait_for_rearm)
				Next_laser_fire_time = GameTime + REARM_TIME;
			else
				Next_laser_fire_time = 0;
			Global_laser_firing_count = 0;
		} else 	{
			if (wait_for_rearm) digi_play_sample( SOUND_ALREADY_SELECTED, F1_0 );
		}
		Primary_weapon = weapon_num;
		weapon_name = PRIMARY_WEAPON_NAMES(weapon_num);
	} else {

		if (Secondary_weapon != weapon_num) {
			if (wait_for_rearm) digi_play_sample_once( SOUND_GOOD_SELECTION_SECONDARY, F1_0 );
			#ifdef NETWORK
			if (Game_mode & GM_MULTI)	{
				if (wait_for_rearm) multi_send_play_sound(SOUND_GOOD_SELECTION_PRIMARY, F1_0);
			}
			#endif
			if (wait_for_rearm)
				Next_missile_fire_time = GameTime + REARM_TIME;
			else
				Next_missile_fire_time = 0;
			Global_missile_firing_count = 0;
		} else	{
			if (wait_for_rearm) digi_play_sample_once( SOUND_ALREADY_SELECTED, F1_0 );
		}
		Secondary_weapon = weapon_num;
		weapon_name = SECONDARY_WEAPON_NAMES(weapon_num);
	}

	if (print_message)
		hud_message(MSGC_WEAPON_SELECT, "%s %s", weapon_name, TXT_SELECTED);

        //added on 2/8/99 by Victor Rachels to add allweapon hud info
        gauge_update_hud_mode=1;
        //end this section additon - VR
}

//	------------------------------------------------------------------------------------
//	Select a weapon, primary or secondary.
void do_weapon_select(int weapon_num, int secondary_flag)
{
        //added on 10/9/98 by Victor Rachels to add laser cycle
        int     oweapon = weapon_num;
        //end this section addition - Victor Rachels
	int	weapon_status = player_has_weapon(weapon_num, secondary_flag);
	char	*weapon_name;


	#ifdef SHAREWARE	// do special hud msg. for picking registered weapon in shareware version.
	if (weapon_num >= NUM_SHAREWARE_WEAPONS) {
		weapon_name = secondary_flag?SECONDARY_WEAPON_NAMES(weapon_num):PRIMARY_WEAPON_NAMES(weapon_num);
		hud_message(MSGC_GAME_FEEDBACK, "%s %s!", weapon_name,TXT_NOT_IN_SHAREWARE);
		digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
		return;
	}
	#endif

	if (!secondary_flag) {

                 //added on 10/9/98 by Victor Rachels to add laser cycle
                 if (weapon_num >= MAX_PRIMARY_WEAPONS)                                   
//                  switch(weapon_num-MAX_PRIMARY_WEAPONS)
//                   {
//                    case 0 :
                    weapon_num = 0;
//                    break;
//                   }
                //end this section addition - Victor Rachels

		weapon_name = PRIMARY_WEAPON_NAMES(weapon_num);
		if ((weapon_status & HAS_WEAPON_FLAG) == 0) {
			hud_message(MSGC_GAME_FEEDBACK, "%s %s!", TXT_DONT_HAVE, weapon_name);
			digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
			return;
		}
		else if ((weapon_status & HAS_AMMO_FLAG) == 0) {
			hud_message(MSGC_GAME_FEEDBACK, "%s %s!", TXT_DONT_HAVE_AMMO, weapon_name);
			digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
			return;
		}
	}
	else {
		weapon_name = SECONDARY_WEAPON_NAMES(weapon_num);
		if (weapon_status != HAS_ALL) {
			hud_message(MSGC_GAME_FEEDBACK, "%s %s%s",TXT_HAVE_NO, weapon_name, TXT_SX);
			digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
			return;
		}
	}

        //added on 10/9/98 by Victor Rachels to add laser cycle
        weapon_num=oweapon;
        //end this section addition - Victor Rachels

	select_weapon(weapon_num, secondary_flag, 1, 1);
}

//added/killed on 10/8/98 by Victor Rachels to remove #if 0
//-killed- // original non-customizable version
//-killed- //      ----------------------------------------------------------------------------------------
//-killed- //      Automatically select next best weapon if unable to fire current weapon.
//-killed- // Weapon type: 0==primary, 1==secondary
//-killed- void auto_select_weapon(int weapon_type)
//-killed- {
//-killed-         int     r;
//-killed- 
//-killed-         if (weapon_type==0) {
//-killed-                 r = player_has_weapon(Primary_weapon, 0);
//-killed-                 if (r != HAS_ALL) {
//-killed-                         int     cur_weapon;
//-killed-                         int     try_again = 1;
//-killed-  
//-killed-                         cur_weapon = Primary_weapon;
//-killed-  
//-killed-                         while (try_again) {
//-killed-                                 cur_weapon--;
//-killed-                                 if (cur_weapon < 0)
//-killed-                                         cur_weapon = MAX_PRIMARY_WEAPONS-1;
//-killed-  
//-killed-                                 //      Hack alert!  Because the fusion uses 0 energy at the end (it's got the weird chargeup)
//-killed-                                 //      it looks like it takes 0 to fire, but it doesn't, so never auto-select.
//-killed-                                 if (cur_weapon == FUSION_INDEX)
//-killed-                                         continue;
//-killed- 
//-killed-                                 if (cur_weapon == Primary_weapon) {
//-killed-                                         hud_message(MSGC_WEAPON_EMPTY, TXT_NO_PRIMARY);
//-killed-                                         try_again = 0;                          // Tried all weapons!
//-killed-                                         select_weapon(0, 0, 0, 1);
//-killed-                                 } else if (player_has_weapon(cur_weapon, 0) == HAS_ALL) {
//-killed-                                         select_weapon(cur_weapon, 0, 1, 1 );
//-killed-                                         try_again = 0;
//-killed-                                 }
//-killed-                         }
//-killed-                 }
//-killed- 
//-killed-         } else {
//-killed- 
//-killed-                 Assert(weapon_type==1);
//-killed- 
//-killed-                 if (Secondary_weapon != PROXIMITY_INDEX) {
//-killed-                         if (!(player_has_weapon(Secondary_weapon, 1) == HAS_ALL)) {
//-killed-                                 if (Secondary_weapon > SMART_INDEX)
//-killed-                                         if (player_has_weapon(SMART_INDEX, 1) == HAS_ALL) {
//-killed-                                                 select_weapon(SMART_INDEX, 1, 1, 1);
//-killed-                                                 goto weapon_selected;
//-killed-                                         }
//-killed-                                 if (player_has_weapon(HOMING_INDEX, 1) == HAS_ALL)
//-killed-                                         select_weapon(HOMING_INDEX, 1, 1, 1);
//-killed-                                 else if (player_has_weapon(CONCUSSION_INDEX, 1) == HAS_ALL)
//-killed-                                         select_weapon(CONCUSSION_INDEX, 1, 1, 1);
//-killed- weapon_selected: ;
//-killed-                         }
//-killed-                 }
//-killed-         }
//-killed- 
//-killed- }

//	----------------------------------------------------------------------------------------
//	Automatically select best available weapon if unable to fire current weapon.
// Weapon type: 0==primary, 1==secondary
void auto_select_weapon(int weapon_type) {
	int i;
	int *order = weapon_type ? secondary_order : primary_order;
	int weapon_count = weapon_type ? MAX_SECONDARY_WEAPONS : MAX_PRIMARY_WEAPONS;
	int best_weapon = -1;
	int best_order = 0;

	if (player_has_weapon(
	    weapon_type ? Secondary_weapon : Primary_weapon,
	    weapon_type) != HAS_ALL)
         {
            //added on 1/21/99 by Victor Rachels for noenergy vulcan select
            if ((weapon_type==0) &&
                (order[VULCAN_INDEX] > 0) &&
                (player_has_weapon(VULCAN_INDEX,0)==HAS_ALL))
             {
               select_weapon(VULCAN_INDEX,0,0,1);
               return;
             }
            //end this section addition - VR

		for (i = 0; i < weapon_count; i++)
			if ((order[i] > best_order) &&
                            (player_has_weapon(i, weapon_type) == HAS_ALL))
                         {
				best_weapon = i;
				best_order = order[i];
                         }
		if (best_weapon >= 0)
			select_weapon(best_weapon, weapon_type, 1, 1);
		else if (weapon_type == 0) {
			hud_message(MSGC_WEAPON_EMPTY, TXT_NO_PRIMARY);
			select_weapon(0, 0, 0, 1);
                }
	}
}


#ifndef RELEASE

//	----------------------------------------------------------------------------------------
//	Show player which weapons he has, how much ammo...
//	Looks like a debug screen now because it writes to mono screen, but that will change...
void show_weapon_status(void)
{
	int	i;

	for (i=0; i<MAX_PRIMARY_WEAPONS; i++) {
		if (Players[Player_num].primary_weapon_flags & (1 << i))
			mprintf((0, "HAVE"));
		else
			mprintf((0, "    "));

		mprintf((0, "  Weapon: %20s, charges: %4i\n", PRIMARY_WEAPON_NAMES(i), Players[Player_num].primary_ammo[i]));
	}

	mprintf((0, "\n"));
	for (i=0; i<MAX_SECONDARY_WEAPONS; i++) {
		if (Players[Player_num].secondary_weapon_flags & (1 << i))
			mprintf((0, "HAVE"));
		else
			mprintf((0, "    "));

		mprintf((0, "  Weapon: %20s, charges: %4i\n", SECONDARY_WEAPON_NAMES(i), Players[Player_num].secondary_ammo[i]));
	}

	mprintf((0, "\n"));
	mprintf((0, "\n"));

}

#endif

// select primary weapon if it has a higher order than the current weapon
void maybe_select_primary(int weapon_index)
{
       if (primary_order[weapon_index] > 0)
        {
         if(LaserPowSelected&&Primary_weapon==0)
          {
           if(primary_order[weapon_index] > primary_order[LaserPowSelected])
            select_weapon(weapon_index, 0, 0, 1);
          }
         else if(primary_order[weapon_index] > primary_order[Primary_weapon])
          select_weapon(weapon_index, 0, 0, 1);
         else{
//         nm_messagebox(NULL,1,TXT_OK,"murp %i==1,%i=prim,%i==7,%i!=7",weapon_index,Primary_weapon,player_has_weapon(weapon_index,0),player_has_weapon(Primary_weapon,0));
          if((weapon_index==VULCAN_INDEX) &&
                 (player_has_weapon(weapon_index,0)==HAS_ALL) &&
                 (player_has_weapon(Primary_weapon,0)!=HAS_ALL))
          select_weapon(weapon_index, 0, 0, 1);
          }
        }
}

// select secondary weapon if it has a higher order than the current weapon
void maybe_select_secondary(int weapon_index)
{
	if ((secondary_order[weapon_index] > 0) &&
	    (secondary_order[weapon_index] > secondary_order[Secondary_weapon]))
		select_weapon(weapon_index, 1, 0, 1);
}


//	---------------------------------------------------------------------
//called when one of these weapons is picked up
//when you pick up a secondary, you always get the weapon & ammo for it
//	Returns true if powerup picked up, else returns false.
int pick_up_secondary(int weapon_index,int count)
{
	int	num_picked_up;

	if (Players[Player_num].secondary_ammo[weapon_index] >= Secondary_ammo_max[weapon_index]) {
		hud_message(MSGC_PICKUP_TOOMUCH, "%s %d %ss!", TXT_ALREADY_HAVE, Players[Player_num].secondary_ammo[weapon_index],SECONDARY_WEAPON_NAMES(weapon_index));
		return 0;
	}

	Players[Player_num].secondary_weapon_flags |= (1<<weapon_index);
	Players[Player_num].secondary_ammo[weapon_index] += count;

	num_picked_up = count;
	if (Players[Player_num].secondary_ammo[weapon_index] > Secondary_ammo_max[weapon_index]) {
		num_picked_up = count - (Players[Player_num].secondary_ammo[weapon_index] - Secondary_ammo_max[weapon_index]);
		Players[Player_num].secondary_ammo[weapon_index] = Secondary_ammo_max[weapon_index];
	}

//added on 9/14/98 by Victor Rachels to make weapon cycle toggle
         if(Allow_secondary_cycle)
          {
//end this section addition - Victor Rachels  (with '}' at below)

           //if you pick up a missile with a higher order than the current,
           //then select it
           maybe_select_secondary(weapon_index);
           //if you pick up a missile and you're out of ammo, and it may be
           //autoselected, then select it
            if ((Players[Player_num].secondary_ammo[Secondary_weapon] == 0) &&
                (secondary_order[weapon_index] > 0))
             select_weapon(weapon_index,1, 0, 1);
          }

	//note: flash for all but concussion was 7,14,21
	if (count>1) {
		PALETTE_FLASH_ADD(15,15,15);
		hud_message(MSGC_PICKUP_OK, "%d %s%s",num_picked_up,SECONDARY_WEAPON_NAMES(weapon_index), TXT_SX);
	}
	else {
		PALETTE_FLASH_ADD(10,10,10);
		hud_message(MSGC_PICKUP_OK, "%s!",SECONDARY_WEAPON_NAMES(weapon_index));
	}

        //added on 2/8/99 by Victor Rachels to add allweapon hud info
        gauge_update_hud_mode=1;
        //end this section additon - VR
	return 1;
}

//called when a primary weapon is picked up
//returns true if actually picked up
int pick_up_primary(int weapon_index)
{
	ubyte old_flags = Players[Player_num].primary_weapon_flags;
	ubyte flag = 1<<weapon_index;

	if (Players[Player_num].primary_weapon_flags & flag) {		//already have
		hud_message(MSGC_PICKUP_ALREADY, "%s %s!", TXT_ALREADY_HAVE_THE, PRIMARY_WEAPON_NAMES(weapon_index));
		return 0;
	}

	Players[Player_num].primary_weapon_flags |= flag;

//added on 9/14/98 by Victor Rachels for weapon cycle toggle
        if (Allow_primary_cycle)
         {
//end this section addition - Victor Rachels
            if (!(old_flags & flag))
             maybe_select_primary(weapon_index);
         }


	PALETTE_FLASH_ADD(7,14,21);
	hud_message(MSGC_PICKUP_OK, "%s!",PRIMARY_WEAPON_NAMES(weapon_index));


        //added on 2/8/99 by Victor Rachels to add allweapon hud info
        gauge_update_hud_mode=1;
        //end this section additon - VR
	return 1;
}

//called when ammo (for the vulcan cannon) is picked up
//	Return true if ammo picked up, else return false.
int pick_up_ammo(int class_flag,int weapon_index,int ammo_count)
{
	int old_ammo=class_flag;		//kill warning

	Assert(class_flag==CLASS_PRIMARY && weapon_index==VULCAN_INDEX);

	if (Players[Player_num].primary_ammo[weapon_index] == Primary_ammo_max[weapon_index])
		return 0;

	old_ammo = Players[Player_num].primary_ammo[weapon_index];

        Players[Player_num].primary_ammo[weapon_index] += ammo_count;

	if (Players[Player_num].primary_ammo[weapon_index] > Primary_ammo_max[weapon_index])
		Players[Player_num].primary_ammo[weapon_index] = Primary_ammo_max[weapon_index];

        if (Players[Player_num].primary_weapon_flags&(1<<weapon_index) && old_ammo==0)
//added on 11/01/98 by Victor Rachels - fix primary autoselect
          if(Allow_primary_cycle) //since this function is vulcan only anyway
//end this section addition
           maybe_select_primary(weapon_index);

        //added on 2/8/99 by Victor Rachels to add allweapon hud info
        gauge_update_hud_mode=1;
        //end this section additon - VR
	return 1;
}
