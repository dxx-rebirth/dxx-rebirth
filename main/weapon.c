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

#ifdef RCS
static char rcsid[] = "$Id: weapon.c,v 1.5 2002-08-02 04:57:19 btb Exp $";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "game.h"
#include "laser.h"
#include "weapon.h"
#include "mono.h"
#include "player.h"
#include "gauges.h"
#include "error.h"
#include "sounds.h"
#include "text.h"
#include "powerup.h"
#include "fireball.h"
#include "newdemo.h"
#include "multi.h"
#include "newmenu.h"
#include "ai.h"
#include "args.h"

#if defined (TACTILE)
#include "tactile.h"
#endif

int POrderList (int num);
int SOrderList (int num);
//	Note, only Vulcan cannon requires ammo.
// NOTE: Now Vulcan and Gauss require ammo. -5/3/95 Yuan
//ubyte	Default_primary_ammo_level[MAX_PRIMARY_WEAPONS] = {255, 0, 255, 255, 255};
//ubyte	Default_secondary_ammo_level[MAX_SECONDARY_WEAPONS] = {3, 0, 0, 0, 0};

//	Convert primary weapons to indices in Weapon_info array.
ubyte Primary_weapon_to_weapon_info[MAX_PRIMARY_WEAPONS] = {LASER_ID, VULCAN_ID, SPREADFIRE_ID, PLASMA_ID, FUSION_ID, SUPER_LASER_ID, GAUSS_ID, HELIX_ID, PHOENIX_ID, OMEGA_ID};
ubyte Secondary_weapon_to_weapon_info[MAX_SECONDARY_WEAPONS] = {CONCUSSION_ID, HOMING_ID, PROXIMITY_ID, SMART_ID, MEGA_ID, FLASH_ID, GUIDEDMISS_ID, SUPERPROX_ID, MERCURY_ID, EARTHSHAKER_ID};

//for each Secondary weapon, which gun it fires out of
ubyte Secondary_weapon_to_gun_num[MAX_SECONDARY_WEAPONS] = {4,4,7,7,7,4,4,7,4,7};

int Primary_ammo_max[MAX_PRIMARY_WEAPONS] = {0, VULCAN_AMMO_MAX, 0, 0, 0, 0, VULCAN_AMMO_MAX, 0, 0, 0};
ubyte Secondary_ammo_max[MAX_SECONDARY_WEAPONS] = {20, 10, 10, 5, 5, 20, 20, 15, 10, 10};

//for each primary weapon, what kind of powerup gives weapon
ubyte Primary_weapon_to_powerup[MAX_PRIMARY_WEAPONS] = {POW_LASER,POW_VULCAN_WEAPON,POW_SPREADFIRE_WEAPON,POW_PLASMA_WEAPON,POW_FUSION_WEAPON,POW_LASER,POW_GAUSS_WEAPON,POW_HELIX_WEAPON,POW_PHOENIX_WEAPON,POW_OMEGA_WEAPON};

//for each Secondary weapon, what kind of powerup gives weapon
ubyte Secondary_weapon_to_powerup[MAX_SECONDARY_WEAPONS] = {POW_MISSILE_1,POW_HOMING_AMMO_1,POW_PROXIMITY_WEAPON,POW_SMARTBOMB_WEAPON,POW_MEGA_WEAPON,POW_SMISSILE1_1,POW_GUIDED_MISSILE_1,POW_SMART_MINE,POW_MERCURY_MISSILE_1,POW_EARTHSHAKER_MISSILE};

weapon_info Weapon_info[MAX_WEAPON_TYPES];
int	N_weapon_types=0;
byte	Primary_weapon, Secondary_weapon;

// autoselect ordering

ubyte PrimaryOrder[]={9,8,7,6,5,4,3,2,1,0,255};
ubyte SecondaryOrder[]={9,8,4,3,1,5,0,255,7,6,2};

ubyte DefaultPrimaryOrder[]={9,8,7,6,5,4,3,2,1,0,255};
ubyte DefaultSecondaryOrder[]={9,8,4,3,1,5,0,255,7,6,2};

// Cycling weapon key pressed?

ubyte Cycling=0;

//allow player to reorder menus?
extern ubyte MenuReordering;

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

