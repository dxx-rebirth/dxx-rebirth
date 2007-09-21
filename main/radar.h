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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/radar.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:41:45 $
 * 
 * Prototypes for radar.
 * 
 * $Log: radar.h,v $
 * Revision 1.1.1.1  2006/03/17 19:41:45  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:13:01  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:33:07  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.3  1994/07/07  14:59:03  john
 * Made radar powerups.
 * 
 * 
 * Revision 1.2  1994/07/06  19:36:36  john
 * Initial version of radar.
 * 
 * Revision 1.1  1994/07/06  17:22:18  john
 * Initial revision
 * 
 * 
 */



#ifndef _RADAR_H
#define _RADAR_H

void radar_render_frame();

#endif
 
//added/moved on 9/17/98 by Victor Rachels - radar toggle
extern int show_radar;
//end this section
//added on 11/12/98 by Victor Rachels for network radar
extern int Network_allow_radar;
//end this section addition - VR
