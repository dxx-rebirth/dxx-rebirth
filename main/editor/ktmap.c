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
 * $Source: /cvs/cvsroot/d2x/main/editor/ktmap.c,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 * 
 * Texture map key bindings.
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:03:34  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:35:37  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.26  1994/08/25  21:57:12  mike
 * IS_CHILD stuff.
 * 
 * Revision 1.25  1994/08/03  10:32:41  mike
 * Texture map stretching.
 * 
 * Revision 1.24  1994/05/14  17:17:35  matt
 * Got rid of externs in source (non-header) files
 * 
 * Revision 1.23  1994/04/28  10:48:38  yuan
 * Fixed undo message for Clear Texture.
 * 
 * Revision 1.22  1994/04/22  17:45:42  john
 * MAde top 2 bits of paste-ons pick the 
 * orientation of the bitmap.
 * 
 * Revision 1.21  1994/04/01  14:36:08  yuan
 * Fixed propogate function so you can propogate and move.
 * 
 * Revision 1.20  1994/03/19  17:22:08  yuan
 * Wall system implemented until specific features need to be added...
 * (Needs to be hammered on though.)
 * 
 * Revision 1.19  1994/02/14  12:06:12  mike
 * change segment data structure.
 * 
 * Revision 1.18  1994/01/25  17:58:47  yuan
 * Added ambient lighting, and also added fixing bogus segments
 * functions to the editor... (they don't work fully... need to
 * check out seguvs.c
 * 
 * Revision 1.17  1994/01/24  11:54:52  yuan
 * Checking everything in
 * 
 * Revision 1.16  1994/01/18  16:05:57  yuan
 * Added clear texture 2 function (shift 0)
 * 
 * Revision 1.15  1994/01/18  10:15:01  yuan
 * added texture stuff
 * 
 * Revision 1.14  1993/12/06  19:33:57  yuan
 * Fixed autosave stuff so that undo restores Cursegp and
 * Markedsegp
 * 
 * Revision 1.13  1993/12/02  12:39:39  matt
 * Removed extra includes
 * 
 * Revision 1.12  1993/11/28  17:31:34  mike
 * Use new segment data structure.
 * 
 * Revision 1.11  1993/11/12  16:38:37  mike
 * Change call to med_propagate_tmaps_to_segments to include new uv_only_flag parameter.
 * 
 * Revision 1.10  1993/11/11  15:53:30  yuan
 * Fixed undo display message
 * 
 * Revision 1.9  1993/11/08  19:13:46  yuan
 * Added Undo command (not working yet)
 * 
 * Revision 1.8  1993/11/05  17:32:48  john
 * added funcs
 * .,
 * 
 * Revision 1.7  1993/11/02  10:31:08  mike
 * Add PropagateTexturesSelected.
 * 
 * Revision 1.6  1993/10/29  11:43:15  mike
 * Write PropagateTextures
 * 
 * Revision 1.5  1993/10/25  13:26:39  mike
 * Force redraw whenever a texture map is assigned.
 * 
 * Revision 1.4  1993/10/15  17:42:53  mike
 * Make AssignTexture also assign texture maps to New_segment.
 * 
 * Revision 1.3  1993/10/15  13:10:24  mike
 * Adapt AssignTexture to new segment structure.
 * 
 * Revision 1.2  1993/10/14  18:09:17  mike
 * Debug code for AssignTexture and comment out code.
 * 
 * Revision 1.1  1993/10/14  14:01:49  mike
 * Initial revision
 * 
 * 
 */


#ifdef RCS
static char rcsid[] = "$Id: ktmap.c,v 1.1 2004-12-19 13:54:27 btb Exp $";
#endif

#include <string.h>

#include "inferno.h"
#include "editor.h"
#include "mono.h"
#include "kdefs.h"

//	Assign CurrentTexture to Curside in *Cursegp
int AssignTexture(void)
{
   autosave_mine( mine_filename );
   strcpy(undo_status[Autosave_count], "Assign Texture UNDONE.");

	Cursegp->sides[Curside].tmap_num = CurrentTexture;

	New_segment.sides[Curside].tmap_num = CurrentTexture;

//	propagate_light_intensity(Cursegp, Curside, CurrentTexture, 0); 
																					 
	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

//	Assign CurrentTexture to Curside in *Cursegp
int AssignTexture2(void)
{
	int texnum, orient, ctexnum, newtexnum;

   autosave_mine( mine_filename );
   strcpy(undo_status[Autosave_count], "Assign Texture 2 UNDONE.");

	texnum = Cursegp->sides[Curside].tmap_num2 & 0x3FFF;
	orient = ((Cursegp->sides[Curside].tmap_num2 & 0xC000) >> 14) & 3;
	ctexnum = CurrentTexture;
	
	if ( ctexnum == texnum )	{
		orient = (orient+1) & 3;
		newtexnum = (orient<<14) | texnum;
	} else {
		newtexnum = ctexnum;
	}

	Cursegp->sides[Curside].tmap_num2 = newtexnum;
	New_segment.sides[Curside].tmap_num2 = newtexnum;

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

int ClearTexture2(void)
{
   autosave_mine( mine_filename );
   strcpy(undo_status[Autosave_count], "Clear Texture 2 UNDONE.");

	Cursegp->sides[Curside].tmap_num2 = 0;

	New_segment.sides[Curside].tmap_num2 = 0;

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}


//	--------------------------------------------------------------------------------------------------
//	Propagate textures from Cursegp through Curside.
//	If uv_flag !0, then only propagate uv coordinates (if 0, then propagate textures as well)
//	If move_flag !0, then move forward to new segment after propagation, else don't
int propagate_textures_common(int uv_flag, int move_flag)
{
   autosave_mine( mine_filename );
   strcpy(undo_status[Autosave_count], "Propogate Textures UNDONE.");
	
	if (IS_CHILD(Cursegp->children[Curside]))
		med_propagate_tmaps_to_segments(Cursegp, &Segments[Cursegp->children[Curside]], uv_flag);

	if (move_flag)
		SelectCurrentSegForward();

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

//	Propagate texture maps from current segment, through current side
int PropagateTextures(void)
{
	return propagate_textures_common(0, 0);
}

//	Propagate texture maps from current segment, through current side
int PropagateTexturesUVs(void)
{
	return propagate_textures_common(-1, 0);
}

//	Propagate texture maps from current segment, through current side
// And move to that segment.
int PropagateTexturesMove(void)
{
	return propagate_textures_common(0, 1);
}

//	Propagate uv coordinate from current segment, through current side
// And move to that segment.
int PropagateTexturesMoveUVs(void)
{
	return propagate_textures_common(-1, 1);
}


//	-------------------------------------------------------------------------------------
int is_selected_segment(int segnum)
{
	int	i;

	for (i=0; i<N_selected_segs; i++)
		if (Selected_segs[i] == segnum)
			return 1;

	return 0;

}

//	-------------------------------------------------------------------------------------
//	Auxiliary function for PropagateTexturesSelected.
//	Recursive parse.
void pts_aux(segment *sp)
{
	int		side;

	Been_visited[sp-Segments] = 1;

	for (side=0; side<MAX_SIDES_PER_SEGMENT; side++) {
		if (IS_CHILD(sp->children[side])) {
			while ((!Been_visited[sp->children[side]]) && is_selected_segment(sp->children[side])) {
				med_propagate_tmaps_to_segments(sp,&Segments[sp->children[side]],0);
				pts_aux(&Segments[sp->children[side]]);
			}
		}
	}
}

//	-------------------------------------------------------------------------------------
//	Propagate texture maps from current segment recursively exploring all children, to all segments in Selected_list
//	until a segment not in Selected_list is reached.
int PropagateTexturesSelected(void)
{
	int		i;

   autosave_mine( mine_filename );
   strcpy(undo_status[Autosave_count], "Propogate Textures Selected UNDONE.");

	for (i=0; i<MAX_SEGMENTS; i++) Been_visited[i] = 0;	//clear visited list
	Been_visited[Cursegp-Segments] = 1;

	pts_aux(Cursegp);

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

