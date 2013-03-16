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
 * Functions for selecting segments
 *
 */


#include <string.h>

#include "inferno.h"
#include "editor/editor.h"
#include "editor/esegment.h"


// ---------------------------------------------------------------------------------------
// Select previous segment.
//	If there is a connection on the side opposite to the current side, then choose that segment.
// If there is no connecting segment on the opposite face, try any segment.
void get_previous_segment(int curseg_num, int curside,int *newseg_num, int *newside)
{
	int     s;

	*newseg_num = curseg_num;

        if (IS_CHILD(Segments[curseg_num].children[(int)Side_opposite[curside]]))
                *newseg_num = Segments[curseg_num].children[(int)Side_opposite[curside]];
	else        // no segment on opposite face, connect to anything
		for (s=0; s<MAX_SIDES_PER_SEGMENT; s++)
			if ((s != curside) && IS_CHILD(Segments[curseg_num].children[s]))
				*newseg_num = Segments[curseg_num].children[s];

	// Now make Curside point at the segment we just left (unless we couldn't leave it).
	if (*newseg_num != curseg_num)
		*newside = find_connect_side(&Segments[curseg_num],&Segments[*newseg_num]);
	else
		*newside = curside;
}


// --------------------------------------------------------------------------------------
// Select next segment.
//	If there is a connection on the current side, then choose that segment.
// If there is no connecting segment on the current side, try any segment.
void get_next_segment(int curseg_num, int curside, int *newseg_num, int *newside)
{
	int	s;

	if (IS_CHILD(Segments[curseg_num].children[curside])) {

		*newseg_num = Segments[curseg_num].children[Curside];

		// Find out what side we came in through and favor side opposite that
		*newside = Side_opposite[find_connect_side(&Segments[curseg_num],&Segments[*newseg_num])];

		// If there is nothing attached on the side opposite to what we came in (*newside), pick any other side
		if (!IS_CHILD(Segments[*newseg_num].children[*newside]))
			for (s=0; s<MAX_SIDES_PER_SEGMENT; s++)
				if ((Segments[*newseg_num].children[s] != curseg_num) && IS_CHILD(Segments[*newseg_num].children[s]))
					*newside = s;
	} else {
		*newseg_num = curseg_num;
		*newside = curside;
	}

}

// ---------- select current segment ----------
int SelectCurrentSegForward()
{
	int	newseg_num,newside;

	get_next_segment(Cursegp-Segments,Curside,&newseg_num,&newside);

	if (newseg_num != Cursegp-Segments) {
		Cursegp = &Segments[newseg_num];
		Curside = newside;
		Update_flags |= UF_ED_STATE_CHANGED;
		if (Lock_view_to_cursegp)
			set_view_target_from_segment(Cursegp);

		med_create_new_segment_from_cursegp();
		mine_changed = 1;
	}

	return 1;
}

// -------------------------------------------------------------------------------------
int SelectCurrentSegBackward()
{
	int	newseg_num,newside;

	get_previous_segment(Cursegp-Segments,Curside,&newseg_num,&newside);

	Cursegp = &Segments[newseg_num];
	Curside = newside;

	if (Lock_view_to_cursegp)
		set_view_target_from_segment(Cursegp);
	Update_flags |= UF_ED_STATE_CHANGED;
	mine_changed = 1;
	med_create_new_segment_from_cursegp();

	return 1;
}


// ---------- select next/previous side on current segment ----------
int SelectNextSide()
{
	if (++Curside >= MAX_SIDES_PER_SEGMENT)
		Curside = 0;
	Update_flags |= UF_ED_STATE_CHANGED;
	mine_changed = 1;
	return 1;
}

int SelectPrevSide()
{
	if (--Curside < 0)
		Curside = MAX_SIDES_PER_SEGMENT-1;
	Update_flags |= UF_ED_STATE_CHANGED;
	mine_changed = 1;
	return 1;
}

//  ---------- Copy current segment and side to marked segment and side ----------

int CopySegToMarked()
{
   autosave_mine(mine_filename);
   strcpy(undo_status[Autosave_count], "Mark Segment UNDONE.");
	Markedsegp = Cursegp;
	Markedside = Curside;
	Update_flags |= UF_ED_STATE_CHANGED;
	mine_changed = 1;
	return 1;
}

// ---------- select absolute face on segment ----------

int SelectBottom()
{
	Curside = WBOTTOM;
	Update_flags |= UF_ED_STATE_CHANGED;
	mine_changed = 1;
	return 1;
}

int SelectFront()
{
	Curside = WFRONT;
	Update_flags |= UF_ED_STATE_CHANGED;
	mine_changed = 1;
	return 1;
}

int SelectTop()
{
	Curside = WTOP;
	Update_flags |= UF_ED_STATE_CHANGED;
	mine_changed = 1;
	return 1;
}

int SelectBack()
{
	Curside = WBACK;
	Update_flags |= UF_ED_STATE_CHANGED;
	mine_changed = 1;
	return 1;
}

int SelectLeft()
{
	Curside = WLEFT;
	Update_flags |= UF_ED_STATE_CHANGED;
	mine_changed = 1;
	return 1;
}

int SelectRight()
{
	Curside = WRIGHT;
	Update_flags |= UF_ED_STATE_CHANGED;
	mine_changed = 1;
	return 1;
}

