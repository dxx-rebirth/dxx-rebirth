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
 * $Source: /cvs/cvsroot/d2x/3d/globvars.c,v $
 * $Revision: 1.3 $
 * $Author: bradleyb $
 * $Date: 2001-10-31 03:54:50 $
 * 
 * Global variables for 3d
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2001/01/31 15:17:48  bradleyb
 * Makefile and conf.h fixes
 *
 * Revision 1.1.1.1  2001/01/19 03:29:58  bradleyb
 * Import of d2x-0.0.8
 *
 * Revision 1.1.1.1  1999/06/14 21:57:45  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.2  1995/09/13  11:30:47  allender
 * added fCanv_w2 and vCanv_h2 for PPC implementation
 *
 * Revision 1.1  1995/05/05  08:50:48  allender
 * Initial revision
 *
 * Revision 1.1  1995/04/17  04:24:57  matt
 * Initial revision
 * 
 * 
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: globvars.c,v 1.3 2001-10-31 03:54:50 bradleyb Exp $";
#endif

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



