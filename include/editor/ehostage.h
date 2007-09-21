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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/include/editor/ehostage.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:46:43 $
 * 
 * .
 * 
 * $Log: ehostage.h,v $
 * Revision 1.1.1.1  2006/03/17 19:46:43  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:02:35  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:35:13  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.2  1994/07/01  17:57:14  john
 * First version of not-working hostage system
 * 
 * 
 * Revision 1.1  1994/07/01  14:24:41  john
 * Initial revision
 * 
 * 
 */



#ifndef _EHOSTAGE_H
#define _EHOSTAGE_H

extern int do_hostage_dialog();
extern void hostage_close_window();
extern void do_hostage_window();

#endif
