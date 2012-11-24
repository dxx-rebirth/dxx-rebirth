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
 * Texture map key bindings.
 *
 */

#include <string.h>
#include "inferno.h"
#include "editor.h"
#include "editor/esegment.h"
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
   autosave_mine( mine_filename );
   strcpy(undo_status[Autosave_count], "Propogate Textures Selected UNDONE.");

	for (unsigned i=0; i<(sizeof(Been_visited)/sizeof(Been_visited[0])); ++i)
		Been_visited[i] = 0;	//clear visited list
	Been_visited[Cursegp-Segments] = 1;

	pts_aux(Cursegp);

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

