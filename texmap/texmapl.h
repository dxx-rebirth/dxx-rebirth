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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/texmap/texmapl.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:46:00 $
 *
 * Local include file for texture mapper library -- NOT to be included by users.
 *
 * $Log: texmapl.h,v $
 * Revision 1.1.1.1  2006/03/17 19:46:00  zicodxx
 * initial import
 *
 * Revision 1.2  1999/12/08 01:03:51  donut
 * allow runtime selection of tmap routines
 *
 * Revision 1.1.1.1  1999/06/14 22:14:11  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.14  1995/02/20  18:23:02  john
 * Put all the externs in the assembly modules into tmap_inc.asm.
 * Also, moved all the C versions of the inner loops into a new module, 
 * scanline.c.
 * 
 * Revision 1.13  1995/02/20  17:09:16  john
 * Added code so that you can build the tmapper with no assembly!
 * 
 * Revision 1.12  1994/11/28  13:34:34  mike
 * optimizations.
 * 
 * Revision 1.11  1994/11/12  16:41:27  mike
 * function prototype.
 * 
 * Revision 1.10  1994/05/24  17:30:00  mike
 * Prototype fix_recip, asm_tmap_scanline_lin_v.
 * 
 * Revision 1.9  1994/04/21  15:04:26  mike
 * Add prototype for texmapl.h
 * 
 * Revision 1.8  1994/03/31  08:34:53  mike
 * *** empty log message ***
 * 
 * Revision 1.7  1994/03/22  20:37:04  mike
 * *** empty log message ***
 * 
 * Revision 1.6  1994/03/14  15:43:03  mike
 * streamline code.
 * 
 * Revision 1.5  1994/01/31  15:43:18  mike
 * window_height, asm_tmap_scanline_lin_sky_v
 * 
 * Revision 1.4  1994/01/21  21:12:27  mike
 * Prototype asm_tmap_scanline_lin_sky
 * 
 * Revision 1.3  1994/01/14  14:01:45  mike
 * Add a bunch of variables.
 * 
 * Revision 1.2  1993/11/22  10:25:11  mike
 * *** empty log message ***
 * 
 * Revision 1.1  1993/09/08  17:29:13  mike
 * Initial revision
 * 
 *
 */

//	Local include file for texture map library.

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
extern fix compute_dx_dy_lin(g3ds_tmap *t,int vrt,int vrb, fix recip_dy);
extern fix compute_du_dy_lin(g3ds_tmap *t,int vlt,int vlb, fix recip_dy);
extern fix compute_du_dy_lin(g3ds_tmap *t,int vrt,int vrb, fix recip_dy);
extern fix compute_dv_dy_lin(g3ds_tmap *t,int vlt,int vlb, fix recip_dy);
extern fix compute_dv_dy_lin(g3ds_tmap *t,int vrt,int vrb, fix recip_dy);


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
extern	int	dither_intensity_lighting;
extern	int	Interlacing_on;

extern ubyte * tmap_flat_cthru_table;
extern ubyte tmap_flat_color;
extern ubyte tmap_flat_shade_value;


extern fix fix_recip[];

extern void init_interface_vars_to_assembler(void);
extern int prevmod(int val,int modulus);

