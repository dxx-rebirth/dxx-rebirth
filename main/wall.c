/* $Id: wall.c,v 1.12 2004-05-22 01:06:29 btb Exp $ */
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
 * Destroyable wall stuff
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:32:08  allender
 * Initial revision
 *
 * Revision 2.1  1995/03/21  14:39:04  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.0  1995/02/27  11:28:32  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.112  1995/02/22  13:53:07  allender
 * remove anonymous unions from object structure
 *
 * Revision 1.111  1995/02/01  17:32:17  adam
 * Took out a bogus int3.
 *
 * Revision 1.110  1995/02/01  17:20:24  john
 * Lintized.
 *
 * Revision 1.109  1995/01/21  17:39:50  matt
 * Cleaned up laser/player hit wall confusions
 *
 * Revision 1.108  1995/01/21  17:14:17  rob
 * Fixed bug in multiplayer door-butting.
 *
 * Revision 1.107  1995/01/18  18:57:11  rob
 * Added new hostage door hooks.
 *
 * Revision 1.106  1995/01/18  18:48:18  allender
 * removed #ifdef newdemo's.  Added function call to record a door that
 * starts to open. This fixes the rewind problem
 *
 * Revision 1.105  1995/01/16  11:55:39  mike
 * make control center (and robots whose id == your playernum) not able to open doors.
 *
 * Revision 1.104  1994/12/11  23:07:21  matt
 * Fixed stuck objects & blastable walls
 *
 * Revision 1.103  1994/12/10  16:44:34  matt
 * Added debugging code to track down door that turns into rock
 *
 * Revision 1.102  1994/12/06  16:27:05  matt
 * Added debugging
 *
 * Revision 1.101  1994/12/02  10:50:27  yuan
 * Localization
 *
 * Revision 1.100  1994/11/30  19:41:22  rob
 * Put in a fix so that door opening sounds travel through the door.
 *
 * Revision 1.99  1994/11/28  11:59:50  yuan
 * *** empty log message ***
 *
 * Revision 1.98  1994/11/28  11:25:45  matt
 * Cleaned up key hud messages
 *
 * Revision 1.97  1994/11/27  23:15:11  matt
 * Made changes for new mprintf calling convention
 *
 * Revision 1.96  1994/11/19  15:18:29  mike
 * rip out unused code and data.
 *
 * Revision 1.95  1994/11/17  14:57:12  mike
 * moved segment validation functions from editor to main.
 *
 * Revision 1.94  1994/11/07  08:47:30  john
 * Made wall state record.
 *
 * Revision 1.93  1994/11/04  16:06:37  rob
 * Fixed network damage of blastable walls.
 *
 * Revision 1.92  1994/11/02  21:54:01  matt
 * Don't let objects with zero size keep door from shutting
 *
 * Revision 1.91  1994/10/31  13:48:42  rob
 * Fixed bug in opening doors over network/modem.  Added a new message
 * type to multi.c that communicates door openings across the net.
 * Changed includes in multi.c and wall.c to accomplish this.
 *
 * Revision 1.90  1994/10/28  14:42:41  john
 * Added sound volumes to all sound calls.
 *
 * Revision 1.89  1994/10/23  19:16:55  matt
 * Fixed bug with "no key" messages
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: wall.c,v 1.12 2004-05-22 01:06:29 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "pstypes.h"
#include "mono.h"
#include "gr.h"
#include "wall.h"
#include "switch.h"
#include "inferno.h"
#include "segment.h"
#include "error.h"
#include "gameseg.h"
#include "game.h"
#include "bm.h"
#include "vclip.h"
#include "player.h"
#include "gauges.h"
#include "text.h"
#include "fireball.h"
#include "textures.h"
#include "sounds.h"
#include "newdemo.h"
#include "multi.h"
#include "gameseq.h"
#include "laser.h"		//	For seeing if a flare is stuck in a wall.
#include "collide.h"
#include "effects.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

//	Special door on boss level which is locked if not in multiplayer...sorry for this awful solution --MK.
#define	BOSS_LOCKED_DOOR_LEVEL	7
#define	BOSS_LOCKED_DOOR_SEG		595
#define	BOSS_LOCKED_DOOR_SIDE	5

wall Walls[MAX_WALLS];					// Master walls array
int Num_walls=0;							// Number of walls

wclip WallAnims[MAX_WALL_ANIMS];		// Wall animations
int Num_wall_anims;
//--unused-- int walls_bm_num[MAX_WALL_ANIMS];

//door Doors[MAX_DOORS];					//	Master doors array

active_door ActiveDoors[MAX_DOORS];
int Num_open_doors;						// Number of open doors

#define CLOAKING_WALL_TIME f1_0

#define MAX_CLOAKING_WALLS 10
cloaking_wall CloakingWalls[MAX_CLOAKING_WALLS];
int Num_cloaking_walls;

//--unused-- grs_bitmap *wall_title_bms[MAX_WALL_ANIMS];

//#define BM_FLAG_TRANSPARENT			1
//#define BM_FLAG_SUPER_TRANSPARENT	2

#ifdef EDITOR
char	Wall_names[7][10] = {
	"NORMAL   ",
	"BLASTABLE",
	"DOOR     ",
	"ILLUSION ",
	"OPEN     ",
	"CLOSED   ",
	"EXTERNAL "
};
#endif

// Function prototypes
void kill_stuck_objects(int wallnum);



// This function determines whether the current segment/side is transparent
//		1 = YES
//		0 = NO
int check_transparency( segment * seg, int side )
{
	if ( (seg->sides[side].tmap_num2 & 0x3FFF) == 0) {
		if (GameBitmaps[Textures[seg->sides[side].tmap_num].index].bm_flags & BM_FLAG_TRANSPARENT )
			return 1;
		else
			return 0;
		}

	if (GameBitmaps[Textures[seg->sides[side].tmap_num2 & 0x3FFF ].index].bm_flags & BM_FLAG_SUPER_TRANSPARENT )
		return 1;
	else
		return 0;
}

//define these here so I don't have to change wall_is_doorway and run
//the risk of screwing it up.
#define WID_WALL						2	// 0/1/0		wall	
#define WID_TRANSPARENT_WALL		6	//	0/1/1		transparent wall
#define WID_ILLUSORY_WALL			3	//	1/1/0		illusory wall
#define WID_TRANSILLUSORY_WALL	7	//	1/1/1		transparent illusory wall
#define WID_NO_WALL					5	//	1/0/1		no wall, can fly through
#define WID_EXTERNAL					8	// 0/0/0/1	don't see it, dont fly through it

