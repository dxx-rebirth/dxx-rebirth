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
 * $Source: /cvs/cvsroot/d2x/main/editor/objpage.h,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 * 
 * object selection stuff.
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:02:40  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:35:32  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.8  1994/11/02  16:19:20  matt
 * Moved draw_model_picture() out of editor, and cleaned up code
 * 
 * Revision 1.7  1994/07/28  16:59:36  mike
 * objects containing objects.
 * 
 * Revision 1.6  1994/05/17  14:45:48  mike
 * Get object type and id from ObjectType and ObjectId.
 * 
 * Revision 1.5  1994/05/17  12:03:55  mike
 * Deal with little known fact that polygon object != robot.
 * 
 * Revision 1.4  1994/05/14  18:00:33  matt
 * Got rid of externs in source (non-header) files
 * 
 * Revision 1.3  1994/04/01  11:17:06  yuan
 * Added objects to objpage. Added buttons for easier tmap scrolling.
 * Objects are selected fully from objpage and add object menu or pad.
 * 
 * Revision 1.2  1993/12/16  17:26:27  john
 * Moved texture and object selection to texpage and objpage
 * 
 * Revision 1.1  1993/12/16  16:13:08  john
 * Initial revision
 * 
 * 
 */

#ifndef _OBJPAGE_H
#define _OBJPAGE_H

#include "ui.h"

int objpage_grab_current(int n);
int objpage_goto_first();

void objpage_init( UI_WINDOW *win );
void objpage_close();
void objpage_do();

extern void draw_robot_picture(int id, vms_angvec *orient_angles, int type);

#endif
