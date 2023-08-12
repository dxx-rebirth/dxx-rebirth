/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * CD Audio functions for Win32
 *
 *
 */

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <mmsystem.h>
#include <climits>

#include "pstypes.h"
#include "dxxerror.h"
#include "args.h"
#include "rbaudio.h"
#include "console.h"
#include "fwd-gr.h"
#include "timer.h"

#if SDL_MAJOR_VERSION == 1
// we don't want this on SDL1 because there we have common/arch/sdl/rbaudio.cpp
// #pragma message("Skipping win32/rbaudio because of SDL1")

#elif SDL_MAJOR_VERSION == 2

namespace dcx {
#define CD_FPS 75

static UINT wCDDeviceID = 0U;
static bool initialised;
static DWORD playEnd;
static DWORD lastFrames;
static bool isPaused;

void RBAExit()
{
	if (wCDDeviceID)
	{
		initialised = false;
		mciSendCommand(wCDDeviceID, MCI_CLOSE, MCI_WAIT, 0);
		wCDDeviceID = 0U;
	}
}

static bool mci_HasMedia()
{
	if (!wCDDeviceID) return false;

	MCI_STATUS_PARMS mciStatusParms;

	mciStatusParms.dwItem = MCI_STATUS_MEDIA_PRESENT;
	if (const MCIERROR mciError{mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&mciStatusParms))})
	{
		Warning("RBAudio win32/MCI: cannot determine MCI media status (%lx)", mciError);
		RBAExit();
		return false;
	}
	return mciStatusParms.dwReturn != 0;
}

static unsigned mci_TotalFramesMsf(const DWORD msf)
{
	return (MCI_MSF_MINUTE(msf) * 60 + MCI_MSF_SECOND(msf)) * CD_FPS + MCI_MSF_FRAME(msf);
}

static unsigned mci_FramesToMsf(const int frames)
{
	int m = frames / (CD_FPS * 60);
	int s = (frames / CD_FPS) % 60;
	int f = frames % CD_FPS;
	return MCI_MAKE_MSF(m, s, f);
}

static unsigned mci_GetTrackOffset(const int track)
{
	MCI_STATUS_PARMS mciStatusParms;

	mciStatusParms.dwItem = MCI_STATUS_POSITION;
	mciStatusParms.dwTrack = track;
	if (const MCIERROR mciError{mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, reinterpret_cast<DWORD_PTR>(&mciStatusParms))})
	{
		Warning("RBAudio win32/MCI: cannot determine track %i offset (%lx)", track, mciError);
		return -1;
	}
	// dwReturn is a 32-bit value in MSF format, so DWORD_PTR > DWORD is not a problem
	return mci_TotalFramesMsf(mciStatusParms.dwReturn);
}

static unsigned mci_GetTrackLength(const int track)
{
	MCI_STATUS_PARMS mciStatusParms;

	mciStatusParms.dwItem = MCI_STATUS_LENGTH;
	mciStatusParms.dwTrack = track;
	if (const MCIERROR mciError{mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, reinterpret_cast<DWORD_PTR>(&mciStatusParms))})
	{
		Warning("RBAudio win32/MCI: cannot determine track %i length (%lx)", track, mciError);
		return -1;
	}
	// dwReturn is a 32-bit value in MSF format, so DWORD_PTR > DWORD is not a problem
	return mci_TotalFramesMsf(mciStatusParms.dwReturn);
}

static unsigned mci_GetTotalLength()
{
	MCI_STATUS_PARMS mciStatusParms;

	mciStatusParms.dwItem = MCI_STATUS_LENGTH;
	if (const MCIERROR mciError{mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&mciStatusParms))})
	{
		Warning("RBAudio win32/MCI: cannot determine media length (%lx)", mciError);
		return -1;
	}
	return mci_TotalFramesMsf(mciStatusParms.dwReturn);
}

