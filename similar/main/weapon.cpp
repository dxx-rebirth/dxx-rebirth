/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

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
 * Functions for weapons...
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "game.h"
#include "laser.h"
#include "weapon.h"
#include "player.h"
#include "gauges.h"
#include "dxxerror.h"
#include "sounds.h"
#include "text.h"
#include "powerup.h"
#include "fireball.h"
#include "newdemo.h"
#include "multi.h"
#include "object.h"
#include "newmenu.h"
#include "gamemine.h"
#include "ai.h"
#include "args.h"
#include "playsave.h"
#include "physfs-serial.h"

#include "compiler-range_for.h"
#include "highest_valid.h"
#include "partial_range.h"

static int POrderList (int num);
static int SOrderList (int num);
//	Note, only Vulcan cannon requires ammo.
// NOTE: Now Vulcan and Gauss require ammo. -5/3/95 Yuan
//ubyte	Default_primary_ammo_level[MAX_PRIMARY_WEAPONS] = {255, 0, 255, 255, 255};
//ubyte	Default_secondary_ammo_level[MAX_SECONDARY_WEAPONS] = {3, 0, 0, 0, 0};

//	Convert primary weapons to indices in Weapon_info array.
#if defined(DXX_BUILD_DESCENT_I)
const ubyte Primary_weapon_to_weapon_info[MAX_PRIMARY_WEAPONS] = {0, VULCAN_ID, 12, PLASMA_ID, FUSION_ID};
const ubyte Secondary_weapon_to_weapon_info[MAX_SECONDARY_WEAPONS] = {CONCUSSION_ID, HOMING_ID, PROXIMITY_ID, SMART_ID, MEGA_ID};

//for each Secondary weapon, which gun it fires out of
const ubyte Secondary_weapon_to_gun_num[MAX_SECONDARY_WEAPONS] = {4,4,7,7,7};
#elif defined(DXX_BUILD_DESCENT_II)
const ubyte Primary_weapon_to_weapon_info[MAX_PRIMARY_WEAPONS] = {LASER_ID, VULCAN_ID, SPREADFIRE_ID, PLASMA_ID, FUSION_ID, SUPER_LASER_ID, GAUSS_ID, HELIX_ID, PHOENIX_ID, OMEGA_ID};
const ubyte Secondary_weapon_to_weapon_info[MAX_SECONDARY_WEAPONS] = {CONCUSSION_ID, HOMING_ID, PROXIMITY_ID, SMART_ID, MEGA_ID, FLASH_ID, GUIDEDMISS_ID, SUPERPROX_ID, MERCURY_ID, EARTHSHAKER_ID};

//for each Secondary weapon, which gun it fires out of
const ubyte Secondary_weapon_to_gun_num[MAX_SECONDARY_WEAPONS] = {4,4,7,7,7,4,4,7,4,7};
#endif

const ubyte Secondary_ammo_max[MAX_SECONDARY_WEAPONS] = {20, 10, 10, 5, 5,
#if defined(DXX_BUILD_DESCENT_II)
	20, 20, 15, 10, 10
#endif
};

//for each primary weapon, what kind of powerup gives weapon
const ubyte Primary_weapon_to_powerup[MAX_PRIMARY_WEAPONS] = {POW_LASER,POW_VULCAN_WEAPON,POW_SPREADFIRE_WEAPON,POW_PLASMA_WEAPON,POW_FUSION_WEAPON,
#if defined(DXX_BUILD_DESCENT_II)
	POW_LASER,POW_GAUSS_WEAPON,POW_HELIX_WEAPON,POW_PHOENIX_WEAPON,POW_OMEGA_WEAPON
#endif
};

//for each Secondary weapon, what kind of powerup gives weapon
const ubyte Secondary_weapon_to_powerup[MAX_SECONDARY_WEAPONS] = {POW_MISSILE_1,POW_HOMING_AMMO_1,POW_PROXIMITY_WEAPON,POW_SMARTBOMB_WEAPON,POW_MEGA_WEAPON,
#if defined(DXX_BUILD_DESCENT_II)
	POW_SMISSILE1_1,POW_GUIDED_MISSILE_1,POW_SMART_MINE,POW_MERCURY_MISSILE_1,POW_EARTHSHAKER_MISSILE
#endif
};

weapon_info_array Weapon_info;
unsigned N_weapon_types;
sbyte   Primary_weapon, Secondary_weapon;

// autoselect ordering

#if defined(DXX_BUILD_DESCENT_I)
static const array<ubyte, MAX_PRIMARY_WEAPONS + 1> DefaultPrimaryOrder{{ 4, 3, 2, 1, 0, 255 }};
static const array<ubyte, MAX_SECONDARY_WEAPONS + 1> DefaultSecondaryOrder{{ 4, 3, 1, 0, 255, 2 }};
#elif defined(DXX_BUILD_DESCENT_II)
static const array<ubyte, MAX_PRIMARY_WEAPONS + 1> DefaultPrimaryOrder={{9,8,7,6,5,4,3,2,1,0,255}};
static const array<ubyte, MAX_SECONDARY_WEAPONS + 1> DefaultSecondaryOrder={{9,8,4,3,1,5,0,255,7,6,2}};

//flags whether the last time we use this weapon, it was the 'super' version
ubyte Primary_last_was_super[MAX_PRIMARY_WEAPONS];
ubyte Secondary_last_was_super[MAX_SECONDARY_WEAPONS];

const sbyte   Weapon_is_energy[MAX_WEAPON_TYPES] = {
	1, 1, 1, 1, 1,
	1, 1, 1, 0, 1,
	1, 0, 1, 1, 1,
	0, 1, 0, 0, 1,
	1, 0, 0, 1, 1,
	1, 1, 1, 0, 1,
	1, 1, 0, 1, 1,
	1
};
#endif

