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
 * $Source: /cvs/cvsroot/d2x/main/editor/seguvs.h,v $
 * $Revision: 1.2 $
 * $Author: schaffner $
 * $Date: 2004-08-28 23:17:45 $
 * 
 * Header for seguvs.c
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2001/10/25 02:27:17  bradleyb
 * attempt at support for editor, makefile changes, etc
 *
 * Revision 1.1.1.1  1999/06/14 22:02:40  donut
 * Import of d1x 1.37 source.
 * 
 */



#ifndef _SEGUVS_H
#define _SEGUVS_H

extern void assign_light_to_side(segment *sp, int sidenum);
extern void assign_default_lighting_all(void);
extern void stretch_uvs_from_curedge(segment *segp, int side);

#endif
