/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

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
 * curve generation stuff
 *
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "inferno.h"
#include "vecmat.h"
#include "gr.h"
#include "key.h"
#include "editor.h"
#include "editor/esegment.h"
#include "gameseg.h"
#include "console.h"
#define ONE_OVER_SQRT2 F1_0 * 0.707106781
#define CURVE_RIGHT 1
#define CURVE_UP 2

#include "compiler-range_for.h"
#include "partial_range.h"

static imsegptridx_t OriginalSeg = segment_none;
static imsegptridx_t OriginalMarkedSeg = segment_none;
static int OriginalSide;
static int OriginalMarkedSide;
static array<segment *, MAX_SEGMENTS> CurveSegs;
static unsigned CurveNumSegs;

static void generate_banked_curve(fix maxscale, vms_equation coeffs);

static void create_curve(vms_vector &p1, vms_vector &p4, vms_vector &r1, vms_vector &r4, vms_equation &coeffs)
{
// Q(t) = (2t^3 - 3t^2 + 1) p1 + (-2t^3 + 3t^2) p4 + (t~3 - 2t^2 + t) r1 + (t^3 - t^2 ) r4

    coeffs.n.x3 = fixmul(2*F1_0,p1.x) - fixmul(2*F1_0,p4.x) + r1.x + r4.x;
    coeffs.n.x2 = fixmul(-3*F1_0,p1.x) + fixmul(3*F1_0,p4.x) - fixmul(2*F1_0,r1.x) - fixmul(1*F1_0,r4.x);
    coeffs.n.x1 = r1.x;
    coeffs.n.x0 = p1.x;
    coeffs.n.y3 = fixmul(2*F1_0,p1.y) - fixmul(2*F1_0,p4.y) + r1.y + r4.y;
    coeffs.n.y2 = fixmul(-3*F1_0,p1.y) + fixmul(3*F1_0,p4.y) - fixmul(2*F1_0,r1.y) - fixmul(1*F1_0,r4.y);
    coeffs.n.y1 = r1.y;
    coeffs.n.y0 = p1.y;
    coeffs.n.z3 = fixmul(2*F1_0,p1.z) - fixmul(2*F1_0,p4.z) + r1.z + r4.z;
    coeffs.n.z2 = fixmul(-3*F1_0,p1.z) + fixmul(3*F1_0,p4.z) - fixmul(2*F1_0,r1.z) - fixmul(1*F1_0,r4.z);
    coeffs.n.z1 = r1.z;
    coeffs.n.z0 = p1.z;

}

vms_vector evaluate_curve(vms_equation *coeffs, int degree, fix t) {
    fix t2, t3;
    vms_vector coord;

    if (degree != 3)
		con_puts(CON_CRITICAL," for Hermite Curves degree must be 3");

    t2 = fixmul(t,t); t3 = fixmul(t2,t);

    coord.x = fixmul(coeffs->n.x3,t3) + fixmul(coeffs->n.x2,t2) + fixmul(coeffs->n.x1,t) + coeffs->n.x0;
    coord.y = fixmul(coeffs->n.y3,t3) + fixmul(coeffs->n.y2,t2) + fixmul(coeffs->n.y1,t) + coeffs->n.y0;
    coord.z = fixmul(coeffs->n.z3,t3) + fixmul(coeffs->n.z2,t2) + fixmul(coeffs->n.z1,t) + coeffs->n.z0;

    return coord;
}


fix curve_dist(vms_equation *coeffs, int degree, fix t0, const vms_vector &p0, fix dist)
{
	 vms_vector coord;
    fix t, diff;

    if (degree != 3)
		con_puts(CON_CRITICAL," for Hermite Curves degree must be 3");

    for (t=t0;t<1*F1_0;t+=0.001*F1_0) {
        coord = evaluate_curve(coeffs, 3, t);
        diff = dist - vm_vec_dist(coord, p0);
        if (diff<ACCURACY)   //&&(diff>-ACCURACY))
            return t;
    }
    return -1*F1_0;

}

