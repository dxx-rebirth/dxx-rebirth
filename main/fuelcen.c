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
 * Functions for refueling centers.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "inferno.h"
#include "fuelcen.h"
#include "gameseg.h"
#include "game.h"		// For FrameTime
#include "error.h"
#include "gauges.h"
#include "vclip.h"
#include "fireball.h"
#include "robot.h"

#include "wall.h"
#include "sounds.h"
#include "morph.h"
#include "3d.h"
#include "bm.h"
#include "polyobj.h"
#include "ai.h"
#include "gamemine.h"
#include "gamesave.h"
#include "player.h"
#include "collide.h"
#include "laser.h"
#include "network.h"
#include "multi.h"
#include "multibot.h"

#include "gameseq.h"

// The max number of fuel stations per mine.

fix Fuelcen_refill_speed = i2f(1);
fix Fuelcen_give_amount = i2f(25);
fix Fuelcen_max_amount = i2f(100);

// Every time a robot is created in the morphing code, it decreases capacity of the morpher
// by this amount... when capacity gets to 0, no more morphers...
fix EnergyToCreateOneRobot = i2f(1);

int Fuelcen_control_center_destroyed = 0;
int Fuelcen_seconds_left = 0;

#define MATCEN_HP_DEFAULT			F1_0*500; // Hitpoints
#define MATCEN_INTERVAL_DEFAULT	F1_0*5;	//  5 seconds

matcen_info RobotCenters[MAX_ROBOT_CENTERS];
int Num_robot_centers;

FuelCenter Station[MAX_NUM_FUELCENS];
int Num_fuelcenters = 0;

segment * PlayerSegment= NULL;

#ifdef EDITOR
char	Special_names[MAX_CENTER_TYPES][11] = {
	"NOTHING   ",
	"FUELCEN   ",
	"REPAIRCEN ",
	"CONTROLCEN",
	"ROBOTMAKER",
};
#endif

//------------------------------------------------------------
// Resets all fuel center info
void fuelcen_reset()
{
	int i;

	Num_fuelcenters = 0;
	for(i=0; i<MAX_SEGMENTS; i++ )
		Segments[i].special = SEGMENT_IS_NOTHING;

	Fuelcen_control_center_destroyed = 0;
	Num_robot_centers = 0;

}

#ifndef NDEBUG		//this is sometimes called by people from the debugger
void reset_all_robot_centers() 
{
	int i;

	// Remove all materialization centers
	for (i=0; i<Num_segments; i++)
		if (Segments[i].special == SEGMENT_IS_ROBOTMAKER) {
			Segments[i].special = SEGMENT_IS_NOTHING;
			Segments[i].matcen_num = -1;
		}
}
#endif

//------------------------------------------------------------
// Turns a segment into a fully charged up fuel center...
void fuelcen_create( segment * segp)
{
	int	station_type;

	station_type = segp->special;

	switch( station_type )	{
	case SEGMENT_IS_NOTHING:
		return;
	case SEGMENT_IS_FUELCEN:
	case SEGMENT_IS_REPAIRCEN:
	case SEGMENT_IS_CONTROLCEN:
	case SEGMENT_IS_ROBOTMAKER:
		break;
	default:
		Error( "Invalid station type %d in fuelcen.c\n", station_type );
	}

	Assert( (segp != NULL) );
	if ( segp == NULL ) return;

	Assert( Num_fuelcenters < MAX_NUM_FUELCENS );
	Assert( Num_fuelcenters > -1 );

	segp->value = Num_fuelcenters;
	Station[Num_fuelcenters].Type = station_type;
	Station[Num_fuelcenters].MaxCapacity = Fuelcen_max_amount;
	Station[Num_fuelcenters].Capacity = Station[Num_fuelcenters].MaxCapacity;
	Station[Num_fuelcenters].segnum = segp-Segments;
	Station[Num_fuelcenters].Timer = -1;
	Station[Num_fuelcenters].Flag = 0;
//	Station[Num_fuelcenters].NextRobotType = -1;
//	Station[Num_fuelcenters].last_created_obj=NULL;
//	Station[Num_fuelcenters].last_created_sig = -1;
	compute_segment_center(&Station[Num_fuelcenters].Center, segp );

//	if (station_type == SEGMENT_IS_ROBOTMAKER)
//		Station[Num_fuelcenters].Capacity = i2f(Difficulty_level + 3);

	Num_fuelcenters++;
}

