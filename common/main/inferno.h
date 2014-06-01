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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Header file for Inferno.  Should be included in all source files.
 *
 */

#ifndef _INFERNO_H
#define _INFERNO_H

struct d_event;

#if defined(__APPLE__) || defined(macintosh)
#define KEY_MAC(x) x
#else
// do not use MAC, it will break MSVC compilation somewhere in rpcdce.h
#define KEY_MAC(x)
#endif

/**
 **	Constants
 **/

//	How close two points must be in all dimensions to be considered the same point.
#define	FIX_EPSILON	10

// the maximum length of a filename
#define FILENAME_LEN 13

// a filename, useful for declaring arrays of filenames
typedef char d_fname[FILENAME_LEN];

/**
 **	Global variables
 **/

extern int Quitting;
extern int Screen_mode;			// editor screen or game screen?
#ifdef DXX_BUILD_DESCENT_I
extern int MacHog;
#endif

// Default event handler for everything except the editor
int standard_handler(struct d_event *event);

#endif