void plot_parametric(vms_equation *coeffs, fix min_t, fix max_t, fix del_t) {
    vms_vector coord, dcoord;
    fix t, dt;

	const uint8_t color = 15;
	gr_box(*grd_curcanv, 75,  40, 325, 290, color);
	gr_box(*grd_curcanv, 75, 310, 325, 560, color);
	gr_box(*grd_curcanv,475, 310, 725, 560, color);
    //gr_pal_fade_in( grd_curscreen->pal );

    for (t=min_t;t<max_t-del_t;t+=del_t) {
        dt = t+del_t;

        coord = evaluate_curve(coeffs, 3, t);
        dcoord = evaluate_curve(coeffs, 3, dt);

		gr_line (*grd_curcanv,  75*F1_0 + coord.x, 290*F1_0 - coord.z,  75*F1_0 + dcoord.x, 290*F1_0 - dcoord.z, 9);
		gr_line (*grd_curcanv,  75*F1_0 + coord.x, 560*F1_0 - coord.y,  75*F1_0 + dcoord.x, 560*F1_0 - dcoord.y, 10);
		gr_line (*grd_curcanv, 475*F1_0 + coord.z, 560*F1_0 - coord.y, 475*F1_0 + dcoord.z, 560*F1_0 - dcoord.y, 12);
    }
}

static vms_vector p1, p4, r1, r4;
static vms_vector r4t, r1save;