//-----------------------------------------------------------------
// This function checks whether we can fly through the given side.
//	In other words, whether or not we have a 'doorway'
//	 Flags:
//		WID_FLY_FLAG				1
//		WID_RENDER_FLAG			2
//		WID_RENDPAST_FLAG			4
//	 Return values:
//		WID_WALL						2	// 0/1/0		wall	
//		WID_TRANSPARENT_WALL		6	//	0/1/1		transparent wall
//		WID_ILLUSORY_WALL			3	//	1/1/0		illusory wall
//		WID_TRANSILLUSORY_WALL	7	//	1/1/1		transparent illusory wall
//		WID_NO_WALL					5	//	1/0/1		no wall, can fly through
int wall_is_doorway ( segment * seg, int side )
{
	int flags, type;
	int state;
//--Covered by macro	// No child.
//--Covered by macro	if (seg->children[side] == -1)
//--Covered by macro		return WID_WALL;

//--Covered by macro	if (seg->children[side] == -2)
//--Covered by macro		return WID_EXTERNAL_FLAG;

//--Covered by macro // No wall present.
//--Covered by macro	if (seg->sides[side].wall_num == -1)
//--Covered by macro		return WID_NO_WALL;

	Assert(seg-Segments>=0 && seg-Segments<=Highest_segment_index);
	Assert(side>=0 && side<6);

	type = Walls[seg->sides[side].wall_num].type;
	flags = Walls[seg->sides[side].wall_num].flags;

	if (type == WALL_OPEN)
		return WID_NO_WALL;

	if (type == WALL_ILLUSION) {
		if (Walls[seg->sides[side].wall_num].flags & WALL_ILLUSION_OFF)
			return WID_NO_WALL;
		else {
			if (check_transparency( seg, side))
				return WID_TRANSILLUSORY_WALL;
		 	else
				return WID_ILLUSORY_WALL;
		}
	}

	if (type == WALL_BLASTABLE) {
	 	if (flags & WALL_BLASTED)
			return WID_TRANSILLUSORY_WALL;

		if (check_transparency( seg, side))
			return WID_TRANSPARENT_WALL;
		else
			return WID_WALL;
	}	
	
	if (flags & WALL_DOOR_OPENED)
		return WID_TRANSILLUSORY_WALL;
	
	if (type == WALL_CLOAKED)
		return WID_RENDER_FLAG | WID_RENDPAST_FLAG | WID_CLOAKED_FLAG;

	state = Walls[seg->sides[side].wall_num].state;
	if ((type == WALL_DOOR) && (state == WALL_DOOR_OPENING))
		return WID_TRANSPARENT_WALL;
	
// If none of the above flags are set, there is no doorway.
	if (check_transparency( seg, side))
		return WID_TRANSPARENT_WALL;
	else
		return WID_WALL; // There are children behind the door.
}

#ifdef EDITOR
//-----------------------------------------------------------------
// Initializes all the walls (in other words, no special walls)
void wall_init()
{
	int i;
	
	Num_walls = 0;
	for (i=0;i<MAX_WALLS;i++) {
		Walls[i].segnum = Walls[i].sidenum = -1;
		Walls[i].type = WALL_NORMAL;
		Walls[i].flags = 0;
		Walls[i].hps = 0;
		Walls[i].trigger = -1;
		Walls[i].clip_num = -1;
		Walls[i].linked_wall = -1;
		}
	Num_open_doors = 0;
	Num_cloaking_walls = 0;

}

//-----------------------------------------------------------------
// Initializes one wall.
void wall_reset(segment *seg, int side)
{
	int i;
	
	i = seg->sides[side].wall_num;

	if (i==-1) {
		mprintf((0, "Resetting Illegal Wall\n"));
		return;
	}

	Walls[i].segnum = seg-Segments;
	Walls[i].sidenum = side;
	Walls[i].type = WALL_NORMAL;
	Walls[i].flags = 0;
	Walls[i].hps = 0;
	Walls[i].trigger = -1;
	Walls[i].clip_num = -1;
	Walls[i].linked_wall = -1;
}
#endif

//set the tmap_num or tmap_num2 field for a wall/door
void wall_set_tmap_num(segment *seg,int side,segment *csegp,int cside,int anim_num,int frame_num)
{
	wclip *anim = &WallAnims[anim_num];
	int tmap = anim->frames[frame_num];

	if ( Newdemo_state==ND_STATE_PLAYBACK ) return;

	if (anim->flags & WCF_TMAP1)	{
		seg->sides[side].tmap_num = csegp->sides[cside].tmap_num = tmap;
		if ( Newdemo_state == ND_STATE_RECORDING )
			newdemo_record_wall_set_tmap_num1(seg-Segments,side,csegp-Segments,cside,tmap);
	} else	{
		Assert(tmap!=0 && seg->sides[side].tmap_num2!=0);
		seg->sides[side].tmap_num2 = csegp->sides[cside].tmap_num2 = tmap;
		if ( Newdemo_state == ND_STATE_RECORDING )
			newdemo_record_wall_set_tmap_num2(seg-Segments,side,csegp-Segments,cside,tmap);
	}
}


// -------------------------------------------------------------------------------
//when the wall has used all its hitpoints, this will destroy it
void blast_blastable_wall(segment *seg, int side)
{
	int Connectside;
	segment *csegp;
	int a, n, cwall_num;

	Assert(seg->sides[side].wall_num != -1);

	Walls[seg->sides[side].wall_num].hps = -1;	//say it's blasted

	csegp = &Segments[seg->children[side]];
	Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != -1);
	cwall_num = csegp->sides[Connectside].wall_num;
	kill_stuck_objects(seg->sides[side].wall_num);
	if (cwall_num > -1)
		kill_stuck_objects(cwall_num);

	//if this is an exploding wall, explode it
	if (WallAnims[Walls[seg->sides[side].wall_num].clip_num].flags & WCF_EXPLODES)
		explode_wall(seg-Segments,side);
	else {
		//if not exploding, set final frame, and make door passable
		a = Walls[seg->sides[side].wall_num].clip_num;
		n = WallAnims[a].num_frames;
		wall_set_tmap_num(seg,side,csegp,Connectside,a,n-1);
		Walls[seg->sides[side].wall_num].flags |= WALL_BLASTED;
		if (cwall_num > -1)
			Walls[cwall_num].flags |= WALL_BLASTED;
	}

}


//-----------------------------------------------------------------
// Destroys a blastable wall.
void wall_destroy(segment *seg, int side)
{
	Assert(seg->sides[side].wall_num != -1);
	Assert(seg-Segments != 0);

	if (Walls[seg->sides[side].wall_num].type == WALL_BLASTABLE)
		blast_blastable_wall( seg, side );
	else
		Error("Hey bub, you are trying to destroy an indestructable wall.");
}

//-----------------------------------------------------------------
// Deteriorate appearance of wall. (Changes bitmap (paste-ons))
void wall_damage(segment *seg, int side, fix damage)
{
	int a, i, n, cwall_num;

	if (seg->sides[side].wall_num == -1) {
		mprintf((0, "Damaging illegal wall\n"));
		return;
	}

	if (Walls[seg->sides[side].wall_num].type != WALL_BLASTABLE)
		return;
	
	if (!(Walls[seg->sides[side].wall_num].flags & WALL_BLASTED) && Walls[seg->sides[side].wall_num].hps >= 0)
		{
		int Connectside;
		segment *csegp;

		csegp = &Segments[seg->children[side]];
		Connectside = find_connect_side(seg, csegp);
		Assert(Connectside != -1);
		cwall_num = csegp->sides[Connectside].wall_num;
		Walls[seg->sides[side].wall_num].hps -= damage;
		if (cwall_num > -1)
			Walls[cwall_num].hps -= damage;
			
		a = Walls[seg->sides[side].wall_num].clip_num;
		n = WallAnims[a].num_frames;
		
		if (Walls[seg->sides[side].wall_num].hps < WALL_HPS*1/n) {
			blast_blastable_wall( seg, side );			
			#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_door_open(seg-Segments, side,Walls[seg->sides[side].wall_num].flags);
			#endif
		}
		else
			for (i=0;i<n;i++)
				if (Walls[seg->sides[side].wall_num].hps < WALL_HPS*(n-i)/n) {
					wall_set_tmap_num(seg,side,csegp,Connectside,a,i);
				}
		}
}