void RBAInit()
{
	MCI_OPEN_PARMS mciOpenParms;
	MCI_SET_PARMS mciSetParms;

	if (initialised) return;
	
	mciOpenParms.lpstrDeviceType = "cdaudio";
	if (const MCIERROR mciError{mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_SHAREABLE, reinterpret_cast<DWORD_PTR>(&mciOpenParms))})
	{
		con_puts(CON_NORMAL, "RBAudio win32/MCI: cannot find MCI cdaudio (no CD drive?)");
		return;
	}

	wCDDeviceID = mciOpenParms.wDeviceID;

	if (!mci_HasMedia())
	{
		con_puts(CON_NORMAL, "RBAudio win32/MCI: no media in CD drive.");
		RBAExit();
		return;
	}

	mciSetParms.dwTimeFormat = MCI_FORMAT_MSF;
	if (const MCIERROR mciError{mciSendCommand(wCDDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, reinterpret_cast<DWORD_PTR>(&mciSetParms))})
	{
		Warning_puts("RBAudio win32/MCI: cannot set time format for CD to MSF (strange)");
		RBAExit();
		return;
	}

	initialised = true;
	RBAList();
}

int RBAEnabled()
{
	return initialised;
}

static void (*redbook_finished_hook)() = nullptr;

int RBAPlayTrack(int a)
{
	if (!wCDDeviceID)
		return 0;

	if (mci_HasMedia())
	{
		MCI_PLAY_PARMS mciPlayParms;
		DWORD playStart;

		con_printf(CON_VERBOSE, "RBAudio win32/MCI: Playing track %i", a);

		playStart = mci_GetTrackOffset(a);
		playEnd = playStart + mci_GetTrackLength(a);

		mciPlayParms.dwFrom = mci_FramesToMsf(playStart);
		mciPlayParms.dwTo = mci_FramesToMsf(playEnd);

		if (const MCIERROR mciError{mciSendCommand(wCDDeviceID, MCI_PLAY, MCI_FROM | MCI_TO, reinterpret_cast<DWORD_PTR>(&mciPlayParms))})
		{
			Warning("RBAudio win32/MCI: could not play track (%lx)", mciError);
			return 0;
		}

		return 1;
	}
	return 0;
}

// plays tracks first through last, inclusive
int RBAPlayTracks(int first, int last, void (*hook_finished)(void))
{
	if (!wCDDeviceID)
		return 0;

	if (mci_HasMedia())
	{
		MCI_PLAY_PARMS mciPlayParms;

		redbook_finished_hook = hook_finished;

		con_printf(CON_VERBOSE, "RBAudio win32/MCI: Playing tracks %i to %i", first, last);

		playEnd = mci_GetTrackOffset(last) + mci_GetTrackLength(last);

		mciPlayParms.dwFrom = mci_FramesToMsf(mci_GetTrackOffset(first));
		mciPlayParms.dwTo = mci_FramesToMsf(playEnd);

		if (const MCIERROR mciError{mciSendCommand(wCDDeviceID, MCI_PLAY, MCI_FROM | MCI_TO, reinterpret_cast<DWORD_PTR>(&mciPlayParms))})
		{
			Warning("RBAudio win32/MCI: could not play tracks (%lx)", mciError);
			return 0;
		}

		return 1;
	}
	return 0;
}

void RBAStop()
{
	if (!wCDDeviceID) return;

	if (const MCIERROR mciError{mciSendCommand(wCDDeviceID, MCI_STOP, 0, 0)})
	{
		Warning("RBAudio win32/MCI: could not stop music (%lx)", mciError);
	}
	redbook_finished_hook = nullptr;
}

void RBAEjectDisk()
{
	if (!wCDDeviceID) return;

	if (const MCIERROR mciError{mciSendCommand(wCDDeviceID, MCI_SET, MCI_SET_DOOR_OPEN | MCI_WAIT, 0)})
	{
		Warning("RBAudio win32/MCI: could not open CD tray (%lx)", mciError);
	}
	initialised = false;
}