int generate_curve( fix r1scale, fix r4scale ) {
    vms_vector vec_dir, tvec;
    vms_vector coord,prev_point;
    vms_equation coeffs;
    fix enddist, nextdist;
    int firstsegflag;
    fix t, maxscale;
    fixang rangle, uangle;

	const vcsegptr_t cursegp = Cursegp;
	compute_center_point_on_side(vcvertptr, p1, cursegp, Curside);

    switch( Curside ) {
        case WLEFT:
            extract_right_vector_from_segment(cursegp, r1);
            vm_vec_scale(r1, -F1_0 );
            break;
        case WTOP:
            extract_up_vector_from_segment(cursegp, r1);
            break;
        case WRIGHT:
            extract_right_vector_from_segment(cursegp, r1);
            break;
        case WBOTTOM:
            extract_up_vector_from_segment(cursegp, r1);
            vm_vec_scale(r1, -F1_0 );
            break;
        case WBACK:
            extract_forward_vector_from_segment(cursegp, r1);
            break;
        case WFRONT:
            extract_forward_vector_from_segment(cursegp, r1);
            vm_vec_scale(r1, -F1_0 );
            break;
        }            

	const vcsegptr_t markedsegp = Markedsegp;
	compute_center_point_on_side(vcvertptr, p4, markedsegp, Markedside);

    switch( Markedside ) {
        case WLEFT:
            extract_right_vector_from_segment(markedsegp, r4);
            extract_up_vector_from_segment(markedsegp, r4t);
            break;
        case WTOP:
            extract_up_vector_from_segment(markedsegp, r4);
            vm_vec_scale(r4, -F1_0 );
            extract_forward_vector_from_segment(markedsegp, r4t);
            vm_vec_scale(r4t, -F1_0 );
            break;
        case WRIGHT:
            extract_right_vector_from_segment(markedsegp, r4);
            vm_vec_scale(r4, -F1_0 );
            extract_up_vector_from_segment(markedsegp, r4t);
            break;
        case WBOTTOM:
            extract_up_vector_from_segment(markedsegp, r4);
            extract_forward_vector_from_segment(markedsegp, r4t);
            break;
        case WBACK:
            extract_forward_vector_from_segment(markedsegp, r4);
            vm_vec_scale(r4, -F1_0 );
            extract_up_vector_from_segment(markedsegp, r4t);
            break;
        case WFRONT:
            extract_forward_vector_from_segment(markedsegp, r4);
            extract_up_vector_from_segment(markedsegp, r4t);
            break;
        }

    r1save = r1;
    tvec = r1;
    vm_vec_scale(r1,r1scale);
    vm_vec_scale(r4,r4scale);

    create_curve( p1, p4, r1, r4, coeffs );
    OriginalSeg = Cursegp;
    OriginalMarkedSeg = Markedsegp;
    OriginalSide = Curside;
    OriginalMarkedSide = Markedside;
    CurveNumSegs = 0;
    coord = prev_point = p1;

    t=0;
    firstsegflag = 1;
    enddist = F1_0; nextdist = 0;
    while ( enddist > fixmul( nextdist, 1.5*F1_0 )) {
            vms_matrix  rotmat;
            if (firstsegflag==1)
                firstsegflag=0;
            else
                extract_forward_vector_from_segment(cursegp, tvec);
            nextdist = vm_vec_mag(tvec);                                   // nextdist := distance to next point
            t = curve_dist(&coeffs, 3, t, prev_point, nextdist);               // t = argument at which function is forward vector magnitude units away from prev_point (in 3-space, not along curve)
            coord = evaluate_curve(&coeffs, 3, t);                                          // coord := point about forward vector magnitude units away from prev_point
            enddist = vm_vec_dist(coord, p4);                  // enddist := distance from current to end point, vec_dir used as a temporary variable
            //vm_vec_normalize(vm_vec_sub(&vec_dir, &coord, &prev_point));
            vm_vec_normalized_dir(vec_dir, coord, prev_point);
			if (!med_attach_segment(Cursegp, vmsegptr(&New_segment), Curside, AttachSide))
		{
			med_extract_matrix_from_segment(cursegp, rotmat);                   // rotmat := matrix describing orientation of Cursegp
			const auto tdest = vm_vec_rotate(vec_dir,rotmat);	// tdest := vec_dir in reference frame of Cursegp
			vec_dir = tdest;

            const auto rotmat2 = vm_vector_2_matrix(vec_dir,nullptr,nullptr);

            med_rotate_segment( Cursegp, rotmat2 );
			prev_point = coord;
            Curside = Side_opposite[AttachSide];

            CurveSegs[CurveNumSegs]=Cursegp;
            CurveNumSegs++;
        } else return 0;
	}

    extract_up_vector_from_segment(cursegp, tvec);
    uangle = vm_vec_delta_ang( tvec, r4t, r4 );
    if (uangle >= F1_0 * 1/8) uangle -= F1_0 * 1/4;
    if (uangle >= F1_0 * 1/8) uangle -= F1_0 * 1/4;
    if (uangle <= -F1_0 * 1/8) uangle += F1_0 * 1/4;
    if (uangle <= -F1_0 * 1/8) uangle += F1_0 * 1/4;
    extract_right_vector_from_segment(cursegp, tvec);
    rangle = vm_vec_delta_ang( tvec, r4t, r4 );
    if (rangle >= F1_0/8) rangle -= F1_0/4;
    if (rangle >= F1_0/8) rangle -= F1_0/4;
    if (rangle <= -F1_0/8) rangle += F1_0/4;
    if (rangle <= -F1_0/8) rangle += F1_0/4;

    if ((uangle != 0) && (rangle != 0)) {
        maxscale = CurveNumSegs*F1_0;
        generate_banked_curve(maxscale, coeffs);
    }

    if (CurveNumSegs) {
        med_form_bridge_segment( Cursegp, Side_opposite[AttachSide], Markedsegp, Markedside );
        CurveSegs[CurveNumSegs] = vmsegptr(Markedsegp->children[Markedside]);
        CurveNumSegs++;
	}

    Cursegp = OriginalSeg;
    Curside = OriginalSide;

	med_create_new_segment_from_cursegp();

	//warn_if_concave_segments();

    if (CurveNumSegs) return 1;
        else return 0;
}

static inline vms_matrix vm_vec_ang_2_matrix (const vms_vector &v, fixang a) __attribute_warn_unused_result;
static inline vms_matrix vm_vec_ang_2_matrix (const vms_vector &v, fixang a)
{
	vms_matrix m;
	return vm_vec_ang_2_matrix(m, v, a), m;
}

