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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>

#include "error.h"

#include "inferno.h"

#include "robot.h"
#include "object.h"
#include "polyobj.h"
#include "mono.h"

int	N_robot_types = 0;
int	N_robot_joints = 0;

//	Robot stuff
robot_info Robot_info[MAX_ROBOT_TYPES];

//Big array of joint positions.  All robots index into this array

#define deg(a) ((int) (a) * 32768 / 180)

//test data for one robot
jointpos Robot_joints[MAX_ROBOT_JOINTS] = {

//gun 0
					{2,{deg(-30),0,0}},		//rest (2 joints)
					{3,{deg(-40),0,0}},

					{2,{deg(0),0,0}},			//alert
					{3,{deg(0),0,0}},
		
					{2,{deg(0),0,0}},			//fire
					{3,{deg(0),0,0}},
		
					{2,{deg(50),0,0}},		//recoil
					{3,{deg(-50),0,0}},
		
					{2,{deg(10),0,deg(70)}},		//flinch
					{3,{deg(0),deg(20),0}},
		
//gun 1
					{4,{deg(-30),0,0}},		//rest (2 joints)
					{5,{deg(-40),0,0}},

					{4,{deg(0),0,0}},			//alert
					{5,{deg(0),0,0}},
		
					{4,{deg(0),0,0}},			//fire
					{5,{deg(0),0,0}},
		
					{4,{deg(50),0,0}},		//recoil
					{5,{deg(-50),0,0}},
		
					{4,{deg(20),0,deg(-50)}},	//flinch
					{5,{deg(0),0,deg(20)}},
		
//rest of body (the head)

					{1,{deg(70),0,0}},		//rest (1 joint, head)

					{1,{deg(0),0,0}},			//alert
		
					{1,{deg(0),0,0}},			//fire
		
					{1,{deg(0),0,0}},			//recoil

					{1,{deg(-20),deg(15),0}},			//flinch


};

//given an object and a gun number, return position in 3-space of gun
//fills in gun_point
void calc_gun_point(vms_vector *gun_point,object *obj,int gun_num)
{
	polymodel *pm;
	robot_info *r;
	vms_vector pnt;
	vms_matrix m;
	int mn;				//submodel number

	Assert(obj->render_type==RT_POLYOBJ || obj->render_type==RT_MORPH);
	Assert(obj->id < N_robot_types);

	r = &Robot_info[obj->id];
	pm =&Polygon_models[r->model_num];

	if (gun_num >= r->n_guns)
	{
		mprintf((1, "Bashing gun num %d to 0.\n", gun_num));
		//Int3();
		gun_num = 0;
	}

//	Assert(gun_num < r->n_guns);

	pnt = r->gun_points[gun_num];
	mn = r->gun_submodels[gun_num];

	//instance up the tree for this gun
	while (mn != 0) {
		vms_vector tpnt;

		vm_angles_2_matrix(&m,&obj->rtype.pobj_info.anim_angles[mn]);
		vm_transpose_matrix(&m);
		vm_vec_rotate(&tpnt,&pnt,&m);

		vm_vec_add(&pnt,&tpnt,&pm->submodel_offsets[mn]);

		mn = pm->submodel_parents[mn];
	}

	//now instance for the entire object

	vm_copy_transpose_matrix(&m,&obj->orient);
	vm_vec_rotate(gun_point,&pnt,&m);
	vm_vec_add2(gun_point,&obj->pos);
	
}

//fills in ptr to list of joints, and returns the number of joints in list
//takes the robot type (object id), gun number, and desired state
int robot_get_anim_state(jointpos **jp_list_ptr,int robot_type,int gun_num,int state)
{

	Assert(gun_num <= Robot_info[robot_type].n_guns);

	*jp_list_ptr = &Robot_joints[Robot_info[robot_type].anim_states[gun_num][state].offset];

	return Robot_info[robot_type].anim_states[gun_num][state].n_joints;

}


//for test, set a robot to a specific state
void set_robot_state(object *obj,int state)
{
	int g,j,jo;
	robot_info *ri;
	jointlist *jl;

	Assert(obj->type == OBJ_ROBOT);

	ri = &Robot_info[obj->id];

	for (g=0;g<ri->n_guns+1;g++) {

		jl = &ri->anim_states[g][state];

		jo = jl->offset;

		for (j=0;j<jl->n_joints;j++,jo++) {
			int jn;

			jn = Robot_joints[jo].jointnum;

			obj->rtype.pobj_info.anim_angles[jn] = Robot_joints[jo].angles;

		}
	}
}

#include "mono.h"

//--unused-- int cur_state=0;

//--unused-- test_anim_states()
//--unused-- {
//--unused-- 	set_robot_state(&Objects[1],cur_state);
//--unused-- 
//--unused-- 	mprintf(0,"Robot in state %d\n",cur_state);
//--unused-- 
//--unused-- 	cur_state = (cur_state+1)%N_ANIM_STATES;
//--unused-- 
//--unused-- }

