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
static int initialised = 0;
static DWORD playEnd;

void RBAExit()
{
    if (wCDDeviceID)
    {
        initialised = 0;
        mciSendCommand(wCDDeviceID, MCI_CLOSE, MCI_WAIT, 0);
        wCDDeviceID = 0U;
    }
}

static int mci_HasMedia()
{
    if (!wCDDeviceID) return 0;

    MCIERROR mciError;
    MCI_STATUS_PARMS mciStatusParms;

    mciStatusParms.dwItem = MCI_STATUS_MEDIA_PRESENT;
    if ((mciError = mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&mciStatusParms))))
    {
		Warning("RBAudio win32: cannot determine MCI media status (%lx)", mciError);
        RBAExit();
		return 0;
    }
    return static_cast<int>(mciStatusParms.dwReturn);
}

static int mci_enableMsf()
{
    if (!wCDDeviceID) return 0;

    MCIERROR mciError;
    MCI_SET_PARMS mciSetParms;

    mciSetParms.dwTimeFormat = MCI_FORMAT_MSF;
    if ((mciError = mciSendCommand(wCDDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, reinterpret_cast<DWORD_PTR>(&mciSetParms))))
    {
		con_puts(CON_NORMAL, "RBAudio win32: cannot set time format for CD to MSF (strange)");
        return 0;
    }
    return 1;
}

static int mci_restoreTmsf()
{
    if (!wCDDeviceID) return 0;

    MCIERROR mciError;
    MCI_SET_PARMS mciSetParms;

    mciSetParms.dwTimeFormat = MCI_FORMAT_TMSF;
    if ((mciError = mciSendCommand(wCDDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, reinterpret_cast<DWORD_PTR>(&mciSetParms))))
    {
		con_puts(CON_NORMAL, "RBAudio win32: cannot set time format for CD to TMSF (strange)");
        return 0;
    }
    return 1;
}

static int mci_tmsfToTrack(int track)
{
    return MCI_MAKE_TMSF(track, 0, 0, 0);
}

void RBAInit()
{
    MCIERROR mciError;
    MCI_OPEN_PARMS mciOpenParms;

    if (initialised) return;
    
    mciOpenParms.lpstrDeviceType = "cdaudio";
    if ((mciError = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_SHAREABLE, reinterpret_cast<DWORD_PTR>(&mciOpenParms))))
    {
		con_puts(CON_NORMAL, "RBAudio win32: cannot find MCI cdaudio (no CD drive?)");
		return;
    }

    wCDDeviceID = mciOpenParms.wDeviceID;

    if (!mci_HasMedia())
    {
		con_puts(CON_NORMAL, "RBAudio win32: no media in CD drive.");
        RBAExit();
		return;
    }

    if (!mci_restoreTmsf())
    {
        RBAExit();
		return;
    }

    initialised = 1;
	RBAList();
}

int RBAEnabled()
{
    return initialised;
}

static void (*redbook_finished_hook)() = NULL;