//-----------------------------------------------------------------
// Opens a door
void wall_open_door(segment *seg, int side)
{
	wall *w;
	active_door *d;
	int Connectside, wall_num, cwall_num;
	segment *csegp;

	Assert(seg->sides[side].wall_num != -1); 	//Opening door on illegal wall

	w = &Walls[seg->sides[side].wall_num];
	wall_num = w - Walls;
	//kill_stuck_objects(seg->sides[side].wall_num);

	if ((w->state == WALL_DOOR_OPENING) ||		//already opening
		 (w->state == WALL_DOOR_WAITING)	||		//open, waiting to close
		 (w->state == WALL_DOOR_OPEN))			//open, & staying open
		return;

	if (w->state == WALL_DOOR_CLOSING) {		//closing, so reuse door

		int i;
	
		d = NULL;

		for (i=0;i<Num_open_doors;i++) {		//find door

			d = &ActiveDoors[i];
	
			if (d->front_wallnum[0]==w-Walls || d->back_wallnum[0]==wall_num ||
				 (d->n_parts==2 && (d->front_wallnum[1]==wall_num || d->back_wallnum[1]==wall_num)))
				break;
		}

		if (i>=Num_open_doors && (Game_mode & GM_MULTI))	
				goto FastFix;
		
		Assert(i<Num_open_doors);				//didn't find door!
		Assert( d!=NULL ); // Get John!

		d->time = WallAnims[w->clip_num].play_time - d->time;

		if (d->time < 0)
			d->time = 0;
	
	}
	else {											//create new door
		Assert(w->state == WALL_DOOR_CLOSED);
		FastFix:
		d = &ActiveDoors[Num_open_doors];
		d->time = 0;
		Num_open_doors++;
		Assert( Num_open_doors < MAX_DOORS );
	}


	w->state = WALL_DOOR_OPENING;

	// So that door can't be shot while opening
	csegp = &Segments[seg->children[side]];
	Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != -1);
	cwall_num = csegp->sides[Connectside].wall_num;
	if (cwall_num > -1)
		Walls[cwall_num].state = WALL_DOOR_OPENING;

	//kill_stuck_objects(csegp->sides[Connectside].wall_num);

	d->front_wallnum[0] = seg->sides[side].wall_num;
	d->back_wallnum[0] = cwall_num;

	Assert( seg-Segments != -1);

	if (Newdemo_state == ND_STATE_RECORDING) {
		newdemo_record_door_opening(seg-Segments, side);
	}

	if (w->linked_wall != -1) {
		wall *w2;
		segment *seg2;

		w2		= &Walls[w->linked_wall];
		seg2	= &Segments[w2->segnum];

		Assert(w2->linked_wall == seg->sides[side].wall_num);
		//Assert(!(w2->flags & WALL_DOOR_OPENING  ||  w2->flags & WALL_DOOR_OPENED));

		w2->state = WALL_DOOR_OPENING;

		csegp = &Segments[seg2->children[w2->sidenum]];
		Connectside = find_connect_side(seg2, csegp);
		Assert(Connectside != -1);
		if (cwall_num > -1)
			Walls[cwall_num].state = WALL_DOOR_OPENING;

		d->n_parts = 2;
		d->front_wallnum[1] = w->linked_wall;
		d->back_wallnum[1] = cwall_num;
	}
	else
		d->n_parts = 1;


	if ( Newdemo_state != ND_STATE_PLAYBACK )
	{
		// NOTE THE LINK TO ABOVE!!!!
		vms_vector cp;
		compute_center_point_on_side(&cp, seg, side );
		if (WallAnims[w->clip_num].open_sound > -1 )
			digi_link_sound_to_pos( WallAnims[w->clip_num].open_sound, seg-Segments, side, &cp, 0, F1_0 );

	}
}

//-----------------------------------------------------------------
// start the transition from closed -> open wall
void start_wall_cloak(segment *seg, int side)
{
	wall *w;
	cloaking_wall *d;
	int Connectside;
	segment *csegp;
	int i, cwall_num;

	if ( Newdemo_state==ND_STATE_PLAYBACK ) return;

	Assert(seg->sides[side].wall_num != -1); 	//Opening door on illegal wall

	w = &Walls[seg->sides[side].wall_num];

	if (w->type == WALL_OPEN || w->state == WALL_DOOR_CLOAKING)		//already open or cloaking
		return;

	csegp = &Segments[seg->children[side]];
	Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != -1);
	cwall_num = csegp->sides[Connectside].wall_num;

	if (w->state == WALL_DOOR_DECLOAKING) {	//decloaking, so reuse door

		int i;

		d = NULL;

		for (i=0;i<Num_cloaking_walls;i++) {		//find door

			d = &CloakingWalls[i];
	
			if (d->front_wallnum==w-Walls || d->back_wallnum==w-Walls )
				break;
		}

		Assert(i<Num_cloaking_walls);				//didn't find door!
		Assert( d!=NULL ); // Get John!

		d->time = CLOAKING_WALL_TIME - d->time;

	}
	else if (w->state == WALL_DOOR_CLOSED) {	//create new door
		d = &CloakingWalls[Num_cloaking_walls];
		d->time = 0;
		if (Num_cloaking_walls >= MAX_CLOAKING_WALLS) {		//no more!
			Int3();		//ran out of cloaking wall slots
			w->type = WALL_OPEN;
			if (cwall_num > -1)
				Walls[cwall_num].type = WALL_OPEN;
			return;
		}
		Num_cloaking_walls++;
	}
	else {
		Int3();		//unexpected wall state
		return;
	}

	w->state = WALL_DOOR_CLOAKING;
	if (cwall_num > -1)
		Walls[cwall_num].state = WALL_DOOR_CLOAKING;

	d->front_wallnum = seg->sides[side].wall_num;
	d->back_wallnum = cwall_num;

	Assert( seg-Segments != -1);

	Assert(w->linked_wall == -1);

	if ( Newdemo_state != ND_STATE_PLAYBACK ) {
		vms_vector cp;
		compute_center_point_on_side(&cp, seg, side );
		digi_link_sound_to_pos( SOUND_WALL_CLOAK_ON, seg-Segments, side, &cp, 0, F1_0 );
	}

	for (i=0;i<4;i++) {
		d->front_ls[i] = seg->sides[side].uvls[i].l;
		if (cwall_num > -1)
			d->back_ls[i] = csegp->sides[Connectside].uvls[i].l;
	}
}

//-----------------------------------------------------------------
// start the transition from open -> closed wall
void start_wall_decloak(segment *seg, int side)
{
	wall *w;
	cloaking_wall *d;
	int Connectside;
	segment *csegp;
	int i, cwall_num;

	if ( Newdemo_state==ND_STATE_PLAYBACK ) return;

	Assert(seg->sides[side].wall_num != -1); 	//Opening door on illegal wall

	w = &Walls[seg->sides[side].wall_num];

	if (w->type == WALL_CLOSED || w->state == WALL_DOOR_DECLOAKING)		//already closed or decloaking
		return;

	if (w->state == WALL_DOOR_CLOAKING) {	//cloaking, so reuse door

		int i;

		d = NULL;

		for (i=0;i<Num_cloaking_walls;i++) {		//find door

			d = &CloakingWalls[i];
	
			if (d->front_wallnum==w-Walls || d->back_wallnum==w-Walls )
				break;
		}

		Assert(i<Num_cloaking_walls);				//didn't find door!
		Assert( d!=NULL ); // Get John!

		d->time = CLOAKING_WALL_TIME - d->time;

	}
	else if (w->state == WALL_DOOR_CLOSED) {	//create new door
		d = &CloakingWalls[Num_cloaking_walls];
		d->time = 0;
		if (Num_cloaking_walls >= MAX_CLOAKING_WALLS) {		//no more!
			Int3();		//ran out of cloaking wall slots
			/* what is this _doing_ here?
			w->type = WALL_CLOSED;
			Walls[csegp->sides[Connectside].wall_num].type = WALL_CLOSED;
			*/
			return;
		}
		Num_cloaking_walls++;
	}
	else {
		Int3();		//unexpected wall state
		return;
	}

	w->state = WALL_DOOR_DECLOAKING;

	// So that door can't be shot while opening
	csegp = &Segments[seg->children[side]];
	Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != -1);
	cwall_num = csegp->sides[Connectside].wall_num;
	if (cwall_num > -1)
		Walls[cwall_num].state = WALL_DOOR_DECLOAKING;

	d->front_wallnum = seg->sides[side].wall_num;
	d->back_wallnum = csegp->sides[Connectside].wall_num;

	Assert( seg-Segments != -1);

	Assert(w->linked_wall == -1);

	if ( Newdemo_state != ND_STATE_PLAYBACK ) {
		vms_vector cp;
		compute_center_point_on_side(&cp, seg, side );
		digi_link_sound_to_pos( SOUND_WALL_CLOAK_OFF, seg-Segments, side, &cp, 0, F1_0 );
	}

	for (i=0;i<4;i++) {
		d->front_ls[i] = seg->sides[side].uvls[i].l;
		if (cwall_num > -1)
			d->back_ls[i] = csegp->sides[Connectside].uvls[i].l;
	}
}

