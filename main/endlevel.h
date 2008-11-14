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
 * Header for endlevel.c
 *
 */

#ifndef _OUTSIDE_H
#define _OUTSIDE_H

#include "object.h"

extern int Endlevel_sequence;
void do_endlevel_frame();
void stop_endlevel_sequence();
void start_endlevel_sequence();
void render_endlevel_frame(fix eye_offset);

void render_external_scene();
void draw_exit_model();
void free_endlevel_data();
void init_endlevel();

extern grs_bitmap *terrain_bitmap;  //*satellite_bitmap,*station_bitmap,
extern int exit_segnum;

//@@extern vms_vector mine_exit_point;
//@@extern object external_explosion;
//@@extern int ext_expl_playing;

//called for each level to load & setup the exit sequence
void load_endlevel_data(int level_num);

extern int exit_modelnum, destroyed_exit_modelnum;

#endif /* _OUTSIDE_H */
