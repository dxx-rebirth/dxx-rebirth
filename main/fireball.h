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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/fireball.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:41:27 $
 * 
 * Header for fireball.c
 * 
 * $Log: fireball.h,v $
 * Revision 1.1.1.1  2006/03/17 19:41:27  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:12:17  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:27:03  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.13  1995/01/17  12:14:38  john
 * Made walls, object explosion vclips load at level start.
 * 
 * Revision 1.12  1995/01/13  15:41:52  rob
 * Added prototype for maybe_replace_powerup_with_energy
 * 
 * Revision 1.11  1994/11/17  16:28:36  rob
 * Changed maybe_drop_cloak_powerup to maybe_drop_net_powerup (more
 * generic and useful)
 * 
 * Revision 1.10  1994/10/12  08:03:42  mike
 * Prototype maybe_drop_cloak_powerup.
 * 
 * Revision 1.9  1994/10/11  12:24:39  matt
 * Cleaned up/change badass explosion calls
 * 
 * Revision 1.8  1994/09/07  16:00:34  mike
 * Add object pointer to parameter list of object_create_badass_explosion.
 * 
 * Revision 1.7  1994/09/02  14:00:39  matt
 * Simplified explode_object() & mutliple-stage explosions
 * 
 * Revision 1.6  1994/08/17  16:49:58  john
 * Added damaging fireballs, missiles.
 * 
 * Revision 1.5  1994/07/14  22:39:19  matt
 * Added exploding doors
 * 
 * Revision 1.4  1994/06/08  10:56:36  matt
 * Improved debris: now get submodel size from new POF files; debris now has
 * limited life; debris can now be blown up.
 * 
 * Revision 1.3  1994/04/01  13:35:44  matt
 * Added multiple-stage explosions
 * 
 * Revision 1.2  1994/02/17  11:33:32  matt
 * Changes in object system
 * 
 * Revision 1.1  1994/02/16  22:41:15  matt
 * Initial revision
 * 
 * 
 */



#ifndef _FIREBALL_H
#define _FIREBALL_H

//explosion types
#define ET_SPARKS			0		//little sparks, like when laser hits wall
#define ET_MULTI_START	1		//first part of multi-part explosion
#define ET_MULTI_SECOND	2		//second part of multi-part explosion

object *object_create_explosion(short segnum, vms_vector * position, fix size, int vclip_type );
object *object_create_muzzle_flash(short segnum, vms_vector * position, fix size, int vclip_type );

object *object_create_badass_explosion(object *objp, short segnum, 
		vms_vector * position, fix size, int vclip_type, 
		fix maxdamage, fix maxdistance, fix maxforce, int parent );

//blows up a badass weapon, creating the badass explosion
//return the explosion object
object *explode_badass_weapon(object *obj);

//blows up the player with a badass explosion
//return the explosion object
object *explode_badass_player(object *obj);

void explode_object(object *obj,fix delay_time);
void do_explosion_sequence(object *obj);
void do_debris_frame(object *obj);		//deal with debris for this frame

void draw_fireball(object *obj);

void explode_wall(int segnum,int sidenum);
void do_exploding_wall_frame(void);
void init_exploding_walls(void);
extern void maybe_drop_net_powerup(int powerup_type);
extern void maybe_replace_powerup_with_energy(object *del_obj);

extern int get_explosion_vclip(object *obj,int stage);

#endif
 
