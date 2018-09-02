/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 * 
 * Global variables for 3d
 * 
 */


#include "3d.h"
#include "globvars.h"

namespace dcx {

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

//list of 2d coords



}
