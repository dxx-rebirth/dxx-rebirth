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
 * Functions to make faces planar and probably other things.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "key.h"
#include "gr.h"
#include "inferno.h"
#include "segment.h"
#include	"editor.h"
#include "editor/esegment.h"
#include "dxxerror.h"
#include "gameseg.h"

#define SWAP(a,b) {temp = (a); (a) = (b); (b) = temp;}

//	-----------------------------------------------------------------------------------------------------------------
//	Gauss-Jordan elimination solution of a system of linear equations.
//	a[1..n][1..n] is the input matrix.  b[1..n][1..m] is input containing the m right-hand side vectors.
//	On output, a is replaced by its matrix inverse and b is replaced by the corresponding set of solution vectors.
void gaussj(fix **a, int n, fix **b, int m)
{
	int	indxc[4], indxr[4], ipiv[4];
        int     i, icol=0, irow=0, j, k, l, ll;
	fix	big, dum, pivinv, temp;

	if (n > 4) {
		Int3();
	}

	for (j=1; j<=n; j++)
		ipiv[j] = 0;

	for (i=1; i<=n; i++) {
		big = 0;
		for (j=1; j<=n; j++)
			if (ipiv[j] != 1)
				for (k=1; k<=n; k++) {
					if (ipiv[k] == 0) {
						if (abs(a[j][k]) >= big) {
							big = abs(a[j][k]);
							irow = j;
							icol = k;
						}
					} else if (ipiv[k] > 1) {
						Int3();
					}
				}

		++(ipiv[icol]);

		// We now have the pivot element, so we interchange rows, if needed, to put the pivot
		//	element on the diagonal.  The columns are not physically interchanged, only relabeled:
		//	indxc[i], the column of the ith pivot element, is the ith column that is reduced, while
		//	indxr[i] is the row in which that pivot element was originally located.  If indxr[i] !=
		//	indxc[i] there is an implied column interchange.  With this form of bookkeeping, the
		//	solution b's will end up in the correct order, and the inverse matrix will be scrambled
		//	by columns.

		if (irow != icol) {
			for (l=1; l<=n; l++)
				SWAP(a[irow][l], a[icol][l]);
			for (l=1; l<=m; l++)
				SWAP(b[irow][l], b[icol][l]);
		}

		indxr[i] = irow;
		indxc[i] = icol;
		if (a[icol][icol] == 0) {
			Int3();
		}
		pivinv = fixdiv(F1_0, a[icol][icol]);
		a[icol][icol] = F1_0;

		for (l=1; l<=n; l++)
			a[icol][l] = fixmul(a[icol][l], pivinv);
		for (l=1; l<=m; l++)
			b[icol][l] = fixmul(b[icol][l], pivinv);

		for (ll=1; ll<=n; ll++)
			if (ll != icol) {
				dum = a[ll][icol];
				a[ll][icol] = 0;
				for (l=1; l<=n; l++)
					a[ll][l] -= a[icol][l]*dum;
				for (l=1; l<=m; l++)
					b[ll][l] -= b[icol][l]*dum;
			}
	}

	//	This is the end of the main loop over columns of the reduction.  It only remains to unscramble
	//	the solution in view of the column interchanges.  We do this by interchanging pairs of
	//	columns in the reverse order that the permutation was built up.
	for (l=n; l>=1; l--) {
		if (indxr[l] != indxc[l])
			for (k=1; k<=n; k++)
				SWAP(a[k][indxr[l]], a[k][indxc[l]]);
	}

}


//	-----------------------------------------------------------------------------------------------------------------
//	Return true if side is planar, else return false.
int side_is_planar_p(segment *sp, int side)
{
	sbyte			*vp;
	vms_vector	*v0,*v1,*v2,*v3;
	vms_vector	va,vb;

	vp = Side_to_verts[side];
	v0 = &Vertices[sp->verts[vp[0]]];
	v1 = &Vertices[sp->verts[vp[1]]];
	v2 = &Vertices[sp->verts[vp[2]]];
	v3 = &Vertices[sp->verts[vp[3]]];

	vm_vec_normalize(vm_vec_normal(&va,v0,v1,v2));
	vm_vec_normalize(vm_vec_normal(&vb,v0,v2,v3));
	
	// If the two vectors are very close to being the same, then generate one quad, else generate two triangles.
	return (vm_vec_dist(&va,&vb) < F1_0/1000);
}

//	-------------------------------------------------------------------------------------------------
//	Return coordinates of a vertex which is vertex v moved so that all sides of which it is a part become planar.
void compute_planar_vert(segment *sp, int side, int v, vms_vector *vp)
{
	if ((sp) && (side > -3))
		*vp = Vertices[v];
}

//	-------------------------------------------------------------------------------------------------
//	Making Cursegp:Curside planar.
//	If already planar, return.
//	for each vertex v on side, not part of another segment
//		choose the vertex v which can be moved to make all sides of which it is a part planar, minimizing distance moved
//	if there is no vertex v on side, not part of another segment, give up in disgust
//	Return value:
//		0	curside made planar (or already was)
//		1	did not make curside planar
int make_curside_planar(void)
{
	int			v;
	sbyte			*vp;
	vms_vector	planar_verts[4];			// store coordinates of up to 4 vertices which will make Curside planar, corresponding to each of 4 vertices on side
	int			present_verts[4];			//	set to 1 if vertex is present

	if (side_is_planar_p(Cursegp, Curside))
		return 0;

	//	Look at all vertices in side to find a free one.
	for (v=0; v<4; v++)
		present_verts[v] = 0;

	vp = Side_to_verts[Curside];

	for (v=0; v<4; v++) {
		int v1 = vp[v];		// absolute vertex id
		if (is_free_vertex(Cursegp->verts[v1])) {
			compute_planar_vert(Cursegp, Curside, Cursegp->verts[v1], &planar_verts[v]);
			present_verts[v] = 1;
		}
	}

	//	Now, for each v for which present_verts[v] == 1, there is a vector (point) in planar_verts[v].
	//	See which one is closest to the plane defined by the other three points.
	//	Nah...just use the first one we find.
	for (v=0; v<4; v++)
		if (present_verts[v]) {
			med_set_vertex(vp[v],&planar_verts[v]);
			validate_segment(Cursegp);
			// -- should propagate tmaps to segments or something here...
			
			return 0;
		}
	//	We tried, but we failed, to make Curside planer.
	return 1;
}

