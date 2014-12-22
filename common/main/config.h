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
 * prototype definitions for descent.cfg reading/writing
 *
 */


#ifndef _CONFIG_H
#define _CONFIG_H

#include "player.h"
#include "mission.h"

#ifdef __cplusplus
#include "pack.h"
#include "compiler-array.h"
#include "ntstring.h"

struct Cfg : prohibit_void_ptr<Cfg>
{
	ubyte DigiVolume;
	ubyte MusicVolume;
	int ReverseStereo;
	int OrigTrackOrder;
	int MusicType;
	int CMLevelMusicPlayOrder;
	int CMLevelMusicTrack[2];
	ntstring<PATH_MAX - 1> CMLevelMusicPath;
	char CMMiscMusic[5][PATH_MAX+1];
	int GammaLevel;
	callsign_t LastPlayer;
	char LastMission[MISSION_NAME_LEN+1];
	int ResolutionX;
	int ResolutionY;
	int AspectX;
	int AspectY;
	int WindowMode;
	int TexFilt;
	int VSync;
	int Multisample;
	int FPSIndicator;
	int Grabinput;
#ifdef DXX_BUILD_DESCENT_II
	int MovieTexFilt;
	int MovieSubtitles;
#endif
};

extern struct Cfg GameCfg;

//#ifdef USE_SDLMIXER
//#define EXT_MUSIC_ON (GameCfg.SndEnableRedbook || GameCfg.JukeboxOn)
//#else
//#define EXT_MUSIC_ON (GameCfg.SndEnableRedbook)		// JukeboxOn shouldn't do anything if it's not supported
//#endif

extern int ReadConfigFile(void);
extern int WriteConfigFile(void);

#endif

#endif