//-----------------------------------------------------------------
// This function closes the specified door and restores the closed
//  door texture.  This is called when the animation is done
void wall_close_door_num(int door_num)
{
	int p;
	active_door *d;
	int i, cwall_num;

	d = &ActiveDoors[door_num];

	for (p=0;p<d->n_parts;p++) {
		wall *w;
		int Connectside, side;
		segment *csegp, *seg;
	
		w = &Walls[d->front_wallnum[p]];

		seg = &Segments[w->segnum];
		side = w->sidenum;
	
		Assert(seg->sides[side].wall_num != -1);		//Closing door on illegal wall
		
		csegp = &Segments[seg->children[side]];
		Connectside = find_connect_side(seg, csegp);
		Assert(Connectside != -1);
		cwall_num = csegp->sides[Connectside].wall_num;
		Walls[seg->sides[side].wall_num].state = WALL_DOOR_CLOSED;
		if (cwall_num > -1)
			Walls[cwall_num].state = WALL_DOOR_CLOSED;
	
		wall_set_tmap_num(seg,side,csegp,Connectside,w->clip_num,0);

	}
	
	for (i=door_num;i<Num_open_doors;i++)
		ActiveDoors[i] = ActiveDoors[i+1];

	Num_open_doors--;

}

int check_poke(int objnum,int segnum,int side)
{
	object *obj = &Objects[objnum];

	//note: don't let objects with zero size block door

	if (obj->size && get_seg_masks(&obj->pos, segnum, obj->size, __FILE__, __LINE__).sidemask & (1 << side))
		return 1;		//pokes through side!
	else
		return 0;		//does not!

}

//returns true of door in unobjstructed (& thus can close)
int is_door_free(segment *seg,int side)
{
	int Connectside;
	segment *csegp;
	int objnum;
	
	csegp = &Segments[seg->children[side]];
	Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != -1);

	//go through each object in each of two segments, and see if
	//it pokes into the connecting seg

	for (objnum=seg->objects;objnum!=-1;objnum=Objects[objnum].next)
		if (Objects[objnum].type!=OBJ_WEAPON && Objects[objnum].type!=OBJ_FIREBALL && check_poke(objnum,seg-Segments,side))
			return 0;	//not free

	for (objnum=csegp->objects;objnum!=-1;objnum=Objects[objnum].next)
		if (Objects[objnum].type!=OBJ_WEAPON && Objects[objnum].type!=OBJ_FIREBALL && check_poke(objnum,csegp-Segments,Connectside))
			return 0;	//not free

	return 1; 	//doorway is free!
}



//-----------------------------------------------------------------
// Closes a door
void wall_close_door(segment *seg, int side)
{
	wall *w;
	active_door *d;
	int Connectside, wall_num, cwall_num;
	segment *csegp;

	Assert(seg->sides[side].wall_num != -1); 	//Opening door on illegal wall

	w = &Walls[seg->sides[side].wall_num];
	wall_num = w - Walls;
	if ((w->state == WALL_DOOR_CLOSING) ||		//already closing
		 (w->state == WALL_DOOR_WAITING)	||		//open, waiting to close
		 (w->state == WALL_DOOR_CLOSED))			//closed
		return;

	if (!is_door_free(seg,side))
		return;

	if (w->state == WALL_DOOR_OPENING) {	//reuse door

		int i;
	
		d = NULL;

		for (i=0;i<Num_open_doors;i++) {		//find door

			d = &ActiveDoors[i];
	
			if (d->front_wallnum[0]==wall_num || d->back_wallnum[0]==wall_num ||
				 (d->n_parts==2 && (d->front_wallnum[1]==wall_num || d->back_wallnum[1]==wall_num)))
				break;
		}

		Assert(i<Num_open_doors);				//didn't find door!
		Assert( d!=NULL ); // Get John!

		d->time = WallAnims[w->clip_num].play_time - d->time;

		if (d->time < 0)
			d->time = 0;
	
	}
	else {											//create new door
		Assert(w->state == WALL_DOOR_OPEN);
		d = &ActiveDoors[Num_open_doors];
		d->time = 0;
		Num_open_doors++;
		Assert( Num_open_doors < MAX_DOORS );
	}

	w->state = WALL_DOOR_CLOSING;

	// So that door can't be shot while opening
	csegp = &Segments[seg->children[side]];
	Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != -1);
	cwall_num = csegp->sides[Connectside].wall_num;
	if (cwall_num > -1)
		Walls[cwall_num].state = WALL_DOOR_CLOSING;

	d->front_wallnum[0] = seg->sides[side].wall_num;
	d->back_wallnum[0] = cwall_num;

	Assert( seg-Segments != -1);

	if (Newdemo_state == ND_STATE_RECORDING) {
		newdemo_record_door_opening(seg-Segments, side);
	}

	if (w->linked_wall != -1) {
		Int3();		//don't think we ever used linked walls
	}
	else
		d->n_parts = 1;


	if ( Newdemo_state != ND_STATE_PLAYBACK )
	{
		// NOTE THE LINK TO ABOVE!!!!
		vms_vector cp;
		compute_center_point_on_side(&cp, seg, side );
		if (WallAnims[w->clip_num].open_sound > -1 )
			digi_link_sound_to_pos( WallAnims[w->clip_num].open_sound, seg-Segments, side, &cp, 0, F1_0 );

	}
}