//set the animation angles for this robot.  Gun fields of robot info must
//be filled in.
void robot_set_angles(robot_info *r,polymodel *pm,vms_angvec angs[N_ANIM_STATES][MAX_SUBMODELS])
{
	int m,g,state;
	int gun_nums[MAX_SUBMODELS];			//which gun each submodel is part of

	for (m=0;m<pm->n_models;m++)
		gun_nums[m] = r->n_guns;		//assume part of body...

	gun_nums[0] = -1;		//body never animates, at least for now

	for (g=0;g<r->n_guns;g++) {
		m = r->gun_submodels[g];

		while (m != 0) {
			gun_nums[m] = g;				//...unless we find it in a gun
			m = pm->submodel_parents[m];
		}
	}

	for (g=0;g<r->n_guns+1;g++) {

		//mprintf(0,"Gun %d:\n",g);

		for (state=0;state<N_ANIM_STATES;state++) {

			//mprintf(0," State %d:\n",state);

			r->anim_states[g][state].n_joints = 0;
			r->anim_states[g][state].offset = N_robot_joints;

			for (m=0;m<pm->n_models;m++) {
				if (gun_nums[m] == g) {
					//mprintf(0,"  Joint %d: %x %x %x\n",m,angs[state][m].pitch,angs[state][m].bank,angs[state][m].head);
					Robot_joints[N_robot_joints].jointnum = m;
					Robot_joints[N_robot_joints].angles = angs[state][m];
					r->anim_states[g][state].n_joints++;
					N_robot_joints++;
					Assert(N_robot_joints < MAX_ROBOT_JOINTS);
				}
			}
		}
	}

}

/*
 * reads a jointlist structure from a CFILE
 */
static void jointlist_read(jointlist *jl, CFILE *fp)
{
	jl->n_joints = cfile_read_short(fp);
	jl->offset = cfile_read_short(fp);
}

/*
 * reads a robot_info structure from a CFILE
 */
void robot_info_read(robot_info *ri, CFILE *fp)
{
	int i, j;
	
	ri->model_num = cfile_read_int(fp);
	for (i = 0; i < MAX_GUNS; i++)
		cfile_read_vector(&(ri->gun_points[i]), fp);
	cfread(ri->gun_submodels, MAX_GUNS, 1, fp);

	ri->exp1_vclip_num = cfile_read_short(fp);
	ri->exp1_sound_num = cfile_read_short(fp);

	ri->exp2_vclip_num = cfile_read_short(fp);
	ri->exp2_sound_num = cfile_read_short(fp);

	ri->weapon_type = cfile_read_byte(fp);
	ri->weapon_type2 = cfile_read_byte(fp);
	ri->n_guns = cfile_read_byte(fp);
	ri->contains_id = cfile_read_byte(fp);

	ri->contains_count = cfile_read_byte(fp);
	ri->contains_prob = cfile_read_byte(fp);
	ri->contains_type = cfile_read_byte(fp);
	ri->kamikaze = cfile_read_byte(fp);

	ri->score_value = cfile_read_short(fp);
	ri->badass = cfile_read_byte(fp);
	ri->energy_drain = cfile_read_byte(fp);

	ri->lighting = cfile_read_fix(fp);
	ri->strength = cfile_read_fix(fp);

	ri->mass = cfile_read_fix(fp);
	ri->drag = cfile_read_fix(fp);

	for (i = 0; i < NDL; i++)
		ri->field_of_view[i] = cfile_read_fix(fp);
	for (i = 0; i < NDL; i++)
		ri->firing_wait[i] = cfile_read_fix(fp);
	for (i = 0; i < NDL; i++)
		ri->firing_wait2[i] = cfile_read_fix(fp);
	for (i = 0; i < NDL; i++)
		ri->turn_time[i] = cfile_read_fix(fp);
	for (i = 0; i < NDL; i++)
		ri->max_speed[i] = cfile_read_fix(fp);
	for (i = 0; i < NDL; i++)
		ri->circle_distance[i] = cfile_read_fix(fp);
	cfread(ri->rapidfire_count, NDL, 1, fp);

	cfread(ri->evade_speed, NDL, 1, fp);

	ri->cloak_type = cfile_read_byte(fp);
	ri->attack_type = cfile_read_byte(fp);

	ri->see_sound = cfile_read_byte(fp);
	ri->attack_sound = cfile_read_byte(fp);
	ri->claw_sound = cfile_read_byte(fp);
	ri->taunt_sound = cfile_read_byte(fp);

	ri->boss_flag = cfile_read_byte(fp);
	ri->companion = cfile_read_byte(fp);
	ri->smart_blobs = cfile_read_byte(fp);
	ri->energy_blobs = cfile_read_byte(fp);

	ri->thief = cfile_read_byte(fp);
	ri->pursuit = cfile_read_byte(fp);
	ri->lightcast = cfile_read_byte(fp);
	ri->death_roll = cfile_read_byte(fp);

	ri->flags = cfile_read_byte(fp);
	cfread(ri->pad, 3, 1, fp);

	ri->deathroll_sound = cfile_read_byte(fp);
	ri->glow = cfile_read_byte(fp);
	ri->behavior = cfile_read_byte(fp);
	ri->aim = cfile_read_byte(fp);

	for (i = 0; i < MAX_GUNS + 1; i++)
		for (j = 0; j < N_ANIM_STATES; j++)
			jointlist_read(&ri->anim_states[i][j], fp);

	ri->always_0xabcd = cfile_read_int(fp);
}

/*
 * reads a jointpos structure from a CFILE
 */
void jointpos_read(jointpos *jp, CFILE *fp)
{
	jp->jointnum = cfile_read_short(fp);
	cfile_read_angvec(&jp->angles, fp);
}