//------------------------------------------------------------
// Adds a matcen that already is a special type into the Station array.
// This function is separate from other fuelcens because we don't want values reset.
void matcen_create( segment * segp)
{
	int	station_type = segp->special;

	Assert( (segp != NULL) );
	Assert(station_type == SEGMENT_IS_ROBOTMAKER);
	if ( segp == NULL ) return;

	Assert( Num_fuelcenters < MAX_NUM_FUELCENS );
	Assert( Num_fuelcenters > -1 );

	segp->value = Num_fuelcenters;
	Station[Num_fuelcenters].Type = station_type;
	Station[Num_fuelcenters].Capacity = i2f(Difficulty_level + 3);
	Station[Num_fuelcenters].MaxCapacity = Station[Num_fuelcenters].Capacity;

	Station[Num_fuelcenters].segnum = segp-Segments;
	Station[Num_fuelcenters].Timer = -1;
	Station[Num_fuelcenters].Flag = 0;
//	Station[Num_fuelcenters].NextRobotType = -1;
//	Station[Num_fuelcenters].last_created_obj=NULL;
//	Station[Num_fuelcenters].last_created_sig = -1;
	compute_segment_center(&Station[Num_fuelcenters].Center, segp );

	segp->matcen_num = Num_robot_centers;
	Num_robot_centers++;

	RobotCenters[segp->matcen_num].hit_points = MATCEN_HP_DEFAULT;
	RobotCenters[segp->matcen_num].interval = MATCEN_INTERVAL_DEFAULT;
	RobotCenters[segp->matcen_num].segnum = segp-Segments;
	RobotCenters[segp->matcen_num].fuelcen_num = Num_fuelcenters;

	Num_fuelcenters++;
}

//------------------------------------------------------------
// Adds a segment that already is a special type into the Station array.
void fuelcen_activate( segment * segp, int station_type )
{
	segp->special = station_type;

	if (segp->special == SEGMENT_IS_ROBOTMAKER)
		matcen_create( segp);
	else
		fuelcen_create( segp);
	
}

//	The lower this number is, the more quickly the center can be re-triggered.
//	If it's too low, it can mean all the robots won't be put out, but for about 5
//	robots, that's not real likely.
#define	MATCEN_LIFE (i2f(30-2*Difficulty_level))

//------------------------------------------------------------
//	Trigger (enable) the materialization center in segment segnum
void trigger_matcen(int segnum)
{
	segment		*segp = &Segments[segnum];
	vms_vector	pos, delta;
	FuelCenter	*robotcen;
	int			objnum;

	Assert(segp->special == SEGMENT_IS_ROBOTMAKER);
	Assert(segp->matcen_num < Num_fuelcenters);
	Assert((segp->matcen_num >= 0) && (segp->matcen_num <= Highest_segment_index));

	robotcen = &Station[RobotCenters[segp->matcen_num].fuelcen_num];

	if (robotcen->Enabled == 1)
		return;

	if (!robotcen->Lives)
		return;

	robotcen->Lives--;
	robotcen->Timer = F1_0*1000;	//	Make sure the first robot gets emitted right away.
	robotcen->Enabled = 1;			//	Say this center is enabled, it can create robots.
	robotcen->Capacity = i2f(Difficulty_level + 3);
	robotcen->Disable_time = MATCEN_LIFE;

	//	Create a bright object in the segment.
	pos = robotcen->Center;
	vm_vec_sub(&delta, &Vertices[Segments[segnum].verts[0]], &robotcen->Center);
	vm_vec_scale_add2(&pos, &delta, F1_0/2);
	objnum = obj_create( OBJ_LIGHT, 0, segnum, &pos, NULL, 0, CT_LIGHT, MT_NONE, RT_NONE );
	if (objnum != -1) {
		Objects[objnum].lifeleft = MATCEN_LIFE;
		Objects[objnum].ctype.light_info.intensity = i2f(8);	//	Light cast by a fuelcen.
	} else {
		Int3();
	}
}

