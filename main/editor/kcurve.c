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
 * $Source: /cvs/cvsroot/d2x/main/editor/kcurve.c,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 *
 * Functions for curve stuff.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:03:20  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:35:29  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.17  1994/08/25  21:56:43  mike
 * IS_CHILD stuff.
 * 
 * Revision 1.16  1994/05/14  17:17:54  matt
 * Got rid of externs in source (non-header) files
 * 
 * Revision 1.15  1994/01/28  10:52:24  mike
 * Bind set_average_light_on_curside to DeleteCurve
 * 
 * Revision 1.14  1994/01/25  17:34:47  mike
 * Stupidly bound fix_bogus_uvs_all to delete curve.
 * 
 * Revision 1.13  1993/12/06  19:34:15  yuan
 * Fixed autosave so that undo restores Cursegp
 * and Markedsegp
 * 
 * Revision 1.12  1993/12/02  12:39:28  matt
 * Removed extra includes
 * 
 * Revision 1.11  1993/11/12  13:08:38  yuan
 * Fixed warning for concave segment so it appears after any
 * "less important" diagnostic messages.
 * 
 * Revision 1.10  1993/11/11  17:03:25  yuan
 * Fixed undo-status display
 * 
 * Revision 1.9  1993/11/11  15:55:11  yuan
 * Added undo messages.
 * 
 * Revision 1.8  1993/11/08  19:13:30  yuan
 * Added Undo command (not working yet)
 * 
 * Revision 1.7  1993/11/05  17:32:51  john
 * added funcs
 * .,
 * 
 * Revision 1.6  1993/10/29  19:12:41  yuan
 * Added diagnostic messages
 * 
 * Revision 1.5  1993/10/29  16:26:30  yuan
 * Added diagnostic messages for curve generation
 * 
 * Revision 1.4  1993/10/22  19:47:30  yuan
 * Can't build curve if Marked Seg has a segment attached.
 * 
 * Revision 1.3  1993/10/19  20:54:50  matt
 * Changed/cleaned up window updates
 * 
 * Revision 1.2  1993/10/14  13:52:17  mike
 * Add return value to AssignTexture
 * 
 * Revision 1.1  1993/10/13  18:53:11  john
 * Initial revision
 * 
 *
 */

#ifdef RCS
static char rcsid[] = "$Id: kcurve.c,v 1.1 2004-12-19 13:54:27 btb Exp $";
#endif

#include <string.h>

#include "inferno.h"
#include "editor.h"
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

