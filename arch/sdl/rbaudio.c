/* $Id: rbaudio.c,v 1.1.1.1 2006/03/17 19:53:40 zicodxx Exp $ */
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

#include <SDL/SDL.h>

#ifdef __linux__
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#endif

#include "pstypes.h"
#include "error.h"
#include "args.h"
#include "rbaudio.h"
#include "console.h"

static SDL_CD *s_cd = NULL;
static int initialised = 0;

void RBAExit()
{
	if (s_cd)
	{
		SDL_CDStop(s_cd);
		SDL_CDClose(s_cd);
	}
}

void RBAInit()
{
	int num_cds;
	int i,j;
	
	if (initialised) return;

	if (SDL_Init(SDL_INIT_CDROM) < 0)
	{
		Warning("SDL library initialisation failed: %s.",SDL_GetError());
		return;
	}

	num_cds = SDL_CDNumDrives();
	if (num_cds < 1)
	{
		Warning("No cdrom drives found!\n");
#if defined(__APPLE__) || defined(macintosh)
		SDL_QuitSubSystem(SDL_INIT_CDROM);	// necessary for rescanning CDROMs
#endif
		return;
	}
	
	for (i = 0; i < num_cds; i++)
	{
		if (s_cd)
			SDL_CDClose(s_cd);
		s_cd = SDL_CDOpen(i);
		
		if (s_cd && CD_INDRIVE(SDL_CDStatus(s_cd)))
		{
			for (j = 0; j < s_cd->numtracks; j++)
			{
				if (s_cd->track[j].type == SDL_AUDIO_TRACK)
					break;
			}
			
			if (j != s_cd->numtracks)
				break;	// we've found an audio CD
		}
		else if (s_cd == NULL)
			Warning("Could not open cdrom %i for redbook audio:%s\n", i, SDL_GetError());
	}
	
	if (i == num_cds)
	{
		Warning("No audio CDs found\n");
		if (s_cd)	// if there's no audio CD, say that there's no redbook and hence play MIDI instead
		{
			SDL_CDClose(s_cd);
			s_cd = NULL;
		}
#if defined(__APPLE__) || defined(macintosh)
		SDL_QuitSubSystem(SDL_INIT_CDROM);	// necessary for rescanning CDROMs
#endif
		return;
	}
	
	initialised = 1;
}

int RBAEnabled()
{
	return initialised;
}

int RBAPlayTrack(int a)
{
	if (!s_cd) return -1;

	if (CD_INDRIVE(SDL_CDStatus(s_cd)) ) {
		SDL_CDPlayTracks(s_cd, a-1, 0, 0, 0);
	}
	return a;
}

void RBAStop()
{
	if (!s_cd) return;
	SDL_CDStop(s_cd);
}

void RBAEjectDisk()
{
	if (!s_cd) return;
	SDL_CDEject(s_cd);	// play nothing until it tries to load a song
#if defined(__APPLE__) || defined(macintosh)
	SDL_QuitSubSystem(SDL_INIT_CDROM);	// necessary for rescanning CDROMs
#endif
	initialised = 0;
}

void RBASetVolume(int volume)
{
#ifdef __linux__
	int cdfile, level;
	struct cdrom_volctrl volctrl;

	if (!s_cd) return;

	cdfile = s_cd->id;
	level = volume * 3;

	if ((level<0) || (level>255)) {
		con_printf(CON_CRITICAL, "illegal volume value (allowed values 0-255)\n");
		return;
	}

	volctrl.channel0
		= volctrl.channel1
		= volctrl.channel2
		= volctrl.channel3
		= level;
	if ( ioctl(cdfile, CDROMVOLCTRL, &volctrl) == -1 ) {
		con_printf(CON_CRITICAL, "CDROMVOLCTRL ioctl failed\n");
		return;
	}
#endif
}

void RBAPause()
{
	if (!s_cd) return;
	SDL_CDPause(s_cd);
}

int RBAResume()
{
	if (!s_cd) return -1;
	SDL_CDResume(s_cd);
	return 1;
}

int RBAPauseResume()
{
	if (!s_cd) return 0;

	if (SDL_CDStatus(s_cd) == CD_PLAYING)
		SDL_CDPause(s_cd);
	else if (SDL_CDStatus(s_cd) == CD_PAUSED)
		SDL_CDResume(s_cd);
	else
		return 0;

	return 1;
}

int RBAGetNumberOfTracks()
{
	if (!s_cd) return -1;
	SDL_CDStatus(s_cd);
	return s_cd->numtracks;
}

// plays tracks first through last, inclusive
int RBAPlayTracks(int first, int last)
{
	if (!s_cd)
		return 0;

	if (CD_INDRIVE(SDL_CDStatus(s_cd)))
	{
		SDL_CDPlayTracks(s_cd, first - 1, 0, last - first + 1, 0);
	}
	return 1;
}

// return the track number currently playing.  Useful if RBAPlayTracks()
// is called.  Returns 0 if no track playing, else track number
int RBAGetTrackNum()
{
	if (!s_cd)
		return 0;

	if (SDL_CDStatus(s_cd) != CD_PLAYING)
		return 0;

	return s_cd->cur_track + 1;
}

int RBAPeekPlayStatus()
{
	if (!s_cd)
		return 0;

	if (SDL_CDStatus(s_cd) == CD_PLAYING)
		return 1;
	else if (SDL_CDStatus(s_cd) == CD_PAUSED)	// hack so it doesn't keep restarting paused music
		return -1;
	
	return 0;
}

static int cddb_sum(int n)
{
	int ret;

	/* For backward compatibility this algorithm must not change */

	ret = 0;

	while (n > 0) {
		ret = ret + (n % 10);
		n = n / 10;
	}

	return (ret);
}


unsigned long RBAGetDiscID()
{
	int i, t = 0, n = 0;

	if (!s_cd)
		return 0;

	/* For backward compatibility this algorithm must not change */

	i = 0;

	while (i < s_cd->numtracks) {
		n += cddb_sum(s_cd->track[i].offset / CD_FPS);
		i++;
	}

	t = (s_cd->track[s_cd->numtracks].offset / CD_FPS) -
	    (s_cd->track[0].offset / CD_FPS);

	return ((n % 0xff) << 24 | t << 8 | s_cd->numtracks);
}