byte	Weapon_is_energy[MAX_WEAPON_TYPES] = {
	1, 1, 1, 1, 1,
	1, 1, 1, 0, 1,
	1, 0, 1, 1, 1,
	0, 1, 0, 0, 1,
	1, 0, 0, 1, 1,
	1, 1, 1, 0, 1,
	1, 1, 0, 1, 1,
	1
};

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
		weapon_index = Primary_weapon_to_weapon_info[weapon_num];

		if (Players[Player_num].primary_weapon_flags & (1 << weapon_num))
			return_value |= HAS_WEAPON_FLAG;

		// Special case: Gauss cannon uses vulcan ammo.		
		if (weapon_num == GAUSS_INDEX) {
			if (Weapon_info[weapon_index].ammo_usage <= Players[Player_num].primary_ammo[VULCAN_INDEX])
				return_value |= HAS_AMMO_FLAG;
		} else
			if (Weapon_info[weapon_index].ammo_usage <= Players[Player_num].primary_ammo[weapon_num])
				return_value |= HAS_AMMO_FLAG;

		if (weapon_num == OMEGA_INDEX) {	// Hack: Make sure player has energy to omega
			if (Players[Player_num].energy || Omega_charge)
				return_value |= HAS_ENERGY_FLAG;
		} else		
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

void InitWeaponOrdering ()
 {
  // short routine to setup default weapon priorities for new pilots

  int i;

  for (i=0;i<MAX_PRIMARY_WEAPONS+1;i++)
	PrimaryOrder[i]=DefaultPrimaryOrder[i];
  for (i=0;i<MAX_SECONDARY_WEAPONS+1;i++)
	SecondaryOrder[i]=DefaultSecondaryOrder[i];
 }	

void CyclePrimary ()
{
	mprintf ((0,"Cycling primary!\n"));
	Cycling=1;
	auto_select_weapon (0);
	Cycling=0;
}

void CycleSecondary ()
{
	mprintf ((0,"Cycling secondary!\n"));
	Cycling=1;
	auto_select_weapon (1);
	Cycling=0;
}


//	------------------------------------------------------------------------------------
//if message flag set, print message saying selected
void select_weapon(int weapon_num, int secondary_flag, int print_message, int wait_for_rearm)
{
	char	*weapon_name;

	if (Newdemo_state==ND_STATE_RECORDING )
		newdemo_record_player_weapon(secondary_flag, weapon_num);

	if (!secondary_flag) {
		if (Primary_weapon != weapon_num) {
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
			// Select super version if available.
			if (wait_for_rearm) 
			{
			 if (!Cycling)
				; // -- MK, only plays when can't fire weapon anyway, fixes bug -- digi_play_sample_once( SOUND_ALREADY_SELECTED, F1_0 );
			 else
				digi_play_sample_once( SOUND_BAD_SELECTION, F1_0 );
			}
		}
		Primary_weapon = weapon_num;
		weapon_name = PRIMARY_WEAPON_NAMES(weapon_num);
      #if defined (TACTILE)
 		tactile_set_button_jolt();
		#endif


		//save flag for whether was super version
		Primary_last_was_super[weapon_num % SUPER_WEAPON] = (weapon_num >= SUPER_WEAPON);

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
			if (wait_for_rearm) 
			{
			 if (!Cycling)
				digi_play_sample_once( SOUND_ALREADY_SELECTED, F1_0 );
			 else
				digi_play_sample_once( SOUND_BAD_SELECTION, F1_0 );
			}
				
		}
		Secondary_weapon = weapon_num;
		weapon_name = SECONDARY_WEAPON_NAMES(weapon_num);

		//save flag for whether was super version
		Secondary_last_was_super[weapon_num % SUPER_WEAPON] = (weapon_num >= SUPER_WEAPON);
	}

	if (print_message)
	{
		if (weapon_num == LASER_INDEX && !secondary_flag)
			HUD_init_message("%s Level %d %s", weapon_name, Players[Player_num].laser_level+1, TXT_SELECTED);
		else
			HUD_init_message("%s %s", weapon_name, TXT_SELECTED);
	}

}

//flags whether the last time we use this weapon, it was the 'super' version
ubyte Primary_last_was_super[MAX_PRIMARY_WEAPONS];
ubyte Secondary_last_was_super[MAX_SECONDARY_WEAPONS];