// ; (0) Laser Level 1
// ; (1) Laser Level 2
// ; (2) Laser Level 3
// ; (3) Laser Level 4
// ; (4) Unknown Use
// ; (5) Josh Blobs
// ; (6) Unknown Use
// ; (7) Unknown Use
// ; (8) ---------- Concussion Missile ----------
// ; (9) ---------- Flare ----------
// ; (10) ---------- Blue laser that blue guy shoots -----------
// ; (11) ---------- Vulcan Cannon ----------
// ; (12) ---------- Spreadfire Cannon ----------
// ; (13) ---------- Plasma Cannon ----------
// ; (14) ---------- Fusion Cannon ----------
// ; (15) ---------- Homing Missile ----------
// ; (16) ---------- Proximity Bomb ----------
// ; (17) ---------- Smart Missile ----------
// ; (18) ---------- Mega Missile ----------
// ; (19) ---------- Children of the PLAYER'S Smart Missile ----------
// ; (20) ---------- Bad Guy Spreadfire Laser ----------
// ; (21) ---------- SuperMech Homing Missile ----------
// ; (22) ---------- Regular Mech's missile -----------
// ; (23) ---------- Silent Spreadfire Laser ----------
// ; (24) ---------- Red laser that baby spiders shoot -----------
// ; (25) ---------- Green laser that rifleman shoots -----------
// ; (26) ---------- Plasma gun that 'plasguy' fires ------------
// ; (27) ---------- Blobs fired by Red Spiders -----------
// ; (28) ---------- Final Boss's Mega Missile ----------
// ; (29) ---------- Children of the ROBOT'S Smart Missile ----------
// ; (30) Laser Level 5
// ; (31) Laser Level 6
// ; (32) ---------- Super Vulcan Cannon ----------
// ; (33) ---------- Super Spreadfire Cannon ----------
// ; (34) ---------- Super Plasma Cannon ----------
// ; (35) ---------- Super Fusion Cannon ----------

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
#if defined(DXX_BUILD_DESCENT_I)
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
#endif
		weapon_index = Primary_weapon_to_weapon_info[weapon_num];

		if (Players[Player_num].primary_weapon_flags & HAS_PRIMARY_FLAG(weapon_num))
			return_value |= HAS_WEAPON_FLAG;

		// Special case: Gauss cannon uses vulcan ammo.
		if (weapon_index_uses_vulcan_ammo(weapon_num)) {
			if (Weapon_info[weapon_index].ammo_usage <= Players[Player_num].vulcan_ammo)
				return_value |= HAS_AMMO_FLAG;
		}
		/* Hack to work around check in do_weapon_select */
		else
			return_value |= HAS_AMMO_FLAG;

#if defined(DXX_BUILD_DESCENT_I)
		//added on 1/21/99 by Victor Rachels... yet another hack
		//fusion has 0 energy usage, HAS_ENERGY_FLAG was always true
		if(weapon_num==FUSION_INDEX)
		{
			if(Players[Player_num].energy >= F1_0*2)
				return_value |= HAS_ENERGY_FLAG;
		}
#elif defined(DXX_BUILD_DESCENT_II)
		if (weapon_num == OMEGA_INDEX) {	// Hack: Make sure player has energy to omega
			if (Players[Player_num].energy || Omega_charge)
				return_value |= HAS_ENERGY_FLAG;
		}
#endif
		else
			if (Weapon_info[weapon_index].energy_usage <= Players[Player_num].energy)
				return_value |= HAS_ENERGY_FLAG;

	} else {
		weapon_index = Secondary_weapon_to_weapon_info[weapon_num];

		if (Players[Player_num].secondary_weapon_flags & HAS_SECONDARY_FLAG(weapon_num))
			return_value |= HAS_WEAPON_FLAG;

		if (Weapon_info[weapon_index].ammo_usage <= Players[Player_num].secondary_ammo[weapon_num])
			return_value |= HAS_AMMO_FLAG;

		if (Weapon_info[weapon_index].energy_usage <= Players[Player_num].energy)
			return_value |= HAS_ENERGY_FLAG;
	}

	return return_value;
}

void InitWeaponOrdering ()
 {
  // short routine to setup default weapon priorities for new pilots
	PlayerCfg.PrimaryOrder = DefaultPrimaryOrder;
	PlayerCfg.SecondaryOrder = DefaultSecondaryOrder;
 }

void CyclePrimary ()
{
	int cur_order_slot, desired_weapon = Primary_weapon, loop=0;
	const int autoselect_order_slot = POrderList(255);
	
#if defined(DXX_BUILD_DESCENT_II)
	// some remapping for SUPER LASER which is not an actual weapon type at all
	if (Primary_weapon == LASER_INDEX && Players[Player_num].laser_level > MAX_LASER_LEVEL)
		cur_order_slot = POrderList(SUPER_LASER_INDEX);
	else
#endif
		cur_order_slot = POrderList(Primary_weapon);
	const int use_restricted_autoselect = (cur_order_slot < autoselect_order_slot) && (1 < autoselect_order_slot) && (PlayerCfg.CycleAutoselectOnly);

	while (loop<(MAX_PRIMARY_WEAPONS+1))
	{
		loop++;
		cur_order_slot++; // next slot
		if (cur_order_slot >= MAX_PRIMARY_WEAPONS+1) // loop if necessary
			cur_order_slot = 0;
		if (cur_order_slot == autoselect_order_slot) // what to to with non-autoselect weapons?
		{
			if (use_restricted_autoselect)
			{
				cur_order_slot = 0; // loop over or ...
			}
			else
			{
				continue; // continue?
			}
		}
		desired_weapon = PlayerCfg.PrimaryOrder[cur_order_slot]; // now that is the weapon next to our current one
#if defined(DXX_BUILD_DESCENT_II)
		// some remapping for SUPER LASER which is not an actual weapon type at all
		if (desired_weapon == LASER_INDEX && Players[Player_num].laser_level > MAX_LASER_LEVEL)
			continue;
		if (desired_weapon == SUPER_LASER_INDEX)
		{
			if (Players[Player_num].laser_level <= MAX_LASER_LEVEL)
				continue;
			else
				desired_weapon = LASER_INDEX;
		}
#endif
		// select the weapon if we have it
		if (player_has_weapon(desired_weapon, 0) == HAS_ALL)
		{
			select_weapon(desired_weapon, 0, 1, 1);
			return;
		}
	}
}

void CycleSecondary ()
{
	int cur_order_slot = SOrderList(Secondary_weapon), desired_weapon = Secondary_weapon, loop=0;
	const int autoselect_order_slot = SOrderList(255);
	const int use_restricted_autoselect = (cur_order_slot < autoselect_order_slot) && (1 < autoselect_order_slot) && (PlayerCfg.CycleAutoselectOnly);
	
	while (loop<(MAX_SECONDARY_WEAPONS+1))
	{
		loop++;
		cur_order_slot++; // next slot
		if (cur_order_slot >= MAX_SECONDARY_WEAPONS+1) // loop if necessary
			cur_order_slot = 0;
		if (cur_order_slot == autoselect_order_slot) // what to to with non-autoselect weapons?
		{
			if (use_restricted_autoselect)
			{
				cur_order_slot = 0; // loop over or ...
			}
			else
			{
				continue; // continue?
			}
		}
		desired_weapon = PlayerCfg.SecondaryOrder[cur_order_slot]; // now that is the weapon next to our current one
		// select the weapon if we have it
		if (player_has_weapon(desired_weapon, 1) == HAS_ALL)
		{
			select_weapon(desired_weapon, 1, 1, 1);
			return;
		}
	}
}


