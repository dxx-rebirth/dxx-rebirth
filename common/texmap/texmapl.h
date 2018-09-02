/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once
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
 * Local include file for texture mapper library -- NOT to be included by users.
 *
 */

//	Local include file for texture map library.

#include "maths.h"
#include "pstypes.h"

#ifdef __cplusplus
#include <cstddef>
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-array.h"

namespace dcx {

#if !DXX_USE_OGL
struct g3ds_tmap;

extern	int prevmod(int val,int modulus);
extern	int succmod(int val,int modulus);

fix compute_dx_dy(const g3ds_tmap &t, int top_vertex,int bottom_vertex, fix recip_dy);
void compute_y_bounds(const g3ds_tmap &t, int &vlt, int &vlb, int &vrt, int &vrb,int &bottom_y_ind);

extern int	fx_y,fx_xleft,fx_xright;
extern const unsigned char *pixptr;
// texture mapper scanline renderers
// Interface variables to assembler code
extern	fix	fx_u,fx_v,fx_z,fx_du_dx,fx_dv_dx,fx_dz_dx;
extern	fix	fx_dl_dx,fx_l;
extern	int	bytes_per_row;
extern  unsigned char *write_buffer;

extern uint8_t tmap_flat_color;

constexpr std::integral_constant<std::size_t, 641> FIX_RECIP_TABLE_SIZE{};	//increased from 321 to 641, since this res is now quite achievable.. slight fps boost -MM
extern const array<fix, FIX_RECIP_TABLE_SIZE> fix_recip_table;
static inline fix fix_recip(unsigned i)
{
	if (i < fix_recip_table.size())
		return fix_recip_table[i];
	else
		return F1_0 / i;
}
#endif

}

#endif

