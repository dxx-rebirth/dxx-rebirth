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
 *
 * Functions for changing viewer's position
 *
 */


#include "inferno.h"
#include "editor.h"
#include "editor/esegment.h"

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