//	------------------------------------------------------------------------------------
//	Select a weapon, primary or secondary.
void do_weapon_select(int weapon_num, int secondary_flag)
{
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
			HUD_init_message("%s %s!", TXT_DONT_HAVE, PRIMARY_WEAPON_NAMES(weapon_num));
		}
		else 
			HUD_init_message("%s %s%s",TXT_HAVE_NO, SECONDARY_WEAPON_NAMES(weapon_num), TXT_SX);
		digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
		return;
	}

	//now actually select the weapon
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
		if (r != HAS_ALL || Cycling) {
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
						if (!Cycling)
						{
							HUD_init_message(TXT_NO_PRIMARY);
							#ifdef TACTILE
							if (TactileStick)
							 ButtonReflexClear(0);
							#endif
							select_weapon(0, 0, 0, 1);
						}
						else
							select_weapon (Primary_weapon,0,0,1);

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
				// if (PrimaryOrder[cur_weapon] == FUSION_INDEX)
				//	continue;

				if (PrimaryOrder[cur_weapon] == Primary_weapon) {
					if (!Cycling)
					{
						HUD_init_message(TXT_NO_PRIMARY);
						#ifdef TACTILE
						if (TactileStick)
						 ButtonReflexClear(0);
						#endif

						//	if (POrderList(0)<POrderList(255))
						select_weapon(0, 0, 0, 1);
					}
					else
						select_weapon (Primary_weapon,0,0,1);

					try_again = 0;			// Tried all weapons!

				} else if (PrimaryOrder[cur_weapon]!=255 && player_has_weapon(PrimaryOrder[cur_weapon], 0) == HAS_ALL) {
					select_weapon(PrimaryOrder[cur_weapon], 0, 1, 1 );
					try_again = 0;
				}
			}
		}

	} else {

		Assert(weapon_type==1);
		r = player_has_weapon(Secondary_weapon, 1);
		if (r != HAS_ALL || Cycling) {
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
						if (!Cycling)
							HUD_init_message("No secondary weapons selected!");
						else
							select_weapon (Secondary_weapon,1,0,1);
						try_again = 0;
						continue;
					}
					cur_weapon=0;
					looped=1;
				}

				if (cur_weapon==MAX_SECONDARY_WEAPONS)
					cur_weapon = 0;

				if (SecondaryOrder[cur_weapon] == Secondary_weapon) {
					if (!Cycling)
						HUD_init_message("No secondary weapons available!");
					else
						select_weapon (Secondary_weapon,1,0,1);

					try_again = 0;				// Tried all weapons!
				} else if (player_has_weapon(SecondaryOrder[cur_weapon], 1) == HAS_ALL) {
					select_weapon(SecondaryOrder[cur_weapon], 1, 1, 1 );
					try_again = 0;
				}
			}
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

	if (Players[Player_num].flags & PLAYER_FLAGS_AMMO_RACK)
		max *= 2;

	if (Players[Player_num].secondary_ammo[weapon_index] >= max) {
		HUD_init_message("%s %i %ss!", TXT_ALREADY_HAVE, Players[Player_num].secondary_ammo[weapon_index],SECONDARY_WEAPON_NAMES(weapon_index));
		return 0;
	}

	Players[Player_num].secondary_weapon_flags |= (1<<weapon_index);
	Players[Player_num].secondary_ammo[weapon_index] += count;

	num_picked_up = count;
	if (Players[Player_num].secondary_ammo[weapon_index] > max) {
		num_picked_up = count - (Players[Player_num].secondary_ammo[weapon_index] - max);
		Players[Player_num].secondary_ammo[weapon_index] = max;
	}

	cutpoint=SOrderList (255);
	if (SOrderList (weapon_index)<cutpoint && ((SOrderList (weapon_index) < SOrderList(Secondary_weapon)) || (Players[Player_num].secondary_ammo[Secondary_weapon] == 0))   )
		select_weapon(weapon_index,1, 0, 1);
	else {
		//if we don't auto-select this weapon, but it's a proxbomb or smart mine,
		//we want to do a mini-auto-selection that applies to the drop bomb key

		if ((weapon_index == PROXIMITY_INDEX || weapon_index == SMART_MINE_INDEX) &&
				!(Secondary_weapon == PROXIMITY_INDEX || Secondary_weapon == SMART_MINE_INDEX)) {
			int cur;

			cur = Secondary_last_was_super[PROXIMITY_INDEX]?SMART_MINE_INDEX:PROXIMITY_INDEX;

			if (SOrderList (weapon_index) < SOrderList(cur))
				Secondary_last_was_super[PROXIMITY_INDEX] = (weapon_index == SMART_MINE_INDEX);
		}
	}

	//note: flash for all but concussion was 7,14,21
	if (count>1) {
		PALETTE_FLASH_ADD(15,15,15);
		HUD_init_message("%d %s%s",num_picked_up,SECONDARY_WEAPON_NAMES(weapon_index), TXT_SX);
	}
	else {
		PALETTE_FLASH_ADD(10,10,10);
		HUD_init_message("%s!",SECONDARY_WEAPON_NAMES(weapon_index));
	}

	return 1;
}