//	------------------------------------------------------------------------------------
//if message flag set, print message saying selected
void select_weapon(int weapon_num, int secondary_flag, int print_message, int wait_for_rearm)
{
	const char	*weapon_name;

	if (Newdemo_state==ND_STATE_RECORDING )
		newdemo_record_player_weapon(secondary_flag, weapon_num);

	if (!secondary_flag) {
		if (Primary_weapon != weapon_num) {
#ifndef FUSION_KEEPS_CHARGE
			//added 8/6/98 by Victor Rachels to fix fusion charge bug
                        Fusion_charge=0;
			//end edit - Victor Rachels
#endif
			if (wait_for_rearm) digi_play_sample_once( SOUND_GOOD_SELECTION_PRIMARY, F1_0 );
			if (Game_mode & GM_MULTI)	{
				if (wait_for_rearm) multi_send_play_sound(SOUND_GOOD_SELECTION_PRIMARY, F1_0);
			}
			if (wait_for_rearm)
				Next_laser_fire_time = GameTime64 + REARM_TIME;
			else
				Next_laser_fire_time = 0;
			Global_laser_firing_count = 0;
		} else 	{
#if defined(DXX_BUILD_DESCENT_I)
			if (wait_for_rearm) digi_play_sample( SOUND_ALREADY_SELECTED, F1_0 );
#endif
		}
		Primary_weapon = weapon_num;
		weapon_name = PRIMARY_WEAPON_NAMES(weapon_num);

#if defined(DXX_BUILD_DESCENT_II)
		//save flag for whether was super version
		Primary_last_was_super[weapon_num % SUPER_WEAPON] = (weapon_num >= SUPER_WEAPON);
#endif

	} else {

		if (Secondary_weapon != weapon_num) {
			if (wait_for_rearm) digi_play_sample_once( SOUND_GOOD_SELECTION_SECONDARY, F1_0 );
			if (Game_mode & GM_MULTI)	{
				if (wait_for_rearm) multi_send_play_sound(SOUND_GOOD_SELECTION_SECONDARY, F1_0);
			}
			if (wait_for_rearm)
				Next_missile_fire_time = GameTime64 + REARM_TIME;
			else
				Next_missile_fire_time = 0;
			Global_missile_firing_count = 0;
		} else	{
			if (wait_for_rearm)
			{
				digi_play_sample_once( SOUND_ALREADY_SELECTED, F1_0 );
			}

		}
		Secondary_weapon = weapon_num;
		weapon_name = SECONDARY_WEAPON_NAMES(weapon_num);

#if defined(DXX_BUILD_DESCENT_II)
		//save flag for whether was super version
		Secondary_last_was_super[weapon_num % SUPER_WEAPON] = (weapon_num >= SUPER_WEAPON);
#endif
	}

	if (print_message)
	{
#if defined(DXX_BUILD_DESCENT_II)
		if (weapon_num == LASER_INDEX && !secondary_flag)
			HUD_init_message(HM_DEFAULT, "%s Level %d %s", weapon_name, Players[Player_num].laser_level+1, TXT_SELECTED);
		else
#endif
			HUD_init_message(HM_DEFAULT, "%s %s", weapon_name, TXT_SELECTED);
	}

}

//	------------------------------------------------------------------------------------
//	Select a weapon, primary or secondary.
void do_weapon_select(int weapon_num, int secondary_flag)
{
#if defined(DXX_BUILD_DESCENT_I)
        //added on 10/9/98 by Victor Rachels to add laser cycle
	int	oweapon = weapon_num;
        //end this section addition - Victor Rachels
	int	weapon_status = player_has_weapon(weapon_num, secondary_flag);
	const char	*weapon_name;


	// do special hud msg. for picking registered weapon in shareware version.
	if (PCSharePig)
		if (weapon_num >= NUM_SHAREWARE_WEAPONS) {
			weapon_name = secondary_flag?SECONDARY_WEAPON_NAMES(weapon_num):PRIMARY_WEAPON_NAMES(weapon_num);
			HUD_init_message(HM_DEFAULT, "%s %s!", weapon_name,TXT_NOT_IN_SHAREWARE);
			digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
			return;
		}

	if (!secondary_flag) {

		if (weapon_num >= MAX_PRIMARY_WEAPONS)
			weapon_num = 0;

		weapon_name = PRIMARY_WEAPON_NAMES(weapon_num);
		if ((weapon_status & HAS_WEAPON_FLAG) == 0) {
			HUD_init_message(HM_DEFAULT, "%s %s!", TXT_DONT_HAVE, weapon_name);
			digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
			return;
		}
		else if ((weapon_status & HAS_AMMO_FLAG) == 0) {
			HUD_init_message(HM_DEFAULT, "%s %s!", TXT_DONT_HAVE_AMMO, weapon_name);
			digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
			return;
		}
	}
	else {
		weapon_name = SECONDARY_WEAPON_NAMES(weapon_num);
		if (weapon_status != HAS_ALL) {
			HUD_init_message(HM_DEFAULT, "%s %s%s",TXT_HAVE_NO, weapon_name, TXT_SX);
			digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
			return;
		}
	}

	weapon_num=oweapon;
#elif defined(DXX_BUILD_DESCENT_II)
	int	weapon_num_save=weapon_num;
	int	weapon_status,current,has_flag;
	ubyte	last_was_super;

	if (!secondary_flag) {
		current = Primary_weapon;
		last_was_super = Primary_last_was_super[weapon_num];
		has_flag = HAS_WEAPON_FLAG;
	}
	else {
		current = Secondary_weapon;
		last_was_super = Secondary_last_was_super[weapon_num];
		has_flag = HAS_WEAPON_FLAG+HAS_AMMO_FLAG;
	}

	if (current == weapon_num || current == weapon_num+SUPER_WEAPON) {

		//already have this selected, so toggle to other of normal/super version

		weapon_num += weapon_num+SUPER_WEAPON - current;
		weapon_status = player_has_weapon(weapon_num, secondary_flag);
	}
	else {

		//go to last-select version of requested missile

		if (last_was_super)
			weapon_num += SUPER_WEAPON;

		weapon_status = player_has_weapon(weapon_num, secondary_flag);

		//if don't have last-selected, try other version

		if ((weapon_status & has_flag) != has_flag) {
			weapon_num = 2*weapon_num_save+SUPER_WEAPON - weapon_num;
			weapon_status = player_has_weapon(weapon_num, secondary_flag);
			if ((weapon_status & has_flag) != has_flag)
				weapon_num = 2*weapon_num_save+SUPER_WEAPON - weapon_num;
		}
	}

	//if we don't have the weapon we're switching to, give error & bail
	if ((weapon_status & has_flag) != has_flag) {
		if (!secondary_flag) {
			if (weapon_num==SUPER_LASER_INDEX)
				return; 		//no such thing as super laser, so no error
			HUD_init_message(HM_DEFAULT, "%s %s!", TXT_DONT_HAVE, PRIMARY_WEAPON_NAMES(weapon_num));
		}
		else
			HUD_init_message(HM_DEFAULT, "%s %s%s",TXT_HAVE_NO, SECONDARY_WEAPON_NAMES(weapon_num), TXT_SX);
		digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
		return;
	}

	//now actually select the weapon
#endif
	select_weapon(weapon_num, secondary_flag, 1, 1);
}

