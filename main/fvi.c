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
 * New home for find_vector_intersection()
 *
 */


#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "inferno.h"
#include "fvi.h"
#include "segment.h"
#include "object.h"
#include "wall.h"
#include "laser.h"
#include "rle.h"
#include "robot.h"
#include "piggy.h"
#include "player.h"
#include "fix.h"

#define face_type_num(nfaces,face_num,tri_edge) ((nfaces==1)?0:(tri_edge*2 + face_num))

//find the point on the specified plane where the line intersects
//returns true if point found, false if line parallel to plane
//new_pnt is the found point on the plane
//plane_pnt & plane_norm describe the plane
//p0 & p1 are the ends of the line
int find_plane_line_intersection(vms_vector *new_pnt,vms_vector *plane_pnt,vms_vector *plane_norm,vms_vector *p0,vms_vector *p1,fix rad)
{
	vms_vector d,w;
	fix num,den;

	vm_vec_sub(&d,p1,p0);
	vm_vec_sub(&w,p0,plane_pnt);

	num =  vm_vec_dot(plane_norm,&w);
	den = -vm_vec_dot(plane_norm,&d);

	num -= rad; //move point out by rad

	if (den == 0) // moving parallel to wall, so can't hit it
		return 0;
	//check for various bad values
	if (den > 0 && (-num>>15) >= den) //will overflow (large negative)
		num = (f1_0-f0_5)*den;
	if (den > 0 && num > den) //frac greater than one
		return 0;
	if (den < 0 && num < den) //frac greater than one
		return 0;
	if (labs (num) / (f1_0 / 2) >= labs (den))
		return 0;

	vm_vec_scale2(&d,num,den);
	vm_vec_add(new_pnt,p0,&d);

	return 1;

}

typedef struct vec2d {
	fix i,j;
} vec2d;

//given largest componant of normal, return i & j
//if largest componant is negative, swap i & j
int ij_table[3][2] =        {
							{2,1},          //pos x biggest
							{0,2},          //pos y biggest
							{1,0},          //pos z biggest
						};

//intersection types
#define IT_NONE 0       //doesn't touch face at all
#define IT_FACE 1       //touches face
#define IT_EDGE 2       //touches edge of face
#define IT_POINT        3       //touches vertex

//see if a point in inside a face by projecting into 2d
uint check_point_to_face(vms_vector *checkp,segment *sp, side *s,int facenum,int nv,int *vertex_list)
{
	vms_vector_array *checkp_array;
	vms_vector_array norm;
	vms_vector t;
	int biggest;
///
	int i,j,edge;
	uint edgemask;
	fix check_i,check_j;
	vms_vector_array *v0,*v1;

	#ifdef COMPACT_SEGS
		get_side_normal(sp, s-sp->sides, facenum, (vms_vector *)&norm );
	#else
		memcpy( &norm, &s->normals[facenum], sizeof(vms_vector_array));
	#endif
	checkp_array = (vms_vector_array *)checkp;

	//now do 2d check to see if point is in side

	//project polygon onto plane by finding largest component of normal
	t.x = labs(norm.xyz[0]); t.y = labs(norm.xyz[1]); t.z = labs(norm.xyz[2]);

	if (t.x > t.y)
         {
          if (t.x > t.z)
           biggest=0;
          else
           biggest=2;
         }
	else
         {
          if (t.y > t.z)
           biggest=1;
          else
           biggest=2;
         }

	if (norm.xyz[biggest] > 0) {
		i = ij_table[biggest][0];
		j = ij_table[biggest][1];
	}
	else {
		i = ij_table[biggest][1];
		j = ij_table[biggest][0];
	}

	//now do the 2d problem in the i,j plane

	check_i = checkp_array->xyz[i];
	check_j = checkp_array->xyz[j];

	for (edge=edgemask=0;edge<nv;edge++) {
		vec2d edgevec,checkvec;
		fix64 d;

		v0 = (vms_vector_array *)&Vertices[vertex_list[facenum*3+edge]];
		v1 = (vms_vector_array *)&Vertices[vertex_list[facenum*3+((edge+1)%nv)]];

		edgevec.i = v1->xyz[i] - v0->xyz[i];
		edgevec.j = v1->xyz[j] - v0->xyz[j];

		checkvec.i = check_i - v0->xyz[i];
		checkvec.j = check_j - v0->xyz[j];

		d = fixmul64(checkvec.i,edgevec.j) - fixmul64(checkvec.j,edgevec.i);

		if (d < 0)              		//we are outside of triangle
			edgemask |= (1<<edge);
	}

	return edgemask;

}


//check if a sphere intersects a face
int check_sphere_to_face(vms_vector *pnt,segment *sp, side *s,int facenum,int nv,fix rad,int *vertex_list)
{
	vms_vector checkp=*pnt;
	uint edgemask;

	//now do 2d check to see if point is in side

	edgemask = check_point_to_face(pnt,sp,s,facenum,nv,vertex_list);

	//we've gone through all the sides, are we inside?

	if (edgemask == 0)
		return IT_FACE;
	else {
		vms_vector edgevec,checkvec;            //this time, real 3d vectors
		vms_vector closest_point;
		fix edgelen,d,dist;
		vms_vector *v0,*v1;
		int itype;
		int edgenum;

		//get verts for edge we're behind

		for (edgenum=0;!(edgemask&1);(edgemask>>=1),edgenum++);

		v0 = &Vertices[vertex_list[facenum*3+edgenum]];
		v1 = &Vertices[vertex_list[facenum*3+((edgenum+1)%nv)]];

		//check if we are touching an edge or point

		vm_vec_sub(&checkvec,&checkp,v0);
		edgelen = vm_vec_normalized_dir(&edgevec,v1,v0);
		
		//find point dist from planes of ends of edge

		d = vm_vec_dot(&edgevec,&checkvec);

		if (d+rad < 0) return IT_NONE;                  //too far behind start point

		if (d-rad > edgelen) return IT_NONE;    //too far part end point

		//find closest point on edge to check point

		itype = IT_POINT;

		if (d < 0) closest_point = *v0;
		else if (d > edgelen) closest_point = *v1;
		else {
			itype = IT_EDGE;

			//vm_vec_scale(&edgevec,d);
			//vm_vec_add(&closest_point,v0,&edgevec);

			vm_vec_scale_add(&closest_point,v0,&edgevec,d);
		}

		dist = vm_vec_dist(&checkp,&closest_point);

		if (dist <= rad)
			return (itype==IT_POINT)?IT_NONE:itype;
		else
			return IT_NONE;
	}


}