#ifdef EDITOR
//------------------------------------------------------------
// Takes away a segment's fuel center properties.
//	Deletes the segment point entry in the FuelCenter list.
void fuelcen_delete( segment * segp )
{
	int i, j;

Restart: ;

	for (i=0; i<Num_fuelcenters; i++ )	{
		if ( Station[i].segnum == segp-Segments )	{

			// If Robot maker is deleted, fix Segments and RobotCenters.
			if (Station[i].Type == SEGMENT_IS_ROBOTMAKER) {
				Num_robot_centers--;

				for (j=segp->matcen_num; j<Num_robot_centers; j++)
					RobotCenters[j] = RobotCenters[j+1];

				for (j=0; j<Num_fuelcenters; j++) {
					if ( Station[j].Type == SEGMENT_IS_ROBOTMAKER )
						if ( Segments[Station[j].segnum].matcen_num > segp->matcen_num )
							Segments[Station[j].segnum].matcen_num--;
				}
			}
		
			Num_fuelcenters--;
			for (j=i; j<Num_fuelcenters; j++ )	{
				Station[i] = Station[i+1];
				Segments[Station[i].segnum].value = i;
			}
			segp->special = 0;
			goto Restart;
		}
	}

}
#endif

#define	ROBOT_GEN_TIME (i2f(5))

object * create_morph_robot( segment *segp, vms_vector *object_pos, int object_id)
{
	short		objnum;
	object	*obj;
	int		default_behavior;

	Players[Player_num].num_robots_level++;
	Players[Player_num].num_robots_total++;

	objnum = obj_create(OBJ_ROBOT, object_id, segp-Segments, object_pos,
				&vmd_identity_matrix, Polygon_models[Robot_info[object_id].model_num].rad,
				CT_AI, MT_PHYSICS, RT_POLYOBJ);

	if ( objnum < 0 ) {
		Int3();
		return NULL;
	}

	obj = &Objects[objnum];

	//Set polygon-object-specific data 

	obj->rtype.pobj_info.model_num = Robot_info[obj->id].model_num;
	obj->rtype.pobj_info.subobj_flags = 0;

	//set Physics info

	obj->mtype.phys_info.mass = Robot_info[obj->id].mass;
	obj->mtype.phys_info.drag = Robot_info[obj->id].drag;

	obj->mtype.phys_info.flags |= (PF_LEVELLING);

	obj->shields = Robot_info[obj->id].strength;
	
	default_behavior = AIB_NORMAL;
	if (object_id == 10)						//	This is a toaster guy!
		default_behavior = AIB_RUN_FROM;

	init_ai_object(obj-Objects, default_behavior, -1 );		//	Note, -1 = segment this robot goes to to hide, should probably be something useful

	create_n_segment_path(obj, 6, -1);		//	Create a 6 segment path from creation point.

	if (default_behavior == AIB_RUN_FROM)
		Ai_local_info[objnum].mode = AIM_RUN_FROM_OBJECT;

	return obj;
}

int Num_extry_robots = 15;

#ifndef NDEBUG
int	FrameCount_last_msg = 0;
#endif

