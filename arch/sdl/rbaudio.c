/* $Id: rbaudio.c,v 1.3 2003-01-15 02:42:41 btb Exp $ */
/*
 *
 * SDL CD Audio functions
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>

#include "pstypes.h"
#include "error.h"
#include "args.h"
#include "rbaudio.h"

static SDL_CD *s_cd = NULL;
extern int Redbook_playing;
static int initialised = 0;

void RBAExit()
{
  if (initialised)
  {
  	SDL_CDClose(s_cd);
  }
}

void RBAInit()
{
	if (initialised) return;
	if (FindArg("-nocdrom")) return; 

	if (SDL_Init(SDL_INIT_CDROM) < 0)
	{
		Warning("SDL library initialisation failed: %s.",SDL_GetError());
		return;
	}

	if (SDL_CDNumDrives() == 0)
	{
		Warning("No cdrom drives found!\n");
		return;
	}
 	s_cd = SDL_CDOpen(0);
	if (s_cd == NULL) {
		Warning("Could not open cdrom for redbook audio!\n");
		return;
	}
 	atexit(RBAExit);
	initialised = 1;
}

int RBAEnabled()
{
 return 1;
}

void RBARegisterCD()
{

}

int RBAPlayTrack(int a)
{
 if (!initialised) return -1;

 if (CD_INDRIVE(SDL_CDStatus(s_cd)) ) {
	 SDL_CDPlayTracks(s_cd, a-1, 0, 0, 0);
 }
 return a;
 
}

void RBAStop()
{
  if (!initialised) return;
  SDL_CDStop(s_cd);
}

void RBASetVolume(int a)
{

}

void RBAPause()
{
  if (!initialised) return;
  SDL_CDPause(s_cd);
}

int RBAResume()
{
  if (!initialised) return -1;
  SDL_CDResume(s_cd);
  return 1;
}

int RBAGetNumberOfTracks()
{
  if (!initialised) return -1;
 SDL_CDStatus(s_cd);
 return s_cd->numtracks;
}

int RBAPlayTracks(int tracknum,int something)
{
  if (!initialised) return -1;
 if (CD_INDRIVE(SDL_CDStatus(s_cd)) ) {
	 SDL_CDPlayTracks(s_cd, tracknum-1, 0, 0, 0);
 }
 return tracknum;
}

int RBAGetTrackNum()
{
  if (!initialised) return -1;
 SDL_CDStatus(s_cd);
 return s_cd->cur_track;
}

int RBAPeekPlayStatus()
{
  return (SDL_CDStatus(s_cd) == CD_PLAYING);
}

int CD_blast_mixer()
{
 return 0;
}