//returns true if line intersects with face. fills in newp with intersection
//point on plane, whether or not line intersects side
//facenum determines which of four possible faces we have
//note: the seg parm is temporary, until the face itself has a point field
int check_line_to_face(vms_vector *newp,vms_vector *p0,vms_vector *p1,segment *seg,int side,int facenum,int nv,fix rad)
{
	vms_vector checkp;
	int pli;
	struct side *s=&seg->sides[side];
	int vertex_list[6];
	int num_faces;
	int vertnum;
	vms_vector norm;

	#ifdef COMPACT_SEGS
		get_side_normal(seg, side, facenum, &norm );
	#else
		norm = seg->sides[side].normals[facenum];
	#endif

        create_abs_vertex_lists(&num_faces,vertex_list,seg-Segments,side ,__FILE__,__LINE__);

	//use lowest point number
	if (num_faces==2) {
		vertnum = min(vertex_list[0],vertex_list[2]);
	}
	else {
		int i;
		vertnum = vertex_list[0];
		for (i=1;i<4;i++)
			if (vertex_list[i] < vertnum)
				vertnum = vertex_list[i];
	}

	pli = find_plane_line_intersection(newp,&Vertices[vertnum],&norm,p0,p1,rad);

	if (!pli) return IT_NONE;

	checkp = *newp;

	//if rad != 0, project the point down onto the plane of the polygon

	if (rad!=0)
		vm_vec_scale_add2(&checkp,&norm,-rad);

	return check_sphere_to_face(&checkp,seg,s,facenum,nv,rad,vertex_list);

}

//returns the value of a determinant
fix calc_det_value(vms_matrix *det)
{
	return 	fixmul(det->rvec.x,fixmul(det->uvec.y,det->fvec.z)) -
			 	fixmul(det->rvec.x,fixmul(det->uvec.z,det->fvec.y)) -
			 	fixmul(det->rvec.y,fixmul(det->uvec.x,det->fvec.z)) +
			 	fixmul(det->rvec.y,fixmul(det->uvec.z,det->fvec.x)) +
			 	fixmul(det->rvec.z,fixmul(det->uvec.x,det->fvec.y)) -
			 	fixmul(det->rvec.z,fixmul(det->uvec.y,det->fvec.x));
}

//computes the parameters of closest approach of two lines 
//fill in two parameters, t0 & t1.  returns 0 if lines are parallel, else 1
int check_line_to_line(fix *t1,fix *t2,vms_vector *p1,vms_vector *v1,vms_vector *p2,vms_vector *v2)
{
	vms_matrix det;
	fix d,cross_mag2;		//mag squared cross product

	vm_vec_sub(&det.rvec,p2,p1);
	vm_vec_cross(&det.fvec,v1,v2);
	cross_mag2 = vm_vec_dot(&det.fvec,&det.fvec);

	if (cross_mag2 == 0)
		return 0;			//lines are parallel

	det.uvec = *v2;
	d = calc_det_value(&det);
	*t1 = fixdiv(d,cross_mag2);

	det.uvec = *v1;
	d = calc_det_value(&det);
	*t2 = fixdiv(d,cross_mag2);

	return 1;		//found point
}

//this version is for when the start and end positions both poke through
//the plane of a side.  In this case, we must do checks against the edge
//of faces
int special_check_line_to_face(vms_vector *newp,vms_vector *p0,vms_vector *p1,segment *seg,int side,int facenum,int nv,fix rad)
{
	vms_vector move_vec;
	fix edge_t=0,move_t=0,edge_t2=0,move_t2=0,closest_dist=0;
	fix edge_len=0,move_len=0;
	int vertex_list[6];
	int num_faces,edgenum;
	uint edgemask;
	vms_vector *edge_v0,*edge_v1,edge_vec;
	struct side *s=&seg->sides[side];
	vms_vector closest_point_edge,closest_point_move;

	//calc some basic stuff
 
        create_abs_vertex_lists(&num_faces,vertex_list,seg-Segments,side ,__FILE__,__LINE__);
	vm_vec_sub(&move_vec,p1,p0);

	//figure out which edge(s) to check against

	edgemask = check_point_to_face(p0,seg,s,facenum,nv,vertex_list);

	if (edgemask == 0)
		return check_line_to_face(newp,p0,p1,seg,side,facenum,nv,rad);

	for (edgenum=0;!(edgemask&1);edgemask>>=1,edgenum++);

	edge_v0 = &Vertices[vertex_list[facenum*3+edgenum]];
	edge_v1 = &Vertices[vertex_list[facenum*3+((edgenum+1)%nv)]];

	vm_vec_sub(&edge_vec,edge_v1,edge_v0);

	//is the start point already touching the edge?

	//??

	//first, find point of closest approach of vec & edge

	edge_len = vm_vec_normalize(&edge_vec);
	move_len = vm_vec_normalize(&move_vec);

	check_line_to_line(&edge_t,&move_t,edge_v0,&edge_vec,p0,&move_vec);

	//make sure t values are in valid range

	if (move_t<0 || move_t>move_len+rad)
		return IT_NONE;

	if (move_t > move_len)
		move_t2 = move_len;
	else
		move_t2 = move_t;

	if (edge_t < 0)		//saturate at points
		edge_t2 = 0;
	else
		edge_t2 = edge_t;
	
	if (edge_t2 > edge_len)		//saturate at points
		edge_t2 = edge_len;
	
	//now, edge_t & move_t determine closest points.  calculate the points.

	vm_vec_scale_add(&closest_point_edge,edge_v0,&edge_vec,edge_t2);
	vm_vec_scale_add(&closest_point_move,p0,&move_vec,move_t2);

	//find dist between closest points

	closest_dist = vm_vec_dist(&closest_point_edge,&closest_point_move);

	//could we hit with this dist?

	//note massive tolerance here
//	if (closest_dist < (rad*18)/20) {		//we hit.  figure out where
	if (closest_dist < (rad*15)/20) {		//we hit.  figure out where

		//now figure out where we hit

		vm_vec_scale_add(newp,p0,&move_vec,move_t-rad);

		return IT_EDGE;

	}
	else
		return IT_NONE;			//no hit

}

