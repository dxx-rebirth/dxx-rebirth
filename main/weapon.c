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
 * Functions for weapons...
 *
 */


#include "game.h"
#include "weapon.h"
#include "player.h"
#include "gauges.h"
#include "error.h"
#include "sounds.h"
#include "text.h"
#include "powerup.h"
#include "newdemo.h"
#include "multi.h"
#include "newmenu.h"
#include "playsave.h"

// Convert primary weapons to indices in Weapon_info array.
ubyte Primary_weapon_to_weapon_info[MAX_PRIMARY_WEAPONS] = {0, 11, 12, 13, 14};
ubyte Secondary_weapon_to_weapon_info[MAX_SECONDARY_WEAPONS] = {8, 15, 16, 17, 18};
int Primary_ammo_max[MAX_PRIMARY_WEAPONS] = {0, VULCAN_AMMO_MAX, 0, 0, 0};
ubyte Secondary_ammo_max[MAX_SECONDARY_WEAPONS] = {20, 10, 10, 5, 5};
weapon_info Weapon_info[MAX_WEAPON_TYPES];
int	N_weapon_types=0;
sbyte	Primary_weapon, Secondary_weapon;
int POrderList (int num);
int SOrderList (int num);
ubyte DefaultPrimaryOrder[] = { 4, 3, 2, 1, 0, 255 };
ubyte DefaultSecondaryOrder[] = { 4, 3, 1, 0, 255, 2 };
extern ubyte MenuReordering;
ubyte Cycling=0;

char	*Primary_weapon_names_short[MAX_PRIMARY_WEAPONS] = {
	"Laser",
	"Vulcan",
	"Spread",
	"Plasma",
	"Fusion"
};

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

void InitWeaponOrdering ()
 {
  // short routine to setup default weapon priorities for new pilots

  int i;

  for (i=0;i<MAX_PRIMARY_WEAPONS+1;i++)
	PlayerCfg.PrimaryOrder[i]=DefaultPrimaryOrder[i];
  for (i=0;i<MAX_SECONDARY_WEAPONS+1;i++)
	PlayerCfg.SecondaryOrder[i]=DefaultSecondaryOrder[i];
 }	

void CyclePrimary ()
{
	Cycling=1;
	auto_select_weapon (0);
	Cycling=0;
}

void CycleSecondary ()
{
	Cycling=1;
	auto_select_weapon (1);
	Cycling=0;
}

