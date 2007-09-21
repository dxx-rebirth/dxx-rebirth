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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/include/editor/info.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:46:43 $
 * 
 * Header for info.c
 * 
 * $Log: info.h,v $
 * Revision 1.1.1.1  2006/03/17 19:46:43  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:02:36  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:34:32  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.2  1994/05/14  17:18:17  matt
 * Got rid of externs in source (non-header) files
 * 
 * Revision 1.1  1994/05/14  16:30:39  matt
 * Initial revision
 * 
 * 
 */



#ifndef _INFO_H
#define _INFO_H

void info_display_all( UI_WINDOW * wnd );

extern int init_info;

#endif