void ReorderPrimary ()
{
	newmenu_item m[MAX_PRIMARY_WEAPONS+1];
	int i;

	for (i=0;i<MAX_PRIMARY_WEAPONS+1;i++)
	{
		m[i].type=NM_TYPE_MENU;
		if (PrimaryOrder[i]==255)
			m[i].text="ˆˆˆˆˆˆˆ Never autoselect ˆˆˆˆˆˆˆ";
		else
			m[i].text=(char *)PRIMARY_WEAPON_NAMES(PrimaryOrder[i]);
		m[i].value=PrimaryOrder[i];
	}
	MenuReordering=1;
	i = newmenu_do("Reorder Primary","Shift+Up/Down arrow to move item", i, m, NULL);
	MenuReordering=0;
	
	for (i=0;i<MAX_PRIMARY_WEAPONS+1;i++)
		PrimaryOrder[i]=m[i].value;
}

void ReorderSecondary ()
{
	newmenu_item m[MAX_SECONDARY_WEAPONS+1];
	int i;

	for (i=0;i<MAX_SECONDARY_WEAPONS+1;i++)
	{
		m[i].type=NM_TYPE_MENU;
		if (SecondaryOrder[i]==255)
			m[i].text="ˆˆˆˆˆˆˆ Never autoselect ˆˆˆˆˆˆˆ";
		else
			m[i].text=(char *)SECONDARY_WEAPON_NAMES(SecondaryOrder[i]);
		m[i].value=SecondaryOrder[i];
	}
	MenuReordering=1;
	i = newmenu_do("Reorder Secondary","Shift+Up/Down arrow to move item", i, m, NULL);
	MenuReordering=0;
	for (i=0;i<MAX_SECONDARY_WEAPONS+1;i++)
		SecondaryOrder[i]=m[i].value;
}

int POrderList (int num)
{
	int i;

	for (i=0;i<MAX_PRIMARY_WEAPONS+1;i++)
	if (PrimaryOrder[i]==num)
	{
		mprintf ((0,"Primary %d has priority of %d!\n",num,i));
		return (i);
	}
	Error ("Primary Weapon is not in order list!!!");
}

int SOrderList (int num)
{
	int i;

	for (i=0;i<MAX_SECONDARY_WEAPONS+1;i++)
		if (SecondaryOrder[i]==num)
		{
			mprintf ((0,"Secondary %d has priority of %d!\n",num,i));
			return (i);
		}
	mprintf ((0,"Error! Secondary Num=%d\n",num));
	Error ("Secondary Weapon is not in order list!!!");
}


//called when a primary weapon is picked up
//returns true if actually picked up
int pick_up_primary(int weapon_index)
{
	//ushort old_flags = Players[Player_num].primary_weapon_flags;
	ushort flag = 1<<weapon_index;
	int cutpoint;
	int supposed_weapon=Primary_weapon;

	if (weapon_index!=LASER_INDEX && Players[Player_num].primary_weapon_flags & flag) {		//already have
		HUD_init_message("%s %s!", TXT_ALREADY_HAVE_THE, PRIMARY_WEAPON_NAMES(weapon_index));
		return 0;
	}

	Players[Player_num].primary_weapon_flags |= flag;
 
	cutpoint=POrderList (255);

	if (Primary_weapon==LASER_INDEX && Players[Player_num].laser_level>=4)	
		supposed_weapon=SUPER_LASER_INDEX;  // allotment for stupid way of doing super laser


	if (POrderList(weapon_index)<cutpoint && POrderList(weapon_index)<POrderList(supposed_weapon))
		select_weapon(weapon_index,0,0,1);

	PALETTE_FLASH_ADD(7,14,21);
	mprintf ((0,"Weapon index: %d\n",weapon_index));
	
   if (weapon_index!=LASER_INDEX)
   	HUD_init_message("%s!",PRIMARY_WEAPON_NAMES(weapon_index));

	return 1;
}
int check_to_use_primary(int weapon_index)
{
	ushort old_flags = Players[Player_num].primary_weapon_flags;
	ushort flag = 1<<weapon_index;
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

	return 1;
}