//	------------------------------------------------------------------------------------
//	if message flag set, print message saying selected
void select_weapon(int weapon_num, int secondary_flag, int print_message, int wait_for_rearm)
{
	char	*weapon_name;

#ifndef SHAREWARE
	if (Newdemo_state==ND_STATE_RECORDING )
		newdemo_record_player_weapon(secondary_flag, weapon_num);
#endif

	if (!secondary_flag) {
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
}

//	------------------------------------------------------------------------------------
//	Select a weapon, primary or secondary.
void do_weapon_select(int weapon_num, int secondary_flag)
{
        //added on 10/9/98 by Victor Rachels to add laser cycle
	int	oweapon = weapon_num;
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

		if (weapon_num >= MAX_PRIMARY_WEAPONS)
			weapon_num = 0;

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

	weapon_num=oweapon;
	select_weapon(weapon_num, secondary_flag, 1, 1);
}

//	----------------------------------------------------------------------------------------
//	Automatically select best available weapon if unable to fire current weapon.
//	Weapon type: 0==primary, 1==secondary
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
				// if (PlayerCfg.PrimaryOrder[cur_weapon] == FUSION_INDEX)
				//	continue;

				if (PlayerCfg.PrimaryOrder[cur_weapon] == Primary_weapon) {
					if (!Cycling)
					{
						HUD_init_message(TXT_NO_PRIMARY);
						//	if (POrderList(0)<POrderList(255))
						select_weapon(0, 0, 0, 1);
					}
					else
						select_weapon (Primary_weapon,0,0,1);

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

				if (PlayerCfg.SecondaryOrder[cur_weapon] == Secondary_weapon) {
					if (!Cycling)
						HUD_init_message("No secondary weapons available!");
					else
						select_weapon (Secondary_weapon,1,0,1);

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
//	called when one of these weapons is picked up
//	when you pick up a secondary, you always get the weapon & ammo for it
//	Returns true if powerup picked up, else returns false.
int pick_up_secondary(int weapon_index,int count)
{
	int	num_picked_up;
	int cutpoint;

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

	if (Players[Player_num].secondary_ammo[weapon_index] == count)	// only autoselect if player didn't have any
	{
		cutpoint=SOrderList (255);
		if (SOrderList (weapon_index)<cutpoint && ((SOrderList (weapon_index) < SOrderList(Secondary_weapon)) || (Players[Player_num].secondary_ammo[Secondary_weapon] == 0))   )
			select_weapon(weapon_index,1, 0, 1);
	}

	if (count>1) {
		PALETTE_FLASH_ADD(15,15,15);
		hud_message(MSGC_PICKUP_OK, "%d %s%s",num_picked_up,SECONDARY_WEAPON_NAMES(weapon_index), TXT_SX);
	}
	else {
		PALETTE_FLASH_ADD(10,10,10);
		hud_message(MSGC_PICKUP_OK, "%s!",SECONDARY_WEAPON_NAMES(weapon_index));
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
		if (PlayerCfg.PrimaryOrder[i]==255)
			m[i].text="--- Never Autoselect below ---";
		else
			m[i].text=(char *)PRIMARY_WEAPON_NAMES(PlayerCfg.PrimaryOrder[i]);
		m[i].value=PlayerCfg.PrimaryOrder[i];
	}
	MenuReordering=1;
	i = newmenu_do("Reorder Primary","Shift+Up/Down arrow to move item", i, m, NULL);
	MenuReordering=0;
	
	for (i=0;i<MAX_PRIMARY_WEAPONS+1;i++)
		PlayerCfg.PrimaryOrder[i]=m[i].value;
}

void ReorderSecondary ()
{
	newmenu_item m[MAX_SECONDARY_WEAPONS+1];
	int i;

	for (i=0;i<MAX_SECONDARY_WEAPONS+1;i++)
	{
		m[i].type=NM_TYPE_MENU;
		if (PlayerCfg.SecondaryOrder[i]==255)
			m[i].text="--- Never Autoselect below ---";
		else
			m[i].text=(char *)SECONDARY_WEAPON_NAMES(PlayerCfg.SecondaryOrder[i]);
		m[i].value=PlayerCfg.SecondaryOrder[i];
	}
	MenuReordering=1;
	i = newmenu_do("Reorder Secondary","Shift+Up/Down arrow to move item", i, m, NULL);
	MenuReordering=0;
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
	Error ("Primary Weapon is not in order list!!!");
}

int SOrderList (int num)
{
	int i;

	for (i=0;i<MAX_SECONDARY_WEAPONS+1;i++)
		if (PlayerCfg.SecondaryOrder[i]==num)
		{
			return (i);
		}
	Error ("Secondary Weapon is not in order list!!!");
}

//called when a primary weapon is picked up
//returns true if actually picked up
int pick_up_primary(int weapon_index)
{
	int cutpoint;
	ubyte flag = 1<<weapon_index;

	if (Players[Player_num].primary_weapon_flags & flag) {		//already have
		hud_message(MSGC_PICKUP_ALREADY, "%s %s!", TXT_ALREADY_HAVE_THE, PRIMARY_WEAPON_NAMES(weapon_index));
		return 0;
	}

	Players[Player_num].primary_weapon_flags |= flag;

	cutpoint=POrderList (255);

	if (POrderList(weapon_index)<cutpoint && POrderList(weapon_index)<POrderList(Primary_weapon))
		select_weapon(weapon_index,0,0,1);

	PALETTE_FLASH_ADD(7,14,21);
	hud_message(MSGC_PICKUP_OK, "%s!",PRIMARY_WEAPON_NAMES(weapon_index));

	return 1;
}

//called when ammo (for the vulcan cannon) is picked up
//Return true if ammo picked up, else return false.
int pick_up_ammo(int class_flag,int weapon_index,int ammo_count)
{
	int cutpoint;
	int old_ammo=class_flag;		//kill warning

	Assert(class_flag==CLASS_PRIMARY && weapon_index==VULCAN_INDEX);

	if (Players[Player_num].primary_ammo[weapon_index] == Primary_ammo_max[weapon_index])
		return 0;

	old_ammo = Players[Player_num].primary_ammo[weapon_index];

	Players[Player_num].primary_ammo[weapon_index] += ammo_count;

	if (Players[Player_num].primary_ammo[weapon_index] > Primary_ammo_max[weapon_index]) {
		ammo_count += (Primary_ammo_max[weapon_index] - Players[Player_num].primary_ammo[weapon_index]);
		Players[Player_num].primary_ammo[weapon_index] =Primary_ammo_max[weapon_index];
	}
	cutpoint=POrderList (255);

	if (Players[Player_num].primary_weapon_flags&(1<<weapon_index) && weapon_index>Primary_weapon && old_ammo==0 &&
		POrderList(weapon_index)<cutpoint && POrderList(weapon_index)<POrderList(Primary_weapon))
		select_weapon(weapon_index,0,0,1);

	return 1;	//return amount used
}

/*
 * reads n weapon_info structs from a CFILE
 */
int weapon_info_read_n(weapon_info *wi, int n, CFILE *fp)
{
	int i, j;
	
	for (i = 0; i < n; i++) {
		wi[i].render_type = cfile_read_byte(fp);
		wi[i].model_num = cfile_read_byte(fp);
		wi[i].model_num_inner = cfile_read_byte(fp);
		wi[i].persistent = cfile_read_byte(fp);
		wi[i].flash_vclip = cfile_read_byte(fp);
		wi[i].flash_sound = cfile_read_short(fp);		
		wi[i].robot_hit_vclip = cfile_read_byte(fp);
		wi[i].robot_hit_sound = cfile_read_short(fp);		
		wi[i].wall_hit_vclip = cfile_read_byte(fp);
		wi[i].wall_hit_sound = cfile_read_short(fp);		
		wi[i].fire_count = cfile_read_byte(fp);
		wi[i].ammo_usage = cfile_read_byte(fp);
		wi[i].weapon_vclip = cfile_read_byte(fp);
		wi[i].destroyable = cfile_read_byte(fp);
		wi[i].matter = cfile_read_byte(fp);
		wi[i].bounce = cfile_read_byte(fp);
		wi[i].homing_flag = cfile_read_byte(fp);
		wi[i].dum1 = cfile_read_byte(fp);
		wi[i].dum2 = cfile_read_byte(fp);
		wi[i].dum3 = cfile_read_byte(fp);
		wi[i].energy_usage = cfile_read_fix(fp);
		wi[i].fire_wait = cfile_read_fix(fp);
		wi[i].bitmap.index = cfile_read_short(fp);	// bitmap_index = short
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
		wi[i].picture.index = cfile_read_short(fp);		// bitmap_index is a short
	}
	return i;
}

