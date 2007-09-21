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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/include/editor/texpage.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:46:43 $
 * 
 * Definitions for texpage.c
 * 
 * $Log: texpage.h,v $
 * Revision 1.1.1.1  2006/03/17 19:46:43  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:02:41  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:34:31  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.3  1994/05/14  17:17:53  matt
 * Got rid of externs in source (non-header) files
 * 
 * Revision 1.2  1993/12/16  15:57:54  john
 * moved texture selection stuff to texpage.c
 * 
 * Revision 1.1  1993/12/16  15:22:34  john
 * Initial revision
 * 
 * 
 */



#ifndef _TEXPAGE_H
#define _TEXPAGE_H

#include "ui.h"

extern int TextureLights;
extern int TextureEffects;
extern int TextureMetals;

int texpage_grab_current(int n);
int texpage_goto_first();
void texpage_init( UI_WINDOW * win );
void texpage_close();
void texpage_do();

#endif