//-----------------------------------------------------------------
// Animates opening of a door.
// Called in the game loop.
void do_door_open(int door_num)
{
	int p;
	active_door *d;

	Assert(door_num != -1);		//Trying to do_door_open on illegal door
	
	d = &ActiveDoors[door_num];

	for (p=0;p<d->n_parts;p++) {
		wall *w;
		int Connectside, side;
		segment *csegp, *seg;
		fix time_elapsed, time_total, one_frame;
		int i, n;
	
		w = &Walls[d->front_wallnum[p]];
		kill_stuck_objects(d->front_wallnum[p]);
		kill_stuck_objects(d->back_wallnum[p]);

		seg = &Segments[w->segnum];
		side = w->sidenum;
	
		Assert(seg->sides[side].wall_num != -1);		//Trying to do_door_open on illegal wall
	
		csegp = &Segments[seg->children[side]];
		Connectside = find_connect_side(seg, csegp);
		Assert(Connectside != -1);

		d->time += FrameTime;
	
		time_elapsed = d->time;
		n = WallAnims[w->clip_num].num_frames;
		time_total = WallAnims[w->clip_num].play_time;
	
		one_frame = time_total/n;	
	
		i = time_elapsed/one_frame;
	
		if (i < n)
			wall_set_tmap_num(seg,side,csegp,Connectside,w->clip_num,i);
	
		if (i> n/2) {
			Walls[seg->sides[side].wall_num].flags |= WALL_DOOR_OPENED;
			Walls[csegp->sides[Connectside].wall_num].flags |= WALL_DOOR_OPENED;
		}
	
		if (i >= n-1) {
			wall_set_tmap_num(seg,side,csegp,Connectside,w->clip_num,n-1);

			// If our door is not automatic just remove it from the list.
			if (!(Walls[seg->sides[side].wall_num].flags & WALL_DOOR_AUTO)) {
				for (i=door_num;i<Num_open_doors;i++)
					ActiveDoors[i] = ActiveDoors[i+1];
				Num_open_doors--;
				Walls[seg->sides[side].wall_num].state = WALL_DOOR_OPEN;
				Walls[csegp->sides[Connectside].wall_num].state = WALL_DOOR_OPEN;
			}
			else {

				Walls[seg->sides[side].wall_num].state = WALL_DOOR_WAITING;
				Walls[csegp->sides[Connectside].wall_num].state = WALL_DOOR_WAITING;

				ActiveDoors[Num_open_doors].time = 0;	//counts up
			}
		}

	}

}

//-----------------------------------------------------------------
// Animates and processes the closing of a door.
// Called from the game loop.
void do_door_close(int door_num)
{
	int p;
	active_door *d;
	wall *w;

	Assert(door_num != -1);		//Trying to do_door_open on illegal door
	
	d = &ActiveDoors[door_num];

	w = &Walls[d->front_wallnum[0]];

	//check for objects in doorway before closing
	if (w->flags & WALL_DOOR_AUTO)
		if (!is_door_free(&Segments[w->segnum],w->sidenum)) {
			digi_kill_sound_linked_to_segment(w->segnum,w->sidenum,-1);
			wall_open_door(&Segments[w->segnum],w->sidenum);		//re-open door
			return;
		}

	for (p=0;p<d->n_parts;p++) {
		wall *w;
		int Connectside, side;
		segment *csegp, *seg;
		fix time_elapsed, time_total, one_frame;
		int i, n;
	
		w = &Walls[d->front_wallnum[p]];

		seg = &Segments[w->segnum];
		side = w->sidenum;
	
		if (seg->sides[side].wall_num == -1) {
			mprintf((0, "Trying to do_door_close on Illegal wall\n"));
			return;
		}
	
		//if here, must be auto door
//		Assert(Walls[seg->sides[side].wall_num].flags & WALL_DOOR_AUTO);		
//don't assert here, because now we have triggers to close non-auto doors
	
		// Otherwise, close it.
		csegp = &Segments[seg->children[side]];
		Connectside = find_connect_side(seg, csegp);
		Assert(Connectside != -1);
	

		if ( Newdemo_state != ND_STATE_PLAYBACK )
			// NOTE THE LINK TO ABOVE!!
			if (p==0)	//only play one sound for linked doors
				if ( d->time==0 )	{		//first time
					vms_vector cp;
					compute_center_point_on_side(&cp, seg, side );
					if (WallAnims[w->clip_num].close_sound  > -1 )
						digi_link_sound_to_pos( WallAnims[Walls[seg->sides[side].wall_num].clip_num].close_sound, seg-Segments, side, &cp, 0, F1_0 );
				}
	
		d->time += FrameTime;

		time_elapsed = d->time;
		n = WallAnims[w->clip_num].num_frames;
		time_total = WallAnims[w->clip_num].play_time;
	
		one_frame = time_total/n;	
	
		i = n-time_elapsed/one_frame-1;
	
		if (i < n/2) {
			Walls[seg->sides[side].wall_num].flags &= ~WALL_DOOR_OPENED;
			Walls[csegp->sides[Connectside].wall_num].flags &= ~WALL_DOOR_OPENED;
		}
	
		// Animate door.
		if (i > 0) {
			wall_set_tmap_num(seg,side,csegp,Connectside,w->clip_num,i);

			Walls[seg->sides[side].wall_num].state = WALL_DOOR_CLOSING;
			Walls[csegp->sides[Connectside].wall_num].state = WALL_DOOR_CLOSING;

			ActiveDoors[Num_open_doors].time = 0;		//counts up

		} else
			wall_close_door_num(door_num);
	}
}


//-----------------------------------------------------------------
// Turns off an illusionary wall (This will be used primarily for
//  wall switches or triggers that can turn on/off illusionary walls.)
void wall_illusion_off(segment *seg, int side)
{
	segment *csegp;
	int cside;

	csegp = &Segments[seg->children[side]];
	cside = find_connect_side(seg, csegp);
	Assert(cside != -1);

	if (seg->sides[side].wall_num == -1) {
		mprintf((0, "Trying to shut off illusion illegal wall\n"));
		return;
	}

	Walls[seg->sides[side].wall_num].flags |= WALL_ILLUSION_OFF;
	Walls[csegp->sides[cside].wall_num].flags |= WALL_ILLUSION_OFF;

	kill_stuck_objects(seg->sides[side].wall_num);
	kill_stuck_objects(csegp->sides[cside].wall_num);
}

//-----------------------------------------------------------------
// Turns on an illusionary wall (This will be used primarily for
//  wall switches or triggers that can turn on/off illusionary walls.)
void wall_illusion_on(segment *seg, int side)
{
	segment *csegp;
	int cside;

	csegp = &Segments[seg->children[side]];
	cside = find_connect_side(seg, csegp);
	Assert(cside != -1);

	if (seg->sides[side].wall_num == -1) {
		mprintf((0, "Trying to turn on illusion illegal wall\n"));
		return;
	}

	Walls[seg->sides[side].wall_num].flags &= ~WALL_ILLUSION_OFF;
	Walls[csegp->sides[cside].wall_num].flags &= ~WALL_ILLUSION_OFF;
}

//	-----------------------------------------------------------------------------
//	Allowed to open the normally locked special boss door if in multiplayer mode.
int special_boss_opening_allowed(int segnum, int sidenum)
{
	if (Game_mode & GM_MULTI)
		return (Current_level_num == BOSS_LOCKED_DOOR_LEVEL) && (segnum == BOSS_LOCKED_DOOR_SEG) && (sidenum == BOSS_LOCKED_DOOR_SIDE);
	else
		return 0;
}

