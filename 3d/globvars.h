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
 * $Source: /cvs/cvsroot/d2x/3d/globvars.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: bradleyb $
 * $Date: 2001-01-19 03:29:58 $
 * 
 * Private (internal) header for 3d library
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 21:57:51  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.2  1995/09/13  11:31:19  allender
 * added fCanv_w2 and vCanv_h2 for PPC implementation
 *
 * Revision 1.1  1995/05/05  08:51:02  allender
 * Initial revision
 *
 * Revision 1.1  1995/04/17  04:07:58  matt
 * Initial revision
 * 
 * 
 */



#ifndef _GLOBVARS_H
#define _GLOBVARS_H

#define MAX_POINTS_IN_POLY 100

extern int Canvas_width,Canvas_height;	//the actual width & height
extern fix Canv_w2,Canv_h2;				//fixed-point width,height/2

#ifdef __powerc
extern double fCanv_w2, fCanv_h2;
#endif

extern vms_vector Window_scale;
extern int free_point_num;

extern fix View_zoom;
extern vms_vector View_position,Matrix_scale;
extern vms_matrix View_matrix,Unscaled_matrix;


//vertex buffers for polygon drawing and clipping
extern g3s_point *Vbuf0[];
extern g3s_point *Vbuf1[];

//list of 2d coords
extern fix Vertex_list[];

#endif
