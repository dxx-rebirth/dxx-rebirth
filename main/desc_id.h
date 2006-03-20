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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/desc_id.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:43:11 $
 * 
 * Header file which contains string for id and timestamp information
 * 
 * $Log: desc_id.h,v $
 * Revision 1.1.1.1  2006/03/17 19:43:11  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:12:14  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:29:38  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.3  1994/10/19  09:52:57  allender
 * Added variable for bogus error number return when game exits
 * 
 * Revision 1.2  1994/10/18  16:43:52  allender
 * Added constants for id and time stamping
 * 
 * Revision 1.1  1994/10/17  09:56:47  allender
 * Initial revision
 * 
 * 
 */



#ifndef _DESC_ID_H
#define _DESC_ID_H

#ifdef __MSDOS__
#include <time.h>
#endif

#define DESC_ID_STR "Midway in our life's journey, I went astray"
#define DESC_ID_CHKSUM "alone in a dark wood."
#define DESC_DEAD_TIME "from the straight road and woke to find myself 000000000000"

extern char desc_id_exit_num;

#endif
 