//-----------------------------------------------------------------
// Determines what happens when a wall is shot
//returns info about wall.  see wall.h for codes
//obj is the object that hit...either a weapon or the player himself
//playernum is the number the player who hit the wall or fired the weapon,
//or -1 if a robot fired the weapon
int wall_hit_process(segment *seg, int side, fix damage, int playernum, object *obj )
{
	wall	*w;
	fix	show_message;

	Assert (seg-Segments != -1);

	// If it is not a "wall" then just return.
	if ( seg->sides[side].wall_num < 0 )
		return WHP_NOT_SPECIAL;

	w = &Walls[seg->sides[side].wall_num];

	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_wall_hit_process( seg-Segments, side, damage, playernum );

	if (w->type == WALL_BLASTABLE) {
		if (obj->ctype.laser_info.parent_type == OBJ_PLAYER)
			wall_damage(seg, side, damage);
		return WHP_BLASTABLE;
	}

	if (playernum != Player_num)	//return if was robot fire
		return WHP_NOT_SPECIAL;

	Assert( playernum > -1 );
	
	//	Determine whether player is moving forward.  If not, don't say negative
	//	messages because he probably didn't intentionally hit the door.
	if (obj->type == OBJ_PLAYER)
		show_message = (vm_vec_dot(&obj->orient.fvec, &obj->mtype.phys_info.velocity) > 0);
	else if (obj->type == OBJ_ROBOT)
		show_message = 0;
	else if ((obj->type == OBJ_WEAPON) && (obj->ctype.laser_info.parent_type == OBJ_ROBOT))
		show_message = 0;
	else
		show_message = 1;

	if (w->keys == KEY_BLUE)
		if (!(Players[playernum].flags & PLAYER_FLAGS_BLUE_KEY)) {
			if ( playernum==Player_num )
				if (show_message)
					HUD_init_message("%s %s",TXT_BLUE,TXT_ACCESS_DENIED);
			return WHP_NO_KEY;
		}

	if (w->keys == KEY_RED)
		if (!(Players[playernum].flags & PLAYER_FLAGS_RED_KEY)) {
			if ( playernum==Player_num )
				if (show_message)
					HUD_init_message("%s %s",TXT_RED,TXT_ACCESS_DENIED);
			return WHP_NO_KEY;
		}
	
	if (w->keys == KEY_GOLD)
		if (!(Players[playernum].flags & PLAYER_FLAGS_GOLD_KEY)) {
			if ( playernum==Player_num )
				if (show_message)
					HUD_init_message("%s %s",TXT_YELLOW,TXT_ACCESS_DENIED);
			return WHP_NO_KEY;
		}

	if (w->type == WALL_DOOR)
	{
		if ((w->flags & WALL_DOOR_LOCKED ) && !(special_boss_opening_allowed(seg-Segments, side)) ) {
			if ( playernum==Player_num )
				if (show_message)
					HUD_init_message(TXT_CANT_OPEN_DOOR);
			return WHP_NO_KEY;
		}
		else {
			if (w->state != WALL_DOOR_OPENING)
			{
				wall_open_door(seg, side);
			#ifdef NETWORK
				if (Game_mode & GM_MULTI)
					multi_send_door_open(seg-Segments, side,w->flags);
			#endif
			}
			return WHP_DOOR;
			
		}
	}

	return WHP_NOT_SPECIAL;		//default is treat like normal wall
}

//-----------------------------------------------------------------
// Opens doors/destroys wall/shuts off triggers.
void wall_toggle(segment *seg, int side)
{
	int wall_num;

	if (seg - Segments > Highest_segment_index)
	{
		Warning("Can't toggle side %d of segment %d - nonexistent segment!\n", side, seg-Segments);
		return;
	}
	Assert( side < MAX_SIDES_PER_SEGMENT );

	wall_num = seg->sides[side].wall_num;

	if (wall_num == -1) {
	 	mprintf((0, "Illegal wall_toggle\n"));
		return;
	}

	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_wall_toggle(seg-Segments, side );

	if (Walls[wall_num].type == WALL_BLASTABLE)
		wall_destroy(seg, side);

	if ((Walls[wall_num].type == WALL_DOOR) && (Walls[wall_num].state == WALL_DOOR_CLOSED))
		wall_open_door(seg, side);

}


//-----------------------------------------------------------------
// Tidy up Walls array for load/save purposes.
void reset_walls()
{
	int i;

	if (Num_walls < 0) {
		mprintf((0, "Illegal Num_walls\n"));
		return;
	}

	for (i=Num_walls;i<MAX_WALLS;i++) {
		Walls[i].type = WALL_NORMAL;
		Walls[i].flags = 0;
		Walls[i].hps = 0;
		Walls[i].trigger = -1;
		Walls[i].clip_num = -1;
		}
}

void do_cloaking_wall_frame(int cloaking_wall_num)
{
	cloaking_wall *d;
	wall *wfront,*wback;

	if ( Newdemo_state==ND_STATE_PLAYBACK ) return;

	d = &CloakingWalls[cloaking_wall_num];
	wfront = &Walls[d->front_wallnum];
	wback = (d->back_wallnum > -1) ? Walls + d->back_wallnum : NULL;

	d->time += FrameTime;

	if (d->time > CLOAKING_WALL_TIME) {
		int i;

		wfront->type = WALL_OPEN;
		wfront->state = WALL_DOOR_CLOSED;		//why closed? why not?
		if (wback) {
			wback->type = WALL_OPEN;
			wback->state = WALL_DOOR_CLOSED;		//why closed? why not?
		}

		for (i=cloaking_wall_num;i<Num_cloaking_walls;i++)
			CloakingWalls[i] = CloakingWalls[i+1];
		Num_cloaking_walls--;

	}
	else if (d->time > CLOAKING_WALL_TIME/2) {
		int old_type=wfront->type;

		wfront->cloak_value = ((d->time - CLOAKING_WALL_TIME/2) * (GR_FADE_LEVELS-2)) / (CLOAKING_WALL_TIME/2);
		if (wback)
			wback->cloak_value = wfront->cloak_value;

		if (old_type != WALL_CLOAKED) {		//just switched
			int i;

			wfront->type = WALL_CLOAKED;
			if (wback)
				wback->type = WALL_CLOAKED;

			for (i=0;i<4;i++) {
				Segments[wfront->segnum].sides[wfront->sidenum].uvls[i].l = d->front_ls[i];
				if (wback)
					Segments[wback->segnum].sides[wback->sidenum].uvls[i].l = d->back_ls[i];
			}
		}
	}
	else {		//fading out
		fix light_scale;
		int i;

		light_scale = fixdiv(CLOAKING_WALL_TIME/2-d->time,CLOAKING_WALL_TIME/2);

		for (i=0;i<4;i++) {
			Segments[wfront->segnum].sides[wfront->sidenum].uvls[i].l = fixmul(d->front_ls[i],light_scale);
			if (wback)
				Segments[wback->segnum].sides[wback->sidenum].uvls[i].l = fixmul(d->back_ls[i],light_scale);
		}
	}

	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_cloaking_wall(d->front_wallnum, d->back_wallnum, wfront->type, wfront->state, wfront->cloak_value, Segments[wfront->segnum].sides[wfront->sidenum].uvls[0].l, Segments[wfront->segnum].sides[wfront->sidenum].uvls[1].l, Segments[wfront->segnum].sides[wfront->sidenum].uvls[2].l, Segments[wfront->segnum].sides[wfront->sidenum].uvls[3].l);

}

