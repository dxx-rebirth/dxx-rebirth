/* $Id: desc_id.h,v 1.2 2003-10-10 09:36:34 btb Exp $ */
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
 * Header file which contains string for id and timestamp information
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:55:53  allender
 * Initial revision
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
 * Header for checksum stuff - No idea what for.
 *
 *
 */

#ifndef _DESC_ID_H
#define _DESC_ID_H

#define DESC_ID_LEN         40      //how long the id string can be
#define DESC_CHECKSUM_LEN   4       //checksum is 4 bytes
#define DESC_DEAD_TIME_LEN  8       //dead time is 8 bytes

#define DESC_ID_TAG         "Midway in our life's journey, I went astray"
#define DESC_ID_CHKSUM_TAG  "alone in a dark wood."
#define DESC_DEAD_TIME_TAG  "from the straight road and woke to find myself"

extern char desc_id_exit_num;

#endif /* _DESC_ID_H */
