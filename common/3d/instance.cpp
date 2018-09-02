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

namespace {

struct instance_context {
	vms_matrix m;
	vms_vector p;
};

}

static array<instance_context, 5> instance_stack;

int instance_depth = 0;

//instance at specified point with specified orientation
//if matrix==NULL, don't modify matrix.  This will be like doing an offset   
void g3_start_instance_matrix()
{
	assert(instance_depth < instance_stack.size());

	instance_stack[instance_depth].m = View_matrix;
	instance_stack[instance_depth].p = View_position;
	instance_depth++;
}

void g3_start_instance_matrix(const vms_vector &pos, const vms_matrix &orient)
{
	g3_start_instance_matrix();
	//step 1: subtract object position from view position

		const auto tempv = vm_vec_sub(View_position, pos);
		//step 2: rotate view vector through object matrix

	vm_vec_rotate(View_position, tempv, orient);

		//step 3: rotate object matrix through view_matrix (vm = ob * vm)
	View_matrix = vm_matrix_x_matrix(vm_transposed_matrix(orient), View_matrix);
}


//instance at specified point with specified orientation
//if angles==NULL, don't modify matrix.  This will be like doing an offset
void g3_start_instance_angles(const vms_vector &pos, const vms_angvec &angles)
{
	const auto &&tm = vm_angles_2_matrix(angles);
	g3_start_instance_matrix(pos, tm);
}


//pops the old context
void g3_done_instance()
{
	instance_depth--;

	Assert(instance_depth >= 0);

	View_position = instance_stack[instance_depth].p;
	View_matrix = instance_stack[instance_depth].m;
}


}
