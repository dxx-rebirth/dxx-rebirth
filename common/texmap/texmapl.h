/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
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
#include "compiler-array.h"

struct g3ds_tmap;

extern	int prevmod(int val,int modulus);
extern	int succmod(int val,int modulus);
extern	void texture_map_flat(g3ds_tmap *t,int color);

extern fix compute_dx_dy(g3ds_tmap *t, int top_vertex,int bottom_vertex, fix recip_dy);
extern void compute_y_bounds(g3ds_tmap *t, int *vlt, int *vlb, int *vrt, int *vrb,int *bottom_y_ind);
extern void asm_tmap_scanline_lin_v(void);

extern int	fx_y,fx_xleft,fx_xright,per2_flag;
extern unsigned char tmap_flat_color;
extern unsigned char *pixptr;

// texture mapper scanline renderers
extern	void asm_tmap_scanline_per(void);
extern	void asm_pent_tmap_scanline_per(void);
extern	void asm_ppro_tmap_scanline_per(void);
//extern  void asm_tmap_scanline_per_doubled(void);
extern	void asm_tmap_scanline_lin(void);
//extern  void asm_tmap_scanline_lin_16(void);
//extern  void asm_tmap_scanline_per_16(void);
extern	void asm_tmap_scanline_lin_lighted(void);
extern  void asm_tmap_scanline_flat(void);
extern  void asm_tmap_scanline_shaded(void);
//extern  void asm_tmap_scanline_lin_lighted_k(void);
//extern  void asm_tmap_scanline_lin_rgb(void);
//extern  void asm_tmap_scanline_lin_rgb_16(void);
//extern  void asm_tmap_scanline_lin_rgb_16g(void);
//extern  void asm_tmap_scanline_lin_ld(void);
//extern  void asm_tmap_scanline_lin_sky(void);
//extern  void asm_tmap_scanline_lin_sky_v(void);

extern fix compute_dx_dy_lin(g3ds_tmap *t,int vlt,int vlb, fix recip_dy);
extern fix compute_du_dy_lin(g3ds_tmap *t,int vlt,int vlb, fix recip_dy);
extern fix compute_dv_dy_lin(g3ds_tmap *t,int vlt,int vlb, fix recip_dy);


// Interface variables to assembler code
extern	fix	fx_u,fx_v,fx_z,fx_du_dx,fx_dv_dx,fx_dz_dx;
extern	fix	fx_dl_dx,fx_l;
extern	int	fx_r,fx_g,fx_b,fx_dr_dx,fx_dg_dx,fx_db_dx;
extern	unsigned char *pixptr;

extern	int	bytes_per_row;
extern  unsigned char *write_buffer;
extern	int  	window_left;
extern	int	window_right;
extern	int	window_top;
extern	int	window_bottom;
extern	int  	window_width;
extern	int  	window_height;
extern	int	scan_doubling_flag;
extern	int	linear_if_far_flag;
extern	int	Interlacing_on;

extern ubyte * tmap_flat_cthru_table;
extern ubyte tmap_flat_color;
extern ubyte tmap_flat_shade_value;

static const std::size_t FIX_RECIP_TABLE_SIZE = 641;	//increased from 321 to 641, since this res is now quite achievable.. slight fps boost -MM
extern const array<fix, FIX_RECIP_TABLE_SIZE> fix_recip_table;

static inline fix fix_recip(unsigned i)
{
	if (i < fix_recip_table.size())
		return fix_recip_table[i];
	else
		return F1_0 / i;
}

#endif

