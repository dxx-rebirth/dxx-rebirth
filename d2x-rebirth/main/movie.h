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
 * Header for movie.c
 *
 */

#pragma once

#include <limits.h>
#include <span>
#include "d2x-rebirth/libmve/mvelib.h"
#include "dsx-ns.h"
#include "physfsrwops.h"

namespace dsx {

enum class play_movie_warn_missing : uint8_t
{
	verbose,
	urgent,
};

enum class movie_play_status : uint8_t
{
	skipped,   // movie wasn't present
	started,	// movie was present and started; it may or may not have completed
};

enum class movie_resolution : uint8_t
{
	high,
	low,
};

#if DXX_USE_OGL
#define MOVIE_WIDTH  (!GameArg.GfxSkipHiresMovie && grd_curscreen->get_screen_width() < 640 ? static_cast<uint16_t>(640) : grd_curscreen->get_screen_width())
#define MOVIE_HEIGHT (!GameArg.GfxSkipHiresMovie && grd_curscreen->get_screen_height() < 480 ? static_cast<uint16_t>(480) : grd_curscreen->get_screen_height())
#else
#define MOVIE_WIDTH  static_cast<uint16_t>(!GameArg.GfxSkipHiresMovie? 640 : 320)
#define MOVIE_HEIGHT static_cast<uint16_t>(!GameArg.GfxSkipHiresMovie? 480 : 200)
#endif

movie_play_status PlayMovie(std::span<const char> subtitles, const char *filename, play_movie_warn_missing);
RWops_ptr InitRobotMovie(const char *filename, MVESTREAM_ptr_t &pMovie);
int RotateRobot(MVESTREAM_ptr_t &pMovie, SDL_RWops *);
void DeInitRobotMovie(MVESTREAM_ptr_t &pMovie);

struct LoadedMovie
{
	std::array<char, PATH_MAX> pathname;
	LoadedMovie()
	{
		pathname[0] = 0;
	}
	~LoadedMovie();
};

struct LoadedMovieWithResolution : LoadedMovie
{
	movie_resolution resolution;
};

struct BuiltinMovies
{
	std::array<LoadedMovie, 3> movies;
};

// find and initialize the movie libraries
[[nodiscard]]
std::unique_ptr<BuiltinMovies> init_movies();
[[nodiscard]]
std::unique_ptr<LoadedMovieWithResolution> init_extra_robot_movie(const char *filename);

}
