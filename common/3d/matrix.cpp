/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 * 
 * Matrix setup & manipulation routines
 * 
 */
 

#include "3d.h"
#include "globvars.h"

namespace dcx {

static void scale_matrix(void);

//set view from x,y,z, viewer matrix, and zoom.  Must call one of g3_set_view_*() 
void g3_set_view_matrix(const vms_vector &view_pos,const vms_matrix &view_matrix,fix zoom)
{
	View_zoom = zoom;
	View_position = view_pos;
	View_matrix = view_matrix;
	scale_matrix();
}

//performs aspect scaling on global view matrix
static void scale_matrix(void)
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

	vm_vec_scale(View_matrix.rvec,Matrix_scale.x);
	vm_vec_scale(View_matrix.uvec,Matrix_scale.y);
	vm_vec_scale(View_matrix.fvec,Matrix_scale.z);

}

}
