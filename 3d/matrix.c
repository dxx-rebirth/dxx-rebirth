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
 * $Source: /cvs/cvsroot/d2x/3d/matrix.c,v $
 * $Revision: 1.3 $
 * $Author: bradleyb $
 * $Date: 2001-10-31 03:54:51 $
 * 
 * Matrix setup & manipulation routines
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2001/01/31 15:17:48  bradleyb
 * Makefile and conf.h fixes
 *
 * Revision 1.1.1.1  2001/01/19 03:29:58  bradleyb
 * Import of d2x-0.0.8
 *
 * Revision 1.1.1.1  1999/06/14 21:57:48  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.1  1995/05/05  08:52:11  allender
 * Initial revision
 *
 * Revision 1.1  1995/04/17  04:14:34  matt
 * Initial revision
 * 
 * 
 */
 
#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: matrix.c,v 1.3 2001-10-31 03:54:51 bradleyb Exp $";
#endif

#include "3d.h"
#include "globvars.h"

void scale_matrix(void);

//set view from x,y,z & p,b,h, zoom.  Must call one of g3_set_view_*() 
void g3_set_view_angles(vms_vector *view_pos,vms_angvec *view_orient,fix zoom)
{
	View_zoom = zoom;
	View_position = *view_pos;

	vm_angles_2_matrix(&View_matrix,view_orient);

#ifdef D1XD3D
	Win32_set_view_matrix ();
#endif

	scale_matrix();
}

//set view from x,y,z, viewer matrix, and zoom.  Must call one of g3_set_view_*() 
void g3_set_view_matrix(vms_vector *view_pos,vms_matrix *view_matrix,fix zoom)
{
	View_zoom = zoom;
	View_position = *view_pos;

	View_matrix = *view_matrix;

#ifdef D1XD3D
	Win32_set_view_matrix ();
#endif

	scale_matrix();
}

//performs aspect scaling on global view matrix
void scale_matrix(void)
{
	Unscaled_matrix = View_matrix;		//so we can use unscaled if we want

	Matrix_scale = Window_scale;

	if (View_zoom <= f1_0) 		//zoom in by scaling z

		Matrix_scale.z =  fixmul(Matrix_scale.z,View_zoom);

	else {			//zoom out by scaling x&y

		fix s = fixdiv(f1_0,View_zoom);

		Matrix_scale.x = fixmul(Matrix_scale.x,s);
		Matrix_scale.y = fixmul(Matrix_scale.y,s);
	}

	//now scale matrix elements

	vm_vec_scale(&View_matrix.rvec,Matrix_scale.x);
	vm_vec_scale(&View_matrix.uvec,Matrix_scale.y);
	vm_vec_scale(&View_matrix.fvec,Matrix_scale.z);

}