void RBASetVolume(int)
{
	// MCI does not support this
}

void RBAPause()
{
	if (!wCDDeviceID) return;

	if (const MCIERROR mciError{mciSendCommand(wCDDeviceID, MCI_PAUSE, 0, 0)})
	{
		Warning("RBAudio win32/MCI: could not pause music (%lx)", mciError);
		return;
	}
	con_puts(CON_VERBOSE, "RBAudio win32/MCI: Playback paused");
	isPaused = true;
}

int RBAResume()
{
	if (!wCDDeviceID) return -1;

	if (const MCIERROR mciError{mciSendCommand(wCDDeviceID, MCI_RESUME, 0, 0)})
	{
		Warning("RBAudio win32/MCI: could not resume music (%lx)", mciError);
		return -1;
	}
	con_puts(CON_VERBOSE, "RBAudio win32/MCI: Playback resumed");
	isPaused = false;
	return 1;
}

int RBAPauseResume()
{
	if (!wCDDeviceID) return 0;
	MCI_STATUS_PARMS mciStatusParms;

	mciStatusParms.dwItem = MCI_STATUS_MODE;
	if (const MCIERROR mciError{mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&mciStatusParms))})
	{
		Warning("RBAudio win32/MCI: cannot determine MCI media status (%lx)", mciError);
		return 0;
	}

	if (mciStatusParms.dwReturn == MCI_MODE_PLAY)
	{
		con_puts(CON_VERBOSE, "RBAudio win32/MCI: Toggle Playback pause");
		RBAPause();
	}
	// MCI may also use MCI_MODE_STOP for pause instead of MCI_MODE_PAUSE
	else if (mciStatusParms.dwReturn == MCI_MODE_PAUSE || (isPaused && mciStatusParms.dwReturn == MCI_MODE_STOP))
	{
		con_puts(CON_VERBOSE, "RBAudio win32/MCI: Toggle Playback resume");
		return RBAResume() > 0;
	}
	else
		return 0;

	return 1;
}

int RBAGetNumberOfTracks()
{
	if (!wCDDeviceID) return -1;
	MCI_STATUS_PARMS mciStatusParms;

	mciStatusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
	if (const MCIERROR mciError{mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&mciStatusParms))})
	{
		Warning("RBAudio win32/MCI: could not get track count (%lx)", mciError);
		return -1;
	}

	return static_cast<int>(mciStatusParms.dwReturn);
}

// check if we need to call the 'finished' hook
// needs to go in all event loops
// MCI has a hook function via Win32 messages, but it doesn't work for CDs
//   for whatever reason
void RBACheckFinishedHook()
{
	static fix64 last_check_time = 0;
	
	if (!wCDDeviceID) return;

	if ((timer_query() - last_check_time) >= F2_0)
	{
		MCI_STATUS_PARMS mciStatusParms;

		mciStatusParms.dwItem = MCI_STATUS_POSITION;
		if (const MCIERROR mciError{mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&mciStatusParms))})
		{
			Warning("RBAudio win32/MCI: cannot determine MCI position (%lx)", mciError);
			return;
		}

		// time for a hack. for some reason, MCI sometimes stops
		// from around a few to few dozen frames before it should,
		// so we will check if we haven't moved from the last check
		// and allow a bit of a leeway when checking if so.

		DWORD checkValue = playEnd;
		// dwReturn is a 32-bit value in MSF format, so DWORD_PTR > DWORD is not a problem
		DWORD thisFrames = mci_TotalFramesMsf(mciStatusParms.dwReturn);

		if (thisFrames == lastFrames)
			checkValue = checkValue < 64 ? 0 : checkValue - 64; // prevent underflow

		if (redbook_finished_hook && playEnd > 0 && thisFrames >= checkValue)
		{
			con_puts(CON_VERBOSE, "RBAudio win32/MCI: Playback done, calling finished-hook");
			redbook_finished_hook();
		}
		lastFrames = thisFrames;
		last_check_time = timer_query();
	}
}