//maybe this routine should just return the distance and let the caller
//decide it it's close enough to hit
//determine if and where a vector intersects with a sphere
//vector defined by p0,p1 
//returns dist if intersects, and fills in intp
//else returns 0
int check_vector_to_sphere_1(vms_vector *intp,vms_vector *p0,vms_vector *p1,vms_vector *sphere_pos,fix sphere_rad)
{
	vms_vector d,dn,w,closest_point;
	fix mag_d,dist,w_dist,int_dist;

	//this routine could be optimized if it's taking too much time!

	vm_vec_sub(&d,p1,p0);
	vm_vec_sub(&w,sphere_pos,p0);

	mag_d = vm_vec_copy_normalize(&dn,&d);

	if (mag_d == 0) {
		int_dist = vm_vec_mag(&w);
		*intp = *p0;
		return (int_dist<sphere_rad)?int_dist:0;
	}

	w_dist = vm_vec_dot(&dn,&w);

	if (w_dist < 0)		//moving away from object
		 return 0;

	if (w_dist > mag_d+sphere_rad)
		return 0;		//cannot hit

	vm_vec_scale_add(&closest_point,p0,&dn,w_dist);

	dist = vm_vec_dist(&closest_point,sphere_pos);

	if (dist < sphere_rad) {
		fix dist2,rad2,shorten;

		dist2 = fixmul(dist,dist);
		rad2 = fixmul(sphere_rad,sphere_rad);

		shorten = fix_sqrt(rad2 - dist2);

		int_dist = w_dist-shorten;

		if (int_dist > mag_d || int_dist < 0) {
			//past one or the other end of vector, which means we're inside

			*intp = *p0;		//don't move at all
			return 1;
		}

		vm_vec_scale_add(intp,p0,&dn,int_dist);         //calc intersection point

		return int_dist;
	}
	else
		return 0;
}

//$$fix get_sphere_int_dist(vms_vector *w,fix dist,fix rad);
//$$
//$$#pragma aux get_sphere_int_dist parm [esi] [ebx] [ecx] value [eax] modify exact [eax ebx ecx edx] = 
//$$    "mov eax,ebx"           
//$$    "imul eax"                      
//$$                                                    
//$$    "mov ebx,eax"           
//$$   "mov eax,ecx"            
//$$    "mov ecx,edx"           
//$$                                                    
//$$    "imul eax"                      
//$$                                                    
//$$    "sub eax,ebx"           
//$$    "sbb edx,ecx"           
//$$                                                    
//$$    "call quad_sqrt"        
//$$                                                    
//$$    "push eax"                      
//$$                                                    
//$$    "push ebx"                      
//$$    "push ecx"                      
//$$                                                    
//$$    "mov eax,[esi]" 
//$$    "imul eax"                      
//$$    "mov ebx,eax"           
//$$    "mov ecx,edx"           
//$$    "mov eax,4[esi]"        
//$$    "imul eax"                      
//$$    "add ebx,eax"           
//$$    "adc ecx,edx"           
//$$    "mov eax,8[esi]"        
//$$    "imul eax"                      
//$$    "add eax,ebx"           
//$$    "adc edx,ecx"           
//$$                                                    
//$$    "pop ecx"                       
//$$    "pop ebx"                       
//$$                                                    
//$$    "sub eax,ebx"           
//$$    "sbb edx,ecx"           
//$$                                                    
//$$    "call quad_sqrt"        
//$$                                                    
//$$    "pop ebx"                       
//$$	"sub eax,ebx";
//$$
//$$
//$$//determine if and where a vector intersects with a sphere
//$$//vector defined by p0,p1 
//$$//returns dist if intersects, and fills in intp. if no intersect, return 0
//$$fix check_vector_to_sphere_2(vms_vector *intp,vms_vector *p0,vms_vector *p1,vms_vector *sphere_pos,fix sphere_rad)
//$${
//$$	vms_vector d,w,c;
//$$	fix mag_d,dist,mag_c,mag_w;
//$$	vms_vector wn,dn;
//$$
//$$	vm_vec_sub(&d,p1,p0);
//$$	vm_vec_sub(&w,sphere_pos,p0);
//$$
//$$	//wn = w; mag_w = vm_vec_normalize(&wn);
//$$	//dn = d; mag_d = vm_vec_normalize(&dn);
//$$
//$$	mag_w = vm_vec_copy_normalize(&wn,&w);
//$$	mag_d = vm_vec_copy_normalize(&dn,&d);
//$$
//$$	//vm_vec_cross(&c,&w,&d);
//$$	vm_vec_cross(&c,&wn,&dn);
//$$
//$$	mag_c = vm_vec_mag(&c);
//$$	//mag_d = vm_vec_mag(&d);
//$$
//$$	//dist = fixdiv(mag_c,mag_d);
//$$
//$$dist = fixmul(mag_c,mag_w);
//$$
//$$	if (dist < sphere_rad) {        //we intersect.  find point of intersection
//$$		fix int_dist;                   //length of vector to intersection point
//$$		fix k;                                  //portion of p0p1 we want
//$$//@@		fix dist2,rad2,shorten,mag_w2;
//$$
//$$//@@		mag_w2 = vm_vec_dot(&w,&w);     //the square of the magnitude
//$$//@@		//WHAT ABOUT OVERFLOW???
//$$//@@		dist2 = fixmul(dist,dist);
//$$//@@		rad2 = fixmul(sphere_rad,sphere_rad);
//$$//@@		shorten = fix_sqrt(rad2 - dist2);
//$$//@@		int_dist = fix_sqrt(mag_w2 - dist2) - shorten;
//$$
//$$		int_dist = get_sphere_int_dist(&w,dist,sphere_rad);
//$$
//$$if (labs(int_dist) > mag_d)	//I don't know why this would happen
//$$	if (int_dist > 0)
//$$		k = f1_0;
//$$	else
//$$		k = -f1_0;
//$$else
//$$		k = fixdiv(int_dist,mag_d);
//$$
//$$//		vm_vec_scale(&d,k);                     //vec from p0 to intersection point
//$$//		vm_vec_add(intp,p0,&d);         //intersection point
//$$		vm_vec_scale_add(intp,p0,&d,k);	//calc new intersection point
//$$
//$$		return int_dist;
//$$	}
//$$	else
//$$		return 0;       //no intersection
//$$}