//	----------------------------------------------------------------------------------------
//	Automatically select next best weapon if unable to fire current weapon.
// Weapon type: 0==primary, 1==secondary
void auto_select_weapon(int weapon_type)
{
	int	r;
	int cutpoint;
	int looped=0;

	if (weapon_type==0) {
		r = player_has_weapon(Primary_weapon, 0);
		if (r != HAS_ALL) {
			int	cur_weapon;
			int	try_again = 1;

			cur_weapon = POrderList(Primary_weapon);
			cutpoint = POrderList (255);

			while (try_again) {
				cur_weapon++;

				if (cur_weapon>=cutpoint)
				{
					if (looped)
					{
						HUD_init_message_literal(HM_DEFAULT, TXT_NO_PRIMARY);
						select_weapon(0, 0, 0, 1);
						try_again = 0;
						continue;
					}
					cur_weapon=0;
					looped=1;
				}


				if (cur_weapon==MAX_PRIMARY_WEAPONS)
					cur_weapon = 0;

				//	Hack alert!  Because the fusion uses 0 energy at the end (it's got the weird chargeup)
				//	it looks like it takes 0 to fire, but it doesn't, so never auto-select.
				// if (PlayerCfg.PrimaryOrder[cur_weapon] == FUSION_INDEX)
				//	continue;

				if (PlayerCfg.PrimaryOrder[cur_weapon] == Primary_weapon) {
					HUD_init_message_literal(HM_DEFAULT, TXT_NO_PRIMARY);
					select_weapon(0, 0, 0, 1);
					try_again = 0;			// Tried all weapons!

				} else if (PlayerCfg.PrimaryOrder[cur_weapon]!=255 && player_has_weapon(PlayerCfg.PrimaryOrder[cur_weapon], 0) == HAS_ALL) {
					select_weapon(PlayerCfg.PrimaryOrder[cur_weapon], 0, 1, 1 );
					try_again = 0;
				}
			}
		}

	} else {

		Assert(weapon_type==1);
		r = player_has_weapon(Secondary_weapon, 1);
		if (r != HAS_ALL) {
			int	cur_weapon;
			int	try_again = 1;

			cur_weapon = SOrderList(Secondary_weapon);
			cutpoint = SOrderList (255);


			while (try_again) {
				cur_weapon++;

				if (cur_weapon>=cutpoint)
				{
					if (looped)
					{
						HUD_init_message_literal(HM_DEFAULT, "No secondary weapons selected!");
						try_again = 0;
						continue;
					}
					cur_weapon=0;
					looped=1;
				}

				if (cur_weapon==MAX_SECONDARY_WEAPONS)
					cur_weapon = 0;

				if (PlayerCfg.SecondaryOrder[cur_weapon] == Secondary_weapon) {
					HUD_init_message_literal(HM_DEFAULT, "No secondary weapons available!");
					try_again = 0;				// Tried all weapons!
				} else if (player_has_weapon(PlayerCfg.SecondaryOrder[cur_weapon], 1) == HAS_ALL) {
					select_weapon(PlayerCfg.SecondaryOrder[cur_weapon], 1, 1, 1 );
					try_again = 0;
				}
			}
		}


	}

}

//	---------------------------------------------------------------------
//called when one of these weapons is picked up
//when you pick up a secondary, you always get the weapon & ammo for it
//	Returns true if powerup picked up, else returns false.
int pick_up_secondary(int weapon_index,int count)
{
	int max;
	int	num_picked_up;
	int cutpoint;

	max = Secondary_ammo_max[weapon_index];

#if defined(DXX_BUILD_DESCENT_II)
	if (Players[Player_num].flags & PLAYER_FLAGS_AMMO_RACK)
		max *= 2;
#endif

	if (Players[Player_num].secondary_ammo[weapon_index] >= max) {
		HUD_init_message(HM_DEFAULT|HM_REDUNDANT|HM_MAYDUPL, "%s %i %ss!", TXT_ALREADY_HAVE, Players[Player_num].secondary_ammo[weapon_index],SECONDARY_WEAPON_NAMES(weapon_index));
		return 0;
	}

	Players[Player_num].secondary_weapon_flags |= (1<<weapon_index);
	Players[Player_num].secondary_ammo[weapon_index] += count;

	num_picked_up = count;
	if (Players[Player_num].secondary_ammo[weapon_index] > max) {
		num_picked_up = count - (Players[Player_num].secondary_ammo[weapon_index] - max);
		Players[Player_num].secondary_ammo[weapon_index] = max;
	}

	if (Players[Player_num].secondary_ammo[weapon_index] == count)	// only autoselect if player didn't have any
	{
		cutpoint=SOrderList (255);
		if (((Controls.state.fire_secondary && PlayerCfg.NoFireAutoselect)?0:1) && SOrderList (weapon_index) < cutpoint && ((SOrderList (weapon_index) < SOrderList(Secondary_weapon)) || (Players[Player_num].secondary_ammo[Secondary_weapon] == 0))   )
			select_weapon(weapon_index,1, 0, 1);
		else {
#if defined(DXX_BUILD_DESCENT_II)
			//if we don't auto-select this weapon, but it's a proxbomb or smart mine,
			//we want to do a mini-auto-selection that applies to the drop bomb key

			if (weapon_index_is_player_bomb(weapon_index) &&
					!weapon_index_is_player_bomb(Secondary_weapon)) {
				int cur;

				cur = Secondary_last_was_super[PROXIMITY_INDEX]?SMART_MINE_INDEX:PROXIMITY_INDEX;

				if (SOrderList (weapon_index) < SOrderList(cur))
					Secondary_last_was_super[PROXIMITY_INDEX] = (weapon_index == SMART_MINE_INDEX);
			}
#endif
		}
	}

	//note: flash for all but concussion was 7,14,21
	if (num_picked_up>1) {
		PALETTE_FLASH_ADD(15,15,15);
		HUD_init_message(HM_DEFAULT, "%d %s%s",num_picked_up,SECONDARY_WEAPON_NAMES(weapon_index), TXT_SX);
	}
	else {
		PALETTE_FLASH_ADD(10,10,10);
		HUD_init_message(HM_DEFAULT, "%s!",SECONDARY_WEAPON_NAMES(weapon_index));
	}

	return 1;
}