//called when ammo (for the vulcan cannon) is picked up
//	Returns the amount picked up
int pick_up_ammo(int class_flag,int weapon_index,int ammo_count)
{
	int max,cutpoint,supposed_weapon=Primary_weapon;
	int old_ammo=class_flag;		//kill warning

	Assert(class_flag==CLASS_PRIMARY && weapon_index==VULCAN_INDEX);

	max = Primary_ammo_max[weapon_index];
	if (Players[Player_num].flags & PLAYER_FLAGS_AMMO_RACK)
		max *= 2;

	if (Players[Player_num].primary_ammo[weapon_index] == max)
		return 0;

	old_ammo = Players[Player_num].primary_ammo[weapon_index];

	Players[Player_num].primary_ammo[weapon_index] += ammo_count;

	if (Players[Player_num].primary_ammo[weapon_index] > max) {
		ammo_count += (max - Players[Player_num].primary_ammo[weapon_index]);
		Players[Player_num].primary_ammo[weapon_index] = max;
	}
	cutpoint=POrderList (255);

	if (Primary_weapon==LASER_INDEX && Players[Player_num].laser_level>=4)	
		supposed_weapon=SUPER_LASER_INDEX;  // allotment for stupid way of doing super laser
  

	if (Players[Player_num].primary_weapon_flags&(1<<weapon_index) && weapon_index>Primary_weapon && old_ammo==0 && 
		POrderList(weapon_index)<cutpoint && POrderList(weapon_index)<POrderList(supposed_weapon))
		select_weapon(weapon_index,0,0,1);

	return ammo_count;	//return amount used
}

#define	SMEGA_SHAKE_TIME		(F1_0*2)
#define	MAX_SMEGA_DETONATES	4
fix	Smega_detonate_times[MAX_SMEGA_DETONATES];

//	Call this to initialize for a new level.
//	Sets all super mega missile detonation times to 0 which means there aren't any.
void init_smega_detonates(void)
{
	int	i;

	for (i=0; i<MAX_SMEGA_DETONATES; i++)
		Smega_detonate_times[i] = 0;	
}

fix	Seismic_tremor_magnitude;
fix	Next_seismic_sound_time;
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
			fix	delta_time;
			delta_time = GameTime - Smega_detonate_times[i];

			if (!Seismic_sound_playing) {
				digi_play_sample_looping(Seismic_sound, F1_0, -1, -1);
				Seismic_sound_playing = 1;
				Next_seismic_sound_time = GameTime + d_rand()/2;
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

				rx = fixmul(d_rand() - 16384, 3*F1_0/16 + (F1_0*(16-fc))/32);
				rz = fixmul(d_rand() - 16384, 3*F1_0/16 + (F1_0*(16-fc))/32);

				ConsoleObject->mtype.phys_info.rotvel.x += rx;
				ConsoleObject->mtype.phys_info.rotvel.z += rz;

				//	Shake the buddy!
				if (Buddy_objnum != -1) {
					Objects[Buddy_objnum].mtype.phys_info.rotvel.x += rx*4;
					Objects[Buddy_objnum].mtype.phys_info.rotvel.z += rz*4;
				}

				//	Shake a guided missile!
				Seismic_tremor_magnitude += rx;

			} else
				Smega_detonate_times[i] = 0;

		}
	}

	//	Hook in the rumble sound effect here.
}

extern	int	Level_shake_frequency, Level_shake_duration;
#ifdef NETWORK
extern void multi_send_seismic (fix,fix);
#endif

#define	SEISMIC_DISTURBANCE_DURATION	(F1_0*5)
fix	Seismic_disturbance_start_time = 0, Seismic_disturbance_end_time;

int Seismic_level=0;

int	on_seismic_level(void)
{
	return Seismic_level;
}

void init_seismic_disturbances(void)
{
	Seismic_disturbance_start_time = 0;
	Seismic_disturbance_end_time = 0;
}

//	Return true if time to start a seismic disturbance.
int start_seismic_disturbance(void)
{
	int	rval;

	if (Level_shake_duration < 1)
		return 0;

	rval =  (2 * fixmul(d_rand(), Level_shake_frequency)) < FrameTime;

	if (rval) {
		Seismic_disturbance_start_time = GameTime;
		Seismic_disturbance_end_time = GameTime + Level_shake_duration;
		if (!Seismic_sound_playing) {
			digi_play_sample_looping(Seismic_sound, F1_0, -1, -1);
			Seismic_sound_playing = 1;
			Next_seismic_sound_time = GameTime + d_rand()/2;
		}

#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_send_seismic (Seismic_disturbance_start_time,Seismic_disturbance_end_time);
#endif
	}

	return rval;
}