//	----------------------------------------------------------------------------------------------------------
void robotmaker_proc( FuelCenter * robotcen )
{
	fix		dist_to_player;
	vms_vector	cur_object_loc; //, direction;
	int		matcen_num, segnum, objnum;
	object	*obj;
	fix		top_time;
	vms_vector	direction;

	if (robotcen->Enabled == 0)
		return;

	if (robotcen->Disable_time > 0) {
		robotcen->Disable_time -= FrameTime;
		if (robotcen->Disable_time <= 0) {
			robotcen->Enabled = 0;
		}
	}

	//	No robot making in multiplayer mode.
#ifdef NETWORK
#ifndef SHAREWARE
	if ((Game_mode & GM_MULTI) && (!(Game_mode & GM_MULTI_ROBOTS) || !network_i_am_master()))
		return;
#else
	if (Game_mode & GM_MULTI)
		return;
#endif
#endif

	// Wait until transmorgafier has capacity to make a robot...
	if ( robotcen->Capacity <= 0 ) {
		return;
	}

	matcen_num = Segments[robotcen->segnum].matcen_num;

	if ( matcen_num == -1 ) {
		return;
	}

	if (RobotCenters[matcen_num].robot_flags == 0) {
		return;
	}

	// Wait until we have a free slot for this puppy...
   //	  <<<<<<<<<<<<<<<< Num robots in mine >>>>>>>>>>>>>>>>>>>>>>>>>>    <<<<<<<<<<<< Max robots in mine >>>>>>>>>>>>>>>
	if ( (Players[Player_num].num_robots_level - Players[Player_num].num_kills_level) >= (Gamesave_num_org_robots + Num_extry_robots ) ) {
		#ifndef NDEBUG
		if (FrameCount > FrameCount_last_msg + 20) {
			FrameCount_last_msg = FrameCount;
		}
		#endif
		return;
	}

	robotcen->Timer += FrameTime;

	switch( robotcen->Flag )	{
	case 0:		// Wait until next robot can generate
		if (Game_mode & GM_MULTI) 
		{
			top_time = ROBOT_GEN_TIME;	
		}
		else 
		{
			dist_to_player = vm_vec_dist_quick( &ConsoleObject->pos, &robotcen->Center );
			top_time = dist_to_player/64 + d_rand() * 2 + F1_0*2;
			if ( top_time > ROBOT_GEN_TIME )
				top_time = ROBOT_GEN_TIME + d_rand();
			if ( top_time < F1_0*2 )
				top_time = F1_0*3/2 + d_rand()*2;
		}

		if (robotcen->Timer > top_time )	{
			int	count=0;
			int	i, my_station_num = robotcen-Station;
			object *obj;

			//	Make sure this robotmaker hasn't put out its max without having any of them killed.
			for (i=0; i<=Highest_object_index; i++)
				if (Objects[i].type == OBJ_ROBOT)
					if ((Objects[i].matcen_creator^0x80) == my_station_num)
						count++;
			if (count > Difficulty_level + 3) {
				robotcen->Timer /= 2;
				return;
			}

			//	Whack on any robot or player in the matcen segment.
			count=0;
			segnum = robotcen->segnum;
			for (objnum=Segments[segnum].objects;objnum!=-1;objnum=Objects[objnum].next)	{
				count++;
				if ( count > MAX_OBJECTS )	{
					Int3();
					return;
				}
				if (Objects[objnum].type==OBJ_ROBOT) {
					collide_robot_and_materialization_center(&Objects[objnum]);
					robotcen->Timer = top_time/2;
					return;
				} else if (Objects[objnum].type==OBJ_PLAYER ) {
					collide_player_and_materialization_center(&Objects[objnum]);
					robotcen->Timer = top_time/2;
					return;
				}
			}

			compute_segment_center(&cur_object_loc, &Segments[robotcen->segnum]);
			// HACK!!! The 10 under here should be something equal to the 1/2 the size of the segment.
			obj = object_create_explosion(robotcen->segnum, &cur_object_loc, i2f(10), VCLIP_MORPHING_ROBOT );

			if (obj)
				extract_orient_from_segment(&obj->orient,&Segments[robotcen->segnum]);

			if ( Vclip[VCLIP_MORPHING_ROBOT].sound_num > -1 )		{
				digi_link_sound_to_pos( Vclip[VCLIP_MORPHING_ROBOT].sound_num, robotcen->segnum, 0, &cur_object_loc, 0, F1_0 );
			}
			robotcen->Flag	= 1;
			robotcen->Timer = 0;

		}
		break;
	case 1:			// Wait until 1/2 second after VCLIP started.
		if (robotcen->Timer > (Vclip[VCLIP_MORPHING_ROBOT].play_time/2) )	{

			robotcen->Capacity -= EnergyToCreateOneRobot;
			robotcen->Flag = 0;

			robotcen->Timer = 0;
			compute_segment_center(&cur_object_loc, &Segments[robotcen->segnum]);

			// If this is the first materialization, set to valid robot.
			if (RobotCenters[matcen_num].robot_flags != 0) {
				int	type;
				uint	flags;
				sbyte	legal_types[32];		//	32 bits in a word, the width of robot_flags.
				int	num_types, robot_index;

				robot_index = 0;
				num_types = 0;
				flags = RobotCenters[matcen_num].robot_flags;
				while (flags) {
					if (flags & 1)
						legal_types[num_types++] = robot_index;
					flags >>= 1;
					robot_index++;
				}

				if (num_types == 1)
					type = legal_types[0];
				else
					type = legal_types[(d_rand() * num_types) / 32768];

				obj = create_morph_robot(&Segments[robotcen->segnum], &cur_object_loc, type );
				if (obj != NULL) {
#ifndef SHAREWARE
#ifdef NETWORK
					if (Game_mode & GM_MULTI)
						multi_send_create_robot(robotcen-Station, obj-Objects, type);
#endif
#endif
					obj->matcen_creator = (robotcen-Station) | 0x80;

					// Make object faces player...
					vm_vec_sub( &direction, &ConsoleObject->pos,&obj->pos );
					vm_vector_2_matrix( &obj->orient, &direction, &obj->orient.uvec, NULL);
	
					morph_start( obj );
					//robotcen->last_created_obj = obj;
					//robotcen->last_created_sig = robotcen->last_created_obj->signature;
				}
			}
  
		}
		break;
	default:
		robotcen->Flag = 0;
		robotcen->Timer = 0;
	}
}