#define DXX_WEAPON_TEXT_NEVER_AUTOSELECT	"--- Never Autoselect below ---"
void ReorderPrimary ()
{
	newmenu_item m[MAX_PRIMARY_WEAPONS+1];
	int i;

	for (i=0;i<MAX_PRIMARY_WEAPONS+1;i++)
	{
		ubyte order = PlayerCfg.PrimaryOrder[i];
		nm_set_item_menu(m[i], (order==255) ? DXX_WEAPON_TEXT_NEVER_AUTOSELECT : PRIMARY_WEAPON_NAMES(order));
		m[i].value=order;
	}
	newmenu_doreorder("Reorder Primary","Shift+Up/Down arrow to move item", i, m);

	for (i=0;i<MAX_PRIMARY_WEAPONS+1;i++)
		PlayerCfg.PrimaryOrder[i]=m[i].value;
}

void ReorderSecondary ()
{
	newmenu_item m[MAX_SECONDARY_WEAPONS+1];
	int i;

	for (i=0;i<MAX_SECONDARY_WEAPONS+1;i++)
	{
		ubyte order = PlayerCfg.SecondaryOrder[i];
		nm_set_item_menu(m[i], (order==255) ? DXX_WEAPON_TEXT_NEVER_AUTOSELECT : SECONDARY_WEAPON_NAMES(order));
		m[i].value=order;
	}
	newmenu_doreorder("Reorder Secondary","Shift+Up/Down arrow to move item", i, m);
	for (i=0;i<MAX_SECONDARY_WEAPONS+1;i++)
		PlayerCfg.SecondaryOrder[i]=m[i].value;
}

int POrderList (int num)
{
	int i;

	for (i=0;i<MAX_PRIMARY_WEAPONS+1;i++)
	if (PlayerCfg.PrimaryOrder[i]==num)
	{
		return (i);
	}
	throw std::runtime_error("primary weapon list corrupt");
}

int SOrderList (int num)
{
	int i;

	for (i=0;i<MAX_SECONDARY_WEAPONS+1;i++)
		if (PlayerCfg.SecondaryOrder[i]==num)
		{
			return (i);
		}
	throw std::runtime_error("secondary weapon list corrupt");
}


//called when a primary weapon is picked up
//returns true if actually picked up
int pick_up_primary(int weapon_index)
{
	//ushort old_flags = Players[Player_num].primary_weapon_flags;
	ushort flag = HAS_PRIMARY_FLAG(weapon_index);
	int cutpoint, supposed_weapon=Primary_weapon;

	if (weapon_index!=LASER_INDEX && Players[Player_num].primary_weapon_flags & flag) {		//already have
		HUD_init_message(HM_DEFAULT|HM_REDUNDANT|HM_MAYDUPL, "%s %s!", TXT_ALREADY_HAVE_THE, PRIMARY_WEAPON_NAMES(weapon_index));
		return 0;
	}

	Players[Player_num].primary_weapon_flags |= flag;

	cutpoint=POrderList (255);

#if defined(DXX_BUILD_DESCENT_II)
	if (Primary_weapon==LASER_INDEX && Players[Player_num].laser_level>=4)
		supposed_weapon=SUPER_LASER_INDEX;  // allotment for stupid way of doing super laser
#endif

	if (((Controls.state.fire_primary && PlayerCfg.NoFireAutoselect)?0:1) && POrderList(weapon_index) < cutpoint && POrderList(weapon_index)<POrderList(supposed_weapon))
		select_weapon(weapon_index,0,0,1);

	PALETTE_FLASH_ADD(7,14,21);

   if (weapon_index!=LASER_INDEX)
   	HUD_init_message(HM_DEFAULT, "%s!",PRIMARY_WEAPON_NAMES(weapon_index));

	return 1;
}

#if defined(DXX_BUILD_DESCENT_II)
void check_to_use_primary(int weapon_index)
{
	ushort old_flags = Players[Player_num].primary_weapon_flags;
	ushort flag = HAS_PRIMARY_FLAG(weapon_index);
	int cutpoint;

	cutpoint=POrderList (255);

	if (!(old_flags & flag) && POrderList(weapon_index)<cutpoint && POrderList(weapon_index)<POrderList(Primary_weapon))
	{
		if (weapon_index==SUPER_LASER_INDEX)
			select_weapon(LASER_INDEX,0,0,1);
		else
			select_weapon(weapon_index,0,0,1);
	}

	PALETTE_FLASH_ADD(7,14,21);
}
#endif

//called when ammo (for the vulcan cannon) is picked up
//	Returns the amount picked up
int pick_up_ammo(int class_flag,int weapon_index,int ammo_count)
{
	int max,cutpoint,supposed_weapon=Primary_weapon;
	int old_ammo=class_flag;		//kill warning

	Assert(class_flag==CLASS_PRIMARY && weapon_index==VULCAN_INDEX);

	max = VULCAN_AMMO_MAX;
#if defined(DXX_BUILD_DESCENT_II)
	if (Players[Player_num].flags & PLAYER_FLAGS_AMMO_RACK)
		max *= 2;
#endif

	if (Players[Player_num].vulcan_ammo == max)
		return 0;

	old_ammo = Players[Player_num].vulcan_ammo;

	Players[Player_num].vulcan_ammo += ammo_count;

	if (Players[Player_num].vulcan_ammo > max) {
		ammo_count += (max - Players[Player_num].vulcan_ammo);
		Players[Player_num].vulcan_ammo = max;
	}
	cutpoint=POrderList (255);

#if defined(DXX_BUILD_DESCENT_II)
	if (Primary_weapon==LASER_INDEX && Players[Player_num].laser_level>=4)
		supposed_weapon=SUPER_LASER_INDEX;  // allotment for stupid way of doing super laser
#endif


	if (((Controls.state.fire_primary && PlayerCfg.NoFireAutoselect)?0:1) && Players[Player_num].primary_weapon_flags&HAS_PRIMARY_FLAG(weapon_index) && weapon_index>Primary_weapon && old_ammo==0 &&
		POrderList(weapon_index)<cutpoint && POrderList(weapon_index)<POrderList(supposed_weapon))
		select_weapon(weapon_index,0,0,1);

	return ammo_count;	//return amount used
}

