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

#pragma once

#define RBA_MEDIA_CHANGED	-1

#ifdef __cplusplus
#include <SDL_version.h>

namespace dcx {

#if SDL_MAJOR_VERSION == 1
struct RBACHANNELCTL
{
	unsigned int out0in, out0vol;
	unsigned int out1in, out1vol;
	unsigned int out2in, out2vol;
	unsigned int out3in, out3vol;
};

extern void RBAInit(void);
extern void RBAExit();
extern long RBAGetDeviceStatus(void);
extern int RBAPlayTrack(int track);
extern int RBAPlayTracks(int first, int last, void (*hook_finished)(void));	//plays tracks first through last, inclusive
extern int RBACheckMediaChange();
extern long	RBAGetHeadLoc(int *min, int *sec, int *frame);
extern int	RBAPeekPlayStatus(void);
extern void RBAStop(void);
extern void RBAEjectDisk(void);
extern void RBASetStereoAudio(RBACHANNELCTL *channels);
extern void RBASetQuadAudio(RBACHANNELCTL *channels);
extern void RBAGetAudioInfo(RBACHANNELCTL *channels);
extern void RBASetChannelVolume(int channel, int volume);
#ifdef __linux__
extern void RBASetVolume(int volume);
#else
static inline void RBASetVolume(int)
{
}
#endif
extern int	RBAEnabled(void);
extern void RBADisable(void);
extern void RBAEnable(void);
extern int	RBAGetNumberOfTracks(void);
extern void RBACheckFinishedHook();
extern void	RBAPause();
extern int	RBAResume();
extern int	RBAPauseResume();

//return the track number currently playing.  Useful if RBAPlayTracks() 
//is called.  Returns 0 if no track playing, else track number
int RBAGetTrackNum();

// get the cddb discid for the current cd.
unsigned long RBAGetDiscID();

// List the tracks on the CD
void RBAList(void);
#endif

}
#endif
