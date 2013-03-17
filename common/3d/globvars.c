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
 * Global variables for 3d
 * 
 */


#include "3d.h"
#include "globvars.h"

vms_vector	View_position;
fix			View_zoom;

vms_matrix	Unscaled_matrix;	//before scaling
vms_matrix	View_matrix;

vms_vector	Window_scale;		//scaling for window aspect
vms_vector	Matrix_scale;		//how the matrix is scaled, window_scale * zoom

int			Canvas_width;		//the actual width
int			Canvas_height;		//the actual height

fix			Canv_w2;				//fixed-point width/2
fix			Canv_h2;				//fixed-point height/2

#ifdef __powerc
double		fCanv_w2;
double		fCanv_h2;
#endif

//vertex buffers for polygon drawing and clipping
g3s_point * Vbuf0[MAX_POINTS_IN_POLY];
g3s_point *Vbuf1[MAX_POINTS_IN_POLY];

//list of 2d coords
fix Vertex_list[MAX_POINTS_IN_POLY*2];