#if defined(DXX_BUILD_DESCENT_II)
#define	SMEGA_SHAKE_TIME		(F1_0*2)
#define	MAX_SMEGA_DETONATES	4
fix64	Smega_detonate_times[MAX_SMEGA_DETONATES];

//	Call this to initialize for a new level.
//	Sets all super mega missile detonation times to 0 which means there aren't any.
void init_smega_detonates(void)
{
	int	i;

	for (i=0; i<MAX_SMEGA_DETONATES; i++)
		Smega_detonate_times[i] = 0;
}

fix	Seismic_tremor_magnitude;
fix64	Next_seismic_sound_time;
int	Seismic_sound_playing = 0;
int	Seismic_tremor_volume;

int	Seismic_sound = SOUND_SEISMIC_DISTURBANCE_START;

//	If a smega missile been detonated, rock the mine!
//	This should be called every frame.
//	Maybe this should affect all robots, being called when they get their physics done.
void rock_the_mine_frame(void)
{
	int	i;

	for (i=0; i<MAX_SMEGA_DETONATES; i++) {

		if (Smega_detonate_times[i] != 0) {
			fix	delta_time = GameTime64 - Smega_detonate_times[i];

			if (!Seismic_sound_playing) {
				digi_play_sample_looping(Seismic_sound, F1_0, -1, -1);
				Seismic_sound_playing = 1;
				Next_seismic_sound_time = GameTime64 + d_rand()/2;
			}

			if (delta_time < SMEGA_SHAKE_TIME) {

				//	Control center destroyed, rock the player's ship.
				int	fc, rx, rz;
				// -- fc = abs(delta_time - SMEGA_SHAKE_TIME/2);
				//	Changed 10/23/95 to make decreasing for super mega missile.
				fc = (SMEGA_SHAKE_TIME - delta_time)/2;
				fc /= SMEGA_SHAKE_TIME/32;
				if (fc > 16)
					fc = 16;

				if (fc == 0)
					fc = 1;

				Seismic_tremor_volume += fc;

				if (d_tick_step)
				{
					rx = fixmul(d_rand() - 16384, 3*F1_0/16 + (F1_0*(16-fc))/32);
					rz = fixmul(d_rand() - 16384, 3*F1_0/16 + (F1_0*(16-fc))/32);

					ConsoleObject->mtype.phys_info.rotvel.x += rx;
					ConsoleObject->mtype.phys_info.rotvel.z += rz;

					//	Shake the buddy!
					if (Buddy_objnum != object_none) {
						Objects[Buddy_objnum].mtype.phys_info.rotvel.x += rx*4;
						Objects[Buddy_objnum].mtype.phys_info.rotvel.z += rz*4;
					}
					//	Shake a guided missile!
					Seismic_tremor_magnitude += rx;
				}

			} else
				Smega_detonate_times[i] = 0;

		}
	}

	//	Hook in the rumble sound effect here.
}

#define	SEISMIC_DISTURBANCE_DURATION	(F1_0*5)
fix64	Seismic_disturbance_start_time = 0, Seismic_disturbance_end_time;

int Seismic_level=0;

void init_seismic_disturbances(void)
{
	Seismic_disturbance_start_time = 0;
	Seismic_disturbance_end_time = 0;
}

//	Return true if time to start a seismic disturbance.
static int start_seismic_disturbance(void)
{
	int	rval;

	if (Level_shake_duration < 1)
		return 0;

	rval =  (2 * fixmul(d_rand(), Level_shake_frequency)) < FrameTime;

	if (rval) {
		Seismic_disturbance_start_time = GameTime64;
		Seismic_disturbance_end_time = GameTime64 + Level_shake_duration;
		if (!Seismic_sound_playing) {
			digi_play_sample_looping(Seismic_sound, F1_0, -1, -1);
			Seismic_sound_playing = 1;
			Next_seismic_sound_time = GameTime64 + d_rand()/2;
		}

		if (Game_mode & GM_MULTI)
			multi_send_seismic (Seismic_disturbance_start_time,Seismic_disturbance_end_time);
	}

	return rval;
}

static void seismic_disturbance_frame(void)
{
	if (Level_shake_frequency) {
		if (((Seismic_disturbance_start_time < GameTime64) && (Seismic_disturbance_end_time > GameTime64)) || start_seismic_disturbance()) {
			fix	delta_time;
			int	fc, rx, rz;

			delta_time = GameTime64 - Seismic_disturbance_start_time;

			fc = abs(delta_time - (Seismic_disturbance_end_time - Seismic_disturbance_start_time)/2);
			fc /= F1_0/16;
			if (fc > 16)
				fc = 16;

			if (fc == 0)
				fc = 1;

			Seismic_tremor_volume += fc;

			if (d_tick_step)
			{
				rx = fixmul(d_rand() - 16384, 3*F1_0/16 + (F1_0*(16-fc))/32);
				rz = fixmul(d_rand() - 16384, 3*F1_0/16 + (F1_0*(16-fc))/32);

				ConsoleObject->mtype.phys_info.rotvel.x += rx;
				ConsoleObject->mtype.phys_info.rotvel.z += rz;

				//	Shake the buddy!
				if (Buddy_objnum != object_none) {
					Objects[Buddy_objnum].mtype.phys_info.rotvel.x += rx*4;
					Objects[Buddy_objnum].mtype.phys_info.rotvel.z += rz*4;
				}
				//	Shake a guided missile!
				Seismic_tremor_magnitude += rx;
			}
		}
	}
}


//	Call this when a smega detonates to start the process of rocking the mine.
void smega_rock_stuff(void)
{
	fix64 *least = &Smega_detonate_times[0];
	range_for (auto &i, Smega_detonate_times)
	{
		if (i + SMEGA_SHAKE_TIME < GameTime64)
			i = 0;
		if (*least > i)
			least = &i;
	}
	*least = GameTime64;
}

int	Super_mines_yes = 1;