#define	BASE_CONTROL_CENTER_EXPLOSION_TIME	30
#define	DIFF_CONTROL_CENTER_EXPLOSION_TIME	(BASE_CONTROL_CENTER_EXPLOSION_TIME + (NDL-Difficulty_level-1)*5)

#define COUNTDOWN_VOICE_TIME (i2f(DIFF_CONTROL_CENTER_EXPLOSION_TIME)-fl2f(12.75))

void controlcen_proc( FuelCenter * controlcen )
{
	fix old_time;
	int	fc;

	if (!Fuelcen_control_center_destroyed)	return;

	//	Control center destroyed, rock the player's ship.
	fc = Fuelcen_seconds_left;
	if (fc > 16)
		fc = 16;
	if (FixedStep & EPS20)
	{
		ConsoleObject->mtype.phys_info.rotvel.x += fixmul(d_rand() - 16384, 3*F1_0/16 + (F1_0*(16-fc))/32);
		ConsoleObject->mtype.phys_info.rotvel.z += fixmul(d_rand() - 16384, 3*F1_0/16 + (F1_0*(16-fc))/32);
	}
	//	Hook in the rumble sound effect here.

	old_time = controlcen->Timer;
	controlcen->Timer += FrameTime;			//timer_get_approx_seconds
	Fuelcen_seconds_left = DIFF_CONTROL_CENTER_EXPLOSION_TIME - f2i(controlcen->Timer);
	if ( (old_time < COUNTDOWN_VOICE_TIME ) && (controlcen->Timer >= COUNTDOWN_VOICE_TIME) )	{
			digi_play_sample( SOUND_COUNTDOWN_13_SECS, F3_0 );
	}
	if ( f2i(old_time) != f2i(controlcen->Timer) )	{
		if ( (Fuelcen_seconds_left>=0) && (Fuelcen_seconds_left<10) ) 
			digi_play_sample( SOUND_COUNTDOWN_0_SECS+Fuelcen_seconds_left, F3_0 );
		if ( Fuelcen_seconds_left==DIFF_CONTROL_CENTER_EXPLOSION_TIME-1)
			digi_play_sample( SOUND_COUNTDOWN_29_SECS, F3_0 );
	}						

	if (controlcen->Timer < i2f(DIFF_CONTROL_CENTER_EXPLOSION_TIME)) {
		vms_vector vp;	//,v,c;
		fix size;
		compute_segment_center(&vp, &Segments[controlcen->segnum]);
		size = (0x50000*f2i(controlcen->Timer)*(FrameTime & 0xF))/16;
		size = controlcen->Timer / (fl2f(0.65));
		old_time = old_time / (fl2f(0.65));
		if (size != old_time && (controlcen->Timer > (5*F1_0) ))		{			// Every 2 seconds!
			//@@object_create_explosion( controlcen->segnum, &vp, size*10, FrameTime & 7);
			object_create_explosion( controlcen->segnum, &vp, size*10, VCLIP_SMALL_EXPLOSION);
			digi_play_sample( SOUND_CONTROL_CENTER_WARNING_SIREN, F3_0 );
		}
	}  else {
		int flash_value;

		if (old_time < i2f(DIFF_CONTROL_CENTER_EXPLOSION_TIME))
			digi_play_sample( SOUND_MINE_BLEW_UP, F1_0 );

		flash_value = f2i( (controlcen->Timer-i2f(DIFF_CONTROL_CENTER_EXPLOSION_TIME))*(64/4));	// 4 seconds to total whiteness
		PALETTE_FLASH_SET(flash_value,flash_value,flash_value);

		//gauge_message( "YOU'RE TOO SLOW! THE MINE BLEW UP!" );
		if (PaletteBlueAdd > 64 )	{
			gr_set_current_canvas( NULL );		
			gr_clear_canvas(BM_XRGB(31,31,31));		//make screen all white to match palette effect
			reset_cockpit();								//force cockpit redraw next time
			reset_palette_add();							//restore palette for death message
			controlcen->Timer = -1;
			controlcen->MaxCapacity = Fuelcen_max_amount;
			//gauge_message( "Control Center Reset" );
			DoPlayerDead();		//kill_player();
		}																				
	}
}