void do_decloaking_wall_frame(int cloaking_wall_num)
{
	cloaking_wall *d;
	wall *wfront,*wback;

	if ( Newdemo_state==ND_STATE_PLAYBACK ) return;

	d = &CloakingWalls[cloaking_wall_num];
	wfront = &Walls[d->front_wallnum];
	wback = (d->back_wallnum > -1) ? Walls + d->back_wallnum : NULL;

	d->time += FrameTime;

	if (d->time > CLOAKING_WALL_TIME) {
		int i;

		wfront->state = WALL_DOOR_CLOSED;
		if (wback)
			wback->state = WALL_DOOR_CLOSED;

		for (i=0;i<4;i++) {
			Segments[wfront->segnum].sides[wfront->sidenum].uvls[i].l = d->front_ls[i];
			if (wback)
				Segments[wback->segnum].sides[wback->sidenum].uvls[i].l = d->back_ls[i];
		}

		for (i=cloaking_wall_num;i<Num_cloaking_walls;i++)
			CloakingWalls[i] = CloakingWalls[i+1];
		Num_cloaking_walls--;

	}
	else if (d->time > CLOAKING_WALL_TIME/2) {		//fading in
		fix light_scale;
		int i;

		wfront->type = wback->type = WALL_CLOSED;

		light_scale = fixdiv(d->time-CLOAKING_WALL_TIME/2,CLOAKING_WALL_TIME/2);

		for (i=0;i<4;i++) {
			Segments[wfront->segnum].sides[wfront->sidenum].uvls[i].l = fixmul(d->front_ls[i],light_scale);
			if (wback)
			Segments[wback->segnum].sides[wback->sidenum].uvls[i].l = fixmul(d->back_ls[i],light_scale);
		}
	}
	else {		//cloaking in
		wfront->cloak_value = ((CLOAKING_WALL_TIME/2 - d->time) * (GR_FADE_LEVELS-2)) / (CLOAKING_WALL_TIME/2);
		wfront->type = WALL_CLOAKED;
		if (wback) {
			wback->cloak_value = wfront->cloak_value;
			wback->type = WALL_CLOAKED;
		}
	}

	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_cloaking_wall(d->front_wallnum, d->back_wallnum, wfront->type, wfront->state, wfront->cloak_value, Segments[wfront->segnum].sides[wfront->sidenum].uvls[0].l, Segments[wfront->segnum].sides[wfront->sidenum].uvls[1].l, Segments[wfront->segnum].sides[wfront->sidenum].uvls[2].l, Segments[wfront->segnum].sides[wfront->sidenum].uvls[3].l);

}

void wall_frame_process()
{
	int i;

	for (i=0;i<Num_open_doors;i++) {
		active_door *d;
		wall *w;

		d = &ActiveDoors[i];
		w = &Walls[d->front_wallnum[0]];

		if (w->state == WALL_DOOR_OPENING)
			do_door_open(i);
		else if (w->state == WALL_DOOR_CLOSING)
			do_door_close(i);
		else if (w->state == WALL_DOOR_WAITING) {
			d->time += FrameTime;

			//set flags to fix occatsional netgame problem where door is
			//waiting to close but open flag isn't set
			Assert(d->n_parts == 1);
			w->flags |= WALL_DOOR_OPENED;
			if (d->back_wallnum[0] > -1)
				Walls[d->back_wallnum[0]].flags |= WALL_DOOR_OPENED;

			if (d->time > DOOR_WAIT_TIME && is_door_free(&Segments[w->segnum],w->sidenum)) {
				w->state = WALL_DOOR_CLOSING;
				d->time = 0;
			}
		}
		else if (w->state == WALL_DOOR_CLOSED || w->state == WALL_DOOR_OPEN) {
			//this shouldn't happen.  if the wall is in one of these states,
			//there shouldn't be an activedoor entry for it.  So we'll kill
			//the activedoor entry.  Tres simple.
			int t;
			Int3();		//a bad thing has happened, but I'll try to fix it up
			for (t=i;t<Num_open_doors;t++)
				ActiveDoors[t] = ActiveDoors[t+1];
			Num_open_doors--;
		}
	}

	for (i=0;i<Num_cloaking_walls;i++) {
		cloaking_wall *d;
		wall *w;

		d = &CloakingWalls[i];
		w = &Walls[d->front_wallnum];

		if (w->state == WALL_DOOR_CLOAKING)
			do_cloaking_wall_frame(i);
		else if (w->state == WALL_DOOR_DECLOAKING)
			do_decloaking_wall_frame(i);
#ifdef _DEBUG
		else
			Int3();	//unexpected wall state
#endif
	}
}

int	Num_stuck_objects=0;

stuckobj	Stuck_objects[MAX_STUCK_OBJECTS];

//	An object got stuck in a door (like a flare).
//	Add global entry.
void add_stuck_object(object *objp, int segnum, int sidenum)
{
	int	i;
	int	wallnum;

	wallnum = Segments[segnum].sides[sidenum].wall_num;

	if (wallnum != -1) {
		if (Walls[wallnum].flags & WALL_BLASTED)
			objp->flags |= OF_SHOULD_BE_DEAD;

		for (i=0; i<MAX_STUCK_OBJECTS; i++) {
			if (Stuck_objects[i].wallnum == -1) {
				Stuck_objects[i].wallnum = wallnum;
				Stuck_objects[i].objnum = objp-Objects;
				Stuck_objects[i].signature = objp->signature;
				// mprintf((0, "Added wall %i at index %i\n", wallnum, i));
				Num_stuck_objects++;
				break;
			}
		}
		if (i == MAX_STUCK_OBJECTS)
			mprintf((1, "Warning: Unable to add object %i which got stuck in wall %i to Stuck_objects\n", objp-Objects, wallnum));
	}



}

//	--------------------------------------------------------------------------------------------------
//	Look at the list of stuck objects, clean up in case an object has gone away, but not been removed here.
//	Removes up to one/frame.
void remove_obsolete_stuck_objects(void)
{
	int	objnum;

	//	Safety and efficiency code.  If no stuck objects, should never get inside the IF, but this is faster.
	if (!Num_stuck_objects)
		return;

	objnum = FrameCount % MAX_STUCK_OBJECTS;

	if (Stuck_objects[objnum].wallnum != -1)
		if ((Walls[Stuck_objects[objnum].wallnum].state != WALL_DOOR_CLOSED) || (Objects[Stuck_objects[objnum].objnum].signature != Stuck_objects[objnum].signature)) {
			Num_stuck_objects--;
			Objects[Stuck_objects[objnum].objnum].lifeleft = F1_0/8;
			Stuck_objects[objnum].wallnum = -1;
		}

}

extern void flush_fcd_cache(void);

//	----------------------------------------------------------------------------------------------------
//	Door with wall index wallnum is opening, kill all objects stuck in it.
void kill_stuck_objects(int wallnum)
{
	int	i;

	if (wallnum < 0 || Num_stuck_objects == 0) {
		return;
	}

	Num_stuck_objects=0;

	for (i=0; i<MAX_STUCK_OBJECTS; i++)
		if (Stuck_objects[i].wallnum == wallnum) {
			if (Objects[Stuck_objects[i].objnum].type == OBJ_WEAPON) {
				Objects[Stuck_objects[i].objnum].lifeleft = F1_0/8;
			} else
				mprintf((1, "Warning: Stuck object of type %i, expected to be of type %i, see wall.c\n", Objects[Stuck_objects[i].objnum].type, OBJ_WEAPON));
				// Int3();	//	What?  This looks bad.  Object is not a weapon and it is stuck in a wall!
			Stuck_objects[i].wallnum = -1;
		} else if (Stuck_objects[i].wallnum != -1) {
			Num_stuck_objects++;
		}
	//	Ok, this is awful, but we need to do things whenever a door opens/closes/disappears, etc.
	flush_fcd_cache();

}


// -- unused -- // -----------------------------------------------------------------------------------
// -- unused -- //	Return object id of first flare found embedded in segp:sidenum.
// -- unused -- //	If no flare, return -1.
// -- unused -- int contains_flare(segment *segp, int sidenum)
// -- unused -- {
// -- unused -- 	int	i;
// -- unused --
// -- unused -- 	for (i=0; i<Num_stuck_objects; i++) {
// -- unused -- 		object	*objp = &Objects[Stuck_objects[i].objnum];
// -- unused --
// -- unused -- 		if ((objp->type == OBJ_WEAPON) && (objp->id == FLARE_ID)) {
// -- unused -- 			if (Walls[Stuck_objects[i].wallnum].segnum == segp-Segments)
// -- unused -- 				if (Walls[Stuck_objects[i].wallnum].sidenum == sidenum)
// -- unused -- 					return objp-Objects;
// -- unused -- 		}
// -- unused -- 	}
// -- unused --
// -- unused -- 	return -1;
// -- unused -- }

