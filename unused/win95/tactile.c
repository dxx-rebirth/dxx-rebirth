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


#pragma off (unreferenced)
static char rcsid[] = "$Id: tactile.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)


#include "types.h"

#include "iforce.h"
#include "tactile.h"

//@@#include "win\cyberimp.h"

#include "mono.h"
#include "vecmat.h"



int TactileStick=0;
void CloseTactileStick (void);

int Tactile_open (int port)
 {
  switch (TactileStick)
   {
	 case TACTILE_IMMERSION:
	#if defined (__NT__) 
		if (!IForce_Init(port)) {
			TactileStick = 0;
		}
	#endif
		break;

//@@	case TACTILE_CYBERNET:
//@@		if (!CyberImpactInit()) {	
//@@			mprintf((0, "Unable to initialize CyberImpact Device.\n"));
//@@			atexit(CyberImpactClose);
//@@		}
//@@		else {
//@@			mprintf((0, "CyberImpact Device initialized.\n"));
//@@			TactileStick = 0;
//@@		}	
//@@		break;

	 default:
 		break;
	}
	return (TactileStick);
 }	

#define MAX_FORCE (i2f(10))

void Tactile_apply_force (vms_vector *force_vec,vms_matrix *orient)
 {  
    int feedforce;
    fix feedmag,tempfix=0;
    vms_angvec feedang;
    vms_vector feedvec;
	 unsigned short tempangle;
	 int realangle;
 		 
    if (TactileStick==TACTILE_IMMERSION)
	  {
		 vm_vec_rotate (&feedvec,force_vec,orient);
		 vm_extract_angles_vector(&feedang,&feedvec);
		 feedmag=vm_vec_mag_quick (force_vec);	
	    feedforce=f2i(fixmuldiv (feedmag,i2f(100),MAX_FORCE));
				
		 mprintf ((0,"feedforce=%d\n",feedforce));
	       
		 if (feedforce<0)
		 	feedforce=0;
		 if (feedforce>100)
	   	feedforce=100;
			
		 tempangle=(unsigned short)feedang.h;			  	
		 tempfix=tempangle;
			
		 realangle=f2i(fixmul(tempfix,i2f(360)));
		 realangle-=180;
		 if (realangle<0)
			realangle+=360;	
	
		 Jolt (feedforce,realangle,feedforce*7);
	  }
			

 }

void Tactile_jolt (int mag,int angle,int duration)
 {
  if (TactileStick==TACTILE_IMMERSION)
	{
	 Jolt (mag,angle,duration);
	}
 }
			
void Tactile_Xvibrate (int mag,int freq)
 {
  if (TactileStick==TACTILE_IMMERSION)
   {
    XVibration (mag,mag,freq);
   }
 }

void Tactile_Xvibrate_clear ()
 {
  if (TactileStick==TACTILE_IMMERSION)
   {
    XVibrationClear();
   }    
 }  

void Tactile_do_collide ()
 {
 }

void CloseTactileStick ()
 {
  int i=0;	
  if (TactileStick==TACTILE_IMMERSION)
   {
    while (i<5)
     {
      if (CloseStick())
		 break;
		i++;
	  }
	}
 }
		
