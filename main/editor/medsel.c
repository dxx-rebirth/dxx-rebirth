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
 * $Source: /cvs/cvsroot/d2x/main/editor/medsel.c,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 * 
 * Routines stripped from med.c for segment selection
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:03:57  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:35:20  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.10  1994/08/09  16:05:59  john
 * Added the ability to place players.  Made old
 * Player variable be ConsoleObject.
 * 
 * Revision 1.9  1994/05/23  14:56:27  mike
 * make current segment be add segment.
 * 
 * Revision 1.8  1994/05/14  18:00:50  matt
 * Got rid of externs in source (non-header) files
 * 
 * Revision 1.7  1994/03/31  12:03:33  matt
 * Cleaned up includes
 * 
 * Revision 1.6  1994/02/17  12:52:13  yuan
 * Unbackdated
 * y
 * 
 * Revision 1.4  1994/02/17  09:46:53  matt
 * Removed include of slew.h
 * 
 * Revision 1.3  1994/01/05  10:54:23  john
 * New object code by John
 * 
 * Revision 1.2  1993/12/17  12:18:22  john
 * Moved selection stuff out of med.c
 * 
 * Revision 1.1  1993/12/17  09:29:34  john
 * Initial revision
 * 
 * 
 */


#ifdef RCS
static char rcsid[] = "$Id: medsel.c,v 1.1 2004-12-19 13:54:27 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifdef __MSDOS__
#include <process.h>
#endif

#include "gr.h"
#include "ui.h"
#include "key.h"
#include "mono.h"
#include "error.h"
#include "u_mem.h"

#include "inferno.h"
#include "editor.h"
#include "segment.h"
#include "object.h"

typedef struct sort_element {
	short segnum;
	fix dist;
} sort_element;

//compare the distance of two segments.  slow, since it computes the
//distance each time
int segdist_cmp(sort_element *s0,sort_element *s1)
{
	return (s0->dist==s1->dist)?0:((s0->dist<s1->dist)?-1:1);

}


//find the distance between a segment and a point
fix compute_dist(segment *seg,vms_vector *pos)
{
	vms_vector delta;

	compute_segment_center(&delta,seg);
	vm_vec_sub2(&delta,pos);

	return vm_vec_mag(&delta);

}

//sort a list of segments, in order of closeness to pos
void sort_seg_list(int n_segs,short *segnumlist,vms_vector *pos)
{
	int i;
	sort_element *sortlist;

	sortlist = calloc(n_segs,sizeof(*sortlist));

	for (i=0;i<n_segs;i++) {
		sortlist[i].segnum = segnumlist[i];
		sortlist[i].dist = compute_dist(&Segments[segnumlist[i]],pos);
	}

	qsort(sortlist,n_segs,sizeof(*sortlist),(int (*)(const void *, const void *))segdist_cmp);

	for (i=0;i<n_segs;i++)
		segnumlist[i] = sortlist[i].segnum;

	free(sortlist);
}

int SortSelectedList(void)
{
	sort_seg_list(N_selected_segs,Selected_segs,&ConsoleObject->pos);
	editor_status("%i element selected list sorted.",N_selected_segs);

	return 1;
}

int SelectNextFoundSeg(void)
{
	if (++Found_seg_index >= N_found_segs)
		Found_seg_index = 0;

	Cursegp = &Segments[Found_segs[Found_seg_index]];
	med_create_new_segment_from_cursegp();

	Update_flags |= UF_WORLD_CHANGED;

	if (Lock_view_to_cursegp)
		set_view_target_from_segment(Cursegp);

	editor_status("Curseg assigned to next found segment.");

	return 1;
}

int SelectPreviousFoundSeg(void)
{
	if (Found_seg_index > 0)
		Found_seg_index--;
	else
		Found_seg_index = N_found_segs-1;

	Cursegp = &Segments[Found_segs[Found_seg_index]];
	med_create_new_segment_from_cursegp();

	Update_flags |= UF_WORLD_CHANGED;

	if (Lock_view_to_cursegp)
		set_view_target_from_segment(Cursegp);

	editor_status("Curseg assigned to previous found segment.");

	return 1;
}