void seismic_disturbance_frame(void)
{
	if (Level_shake_frequency) {
		if (((Seismic_disturbance_start_time < GameTime) && (Seismic_disturbance_end_time > GameTime)) || start_seismic_disturbance()) {
			fix	delta_time;
			int	fc, rx, rz;

			delta_time = GameTime - Seismic_disturbance_start_time;

			fc = abs(delta_time - (Seismic_disturbance_end_time - Seismic_disturbance_start_time)/2);
			fc /= F1_0/16;
			if (fc > 16)
				fc = 16;

			if (fc == 0)
				fc = 1;

			Seismic_tremor_volume += fc;

			rx = fixmul(d_rand() - 16384, 3*F1_0/16 + (F1_0*(16-fc))/32);
			rz = fixmul(d_rand() - 16384, 3*F1_0/16 + (F1_0*(16-fc))/32);

			ConsoleObject->mtype.phys_info.rotvel.x += rx;
			ConsoleObject->mtype.phys_info.rotvel.z += rz;

			//	Shake the buddy!
			if (Buddy_objnum != -1) {
				Objects[Buddy_objnum].mtype.phys_info.rotvel.x += rx*4;
				Objects[Buddy_objnum].mtype.phys_info.rotvel.z += rz*4;
			}

			//	Shake a guided missile!
			Seismic_tremor_magnitude += rx;
		}
	}
}


//	Call this when a smega detonates to start the process of rocking the mine.
void smega_rock_stuff(void)
{
	int	i;

	for (i=0; i<MAX_SMEGA_DETONATES; i++) {
		if (Smega_detonate_times[i] + SMEGA_SHAKE_TIME < GameTime)
			Smega_detonate_times[i] = 0;
	}

	for (i=0; i<MAX_SMEGA_DETONATES; i++) {
		if (Smega_detonate_times[i] == 0) {
			Smega_detonate_times[i] = GameTime;
			break;
		}
	}
}

int	Super_mines_yes = 1;