//determine if a vector intersects with an object
//if no intersects, returns 0, else fills in intp and returns dist
fix check_vector_to_object(vms_vector *intp,vms_vector *p0,vms_vector *p1,fix rad,object *obj,object *otherobj)
{
	fix size = obj->size;

	if (obj->type == OBJ_ROBOT && Robot_info[obj->id].attack_type)
		size = (size*3)/4;

	//if obj is player, and bumping into other player or a weapon of another coop player, reduce radius
	if (obj->type == OBJ_PLAYER && 
		 	((otherobj->type == OBJ_PLAYER) ||
	 		((Game_mode&GM_MULTI_COOP) && otherobj->type == OBJ_WEAPON && otherobj->ctype.laser_info.parent_type == OBJ_PLAYER)))
		size = size/2;

	return check_vector_to_sphere_1(intp,p0,p1,&obj->pos,size+rad);

}


#define MAX_SEGS_VISITED 100
int n_segs_visited;
short segs_visited[MAX_SEGS_VISITED];

int fvi_nest_count;

//these vars are used to pass vars from fvi_sub() to find_vector_intersection()
int fvi_hit_object;	// object number of object hit in last find_vector_intersection call.
int fvi_hit_seg;		// what segment the hit point is in
int fvi_hit_side;		// what side was hit
int fvi_hit_side_seg;// what seg the hitside is in
vms_vector wall_norm;	//ptr to surface normal of hit wall
int fvi_hit_seg2;		// what segment the hit point is in

int fvi_sub(vms_vector *intp,int *ints,vms_vector *p0,int startseg,vms_vector *p1,fix rad,short thisobjnum,int *ignore_obj_list,int flags,int *seglist,int *n_segs,int entry_seg);

//What the hell is fvi_hit_seg for???