//	Call this once/frame to process all super mines in the level.
void process_super_mines_frame(void)
{
	int	i;
	int	start, add;

	//	If we don't know of there being any super mines in the level, just
	//	check every 8th object each frame.
	if (Super_mines_yes == 0) {
		start = d_tick_count & 7;
		add = 8;
	} else {
		start = 0;
		add = 1;
	}

	Super_mines_yes = 0;

	for (i=start; i<=Highest_object_index; i+=add) {
		if ((Objects[i].type == OBJ_WEAPON) && (get_powerup_id(&Objects[i]) == SUPERPROX_ID)) {
			int	parent_num;

			parent_num = Objects[i].ctype.laser_info.parent_num;

			Super_mines_yes = 1;
			if (Objects[i].lifeleft + F1_0*2 < Weapon_info[SUPERPROX_ID].lifetime) {
				const auto &bombpos = Objects[i].pos;

				range_for (auto j, highest_valid(Objects))
				{
					if ((Objects[j].type == OBJ_PLAYER) || (Objects[j].type == OBJ_ROBOT)) {
						if (j == parent_num)
							continue;
						fix	dist;

						dist = vm_vec_dist_quick(bombpos, Objects[j].pos);

							if (dist - Objects[j].size < F1_0*20)
							{
								if (Objects[i].segnum == Objects[j].segnum)
									Objects[i].lifeleft = 1;
								else {
									//	Object which is close enough to detonate smart mine is not in same segment as smart mine.
									//	Need to do a more expensive check to make sure there isn't an obstruction.
									if (((d_tick_count ^ (i+j)) % 4) == 0) {
										fvi_query	fq;
										fvi_info		hit_data;
										int			fate;

										fq.startseg = Objects[i].segnum;
										fq.p0						= &Objects[i].pos;
										fq.p1						= &Objects[j].pos;
										fq.rad					= 0;
										fq.thisobjnum			= i;
										fq.ignore_obj_list	= NULL;
										fq.flags					= 0;

										fate = find_vector_intersection(&fq, &hit_data);
										if (fate != HIT_WALL)
											Objects[i].lifeleft = 1;
									}
								}
							}
					}
				}
			}
		}
	}

}

#define SPIT_SPEED 20

//this function is for when the player intentionally drops a powerup
//this function is based on drop_powerup()
objptridx_t spit_powerup(const vobjptr_t spitter, int id,int seed)
{
	d_srand(seed);

	auto new_velocity = vm_vec_scale_add(spitter->mtype.phys_info.velocity,spitter->orient.fvec,i2f(SPIT_SPEED));

	new_velocity.x += (d_rand() - 16384) * SPIT_SPEED * 2;
	new_velocity.y += (d_rand() - 16384) * SPIT_SPEED * 2;
	new_velocity.z += (d_rand() - 16384) * SPIT_SPEED * 2;

	// Give keys zero velocity so they can be tracked better in multi

	if ((Game_mode & GM_MULTI) && (id >= POW_KEY_BLUE) && (id <= POW_KEY_GOLD))
		vm_vec_zero(new_velocity);

	//there's a piece of code which lets the player pick up a powerup if
	//the distance between him and the powerup is less than 2 time their
	//combined radii.  So we need to create powerups pretty far out from
	//the player.

	const auto new_pos = vm_vec_scale_add(spitter->pos,spitter->orient.fvec,spitter->size);

	if (Game_mode & GM_MULTI)
	{
		if (Net_create_loc >= MAX_NET_CREATE_OBJECTS)
		{
			return object_none;
		}
	}

	auto obj = obj_create( OBJ_POWERUP, id, spitter->segnum, new_pos, &vmd_identity_matrix, Powerup_info[id].size, CT_POWERUP, MT_PHYSICS, RT_POWERUP);

	if (obj == object_none ) {
		Int3();
		return obj;
	}
	obj->mtype.phys_info.velocity = new_velocity;
	obj->mtype.phys_info.drag = 512;	//1024;
	obj->mtype.phys_info.mass = F1_0;

	obj->mtype.phys_info.flags = PF_BOUNCE;

	obj->rtype.vclip_info.vclip_num = Powerup_info[get_powerup_id(obj)].vclip_num;
	obj->rtype.vclip_info.frametime = Vclip[obj->rtype.vclip_info.vclip_num].frame_time;
	obj->rtype.vclip_info.framenum = 0;

	if (spitter == ConsoleObject)
		obj->ctype.powerup_info.flags |= PF_SPAT_BY_PLAYER;

	switch (get_powerup_id(obj)) {
		case POW_MISSILE_1:
		case POW_MISSILE_4:
		case POW_SHIELD_BOOST:
		case POW_ENERGY:
			obj->lifeleft = (d_rand() + F1_0*3) * 64;		//	Lives for 3 to 3.5 binary minutes (a binary minute is 64 seconds)
			if (Game_mode & GM_MULTI)
				obj->lifeleft /= 2;
			break;
		default:
			//if (Game_mode & GM_MULTI)
			//	obj->lifeleft = (d_rand() + F1_0*3) * 64;		//	Lives for 5 to 5.5 binary minutes (a binary minute is 64 seconds)
			break;
	}
	return obj;
}

void DropCurrentWeapon ()
{
	int ammo=0,seed;

	if (num_objects >= MAX_USED_OBJECTS)
		return;

	if (Primary_weapon==0)
	{
		HUD_init_message_literal(HM_DEFAULT, "You cannot drop your base weapon!");
		return;
	}

	HUD_init_message(HM_DEFAULT, "%s dropped!",PRIMARY_WEAPON_NAMES(Primary_weapon));
	digi_play_sample (SOUND_DROP_WEAPON,F1_0);

	seed = d_rand();

	auto objnum = spit_powerup(ConsoleObject,Primary_weapon_to_powerup[Primary_weapon],seed);

	if (objnum == object_none)
		return;

	if (weapon_index_uses_vulcan_ammo(Primary_weapon)) {

		//if it's one of these, drop some ammo with the weapon

		ammo = Players[Player_num].vulcan_ammo;

		if ((Players[Player_num].primary_weapon_flags & HAS_VULCAN_FLAG) && (Players[Player_num].primary_weapon_flags & HAS_GAUSS_FLAG))
			ammo /= 2;		//if both vulcan & gauss, drop half

		Players[Player_num].vulcan_ammo -= ammo;

		if (objnum!=object_none)
			objnum->ctype.powerup_info.count = ammo;
	}

	if (Primary_weapon == OMEGA_INDEX) {

		//dropped weapon has current energy

		if (objnum!=object_none)
			objnum->ctype.powerup_info.count = Omega_charge;
	}

	if ((Game_mode & GM_MULTI) && objnum!=object_none)
		multi_send_drop_weapon(objnum,seed);

	Players[Player_num].primary_weapon_flags &= (~HAS_PRIMARY_FLAG(Primary_weapon));
	auto_select_weapon (0);
}