#ifndef M_PI
#define M_PI 3.14159
#endif

//-------------------------------------------------------------
// Called once per frame, replenishes fuel supply.
void fuelcen_update_all()
{
	int i;
	fix AmountToreplenish;
	
	AmountToreplenish = fixmul(FrameTime,Fuelcen_refill_speed);

	for (i=0; i<Num_fuelcenters; i++ )	{
		if ( Station[i].Type == SEGMENT_IS_ROBOTMAKER )	{
			if (! (Game_suspended & SUSP_ROBOTS))
				robotmaker_proc( &Station[i] );
		} else if ( Station[i].Type == SEGMENT_IS_CONTROLCEN )	{
			controlcen_proc( &Station[i] );
	
		} else if ( (Station[i].MaxCapacity > 0) && (PlayerSegment!=&Segments[Station[i].segnum]) )	{
			if ( Station[i].Capacity < Station[i].MaxCapacity )	{
 				Station[i].Capacity += AmountToreplenish;
				if ( Station[i].Capacity >= Station[i].MaxCapacity )		{
					Station[i].Capacity = Station[i].MaxCapacity;
					//gauge_message( "Fuel center is fully recharged!    " );
				}
			}
		}
	}
}

//-------------------------------------------------------------
fix fuelcen_give_fuel(segment *segp, fix MaxAmountCanTake )
{
	static fix last_play_time = 0;
        #define REFUEL_SOUND_DELAY (F1_0/3)

	Assert( segp != NULL );

	PlayerSegment = segp;

	if ( (segp) && (segp->special==SEGMENT_IS_FUELCEN) )	{
		fix amount;

//		if (Station[segp->value].MaxCapacity<=0)	{
//			hud_message( MSGC_MINE_FEEDBACK, "Fuelcenter %d is destroyed.", segp->value );
//			return 0;
//		}

//		if (Station[segp->value].Capacity<=0)	{
//			hud_message( MSGC_MINE_FEEDBACK, "Fuelcenter %d is empty.", segp->value );
//			return 0;
//		}

		if (MaxAmountCanTake <= 0 )	{
//			//gauge_message( "Fueled up!");
			return 0;
		}

		amount = fixmul(FrameTime,Fuelcen_give_amount);

		if (amount > MaxAmountCanTake )
			amount = MaxAmountCanTake;

//		if (!(Game_mode & GM_MULTI))
//			if ( Station[segp->value].Capacity < amount  )	{
//				amount = Station[segp->value].Capacity;
//				Station[segp->value].Capacity = 0;
//			} else {
//				Station[segp->value].Capacity -= amount;
//			}


		if (last_play_time + REFUEL_SOUND_DELAY < GameTime || last_play_time > GameTime)
		{
			last_play_time = GameTime;
			digi_play_sample( SOUND_REFUEL_STATION_GIVING_FUEL, F1_0/2 );

			#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_play_sound(SOUND_REFUEL_STATION_GIVING_FUEL, F1_0/2);
			#endif
		}

		//hud_message( MSGC_MINE_FEEDBACK, "Fuelcen %d has %d/%d fuel", segp->value,f2i(Station[segp->value].Capacity),f2i(Station[segp->value].MaxCapacity) );
		return amount;

	} else {
		return 0;
	}
}