//Find out if a vector intersects with anything.
//Fills in hit_data, an fvi_info structure (see header file).
//Parms:
//  p0 & startseg 	describe the start of the vector
//  p1 					the end of the vector
//  rad 					the radius of the cylinder
//  thisobjnum 		used to prevent an object with colliding with itself
//  ingore_obj			ignore collisions with this object
//  check_obj_flag	determines whether collisions with objects are checked
//Returns the hit_data->hit_type
int find_vector_intersection(fvi_query *fq,fvi_info *hit_data)
{
	int hit_type,hit_seg,hit_seg2;
	vms_vector hit_pnt;
	int i;

	Assert(fq->ignore_obj_list != (int *)(-1));
	Assert((fq->startseg <= Highest_segment_index) && (fq->startseg >= 0));

	fvi_hit_seg = -1;
	fvi_hit_side = -1;

	fvi_hit_object = -1;

	//check to make sure start point is in seg its supposed to be in
	//Assert(check_point_in_seg(p0,startseg,0).centermask==0);	//start point not in seg

	// invalid segnum, so say there is no hit.
	if(fq->startseg < 0 || fq->startseg > Highest_segment_index)
	{

		hit_data->hit_type = HIT_BAD_P0;
		hit_data->hit_pnt = *fq->p0;
		hit_data->hit_seg = hit_data->hit_side = hit_data->hit_object = 0;
		hit_data->hit_side_seg = -1;

		return hit_data->hit_type;
	}

	// Viewer is not in segment as claimed, so say there is no hit.
        if(!(get_seg_masks(fq->p0,fq->startseg,0,__FILE__,__LINE__).centermask==0)) {

		hit_data->hit_type = HIT_BAD_P0;
		hit_data->hit_pnt = *fq->p0;
		hit_data->hit_seg = fq->startseg;
		hit_data->hit_side = hit_data->hit_object = 0;
		hit_data->hit_side_seg = -1;

		return hit_data->hit_type;
	}

	segs_visited[0] = fq->startseg;

	n_segs_visited=1;

	fvi_nest_count = 0;

	hit_seg2 = fvi_hit_seg2 = -1;

	hit_type = fvi_sub(&hit_pnt,&hit_seg2,fq->p0,fq->startseg,fq->p1,fq->rad,fq->thisobjnum,fq->ignore_obj_list,fq->flags,hit_data->seglist,&hit_data->n_segs,-2);
	//!!hit_seg = find_point_seg(&hit_pnt,fq->startseg);
        if (hit_seg2!=-1 && !get_seg_masks(&hit_pnt,hit_seg2,0,__FILE__,__LINE__).centermask)
		hit_seg = hit_seg2;
	else
		hit_seg = find_point_seg(&hit_pnt,fq->startseg);

//MATT: TAKE OUT THIS HACK AND FIX THE BUGS!
	if (hit_type == HIT_WALL && hit_seg==-1)
                if (fvi_hit_seg2!=-1 && get_seg_masks(&hit_pnt,fvi_hit_seg2,0,__FILE__,__LINE__).centermask==0)
			hit_seg = fvi_hit_seg2;

	if (hit_seg == -1) {
		int new_hit_type;
		int new_hit_seg2=-1;
		vms_vector new_hit_pnt;

		//because of code that deal with object with non-zero radius has
		//problems, try using zero radius and see if we hit a wall

		new_hit_type = fvi_sub(&new_hit_pnt,&new_hit_seg2,fq->p0,fq->startseg,fq->p1,0,fq->thisobjnum,fq->ignore_obj_list,fq->flags,hit_data->seglist,&hit_data->n_segs,-2);
		(void)new_hit_type; // FIXME! This should become hit_type, right?

		if (new_hit_seg2 != -1) {
			hit_seg = new_hit_seg2;
			hit_pnt = new_hit_pnt;
		}
	}


if (hit_seg!=-1 && fq->flags&FQ_GET_SEGLIST)
	if (hit_seg != hit_data->seglist[hit_data->n_segs-1] && hit_data->n_segs<MAX_FVI_SEGS-1)
		hit_data->seglist[hit_data->n_segs++] = hit_seg;

if (hit_seg!=-1 && fq->flags&FQ_GET_SEGLIST)
	for (i=0;i<hit_data->n_segs && i<MAX_FVI_SEGS-1;i++)
		if (hit_data->seglist[i] == hit_seg) {
			hit_data->n_segs = i+1;
			break;
		}

//I'm sorry to say that sometimes the seglist isn't correct.  I did my
//best.  Really.


//{	//verify hit list
//
//	int i,ch;
//
//	Assert(hit_data->seglist[0] == startseg);
//
//	for (i=0;i<hit_data->n_segs-1;i++) {
//		for (ch=0;ch<6;ch++)
//			if (Segments[hit_data->seglist[i]].children[ch] == hit_data->seglist[i+1])
//				break;
//		Assert(ch<6);
//	}
//
//	Assert(hit_data->seglist[hit_data->n_segs-1] == hit_seg);
//}
	

//MATT: PUT THESE ASSERTS BACK IN AND FIX THE BUGS!
//!!	Assert(hit_seg!=-1);
//!!	Assert(!((hit_type==HIT_WALL) && (hit_seg == -1)));
	//When this assert happens, get Matt.  Matt:  Look at hit_seg2 & 
	//fvi_hit_seg.  At least one of these should be set.  Why didn't 
	//find_new_seg() find something?

//	Assert(fvi_hit_seg==-1 || fvi_hit_seg == hit_seg);

	Assert(!(hit_type==HIT_OBJECT && fvi_hit_object==-1));

	hit_data->hit_type		= hit_type;
	hit_data->hit_pnt 		= hit_pnt;
	hit_data->hit_seg 		= hit_seg;
	hit_data->hit_side 		= fvi_hit_side;	//looks at global
	hit_data->hit_side_seg	= fvi_hit_side_seg;	//looks at global
	hit_data->hit_object		= fvi_hit_object;	//looks at global
	hit_data->hit_wallnorm	= wall_norm;		//looks at global

//      if(hit_seg!=-1 && get_seg_masks(&hit_data->hit_pnt,hit_data->hit_seg,0,__FILE__,__LINE__).centermask!=0)
//		Int3();

	return hit_type;

}

//--unused-- fix check_dist(vms_vector *v0,vms_vector *v1)
//--unused-- {
//--unused-- 	return vm_vec_dist(v0,v1);
//--unused-- }

int obj_in_list(int objnum,int *obj_list)
{
	int t;

	while ((t=*obj_list)!=-1 && t!=objnum) obj_list++;

	return (t==objnum);

}

int check_trans_wall(vms_vector *pnt,segment *seg,int sidenum,int facenum);