void DropSecondaryWeapon ()
{
	int seed;
	ubyte weapon_drop_id=-1;
	ushort sub_ammo=0;

	if (num_objects >= MAX_USED_OBJECTS)
		return;

	if (Players[Player_num].secondary_ammo[Secondary_weapon] ==0)
	{
		HUD_init_message_literal(HM_DEFAULT, "No secondary weapon to drop!");
		return;
	}

	weapon_drop_id = Secondary_weapon_to_powerup[Secondary_weapon];

	// see if we drop single or 4-pack
	switch (Secondary_weapon_to_powerup[Secondary_weapon])
	{
		case POW_MISSILE_1:
		case POW_HOMING_AMMO_1:
		case POW_SMISSILE1_1:
		case POW_GUIDED_MISSILE_1:
		case POW_MERCURY_MISSILE_1:
			if (Players[Player_num].secondary_ammo[Secondary_weapon]<4)
			{
				sub_ammo = 1;
			}
			else
			{
				sub_ammo = 4;
				weapon_drop_id++; //4-pack always is next index
			}
			break;
		case POW_PROXIMITY_WEAPON:
		case POW_SMART_MINE:
			if (Players[Player_num].secondary_ammo[Secondary_weapon]<4)
			{
				HUD_init_message_literal(HM_DEFAULT, "You need at least 4 to drop!");
				return;
			}
			else
			{
				sub_ammo = 4;
			}
			break;
		case POW_SMARTBOMB_WEAPON:
		case POW_MEGA_WEAPON:
		case POW_EARTHSHAKER_MISSILE:
			sub_ammo = 1;
			break;
	}

	HUD_init_message(HM_DEFAULT, "%s dropped!",SECONDARY_WEAPON_NAMES(Secondary_weapon));
	digi_play_sample (SOUND_DROP_WEAPON,F1_0);

	seed = d_rand();

	auto objnum = spit_powerup(ConsoleObject,weapon_drop_id,seed);

	if (objnum == object_none)
		return;


	if ((Game_mode & GM_MULTI) && objnum!=object_none)
		multi_send_drop_weapon(objnum,seed);

	Players[Player_num].secondary_ammo[Secondary_weapon]-=sub_ammo;

	if (Players[Player_num].secondary_ammo[Secondary_weapon]==0)
	{
		Players[Player_num].secondary_weapon_flags &= (~(1<<Secondary_weapon));
		auto_select_weapon (1);
	}
}

//	---------------------------------------------------------------------------------------
//	Do seismic disturbance stuff including the looping sounds with changing volume.
void do_seismic_stuff(void)
{
	int	stv_save;

	stv_save = Seismic_tremor_volume;
	Seismic_tremor_magnitude = 0;
	Seismic_tremor_volume = 0;

	rock_the_mine_frame();
	seismic_disturbance_frame();

	if (stv_save != 0) {
		if (Seismic_tremor_volume == 0) {
			digi_stop_looping_sound();
			Seismic_sound_playing = 0;
		}

		if ((GameTime64 > Next_seismic_sound_time) && Seismic_tremor_volume) {
			int	volume;

			volume = Seismic_tremor_volume * 2048;
			if (volume > F1_0)
				volume = F1_0;
			digi_change_looping_volume(volume);
			Next_seismic_sound_time = GameTime64 + d_rand()/4 + 8192;
		}
	}

}
#endif

DEFINE_BITMAP_SERIAL_UDT();

#if defined(DXX_BUILD_DESCENT_I)
DEFINE_SERIAL_UDT_TO_MESSAGE(weapon_info, w, (w.render_type, w.model_num, w.model_num_inner, w.persistent, w.flash_vclip, w.flash_sound, w.robot_hit_vclip, w.robot_hit_sound, w.wall_hit_vclip, w.wall_hit_sound, w.fire_count, w.ammo_usage, w.weapon_vclip, w.destroyable, w.matter, w.bounce, w.homing_flag, w.dum1, w.dum2, w.dum3, w.energy_usage, w.fire_wait, w.bitmap, w.blob_size, w.flash_size, w.impact_size, w.strength, w.speed, w.mass, w.drag, w.thrust, w.po_len_to_width_ratio, w.light, w.lifetime, w.damage_radius, w.picture));
#elif defined(DXX_BUILD_DESCENT_II)
namespace {
struct v2_weapon_info : weapon_info {};
}

template <typename Accessor>
void postprocess_udt(Accessor &, v2_weapon_info &w)
{
	w.children = -1;
	w.multi_damage_scale = F1_0;
	w.hires_picture = w.picture;
}

DEFINE_SERIAL_UDT_TO_MESSAGE(v2_weapon_info, w, (w.render_type, w.persistent, w.model_num, w.model_num_inner, w.flash_vclip, w.robot_hit_vclip, w.flash_sound, w.wall_hit_vclip, w.fire_count, w.robot_hit_sound, w.ammo_usage, w.weapon_vclip, w.wall_hit_sound, w.destroyable, w.matter, w.bounce, w.homing_flag, w.speedvar, w.flags, w.flash, w.afterburner_size, w.energy_usage, w.fire_wait, w.bitmap, w.blob_size, w.flash_size, w.impact_size, w.strength, w.speed, w.mass, w.drag, w.thrust, w.po_len_to_width_ratio, w.light, w.lifetime, w.damage_radius, w.picture));
DEFINE_SERIAL_UDT_TO_MESSAGE(weapon_info, w, (w.render_type, w.persistent, w.model_num, w.model_num_inner, w.flash_vclip, w.robot_hit_vclip, w.flash_sound, w.wall_hit_vclip, w.fire_count, w.robot_hit_sound, w.ammo_usage, w.weapon_vclip, w.wall_hit_sound, w.destroyable, w.matter, w.bounce, w.homing_flag, w.speedvar, w.flags, w.flash, w.afterburner_size, w.children, w.energy_usage, w.fire_wait, w.multi_damage_scale, w.bitmap, w.blob_size, w.flash_size, w.impact_size, w.strength, w.speed, w.mass, w.drag, w.thrust, w.po_len_to_width_ratio, w.light, w.lifetime, w.damage_radius, w.picture, w.hires_picture));
#endif

void weapon_info_write(PHYSFS_File *fp, const weapon_info &w)
{
	PHYSFSX_serialize_write(fp, w);
}

/*
 * reads n weapon_info structs from a PHYSFS_file
 */
void weapon_info_read_n(weapon_info_array &wi, std::size_t count, PHYSFS_File *fp, int file_version, std::size_t offset)
{
	auto r = partial_range(wi, offset, count);
#if defined(DXX_BUILD_DESCENT_II)
	if (file_version < 3)
	{
		range_for (auto &w, r)
			PHYSFSX_serialize_read(fp, static_cast<v2_weapon_info &>(w));
		/* Set the type of children correctly when using old
		 * datafiles.  In earlier descent versions this was simply
		 * hard-coded in create_smart_children().
		 */
		wi[SMART_ID].children = PLAYER_SMART_HOMING_ID;
		wi[SUPERPROX_ID].children = SMART_MINE_HOMING_ID;
		return;
	}
#endif
	range_for (auto &w, r)
	{
		PHYSFSX_serialize_read(fp, w);
	}
}
