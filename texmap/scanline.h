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
 * $Source: /cvs/cvsroot/d2x/texmap/scanline.h,v $
 * $Revision: 1.2 $
 * $Author: bradleyb $
 * $Date: 2001-10-25 02:22:46 $
 * 
 * Prototypes for C versions of texture mapped scanlines.
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2001/01/19 03:30:16  bradleyb
 * Import of d2x-0.0.8
 *
 * Revision 1.1.1.1  1999/06/14 22:14:10  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.2  1995/02/20  18:23:40  john
 * Added new module for C versions of inner loops.
 * 
 * Revision 1.1  1995/02/20  17:44:16  john
 * Initial revision
 * 
 * 
 */



#ifndef _SCANLINE_H
#define _SCANLINE_H

extern void c_tmap_scanline_per();
extern void c_tmap_scanline_per_nolight();
extern void c_tmap_scanline_lin();
extern void c_tmap_scanline_lin_nolight();
extern void c_tmap_scanline_flat();
extern void c_tmap_scanline_shaded();

//typedef struct _tmap_scanline_funcs {
extern void (*cur_tmap_scanline_per)(void);
extern void (*cur_tmap_scanline_per_nolight)(void);
extern void (*cur_tmap_scanline_lin)(void);
extern void (*cur_tmap_scanline_lin_nolight)(void);
extern void (*cur_tmap_scanline_flat)(void);
extern void (*cur_tmap_scanline_shaded)(void);
//} tmap_scanline_funcs;

//extern tmap_scanline_funcs tmap_funcs;
void select_tmap(char *type);

#endif

