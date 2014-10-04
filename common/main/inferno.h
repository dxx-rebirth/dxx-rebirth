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

#include <algorithm>
#include "dxxsconf.h"
#include "compiler-array.h"

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
static const std::size_t FILENAME_LEN = 13;

// a filename, useful for declaring arrays of filenames
struct d_fname : array<char, FILENAME_LEN>
{
	void _copy(const char *i, const char *e)
	{
		std::fill(std::copy(i, e, begin()), end(), 0);
	}
	template <std::size_t N>
		void operator=(char (&i)[N]) const = delete;
	template <std::size_t N>
		void operator=(const char (&i)[N])
		{
			static_assert(N <= FILENAME_LEN, "string too long");
			_copy(i, i + N);
		}
	void copy_if(const d_fname &, std::size_t = 0) = delete;
	template <std::size_t N>
		bool copy_if(const char (&i)[N])
		{
			return copy_if(i, N);
		}
	bool copy_if(const char *i, std::size_t N)
	{
		auto n = std::find(i, i + N, 0);
		if (static_cast<size_t>(std::distance(i, n)) >= size())
		{
			fill(0);
			return false;
		}
		_copy(i, n);
		return true;
	}
	operator char *() const = delete;
	operator const char *() const
	{
		return &operator[](0);
	}
};

/**
 **	Global variables
 **/

extern int Quitting;
extern int Screen_mode;			// editor screen or game screen?
#ifdef DXX_BUILD_DESCENT_I
extern int MacHog;
#endif

// Default event handler for everything except the editor
int standard_handler(const d_event &event);

#endif
