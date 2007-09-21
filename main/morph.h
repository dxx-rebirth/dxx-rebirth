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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/morph.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:43:14 $
 * 
 * Header for morph.c
 * 
 * $Log: morph.h,v $
 * Revision 1.1.1.1  2006/03/17 19:43:14  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:12:37  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:32:19  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.9  1995/01/04  12:20:46  john
 * Declearations to work better with game state save.
 * 
 * 
 * Revision 1.8  1995/01/03  20:38:44  john
 * Externed MAX_MORPH_OBJECTS
 * 
 * Revision 1.7  1994/09/26  17:28:33  matt
 * Made new multiple-object morph code work with the demo system
 * 
 * Revision 1.6  1994/09/26  15:40:17  matt
 * Allow multiple simultaneous morphing objects
 * 
 * Revision 1.5  1994/06/28  11:55:19  john
 * Made newdemo system record/play directly to/from disk, so
 * we don't need the 4 MB buffer anymore.
 * 
 * Revision 1.4  1994/06/16  13:57:40  matt
 * Added support for morphing objects in demos
 * 
 * Revision 1.3  1994/06/08  18:22:03  matt
 * Made morphing objects light correctly
 * 
 * Revision 1.2  1994/05/30  22:50:25  matt
 * Added morph effect for robots
 * 
 * Revision 1.1  1994/05/30  12:04:19  matt
 * Initial revision
 * 
 * 
 */



#ifndef _MORPH_H
#define _MORPH_H

#include "object.h"

#define MAX_VECS 200

typedef struct morph_data {
	object *obj;			//object which is morphing
	vms_vector morph_vecs[MAX_VECS];
	vms_vector morph_deltas[MAX_VECS];
	fix morph_times[MAX_VECS];
	int submodel_active[MAX_SUBMODELS];		//which submodels are active
	int n_morphing_points[MAX_SUBMODELS];	//how many active points in each part
	int submodel_startpoints[MAX_SUBMODELS];	//first point for each submodel
	int n_submodels_active;
	ubyte morph_save_control_type;
	ubyte morph_save_movement_type;
	physics_info morph_save_phys_info;
	int Morph_sig;
 } morph_data;

#define MAX_MORPH_OBJECTS 5

extern morph_data morph_objects[];

void morph_start(object *obj);
void draw_morph_object(object *obj);

//process the morphing object for one frame
void do_morph_frame(object *obj);

//called at the start of a level
void init_morphs();

extern morph_data *find_morph_data(object *obj);

#endif
 