// return the track number currently playing.  Useful if RBAPlayTracks()
// is called.  Returns 0 if no track playing, else track number
int RBAGetTrackNum()
{
	if (!wCDDeviceID) return 0;
	MCI_STATUS_PARMS mciStatusParms;

	mciStatusParms.dwItem = MCI_STATUS_MODE;
	if (const MCIERROR mciError{mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&mciStatusParms))})
	{
		Warning("RBAudio win32/MCI: cannot determine MCI media status (%lx)", mciError);
		return 0;
	}

	if (mciStatusParms.dwReturn != MCI_MODE_PLAY)
		return 0;

	mciStatusParms.dwItem = MCI_STATUS_CURRENT_TRACK;
	if (const MCIERROR mciError{mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&mciStatusParms))})
	{
		Warning("RBAudio win32/MCI: cannot determine MCI track number (%lx)", mciError);
		return 0;
	}

	return static_cast<int>(mciStatusParms.dwReturn);
}

int RBAPeekPlayStatus()
{
	if (!wCDDeviceID) return 0;
	MCI_STATUS_PARMS mciStatusParms;

	mciStatusParms.dwItem = MCI_STATUS_MODE;
	if (const MCIERROR mciError{mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&mciStatusParms))})
	{
		Warning("RBAudio win32/MCI: cannot determine MCI media status (%lx)", mciError);
		return 0;
	}

	if (mciStatusParms.dwReturn == MCI_MODE_PLAY)
		return 1;
	// MCI may also use MCI_MODE_STOP for pause instead of MCI_MODE_PAUSE
	else if (mciStatusParms.dwReturn == MCI_MODE_PAUSE || (isPaused && mciStatusParms.dwReturn == MCI_MODE_STOP))
		return -1; // hack so it doesn't keep restarting paused music; -1 is still truthy, unlike 0
	else
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
	int i, t = 0, n = 0, trackCount, totalLength;

	if (!wCDDeviceID)
		return 0;

	trackCount = RBAGetNumberOfTracks();
	totalLength = mci_GetTotalLength();
	if (totalLength < 0)
	{
		Warning_puts("RBAudio win32/MCI: cannot determine total length");
		return 0;
	}

	/* For backward compatibility this algorithm must not change */

	i = 1;

	while (i <= trackCount)
	{
		int offset = mci_GetTrackOffset(i);
		if (offset < 0)
		{
			Warning("RBAudio win32/MCI: cannot determine track %i offset", i);
			return 0;
		}
		n += cddb_sum(offset / CD_FPS);
		i++;
	}

	t = totalLength / CD_FPS;

	return ((n % 0xff) << 24 | t << 8 | trackCount);
}

void RBAList(void)
{
	if (!wCDDeviceID) return;

	MCI_STATUS_PARMS mciStatusParms;
	int trackCount = RBAGetNumberOfTracks();

	for (int i = 1; i <= trackCount; ++i) {
		int isAudioTrack, length, offset;
		mciStatusParms.dwTrack = i;

		mciStatusParms.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
		if (const MCIERROR mciError{mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, reinterpret_cast<DWORD_PTR>(&mciStatusParms))})
		{
			Warning("RBAudio win32/MCI: cannot determine track %d type (%lx)", i, mciError);
			continue;
		}
		isAudioTrack = mciStatusParms.dwReturn == MCI_CDA_TRACK_AUDIO;

		offset = mci_GetTrackOffset(i);
		if (offset < 0)
		{
			continue;
		}

		length = mci_GetTrackLength(i);
		if (length < 0)
		{
			continue;
		}

		con_printf(CON_VERBOSE, "RBAudio win32/MCI: CD track %d, type %s, length %d, offset %d", i, isAudioTrack ? "audio" : "data", length, offset);
	}
}

}

#endif
