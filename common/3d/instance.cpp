/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 * 
 * Instancing routines
 * 
 */

#include <stdlib.h>
#include "dxxerror.h"

#include "3d.h"
#include "globvars.h"

namespace dcx {

//instance at specified point with specified orientation
//if matrix==NULL, don't modify matrix.  This will be like doing an offset   
g3_instance_context g3_start_instance_matrix()
{
	return {View_matrix, View_position};
}

g3_instance_context g3_start_instance_matrix(const vms_vector &pos, const vms_matrix &orient)
{
	auto r{g3_start_instance_matrix()};
	//step 1: vm_vec_sub: subtract object position from view position
	//step 2: vm_vec_rotate: rotate view vector through object matrix
	vm_vec_rotate(View_position, vm_vec_sub(View_position, pos), orient);

		//step 3: rotate object matrix through view_matrix (vm = ob * vm)
	View_matrix = vm_matrix_x_matrix(vm_transposed_matrix(orient), View_matrix);
	return r;
}


//instance at specified point with specified orientation
//if angles==NULL, don't modify matrix.  This will be like doing an offset
g3_instance_context g3_start_instance_angles(const vms_vector &pos, const vms_angvec &angles)
{
	const auto &&tm = vm_angles_2_matrix(angles);
	return g3_start_instance_matrix(pos, tm);
}


//pops the old context
void g3_done_instance(const g3_instance_context &ctx)
{
	View_position = ctx.position;
	View_matrix = ctx.matrix;
}

}