int fvi_sub(vms_vector *intp,int *ints,vms_vector *p0,int startseg,vms_vector *p1,fix rad,short thisobjnum,int *ignore_obj_list,int flags,int *seglist,int *n_segs,int entry_seg)
{
	segment *seg;				//the segment we're looking at
	int startmask,endmask;	//mask of faces
	//@@int sidemask;				//mask of sides - can be on back of face but not side
	int centermask;			//where the center point is
	int objnum;
	segmasks masks;
	vms_vector hit_point,closest_hit_point = { 0, 0, 0 }; 	//where we hit
	fix d,closest_d=0x7fffffff;					//distance to hit point
	int hit_type=HIT_NONE;							//what sort of hit
	int hit_seg=-1;
	int hit_none_seg=-1;
	int hit_none_n_segs=0;
	int hit_none_seglist[MAX_FVI_SEGS];
	int cur_nest_level = fvi_nest_count;

	//fvi_hit_object = -1;

	if (flags&FQ_GET_SEGLIST)
		*seglist = startseg; 
	*n_segs=1;

	seg = &Segments[startseg];

	fvi_nest_count++;

	//first, see if vector hit any objects in this segment
	if (flags & FQ_CHECK_OBJS)
		for (objnum=seg->objects;objnum!=-1;objnum=Objects[objnum].next)
			if (	!(Objects[objnum].flags & OF_SHOULD_BE_DEAD) &&
				 	!(thisobjnum == objnum ) &&
				 	(ignore_obj_list==NULL || !obj_in_list(objnum,ignore_obj_list)) &&
				 	!laser_are_related( objnum, thisobjnum ) &&
				 	!((thisobjnum  > -1)	&&
				  		(CollisionResult[Objects[thisobjnum].type][Objects[objnum].type] == RESULT_NOTHING ) &&
			 	 		(CollisionResult[Objects[objnum].type][Objects[thisobjnum].type] == RESULT_NOTHING ))) {
				int fudged_rad = rad;

				//	If this is a robot:robot collision, only do it if both of them have attack_type != 0 (eg, green guy)
				if (Objects[thisobjnum].type == OBJ_ROBOT)
					if (Objects[objnum].type == OBJ_ROBOT)
						if (!(Robot_info[Objects[objnum].id].attack_type && Robot_info[Objects[thisobjnum].id].attack_type))
							continue;

				if (Objects[thisobjnum].type == OBJ_ROBOT && Robot_info[Objects[thisobjnum].id].attack_type)
					fudged_rad = (rad*3)/4;

				//if obj is player, and bumping into other player or a weapon of another coop player, reduce radius
				if (Objects[thisobjnum].type == OBJ_PLAYER && 
						((Objects[objnum].type == OBJ_PLAYER) ||
						((Game_mode&GM_MULTI_COOP) &&  Objects[objnum].type == OBJ_WEAPON && Objects[objnum].ctype.laser_info.parent_type == OBJ_PLAYER)))
					fudged_rad = rad/2;	//(rad*3)/4;

				d = check_vector_to_object(&hit_point,p0,p1,fudged_rad,&Objects[objnum],&Objects[thisobjnum]);

				if (d)          //we have intersection
					if (d < closest_d) {
						fvi_hit_object = objnum; 
						Assert(fvi_hit_object!=-1);
						closest_d = d; 
						closest_hit_point = hit_point; 
						hit_type=HIT_OBJECT;
					}
			}

	if (	(thisobjnum > -1 ) && (CollisionResult[Objects[thisobjnum].type][OBJ_WALL] == RESULT_NOTHING ) )
		rad = 0;		//HACK - ignore when edges hit walls

	//now, check segment walls

        startmask = get_seg_masks(p0,startseg,rad,__FILE__,__LINE__).facemask;

        masks = get_seg_masks(p1,startseg,rad,__FILE__,__LINE__);    //on back of which faces?
	endmask = masks.facemask;
	//@@sidemask = masks.sidemask;
	centermask = masks.centermask;

	if (centermask==0) hit_none_seg = startseg;

	if (endmask != 0) {                             //on the back of at least one face

		int side,bit,face;

		//for each face we are on the back of, check if intersected

		for (side=0,bit=1;side<6 && endmask>=bit;side++) {
			int num_faces;
			num_faces = get_num_faces(&seg->sides[side]);

			if (num_faces == 0)
				num_faces = 1;

			// commented out by mk on 02/13/94:: if ((num_faces=seg->sides[side].num_faces)==0) num_faces=1;

			for (face=0;face<2;face++,bit<<=1) {

				if (endmask & bit) {            //on the back of this face
					int face_hit_type;      //in what way did we hit the face?


					if (seg->children[side] == entry_seg)
						continue;		//don't go back through entry side

					//did we go through this wall/door?

					if (startmask & bit)		//start was also though.  Do extra check
						face_hit_type = special_check_line_to_face( &hit_point,
										p0,p1,seg,side,
										face,
										((num_faces==1)?4:3),rad);
					else
						//NOTE LINK TO ABOVE!!
						face_hit_type = check_line_to_face( &hit_point,
										p0,p1,seg,side,
										face,
										((num_faces==1)?4:3),rad);

	
					if (face_hit_type) {            //through this wall/door
						int wid_flag;

						//if what we have hit is a door, check the adjoining seg

						if ( (thisobjnum == Players[Player_num].objnum) && (cheats.ghostphysics) )	{
							wid_flag = WALL_IS_DOORWAY(seg, side);
							if (seg->children[side] >= 0 ) 
 								wid_flag |= WID_FLY_FLAG;
						} else {
							wid_flag = WALL_IS_DOORWAY(seg, side);
						}

						if ((wid_flag & WID_FLY_FLAG) ||
							((wid_flag == WID_TRANSPARENT_WALL) && 
								((flags & FQ_TRANSWALL) || (flags & FQ_TRANSPOINT && check_trans_wall(&hit_point,seg,side,face))))) {

							int newsegnum;
							vms_vector sub_hit_point;
							int sub_hit_type,sub_hit_seg;
							vms_vector save_wall_norm = wall_norm;
							int save_hit_objnum=fvi_hit_object;
							int i;

							//do the check recursively on the next seg.

							newsegnum = seg->children[side];

							for (i=0;i<n_segs_visited && newsegnum!=segs_visited[i];i++);

							if (i==n_segs_visited) {                //haven't visited here yet
								int temp_seglist[MAX_FVI_SEGS],temp_n_segs;
								
								segs_visited[n_segs_visited++] = newsegnum;

								if (n_segs_visited >= MAX_SEGS_VISITED)
									goto quit_looking;		//we've looked a long time, so give up

								sub_hit_type = fvi_sub(&sub_hit_point,&sub_hit_seg,p0,newsegnum,p1,rad,thisobjnum,ignore_obj_list,flags,temp_seglist,&temp_n_segs,startseg);

								if (sub_hit_type != HIT_NONE) {

									d = vm_vec_dist(&sub_hit_point,p0);

									if (d < closest_d) {

										closest_d = d; 
										closest_hit_point = sub_hit_point;
										hit_type = sub_hit_type;
										if (sub_hit_seg!=-1) hit_seg = sub_hit_seg;

										//copy seglist
										if (flags&FQ_GET_SEGLIST) {
											int ii;
											for (ii=0;i<temp_n_segs && *n_segs<MAX_FVI_SEGS-1;)
												seglist[(*n_segs)++] = temp_seglist[ii++];
										}

										Assert(*n_segs < MAX_FVI_SEGS);
									}
									else {
										wall_norm = save_wall_norm;     //global could be trashed
										fvi_hit_object = save_hit_objnum;
 									}

								}
								else {
									wall_norm = save_wall_norm;     //global could be trashed
									if (sub_hit_seg!=-1) hit_none_seg = sub_hit_seg;
									//copy seglist
									if (flags&FQ_GET_SEGLIST) {
										int ii;
										for (ii=0;ii<temp_n_segs && ii<MAX_FVI_SEGS-1;ii++)
											hit_none_seglist[ii] = temp_seglist[ii];
									}
									hit_none_n_segs = temp_n_segs;
								}
							}
						}
						else {          //a wall
																
								//is this the closest hit?
	
								d = vm_vec_dist(&hit_point,p0);
	
								if (d < closest_d) {
									closest_d = d; 
									closest_hit_point = hit_point;
									hit_type = HIT_WALL;
									
									#ifdef COMPACT_SEGS
										get_side_normal(seg, side, face, &wall_norm );
									#else
										wall_norm = seg->sides[side].normals[face];	
									#endif
									
	
                                                                        if (get_seg_masks(&hit_point,startseg,rad,__FILE__,__LINE__).centermask==0)
										hit_seg = startseg;             //hit in this segment
									else
										fvi_hit_seg2 = startseg;

									fvi_hit_seg = hit_seg;
									fvi_hit_side =  side;
									fvi_hit_side_seg = startseg;

								}
						}
					}
				}
			}
		}
	}

//      Assert(centermask==0 || hit_seg!=startseg);

//      Assert(sidemask==0);            //Error("Didn't find side we went though");

quit_looking:
	;

	if (hit_type == HIT_NONE)
         {     //didn't hit anything, return end point
		int i;

		*intp = *p1;
		*ints = hit_none_seg;
		//MATT: MUST FIX THIS!!!!
		//Assert(!centermask);

		if (hit_none_seg!=-1) {			///(centermask == 0)
			if (flags&FQ_GET_SEGLIST)
				//copy seglist
				for (i=0;i<hit_none_n_segs && *n_segs<MAX_FVI_SEGS-1;)
					seglist[(*n_segs)++] = hit_none_seglist[i++];
		}
		else
			if (cur_nest_level!=0)
				*n_segs=0;

	}
	else {
		*intp = closest_hit_point;
		if (hit_seg==-1)
                 {
			if (fvi_hit_seg2 != -1)
				*ints = fvi_hit_seg2;
			else
				*ints = hit_none_seg;
                 }
		else
			*ints = hit_seg;
	}

	Assert(!(hit_type==HIT_OBJECT && fvi_hit_object==-1));

	return hit_type;

}