int RBAPlayTrack(int a)
{
	if (!wCDDeviceID)
		return 0;

	if (mci_HasMedia())
	{
        MCIERROR mciError;
        MCI_PLAY_PARMS mciPlayParms;
        int trackCount = RBAGetNumberOfTracks();
        int flags = MCI_FROM;

		con_printf(CON_VERBOSE, "RBAudio win32: Playing track %i", a);

        mciPlayParms.dwFrom = mci_tmsfToTrack(a);
        if (a < trackCount)
        {
            mciPlayParms.dwTo = mci_tmsfToTrack(a + 1);
            flags |= MCI_TO;
        }
        else
            mciPlayParms.dwTo = 0L;

		if ((mciError = mciSendCommand(wCDDeviceID, MCI_PLAY, flags, reinterpret_cast<DWORD_PTR>(&mciPlayParms))))
        {
		    Warning("RBAudio win32: could not play track (%lx)", mciError);
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
        MCIERROR mciError;
        MCI_PLAY_PARMS mciPlayParms;
        int trackCount = RBAGetNumberOfTracks();
        int flags = MCI_FROM;

		redbook_finished_hook = hook_finished;

		con_printf(CON_VERBOSE, "RBAudio win32: Playing tracks %i to %i", first, last);

        mciPlayParms.dwFrom = mci_tmsfToTrack(first);
        if (last < trackCount)
        {
            mciPlayParms.dwTo = mci_tmsfToTrack(last + 1);
            flags |= MCI_TO;
        }
        else
            mciPlayParms.dwTo = 0L;

		if ((mciError = mciSendCommand(wCDDeviceID, MCI_PLAY, flags, reinterpret_cast<DWORD_PTR>(&mciPlayParms))))
        {
		    Warning("RBAudio win32: could not play tracks (%lx)", mciError);
            return 0;
        }

        return 1;
	}
	return 0;
}

void RBAStop()
{
    if (!wCDDeviceID) return;
    
    MCIERROR mciError;

    if ((mciError = mciSendCommand(wCDDeviceID, MCI_STOP, 0, 0)))
    {
		Warning("RBAudio win32: could not stop music (%lx)", mciError);
    }
	redbook_finished_hook = NULL;
}

void RBAEjectDisk()
{
    if (!wCDDeviceID) return;
    
    MCIERROR mciError;

    if ((mciError = mciSendCommand(wCDDeviceID, MCI_SET, MCI_SET_DOOR_OPEN | MCI_WAIT, 0)))
    {
		Warning("RBAudio win32: could not open CD tray (%lx)", mciError);
    }
    initialised = 0;
}

void RBASetVolume(int)
{
    // MCI does not support this
}

void RBAPause()
{
    if (!wCDDeviceID) return;
    
    MCIERROR mciError;

    if ((mciError = mciSendCommand(wCDDeviceID, MCI_PAUSE, MCI_WAIT, 0)))
    {
		Warning("RBAudio win32: could not pause music (%lx)", mciError);
        return;
    }
	con_puts(CON_VERBOSE, "RBAudio win32: Playback paused");
}

int RBAResume()
{
    if (!wCDDeviceID) return -1;
    
    MCIERROR mciError;

    if ((mciError = mciSendCommand(wCDDeviceID, MCI_RESUME, 0, 0)))
    {
		Warning("RBAudio win32: could not resume music (%lx)", mciError);
        return -1;
    }
	con_puts(CON_VERBOSE, "RBAudio win32: Playback resumed");
    return 1;
}

int RBAPauseResume()
{
    if (!wCDDeviceID) return 0;
    
    MCIERROR mciError;
    MCI_STATUS_PARMS mciStatusParms;

    mciStatusParms.dwItem = MCI_STATUS_MODE;
    if ((mciError = mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&mciStatusParms))))
    {
		Warning("RBAudio win32: cannot determine MCI media status (%lx)", mciError);
        return 0;
    }

	if (mciStatusParms.dwReturn == MCI_MODE_PLAY)
	{
		con_puts(CON_VERBOSE, "RBAudio win32: Toggle Playback pause");
		RBAPause();
	}
	else if (mciStatusParms.dwReturn == MCI_MODE_PAUSE)
	{
		con_puts(CON_VERBOSE, "RBAudio win32: Toggle Playback resume");
		return RBAResume() > 0;
	}
	else
		return 0;

	return 1;
}

int RBAGetNumberOfTracks()
{
    if (!wCDDeviceID) return -1;
    
    MCIERROR mciError;
    MCI_STATUS_PARMS mciStatusParms;

    mciStatusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    if ((mciError = mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&mciStatusParms))))
    {
		Warning("RBAudio win32: could not get track count (%lx)", mciError);
        return -1;
    }

    return static_cast<int>(mciStatusParms.dwReturn);
}

// check if we need to call the 'finished' hook
// needs to go in all event loops
// MCI has a hook function via Win32 messages but either it's buggy
//   or SDL won't give it even via syswm events
void RBACheckFinishedHook()
{
	static fix64 last_check_time = 0;
	
	if (!wCDDeviceID) return;


	if ((timer_query() - last_check_time) >= F2_0)
	{
        MCIERROR mciError;
        MCI_STATUS_PARMS mciStatusParms;

        mciStatusParms.dwItem = MCI_STATUS_MODE;
        if ((mciError = mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&mciStatusParms))))
        {
            Warning("RBAudio win32: cannot determine MCI media status (%lx)", mciError);
            return;
        }

        // time for a hack. for some reason, MCI sometimes stops
        // from around a few to few dozen frames before it should,
        // so we will check if we haven't moved from the last check
        // and allow a bit of a leeway when checking if so.

        int checkValue = playEnd;
        int thisFrames = mci_TotalFramesMsf(mciStatusParms.dwReturn);

        if (thisFrames == lastFrames)
            checkValue -= 64;

        if (redbook_finished_hook && playEnd >= 0 && lastFrames >= checkValue)
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
    
    MCIERROR mciError;
    MCI_STATUS_PARMS mciStatusParms;

    mciStatusParms.dwItem = MCI_STATUS_MODE;
    if ((mciError = mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&mciStatusParms))))
    {
		Warning("RBAudio win32: cannot determine MCI media status (%lx)", mciError);
        return 0;
    }

	if (mciStatusParms.dwReturn != MCI_MODE_PLAY)
        return 0;

    mciStatusParms.dwItem = MCI_STATUS_CURRENT_TRACK;
    if ((mciError = mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&mciStatusParms))))
    {
		Warning("RBAudio win32: cannot determine MCI media status (%lx)", mciError);
        return 0;
    }

    return static_cast<int>(mciStatusParms.dwReturn);
}