//	Call this once/frame to process all super mines in the level.
void process_super_mines_frame(void)
{
	int	i, j;
	int	start, add;

	//	If we don't know of there being any super mines in the level, just
	//	check every 8th object each frame.
	if (Super_mines_yes == 0) {
		start = FrameCount & 7;
		add = 8;
	} else {
		start = 0;
		add = 1;
	}

	Super_mines_yes = 0;

	for (i=start; i<=Highest_object_index; i+=add) {
		if ((Objects[i].type == OBJ_WEAPON) && (Objects[i].id == SUPERPROX_ID)) {
			int	parent_num;

			parent_num = Objects[i].ctype.laser_info.parent_num;

			Super_mines_yes = 1;
			if (Objects[i].lifeleft + F1_0*2 < Weapon_info[SUPERPROX_ID].lifetime) {
				vms_vector	*bombpos;
	
				bombpos = &Objects[i].pos;
	
				for (j=0; j<=Highest_object_index; j++) {
					if ((Objects[j].type == OBJ_PLAYER) || (Objects[j].type == OBJ_ROBOT)) {
						fix	dist;
		
						dist = vm_vec_dist_quick(bombpos, &Objects[j].pos);
		
						if (j != parent_num)
							if (dist - Objects[j].size < F1_0*20)
							{
								if (Objects[i].segnum == Objects[j].segnum)
									Objects[i].lifeleft = 1;
								else {
									//	Object which is close enough to detonate smart mine is not in same segment as smart mine.
									//	Need to do a more expensive check to make sure there isn't an obstruction.
									if (((FrameCount ^ (i+j)) % 4) == 0) {
										fvi_query	fq;
										fvi_info		hit_data;
										int			fate;

										mprintf((0, "Expensive proxmine collision check.  Frame %i\n", FrameCount));

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
int spit_powerup(object *spitter, int id,int seed)
{
	int		objnum;
	object	*obj;
	vms_vector	new_velocity, new_pos;

	d_srand(seed);

	vm_vec_scale_add(&new_velocity,&spitter->mtype.phys_info.velocity,&spitter->orient.fvec,i2f(SPIT_SPEED));

	new_velocity.x += (d_rand() - 16384) * SPIT_SPEED * 2;
	new_velocity.y += (d_rand() - 16384) * SPIT_SPEED * 2;
	new_velocity.z += (d_rand() - 16384) * SPIT_SPEED * 2;

	// Give keys zero velocity so they can be tracked better in multi

	if ((Game_mode & GM_MULTI) && (id >= POW_KEY_BLUE) && (id <= POW_KEY_GOLD))
		vm_vec_zero(&new_velocity);

	//there's a piece of code which lets the player pick up a powerup if
	//the distance between him and the powerup is less than 2 time their
	//combined radii.  So we need to create powerups pretty far out from
	//the player.

	vm_vec_scale_add(&new_pos,&spitter->pos,&spitter->orient.fvec,spitter->size);

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{	
		if (Net_create_loc >= MAX_NET_CREATE_OBJECTS)
		{
			mprintf( (0, "WEAPON:Not enough slots to drop all powerups!\n" ));
			return (-1);
		}
	}
#endif

	objnum = obj_create( OBJ_POWERUP, id, spitter->segnum, &new_pos, &vmd_identity_matrix, Powerup_info[id].size, CT_POWERUP, MT_PHYSICS, RT_POWERUP);

	if (objnum < 0 ) {
		mprintf((1, "Can't create object in object_create_egg.  Aborting.\n"));
		Int3();
		return objnum;
	}

	obj = &Objects[objnum];

	obj->mtype.phys_info.velocity = new_velocity;
	obj->mtype.phys_info.drag = 512;	//1024;
	obj->mtype.phys_info.mass = F1_0;

	obj->mtype.phys_info.flags = PF_BOUNCE;

	obj->rtype.vclip_info.vclip_num = Powerup_info[obj->id].vclip_num;
	obj->rtype.vclip_info.frametime = Vclip[obj->rtype.vclip_info.vclip_num].frame_time;
	obj->rtype.vclip_info.framenum = 0;

	if (spitter == ConsoleObject)
		obj->ctype.powerup_info.flags |= PF_SPAT_BY_PLAYER;

	switch (obj->id) {
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

	return objnum;
}

void DropCurrentWeapon ()
{
	int objnum,ammo=0,seed;

	if (Primary_weapon==0) 
	{
		HUD_init_message("You cannot drop your base weapon!");
		return;
	}

	HUD_init_message("%s dropped!",PRIMARY_WEAPON_NAMES(Primary_weapon));
	digi_play_sample (SOUND_DROP_WEAPON,F1_0);

	seed = d_rand();

	objnum = spit_powerup(ConsoleObject,Primary_weapon_to_powerup[Primary_weapon],seed);

   if (objnum<0)
		return;

	if (Primary_weapon == VULCAN_INDEX || Primary_weapon == GAUSS_INDEX) {

		//if it's one of these, drop some ammo with the weapon

		ammo = Players[Player_num].primary_ammo[VULCAN_INDEX];

		if ((Players[Player_num].primary_weapon_flags & HAS_FLAG(VULCAN_INDEX)) && (Players[Player_num].primary_weapon_flags & HAS_FLAG(GAUSS_INDEX)))
			ammo /= 2;		//if both vulcan & gauss, drop half

		Players[Player_num].primary_ammo[VULCAN_INDEX] -= ammo;

		if (objnum!=-1)
			Objects[objnum].ctype.powerup_info.count = ammo;
	}

	if (Primary_weapon == OMEGA_INDEX) {

		//dropped weapon has current energy

		if (objnum!=-1)
			Objects[objnum].ctype.powerup_info.count = Omega_charge;
	}

#ifdef NETWORK
	if ((Game_mode & GM_MULTI) && objnum>-1)
		multi_send_drop_weapon(objnum,seed);
#endif

	Players[Player_num].primary_weapon_flags &= (~(1<<Primary_weapon));
	auto_select_weapon (0);
}


void DropSecondaryWeapon ()
{
	int objnum,seed;

	if (Players[Player_num].secondary_ammo[Secondary_weapon] ==0)
	{
		HUD_init_message("No secondary weapon to drop!");
		return;
	}

	if ((Secondary_weapon_to_powerup[Secondary_weapon]==POW_PROXIMITY_WEAPON || 
		Secondary_weapon_to_powerup[Secondary_weapon]==POW_SMART_MINE) &&
		Players[Player_num].secondary_ammo[Secondary_weapon]<4)
	{
		HUD_init_message("You need at least 4 to drop!");
		return;
	}

	HUD_init_message("%s dropped!",SECONDARY_WEAPON_NAMES(Secondary_weapon));
	digi_play_sample (SOUND_DROP_WEAPON,F1_0);

	seed = d_rand();

	objnum = spit_powerup(ConsoleObject,Secondary_weapon_to_powerup[Secondary_weapon],seed);
	 
   if (objnum<0)
		return;
   

#ifdef NETWORK
	if ((Game_mode & GM_MULTI) && objnum>-1)
		multi_send_drop_weapon(objnum,seed);
#endif

	if (Secondary_weapon_to_powerup[Secondary_weapon]==POW_PROXIMITY_WEAPON ||
		Secondary_weapon_to_powerup[Secondary_weapon]==POW_SMART_MINE)
		Players[Player_num].secondary_ammo[Secondary_weapon]-=4;
	else
		Players[Player_num].secondary_ammo[Secondary_weapon]--;

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

		if ((GameTime > Next_seismic_sound_time) && Seismic_tremor_volume) {
			int	volume;

			volume = Seismic_tremor_volume * 2048;
			if (volume > F1_0)
				volume = F1_0;
			digi_change_looping_volume(volume);
			Next_seismic_sound_time = GameTime + d_rand()/4 + 8192;
		}
	}

}

int tactile_fire_duration[]={120,80,150,250,150,200,100,180,280,100};
int tactile_fire_repeat[]={260,90,160,160,160,210,110,191,291,111};

void tactile_set_button_jolt ()
 {
  #ifdef TACTILE
    
  FILE *infile;
  int t,i;
  static int stickmag=-1;
  int dur,rep;

  dur=tactile_fire_duration[Primary_weapon];
  rep=tactile_fire_repeat[Primary_weapon];
  
  if (TactileStick)
   {
  	 if (stickmag==-1)
	  {
	   if (t=FindArg("-stickmag"))
	 		stickmag=atoi (Args[t+1]);
	   else
	   	stickmag=50;

      infile=(FILE *)fopen ("stick.val","rt");
		if (infile!=NULL)
		 {
			for (i=0;i<10;i++)
			 {
			  fscanf (infile,"%d %d\n",&tactile_fire_duration[i],&tactile_fire_repeat[i]);
			  mprintf ((0,"scan value[%d]=%d\n",i,tactile_fire_duration[i]));
			 }
	 		fclose (infile);  
		 }
	  }							 
    ButtonReflexJolt (0,stickmag,0,dur,rep);
   }
  #endif
 }

/*
 * reads n weapon_info structs from a CFILE
 */
extern int weapon_info_read_n(weapon_info *wi, int n, CFILE *fp, int file_version)
{
	int i, j;

	for (i = 0; i < n; i++) {
		wi[i].render_type = cfile_read_byte(fp);
		wi[i].persistent = cfile_read_byte(fp);
		wi[i].model_num = cfile_read_short(fp);
		wi[i].model_num_inner = cfile_read_short(fp);

		wi[i].flash_vclip = cfile_read_byte(fp);
		wi[i].robot_hit_vclip = cfile_read_byte(fp);
		wi[i].flash_sound = cfile_read_short(fp);

		wi[i].wall_hit_vclip = cfile_read_byte(fp);
		wi[i].fire_count = cfile_read_byte(fp);
		wi[i].robot_hit_sound = cfile_read_short(fp);

		wi[i].ammo_usage = cfile_read_byte(fp);
		wi[i].weapon_vclip = cfile_read_byte(fp);
		wi[i].wall_hit_sound = cfile_read_short(fp);

		wi[i].destroyable = cfile_read_byte(fp);
		wi[i].matter = cfile_read_byte(fp);
		wi[i].bounce = cfile_read_byte(fp);
		wi[i].homing_flag = cfile_read_byte(fp);

		wi[i].speedvar = cfile_read_byte(fp);
		wi[i].flags = cfile_read_byte(fp);
		wi[i].flash = cfile_read_byte(fp);
		wi[i].afterburner_size = cfile_read_byte(fp);

		if (file_version >= 3)
			wi[i].children = cfile_read_byte(fp);
		else
			wi[i].children = -1;

		wi[i].energy_usage = cfile_read_fix(fp);
		wi[i].fire_wait = cfile_read_fix(fp);

		if (file_version >= 3)
			wi[i].multi_damage_scale = cfile_read_fix(fp);
		else
			wi[i].multi_damage_scale = F1_0;

		bitmap_index_read(&wi[i].bitmap, fp);

		wi[i].blob_size = cfile_read_fix(fp);
		wi[i].flash_size = cfile_read_fix(fp);
		wi[i].impact_size = cfile_read_fix(fp);
		for (j = 0; j < NDL; j++)
			wi[i].strength[j] = cfile_read_fix(fp);
		for (j = 0; j < NDL; j++)
			wi[i].speed[j] = cfile_read_fix(fp);
		wi[i].mass = cfile_read_fix(fp);
		wi[i].drag = cfile_read_fix(fp);
		wi[i].thrust = cfile_read_fix(fp);
		wi[i].po_len_to_width_ratio = cfile_read_fix(fp);
		wi[i].light = cfile_read_fix(fp);
		wi[i].lifetime = cfile_read_fix(fp);
		wi[i].damage_radius = cfile_read_fix(fp);
		bitmap_index_read(&wi[i].picture, fp);
		if (file_version >= 3)
			bitmap_index_read(&wi[i].hires_picture, fp);
		else
			wi[i].hires_picture.index = wi[i].picture.index;
	}
	return i;
}