//--unused-- //compute the magnitude of a 2d vector
//--unused-- fix mag2d(vec2d *v);
//--unused-- #pragma aux mag2d parm [esi] value [eax] modify exact [eax ebx ecx edx] = 
//--unused--    "mov    eax,[esi]"              
//--unused--    "imul   eax"                            
//--unused--    "mov    ebx,eax"                        
//--unused--    "mov    ecx,edx"                        
//--unused--    "mov    eax,4[esi]"             
//--unused--    "imul   eax"                            
//--unused--    "add    eax,ebx"                        
//--unused--    "adc    edx,ecx"                        
//--unused-- 	"call	quad_sqrt";

//--unused-- //returns mag
//--unused-- fix normalize_2d(vec2d *v)
//--unused-- {
//--unused-- 	fix mag;
//--unused-- 
//--unused-- 	mag = mag2d(v);
//--unused-- 
//--unused-- 	v->i = fixdiv(v->i,mag);
//--unused-- 	v->j = fixdiv(v->j,mag);
//--unused-- 
//--unused-- 	return mag;
//--unused-- }

#include "textures.h"
#include "texmerge.h"

#define cross(v0,v1) (fixmul((v0)->i,(v1)->j) - fixmul((v0)->j,(v1)->i))

//finds the uv coords of the given point on the given seg & side
//fills in u & v
void find_hitpoint_uv(fix *u,fix *v,vms_vector *pnt,segment *seg,int sidenum,int facenum)
{
	vms_vector_array *pnt_array;
	vms_vector_array normal_array;
	int segnum = seg-Segments;
	int num_faces;
	int biggest,ii,jj;
	side *side = &seg->sides[sidenum];
	int vertex_list[6],vertnum_list[6];
 	vec2d p1,vec0,vec1,checkp;	//@@,checkv;
	uvl uvls[3];
	fix k0,k1;
	int i;

	//do lasers pass through illusory walls?

	//when do I return 0 & 1 for non-transparent walls?

        create_abs_vertex_lists(&num_faces,vertex_list,segnum,sidenum ,__FILE__,__LINE__);
	create_all_vertnum_lists(&num_faces,vertnum_list,segnum,sidenum);

	//now the hard work.

	//1. find what plane to project this wall onto to make it a 2d case

	#ifdef COMPACT_SEGS
		get_side_normal(seg, sidenum, facenum, (vms_vector *)&normal_array );
	#else
		memcpy( &normal_array, &side->normals[facenum], sizeof(vms_vector_array) );
	#endif
  	biggest = 0;

	if (abs(normal_array.xyz[1]) > abs(normal_array.xyz[biggest])) biggest = 1;
	if (abs(normal_array.xyz[2]) > abs(normal_array.xyz[biggest])) biggest = 2;

	if (biggest == 0) ii=1; else ii=0;
	if (biggest == 2) jj=1; else jj=2;

	//2. compute u,v of intersection point

	//vec from 1 -> 0
	pnt_array = (vms_vector_array *)&Vertices[vertex_list[facenum*3+1]];
	p1.i = pnt_array->xyz[ii];
	p1.j = pnt_array->xyz[jj];

	pnt_array = (vms_vector_array *)&Vertices[vertex_list[facenum*3+0]];
	vec0.i = pnt_array->xyz[ii] - p1.i;
	vec0.j = pnt_array->xyz[jj] - p1.j;

	//vec from 1 -> 2
	pnt_array = (vms_vector_array *)&Vertices[vertex_list[facenum*3+2]];
	vec1.i = pnt_array->xyz[ii] - p1.i;
	vec1.j = pnt_array->xyz[jj] - p1.j;

	//vec from 1 -> checkpoint
	pnt_array = (vms_vector_array *)pnt;
	checkp.i = pnt_array->xyz[ii];
	checkp.j = pnt_array->xyz[jj];

	//@@checkv.i = checkp.i - p1.i;
	//@@checkv.j = checkp.j - p1.j;

	k1 = -fixdiv(cross(&checkp,&vec0) + cross(&vec0,&p1),cross(&vec0,&vec1));
	if (vec0.i)
		k0 = fixdiv(fixmul(-k1,vec1.i) + checkp.i - p1.i,vec0.i);
	else
		k0 = fixdiv(fixmul(-k1,vec1.j) + checkp.j - p1.j,vec0.j);

	for (i=0;i<3;i++)
		uvls[i] = side->uvls[vertnum_list[facenum*3+i]];

	*u = uvls[1].u + fixmul( k0,uvls[0].u - uvls[1].u) + fixmul(k1,uvls[2].u - uvls[1].u);
	*v = uvls[1].v + fixmul( k0,uvls[0].v - uvls[1].v) + fixmul(k1,uvls[2].v - uvls[1].v);
}

