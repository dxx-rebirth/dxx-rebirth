/* $Id: slew.h,v 1.2 2003-10-10 09:36:35 btb Exp $ */
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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Prototypes, etc., for slew system
 *
 * Old Log:
 * Revision 1.1  1995/05/16  16:03:09  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:33:05  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.5  1994/12/15  16:43:58  matt
 * Made slew functions compile out for release versions
 *
 * Revision 1.4  1994/02/17  11:32:41  matt
 * Changes in object system
 *
 * Revision 1.3  1994/01/05  10:53:42  john
 * New object code by John.
 *
 * Revision 1.2  1993/12/05  22:48:57  matt
 * Reworked include files in an attempt to cut down on build times
 *
 * Revision 1.1  1993/12/05  20:20:16  matt
 * Initial revision
 *
 *
 */


#ifndef _SLEW_H
#define _SLEW_H

#include "object.h"

//from slew.c

#ifndef RELEASE

void slew_init(object *obj);                // say this is slew obj
int slew_stop();                            // Stops object
void slew_reset_orient();                   // Resets orientation
int slew_frame(int dont_check_keys);        // Does slew frame

#else

#define slew_init(obj)
#define slew_stop(obj)
#define slew_reset_orient()
#define slew_frame(dont_check_keys)

#endif

#endif /* _SLEW_H */
