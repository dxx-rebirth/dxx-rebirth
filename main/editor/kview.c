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
 * $Source: /cvs/cvsroot/d2x/main/editor/kview.c,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 *
 * Functions for changing viewer's position
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:03:34  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:34:21  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.11  1993/12/02  12:39:41  matt
 * Removed extra includes
 * 
 * Revision 1.10  1993/11/16  13:47:54  john
 * Xchanged move away/closer
 * 
 * Revision 1.9  1993/11/16  13:45:32  john
 * Exchanged zoom in/out.
 * 
 * Revision 1.8  1993/11/05  17:32:56  john
 * added funcs
 * .,
 * 
 * Revision 1.7  1993/11/03  12:10:21  yuan
 * No keypress associated with chase mode
 * 
 * Revision 1.6  1993/11/02  17:06:55  yuan
 * Icon stuff added.
 * 
 * Revision 1.5  1993/11/01  12:48:59  yuan
 * Added Chase mode icon to status bar.
 * 
 * Revision 1.4  1993/10/29  19:12:55  yuan
 * Added diagnostic messages
 * 
 * Revision 1.3  1993/10/27  18:26:16  matt
 * Made zoom & related keys not do anything if no current view
 * 
 * Revision 1.2  1993/10/19  20:54:33  matt
 * Changed/cleaned up window updates
 * 
 * Revision 1.1  1993/10/13  18:53:34  john
 * Initial revision
 * 
 *
 */

#ifdef RCS
static char rcsid[] = "$Id: kview.c,v 1.1 2004-12-19 13:54:27 btb Exp $";
#endif

#include "inferno.h"
#include "editor.h"

// ---------- zoom control on current window ----------
int ZoomIn()
{
	if (!current_view) return 0.0;

	current_view->ev_zoom = fixmul(current_view->ev_zoom,62259);
	current_view->ev_changed = 1;
	return 1;
}

int ZoomOut()
{
	if (!current_view) return 0.0;

	current_view->ev_zoom = fixmul(current_view->ev_zoom,68985);
	current_view->ev_changed = 1;
	return 1;
}

// ---------- distance-of-viewer control on current window ----------
int MoveCloser()
{
	if (!current_view) return 0.0;

	current_view->ev_dist = fixmul(current_view->ev_dist,62259);
	current_view->ev_changed = 1;
	return 1;
}

int MoveAway()
{
	if (!current_view) return 0.0;

	current_view->ev_dist = fixmul(current_view->ev_dist,68985);
	current_view->ev_changed = 1;
	return 1;
}

// ---------- Toggle chase mode. ----------

int ToggleChaseMode()
{
	Funky_chase_mode = !Funky_chase_mode;
	set_view_target_from_segment(Cursegp);
    if (Funky_chase_mode == 1) {
        diagnostic_message("Chase mode ON.");
    }
    if (Funky_chase_mode == 0) {
        diagnostic_message("Chase mode OFF.");
    }
    return Funky_chase_mode;
}

