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



#ifndef _POWERUP_H
#define _POWERUP_H

#include "vclip.h"

#define	POW_EXTRA_LIFE 			0
#define	POW_ENERGY					1
#define	POW_SHIELD_BOOST			2
#define	POW_LASER					3

#define	POW_KEY_BLUE				4
#define	POW_KEY_RED					5
#define	POW_KEY_GOLD				6

//#define	POW_RADAR_ROBOTS			7
//#define	POW_RADAR_POWERUPS		8

#define	POW_MISSILE_1				10
#define	POW_MISSILE_4				11		//4-pack MUST follow single missile

#define	POW_QUAD_FIRE				12

#define	POW_VULCAN_WEAPON			13
#define	POW_SPREADFIRE_WEAPON	14
#define	POW_PLASMA_WEAPON			15
#define	POW_FUSION_WEAPON			16
#define	POW_PROXIMITY_WEAPON		17
#define	POW_SMARTBOMB_WEAPON		20
#define	POW_MEGA_WEAPON			21
#define	POW_VULCAN_AMMO			22
#define	POW_HOMING_AMMO_1			18
#define	POW_HOMING_AMMO_4			19		//4-pack MUST follow single missile
#define	POW_CLOAK					23
#define	POW_TURBO					24
#define	POW_INVULNERABILITY		25
#define	POW_MEGAWOW					27

#define	POW_GAUSS_WEAPON			28
#define	POW_HELIX_WEAPON			29
#define	POW_PHOENIX_WEAPON		30
#define	POW_OMEGA_WEAPON			31

#define	POW_SUPER_LASER			32
#define	POW_FULL_MAP				33
#define	POW_CONVERTER				34
#define	POW_AMMO_RACK				35
#define	POW_AFTERBURNER			36
#define	POW_HEADLIGHT				37

#define	POW_SMISSILE1_1			38
#define	POW_SMISSILE1_4			39		//4-pack MUST follow single missile
#define	POW_GUIDED_MISSILE_1		40
#define	POW_GUIDED_MISSILE_4		41		//4-pack MUST follow single missile
#define	POW_SMART_MINE				42
#define	POW_MERCURY_MISSILE_1 	43
#define	POW_MERCURY_MISSILE_4	44		//4-pack MUST follow single missile
#define	POW_EARTHSHAKER_MISSILE	45

#define	POW_FLAG_BLUE				46
#define	POW_FLAG_RED				47

#define 	POW_HOARD_ORB				7		//use unused slot


#define	VULCAN_AMMO_MAX				(392*4)
#define	VULCAN_WEAPON_AMMO_AMOUNT	196
#define	VULCAN_AMMO_AMOUNT			(49*2)

#define	GAUSS_WEAPON_AMMO_AMOUNT	392

#define MAX_POWERUP_TYPES		50

#define	POWERUP_NAME_LENGTH	16		//	Length of a robot or powerup name.
extern char	Powerup_names[MAX_POWERUP_TYPES][POWERUP_NAME_LENGTH];

extern int Headlight_active_default;	//is headlight on when picked up?

typedef struct powerup_type_info {
	int	vclip_num;
	int	hit_sound;
	fix	size;			//3d size of longest dimension
	fix	light;		//	amount of light cast by this powerup, set in bitmaps.tbl
} __pack__ powerup_type_info;

extern int N_powerup_types;
extern powerup_type_info Powerup_info[MAX_POWERUP_TYPES];

void draw_powerup(object *obj);

//returns true if powerup consumed
int do_powerup(object *obj);

//process (animate) a powerup for one frame
void do_powerup_frame(object *obj);

//	Diminish shields and energy towards max in case they exceeded it.
extern void diminish_towards_max(void);

extern void do_megawow_powerup(int quantity);

extern void powerup_basic(int redadd, int greenadd, int blueadd, int score, char *format, ...);

#endif