int RBAPeekPlayStatus()
{
    if (!wCDDeviceID) return 0;
    
    MCIERROR mciError;
    MCI_STATUS_PARMS mciStatusParms;

    mciStatusParms.dwItem = MCI_STATUS_MODE;
    if ((mciError = mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&mciStatusParms))))
    {
		Warning("RBAudio win32: cannot determine MCI media status (%lx)", mciError);
        return 0;
    }

	if (mciStatusParms.dwReturn == MCI_MODE_PLAY)
        return 1;
	else if (mciStatusParms.dwReturn == MCI_MODE_PAUSE)
		return -1; // hack so it doesn't keep restarting paused music
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

static int mci_msf_totalSeconds(int msf)
{
    return MCI_MSF_MINUTE(msf) * 60 + MCI_MSF_SECOND(msf);
}

static int mci_msf_totalFrames(int msf)
{
    return mci_msf_totalSeconds(msf) * CD_FPS + MCI_MSF_FRAME(msf);
}


unsigned long RBAGetDiscID()
{
	int i, t = 0, n = 0, trackCount, totalLength;

	if (!wCDDeviceID)
		return 0;

    MCIERROR mciError;
    MCI_STATUS_PARMS mciStatusParms;
    trackCount = RBAGetNumberOfTracks();

    mciStatusParms.dwItem = MCI_STATUS_LENGTH;
    if ((mciError = mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&mciStatusParms))))
    {
        Warning("RBAudio win32: cannot determine total length (%lx)", mciError);
        return 0;
    }
    totalLength = static_cast<int>(mciStatusParms.dwReturn);

    // we must switch to MSF to actually get starting times
    if (!mci_enableMsf())
    {
        return 0;
    }

	/* For backward compatibility this algorithm must not change */

	i = 1;

	while (i <= trackCount)
    {
        mciStatusParms.dwItem = MCI_STATUS_POSITION;
        mciStatusParms.dwTrack = i;
        if ((mciError = mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, reinterpret_cast<DWORD_PTR>(&mciStatusParms))))
        {
            Warning("RBAudio win32: cannot determine track %i offset (%lx)", i, mciError);
            mci_restoreTmsf();
            return 0;
        }
		n += cddb_sum(mci_msf_totalSeconds(static_cast<int>(mciStatusParms.dwReturn)));
		i++;
	}

	t = mci_msf_totalSeconds(totalLength);

    mci_restoreTmsf();
	return ((n % 0xff) << 24 | t << 8 | trackCount);
}

void RBAList(void)
{
    if (!wCDDeviceID) return;

    MCIERROR mciError;
    MCI_STATUS_PARMS mciStatusParms;
    int trackCount = RBAGetNumberOfTracks();

    mci_enableMsf();

    for (int i = 1; i <= trackCount; ++i) {
        int isAudioTrack, length, offset;
        mciStatusParms.dwTrack = i;

        mciStatusParms.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
        if ((mciError = mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, reinterpret_cast<DWORD_PTR>(&mciStatusParms))))
        {
            Warning("RBAudio win32: cannot determine track %d type (%lx)", i, mciError);
            continue;
        }
        isAudioTrack = mciStatusParms.dwReturn == MCI_CDA_TRACK_AUDIO;

        mciStatusParms.dwItem = MCI_STATUS_POSITION;
        if ((mciError = mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, reinterpret_cast<DWORD_PTR>(&mciStatusParms))))
        {
            Warning("RBAudio win32: cannot determine track %d offset (%lx)", i, mciError);
            continue;
        }
        offset = mci_msf_totalFrames(static_cast<int>(mciStatusParms.dwReturn));

        mciStatusParms.dwItem = MCI_STATUS_LENGTH;
        if ((mciError = mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, reinterpret_cast<DWORD_PTR>(&mciStatusParms))))
        {
            Warning("RBAudio win32: cannot determine track %d length (%lx)", i, mciError);
            continue;
        }
        length = mci_msf_totalFrames(static_cast<int>(mciStatusParms.dwReturn));

		con_printf(CON_VERBOSE, "RBAudio win32: CD track %d, type %s, length %d, offset %d", i, isAudioTrack ? "audio" : "data", length, offset);
    }
    
    mci_restoreTmsf();
}

}

#endif