//check if a particular point on a wall is a transparent pixel
//returns 1 if can pass though the wall, else 0
int check_trans_wall(vms_vector *pnt,segment *seg,int sidenum,int facenum)
{
	grs_bitmap *bm;
	side *side = &seg->sides[sidenum];
	int bmx,bmy;
	fix u,v;

	Assert(WALL_IS_DOORWAY(seg,sidenum) == WID_TRANSPARENT_WALL);

	find_hitpoint_uv(&u,&v,pnt,seg,sidenum,facenum);

	if (side->tmap_num2 != 0)	{
		bm = texmerge_get_cached_bitmap( side->tmap_num, side->tmap_num2 );
	} else {
		bm = &GameBitmaps[Textures[side->tmap_num].index];
		PIGGY_PAGE_IN( Textures[side->tmap_num] );
	}

	if (bm->bm_flags & BM_FLAG_RLE)
		bm = rle_expand_texture(bm);

	bmx = ((unsigned) f2i(u*bm->bm_w)) % bm->bm_w;
	bmy = ((unsigned) f2i(v*bm->bm_h)) % bm->bm_h;

//note: the line above had -v, but that was wrong, so I changed it.  if 
//something doesn't work, and you want to make it negative again, you 
//should figure out what's going on.

	return (gr_gpixel (bm, bmx, bmy) == 255);
}

//new function for Mike
//note: n_segs_visited must be set to zero before this is called
int sphere_intersects_wall(vms_vector *pnt,int segnum,fix rad,int *hseg,int *hside,int *hface)
{
	int facemask;
	segment *seg;

	segs_visited[n_segs_visited++] = segnum;

        facemask = get_seg_masks(pnt,segnum,rad,__FILE__,__LINE__).facemask;

	seg = &Segments[segnum];

	if (facemask != 0) {				//on the back of at least one face

		int side,bit,face;

		//for each face we are on the back of, check if intersected

		for (side=0,bit=1;side<6 && facemask>=bit;side++) {

			for (face=0;face<2;face++,bit<<=1) {

				if (facemask & bit) {            //on the back of this face
					int face_hit_type;      //in what way did we hit the face?
					int num_faces,vertex_list[6];

					//did we go through this wall/door?

                                        create_abs_vertex_lists(&num_faces,vertex_list,seg-Segments,side ,__FILE__,__LINE__);

					face_hit_type = check_sphere_to_face( pnt,seg,&seg->sides[side],
										face,((num_faces==1)?4:3),rad,vertex_list);

					if (face_hit_type) {            //through this wall/door
						int child,i;

						//if what we have hit is a door, check the adjoining seg

						child = seg->children[side];

						for (i=0;i<n_segs_visited && child!=segs_visited[i];i++);

						if (i==n_segs_visited) {                //haven't visited here yet

							if (!IS_CHILD(child))
							{
								if (hseg) *hseg = segnum;
								if (hside) *hside = side;
								if (hface) *hface = face;
								return 1;
							}
							else {

								if (sphere_intersects_wall(pnt,child,rad,hseg,hside,hface))
									return 1;
							}
						}
					}
				}
			}
		}
	}

	return 0;
}

//Returns true if the object is through any walls
int object_intersects_wall(object *objp)
{
	n_segs_visited = 0;

	return sphere_intersects_wall(&objp->pos,objp->segnum,objp->size,NULL,NULL,NULL);
}

int object_intersects_wall_d(object *objp,int *hseg,int *hside,int *hface)
{
	n_segs_visited = 0;

	return sphere_intersects_wall(&objp->pos,objp->segnum,objp->size,hseg,hside,hface);
}