//	--------------------------------------------------------------------------------------------
void disable_matcens(void)
{
	int	i;

	for (i=0; i<Num_robot_centers; i++) {
		Station[i].Enabled = 0;
		Station[i].Disable_time = 0;
	}
}

//	--------------------------------------------------------------------------------------------
//	Initialize all materialization centers.
//	Give them all the right number of lives.
void init_all_matcens(void)
{
	int	i;

	for (i=0; i<Num_fuelcenters; i++)
		if (Station[i].Type == SEGMENT_IS_ROBOTMAKER) {
			Station[i].Lives = 3;
			Station[i].Enabled = 0;
			Station[i].Disable_time = 0;
#ifndef NDEBUG
{
			//	Make sure this fuelcen is pointed at by a matcen.
			int	j;
			for (j=0; j<Num_robot_centers; j++) {
				if (RobotCenters[j].fuelcen_num == i)
					break;
			}
			Assert(j != Num_robot_centers);
}
#endif

		}

#ifndef NDEBUG
	//	Make sure all matcens point at a fuelcen
	for (i=0; i<Num_robot_centers; i++) {
		int	fuelcen_num = RobotCenters[i].fuelcen_num;

		Assert(fuelcen_num < Num_fuelcenters);
		Assert(Station[fuelcen_num].Type == SEGMENT_IS_ROBOTMAKER);
	}
#endif

}

/*
 * reads a matcen_info structure from a CFILE
 */
void matcen_info_read(matcen_info *mi, CFILE *fp, int version)
{
	mi->robot_flags = cfile_read_int(fp);
	if (version > 25)
		/*mi->robot_flags2 =*/ cfile_read_int(fp);
	mi->hit_points = cfile_read_fix(fp);
	mi->interval = cfile_read_fix(fp);
	mi->segnum = cfile_read_short(fp);
	mi->fuelcen_num = cfile_read_short(fp);
}