// -----------------------------------------------------------------------------------
// Initialize stuck objects array.  Called at start of level
void init_stuck_objects(void)
{
	int	i;

	for (i=0; i<MAX_STUCK_OBJECTS; i++)
		Stuck_objects[i].wallnum = -1;

	Num_stuck_objects = 0;
}

// -----------------------------------------------------------------------------------
// Clear out all stuck objects.  Called for a new ship
void clear_stuck_objects(void)
{
	int	i;

	for (i=0; i<MAX_STUCK_OBJECTS; i++) {
		if (Stuck_objects[i].wallnum != -1) {
			int	objnum;

			objnum = Stuck_objects[i].objnum;

			if ((Objects[objnum].type == OBJ_WEAPON) && (Objects[objnum].id == FLARE_ID))
				Objects[objnum].lifeleft = F1_0/8;

			Stuck_objects[i].wallnum = -1;

			Num_stuck_objects--;
		}
	}

	Assert(Num_stuck_objects == 0);

}

// -----------------------------------------------------------------------------------
#define	MAX_BLAST_GLASS_DEPTH	5

void bng_process_segment(object *objp, fix damage, segment *segp, int depth, sbyte *visited)
{
	int	i, sidenum;

	if (depth > MAX_BLAST_GLASS_DEPTH)
		return;

	depth++;

	for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
		int			tm;
		fix			dist;
		vms_vector	pnt;

		//	Process only walls which have glass.
		if ((tm=segp->sides[sidenum].tmap_num2) != 0) {
			int	ec, db;

			tm &= 0x3fff;			//tm without flags

			if ((((ec=TmapInfo[tm].eclip_num)!=-1) && ((db=Effects[ec].dest_bm_num)!=-1 && !(Effects[ec].flags&EF_ONE_SHOT))) ||	(ec==-1 && (TmapInfo[tm].destroyed!=-1))) {
				compute_center_point_on_side(&pnt, segp, sidenum);
				dist = vm_vec_dist_quick(&pnt, &objp->pos);
				if (dist < damage/2) {
					dist = find_connected_distance(&pnt, segp-Segments, &objp->pos, objp->segnum, MAX_BLAST_GLASS_DEPTH, WID_RENDPAST_FLAG);
					if ((dist > 0) && (dist < damage/2))
						check_effect_blowup(segp, sidenum, &pnt, &Objects[objp->ctype.laser_info.parent_num], 1);
				}
			}
		}
	}

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		int	segnum = segp->children[i];

		if (segnum != -1) {
			if (!visited[segnum]) {
				if (WALL_IS_DOORWAY(segp, i) & WID_FLY_FLAG) {
					visited[segnum] = 1;
					bng_process_segment(objp, damage, &Segments[segnum], depth, visited);
				}
			}
		}
	}
}

// -----------------------------------------------------------------------------------
//	objp is going to detonate
//	blast nearby monitors, lights, maybe other things
void blast_nearby_glass(object *objp, fix damage)
{
	int		i;
	sbyte   visited[MAX_SEGMENTS];
	segment	*cursegp;

	cursegp = &Segments[objp->segnum];
	for (i=0; i<=Highest_segment_index; i++)
		visited[i] = 0;

	visited[objp->segnum] = 1;
	bng_process_segment(objp, damage, cursegp, 0, visited);


}

#define MAX_CLIP_FRAMES_D1 20

/*
 * reads a wclip structure from a CFILE
 */
int wclip_read_n_d1(wclip *wc, int n, CFILE *fp)
{
	int i, j;

	for (i = 0; i < n; i++) {
		wc[i].play_time = cfile_read_fix(fp);
		wc[i].num_frames = cfile_read_short(fp);
		for (j = 0; j < MAX_CLIP_FRAMES_D1; j++)
			wc[i].frames[j] = cfile_read_short(fp);
		wc[i].open_sound = cfile_read_short(fp);
		wc[i].close_sound = cfile_read_short(fp);
		wc[i].flags = cfile_read_short(fp);
		cfread(wc[i].filename, 13, 1, fp);
		wc[i].pad = cfile_read_byte(fp);
	}
	return i;
}

#ifndef FAST_FILE_IO
/*
 * reads a wclip structure from a CFILE
 */
int wclip_read_n(wclip *wc, int n, CFILE *fp)
{
	int i, j;

	for (i = 0; i < n; i++) {
		wc[i].play_time = cfile_read_fix(fp);
		wc[i].num_frames = cfile_read_short(fp);
		for (j = 0; j < MAX_CLIP_FRAMES; j++)
			wc[i].frames[j] = cfile_read_short(fp);
		wc[i].open_sound = cfile_read_short(fp);
		wc[i].close_sound = cfile_read_short(fp);
		wc[i].flags = cfile_read_short(fp);
		cfread(wc[i].filename, 13, 1, fp);
		wc[i].pad = cfile_read_byte(fp);
	}
	return i;
}

/*
 * reads a v16_wall structure from a CFILE
 */
extern void v16_wall_read(v16_wall *w, CFILE *fp)
{
	w->type = cfile_read_byte(fp);
	w->flags = cfile_read_byte(fp);
	w->hps = cfile_read_fix(fp);
	w->trigger = cfile_read_byte(fp);
	w->clip_num = cfile_read_byte(fp);
	w->keys = cfile_read_byte(fp);
}

/*
 * reads a v19_wall structure from a CFILE
 */
extern void v19_wall_read(v19_wall *w, CFILE *fp)
{
	w->segnum = cfile_read_int(fp);
	w->sidenum = cfile_read_int(fp);
	w->type = cfile_read_byte(fp);
	w->flags = cfile_read_byte(fp);
	w->hps = cfile_read_fix(fp);
	w->trigger = cfile_read_byte(fp);
	w->clip_num = cfile_read_byte(fp);
	w->keys = cfile_read_byte(fp);
	w->linked_wall = cfile_read_int(fp);
}

/*
 * reads a wall structure from a CFILE
 */
extern void wall_read(wall *w, CFILE *fp)
{
	w->segnum = cfile_read_int(fp);
	w->sidenum = cfile_read_int(fp);
	w->hps = cfile_read_fix(fp);
	w->linked_wall = cfile_read_int(fp);
	w->type = cfile_read_byte(fp);
	w->flags = cfile_read_byte(fp);
	w->state = cfile_read_byte(fp);
	w->trigger = cfile_read_byte(fp);
	w->clip_num = cfile_read_byte(fp);
	w->keys = cfile_read_byte(fp);
	w->controlling_trigger = cfile_read_byte(fp);
	w->cloak_value = cfile_read_byte(fp);
}

/*
 * reads a v19_door structure from a CFILE
 */
extern void v19_door_read(v19_door *d, CFILE *fp)
{
	d->n_parts = cfile_read_int(fp);
	d->seg[0] = cfile_read_short(fp);
	d->seg[1] = cfile_read_short(fp);
	d->side[0] = cfile_read_short(fp);
	d->side[1] = cfile_read_short(fp);
	d->type[0] = cfile_read_short(fp);
	d->type[1] = cfile_read_short(fp);
	d->open = cfile_read_fix(fp);
}

/*
 * reads an active_door structure from a CFILE
 */
extern void active_door_read(active_door *ad, CFILE *fp)
{
	ad->n_parts = cfile_read_int(fp);
	ad->front_wallnum[0] = cfile_read_short(fp);
	ad->front_wallnum[1] = cfile_read_short(fp);
	ad->back_wallnum[0] = cfile_read_short(fp);
	ad->back_wallnum[1] = cfile_read_short(fp);
	ad->time = cfile_read_fix(fp);
}
#endif
