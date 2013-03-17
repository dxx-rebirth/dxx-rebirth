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
 * Functions for curve stuff.
 *
 */


#include <string.h>

#include "inferno.h"
#include "editor.h"
#include "editor/esegment.h"
#include "kdefs.h"

static fix         r1scale, r4scale;
static int         curve;

int InitCurve()
{
	curve = 0;
    return 1;
}

int GenerateCurve()
{
    if ( (Markedsegp != 0) && !IS_CHILD(Markedsegp->children[Markedside])) {
		r1scale = r4scale = F1_0*20;
      autosave_mine( mine_filename );
      diagnostic_message("Curve Generated.");
		Update_flags |= UF_WORLD_CHANGED;
      curve = generate_curve(r1scale, r4scale);
		mine_changed = 1;
        if (curve == 1) {
            strcpy(undo_status[Autosave_count], "Curve Generation UNDONE.\n");
        }
        if (curve == 0) diagnostic_message("Cannot generate curve -- check Current segment.");
    }
    else diagnostic_message("Cannot generate curve -- check Marked segment.");
	warn_if_concave_segments();

	return 1;
}

int DecreaseR4()
{
	if (curve) {
	   Update_flags |= UF_WORLD_CHANGED;
	   delete_curve();
	   r4scale -= F1_0;
	   generate_curve(r1scale, r4scale);
      diagnostic_message("R4 vector decreased.");
	   mine_changed = 1;
		warn_if_concave_segments();
	}
	return 1;
}

int IncreaseR4()
{
	if (curve) {
	   Update_flags |= UF_WORLD_CHANGED;
	   delete_curve();
	   r4scale += F1_0;
	   generate_curve(r1scale, r4scale);
      diagnostic_message("R4 vector increased.");
	   mine_changed = 1;
		warn_if_concave_segments();
	}
	return 1;
}

int DecreaseR1()
{
	if (curve) {
	   Update_flags |= UF_WORLD_CHANGED;
	   delete_curve();
	   r1scale -= F1_0;
	   generate_curve(r1scale, r4scale);
      diagnostic_message("R1 vector decreased.");
	   mine_changed = 1;
		warn_if_concave_segments();
	}
	return 1;
}

int IncreaseR1()
{
	if (curve) {
	   Update_flags |= UF_WORLD_CHANGED;
	   delete_curve();
	   r1scale += F1_0;
	   generate_curve(r1scale, r4scale);
      diagnostic_message("R1 vector increased.");
	   mine_changed = 1;
		warn_if_concave_segments();
	}
	return 1;
}

int DeleteCurve()
{
// fix_bogus_uvs_all();
set_average_light_on_curside();

	if (curve) {
	   Update_flags |= UF_WORLD_CHANGED;
	   delete_curve();
	   curve = 0;
	   mine_changed = 1;
      diagnostic_message("Curve Deleted.");
		warn_if_concave_segments();
	}
	return 1;
}

int SetCurve()
{
	if (curve) curve = 0;
   //autosave_mine( mine_filename );
   //strcpy(undo_status[Autosave_count], "Curve Generation UNDONE.\n");
   return 1;
}