void generate_banked_curve(fix maxscale, vms_equation coeffs) {
    vms_vector vec_dir, tvec, b4r4t;
    vms_vector coord,prev_point;
    fix enddist, nextdist;
    int firstsegflag;
    fixang rangle, uangle, angle, scaled_ang=0;
    fix t;

    if (CurveNumSegs) {

		const vcsegptr_t cursegp = Cursegp;
		extract_up_vector_from_segment(cursegp, b4r4t);
    uangle = vm_vec_delta_ang( b4r4t, r4t, r4 );
    if (uangle >= F1_0 * 1/8) uangle -= F1_0 * 1/4;
    if (uangle >= F1_0 * 1/8) uangle -= F1_0 * 1/4;
    if (uangle <= -F1_0 * 1/8) uangle += F1_0 * 1/4;
    if (uangle <= -F1_0 * 1/8) uangle += F1_0 * 1/4;

		extract_right_vector_from_segment(cursegp, b4r4t);
    rangle = vm_vec_delta_ang( b4r4t, r4t, r4 );
    if (rangle >= F1_0/8) rangle -= F1_0/4;
    if (rangle >= F1_0/8) rangle -= F1_0/4;
    if (rangle <= -F1_0/8) rangle += F1_0/4;
    if (rangle <= -F1_0/8) rangle += F1_0/4;

    angle = uangle;
    if (abs(rangle) < abs(uangle)) angle = rangle;

	delete_curve();

    coord = prev_point = p1;

#define MAGIC_NUM 0.707*F1_0

    if (maxscale)
        scaled_ang = fixdiv(angle,fixmul(maxscale,MAGIC_NUM));

    t=0; 
    tvec = r1save;
    firstsegflag = 1;
    enddist = F1_0; nextdist = 0;
    while ( enddist > fixmul( nextdist, 1.5*F1_0 )) {
            vms_matrix  rotmat;
            if (firstsegflag==1)
                firstsegflag=0;
            else
				extract_forward_vector_from_segment(cursegp, tvec);
            nextdist = vm_vec_mag(tvec);                                   // nextdist := distance to next point
            t = curve_dist(&coeffs, 3, t, prev_point, nextdist);               // t = argument at which function is forward vector magnitude units away from prev_point (in 3-space, not along curve)
            coord = evaluate_curve(&coeffs, 3, t);                                          // coord := point about forward vector magnitude units away from prev_point
            enddist = vm_vec_dist(coord, p4);                  // enddist := distance from current to end point, vec_dir used as a temporary variable
            //vm_vec_normalize(vm_vec_sub(&vec_dir, &coord, &prev_point));
            vm_vec_normalized_dir(vec_dir, coord, prev_point);
			if (!med_attach_segment(Cursegp, vmsegptr(&New_segment), Curside, AttachSide))
			{
				med_extract_matrix_from_segment(cursegp, rotmat);                   // rotmat := matrix describing orientation of Cursegp
			const auto tdest = vm_vec_rotate(vec_dir,rotmat);	// tdest := vec_dir in reference frame of Cursegp
			vec_dir = tdest;
            const auto rotmat2 = vm_vec_ang_2_matrix(vec_dir,scaled_ang);

			med_rotate_segment( Cursegp, rotmat2 );
			prev_point = coord;
            Curside = Side_opposite[AttachSide];

            CurveSegs[CurveNumSegs]=Cursegp;
            CurveNumSegs++;
        }
      }
    }
}


void delete_curve() {
	range_for (auto &i, partial_const_range(CurveSegs, CurveNumSegs))
	{
        if (i->segnum != segment_none)
			med_delete_segment(vmsegptridx(i));
    }
    Markedsegp = OriginalMarkedSeg;
    Markedside = OriginalMarkedSide;
    Cursegp = OriginalSeg;
    Curside = OriginalSide;
	med_create_new_segment_from_cursegp();
    CurveNumSegs = 0;

	//editor_status("");
	//warn_if_concave_segments();
}
