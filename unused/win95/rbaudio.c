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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/


#pragma off (unreferenced)
static char rcsid[] = "$Id: rbaudio.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)

#define _WIN32
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <mmsystem.h>

#include <stdlib.h>
#include <string.h>

#include "winapp.h"
#include "mono.h"
#include "error.h"
#include "rbaudio.h"
#include "fix.h"
#include "winapp.h"


#define HSG_CD_PLAYBACK 0
#define MSF_CD_PLAYBACK	1


#pragma pack(1)

typedef struct CDTrack {
	DWORD msf;
	DWORD length;
} CDTrack;

int RBCDROM_State = 0;							// CD is not used = 0
														// CD is used = 1
														// CD is already used = -1

static CDTrack	CDTrackInfo[255];
static UINT CDDiscID = 0;

static int 		RedBookEnabled=0, RedBookInstalled = 0;
static UINT 	CDDeviceID = 0;
static DWORD	CDNumTracks = 0;
static fix Playback_Start_Time = 0;
static fix Playback_Pause_Time = 0;
static fix Playback_Length = 0;
static int Playback_first_track,Playback_last_track;
static int AUXDevice = -1;
static char MCIErrorMsg[256];

//	Function Prototypes

unsigned long lsn_to_msf(unsigned long lsn);
unsigned long msf_to_lsn(unsigned long msf);
unsigned long msf_add(unsigned long msf1, unsigned long msf2);
unsigned long msf_sub(unsigned long msf1, unsigned long msf2);

UINT MakeCDDiscID(int tracks, unsigned long msflen, unsigned long msftrack1);
UINT GetCDDiscID();


//	Functions

void RBClose()
{
	if (RedBookInstalled) {
		mciSendCommand(CDDeviceID, MCI_CLOSE, 0, NULL);
		RedBookEnabled = 0;
		RedBookInstalled = 0;
	}
}


void RBAInit(void)
{
	MCI_OPEN_PARMS	mciOpenParms;
	MCI_SET_PARMS	mciSetParms;
	DWORD				retval = 0;
	int 				num_devs, i;

	atexit(RBClose);

	num_devs = auxGetNumDevs();
	
//	Find CD-AUDIO device if there is one.
	for (i = 0; i < num_devs; i++)
	{
		AUXCAPS caps;
		auxGetDevCaps(i, &caps, sizeof(caps));
		if (caps.wTechnology == AUXCAPS_CDAUDIO) {
			AUXDevice = i;
			mprintf((0, "CD operating on AUX %d.\n", AUXDevice));
			break;
		}
	}
					
	if (AUXDevice == -1) {
		mprintf((1, "Unable to find CD-AUDIO device. No Redbook music.\n"));
		RedBookEnabled = 0;
		return;
	}

//	We need to identify that a cd audio driver exists
	mciOpenParms.lpstrDeviceType = "cdaudio";

	retval = mciSendCommand(NULL, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_SHAREABLE, (DWORD)(LPVOID)&mciOpenParms);
	if (retval) {
		if (retval == MCIERR_MUST_USE_SHAREABLE) RBCDROM_State = -1;
		else RBCDROM_State = 0;
		mciGetErrorString(retval, MCIErrorMsg, 256);
		mprintf((1,"RBA (%x) MCI:%s.\n", retval, MCIErrorMsg));
		RedBookEnabled = 0;
	}
	else {
	// Now we need to set the time format to MSF for Redbook Compatablity.
		RedBookInstalled = 1;
		RBCDROM_State = 1;
		CDDeviceID = mciOpenParms.wDeviceID;

		logentry("Initializing Redbook device %d.\n", CDDeviceID);

		mciSetParms.dwTimeFormat = MCI_FORMAT_MSF;
		retval = mciSendCommand(CDDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)(LPVOID)&mciSetParms);
		if (retval) {
			mciSendCommand(CDDeviceID, MCI_CLOSE, 0, NULL);
			mprintf((0, "Error %d.  Unable to set Redbook Format.\n", retval));
			RedBookEnabled = 0;
		}
		else RedBookEnabled = 1;
	}
}


void RBARegisterCD(void)
{
	DWORD					i, numtracks, retval = 0;
	MCI_STATUS_PARMS	mciStatusParms;

//	Insure that CD is Redbook, then get track information
	mciStatusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS; 
	retval = mciSendCommand(CDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, 
						(DWORD)(LPVOID)&mciStatusParms);
	if (retval) {
		mciGetErrorString(retval, MCIErrorMsg, 256);
		mprintf((1,"RBA MCI:%s.\n", MCIErrorMsg));
		CDNumTracks = 0;
		return;
	}
	
	CDNumTracks = numtracks = mciStatusParms.dwReturn;

	for (i = 0; i < numtracks; i++)
	{
	  	mciStatusParms.dwTrack = i+1;
		mciStatusParms.dwItem = MCI_STATUS_POSITION;
		retval = mciSendCommand(CDDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD)(LPVOID)&mciStatusParms);	
		if (retval) {
			mprintf((0, "Could not retrieve information on track %d.\n", i+1));
		}
		CDTrackInfo[i].msf = mciStatusParms.dwReturn;
		
		mciStatusParms.dwTrack = i+1;
		mciStatusParms.dwItem = MCI_STATUS_LENGTH;
		retval = mciSendCommand(CDDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD)(LPVOID)&mciStatusParms);
		if (retval) { 
			mprintf((0, "Couldn't get length of track %d.\n", i+1));
		}
		CDTrackInfo[i].length = mciStatusParms.dwReturn;

//		logentry("[%d] (%d:%d):(%d:%d)\n", i+1, MCI_MSF_MINUTE(CDTrackInfo[i].msf), MCI_MSF_SECOND(CDTrackInfo[i].msf), MCI_MSF_MINUTE(CDTrackInfo[i].length), MCI_MSF_SECOND(CDTrackInfo[i].length));
	}	

	CDDiscID = GetCDDiscID();
} 


long RBAGetDeviceStatus()
{
	return 0;
}


int get_track_time(int first,int last)
{
	unsigned long playlen;
	int min, sec;

	playlen = msf_sub(msf_add(CDTrackInfo[last-1].msf,CDTrackInfo[last-1].length), CDTrackInfo[first-1].msf);

	min   = MCI_MSF_MINUTE(playlen);
	sec   = MCI_MSF_SECOND(playlen);

	return (min*60+sec);
}


int RBAPlayTrack(int track)
{
	MCI_PLAY_PARMS	mciPlayParms;
	int min=0; 
	int retval;

//	Register CD if media has changed
	if (!RedBookEnabled) return 0;
	if (RBACheckMediaChange()) {
		mprintf((0, "Unable to play track due to CD change.\n"));
		return 0;
	}

//	Play the track
	min = min + MCI_MSF_MINUTE(CDTrackInfo[track-1].msf) + MCI_MSF_MINUTE(CDTrackInfo[track-1].length);
	mciPlayParms.dwFrom = CDTrackInfo[track-1].msf; // MCI_MAKE_TMSF(track, 0, 0, 0);
	mciPlayParms.dwTo = msf_add(CDTrackInfo[track-1].msf, CDTrackInfo[track-1].length);
	retval = mciSendCommand(CDDeviceID, MCI_PLAY, MCI_FROM | MCI_TO, (DWORD)(LPVOID)&mciPlayParms);

	if (retval) {
		mciGetErrorString(retval, MCIErrorMsg, 256);
		mprintf((1,"RBA MCI:%s.\n", MCIErrorMsg));
		mprintf((0, "Unable to play CD track %d.\n", track));
		return 0;
	}
	else {
		Playback_Length = i2f(get_track_time(track,track));
		Playback_Start_Time = timer_get_fixed_seconds();

		Playback_first_track = Playback_last_track = track;

		mprintf( (0,"Playing Track %d (len: %d secs)\n", track, f2i(Playback_Length)) );
	}

	return 1;
}


int RBAPlayTracks(int start, int end)
{
	MCI_PLAY_PARMS	mciPlayParms;
	int retval;

	Playback_Start_Time = 0;
	Playback_Length = 0;

//	Register CD if media has changed
	if (!RedBookEnabled) return 0;
	if (RBACheckMediaChange()) {
		mprintf((0, "Unable to play track due to CD change.\n"));
		return 0;
	}

//	Play the track
	mciPlayParms.dwFrom = CDTrackInfo[start-1].msf; // MCI_MAKE_TMSF(track, 0, 0, 0);
	mciPlayParms.dwTo = msf_add(CDTrackInfo[end-1].msf, CDTrackInfo[end-1].length);
	retval = mciSendCommand(CDDeviceID, MCI_PLAY, MCI_FROM | MCI_TO, (DWORD)(LPVOID)&mciPlayParms);

	if (retval) {
		mciGetErrorString(retval, MCIErrorMsg, 256);
		mprintf((1,"RBA MCI:%s.\n", MCIErrorMsg));
		mprintf((0, "Unable to play CD tracks %d-%d.\n", start,end));
		return 0;		
	}
	else {
		Playback_Length = i2f(get_track_time(start,end));
		Playback_Start_Time = timer_get_fixed_seconds();

		Playback_first_track = start;
		Playback_last_track = end;

		mprintf( (0,"Playing Tracks %d-%d (len: %d secs)\n", start,end,f2i(Playback_Length)) );
	}

	return 1;
}


void RBAStop()
{
	int retval;

	if (!RedBookEnabled) return;
	
	retval = mciSendCommand(CDDeviceID, MCI_STOP,0,NULL);
	if (retval) {
		mciGetErrorString(retval, MCIErrorMsg, 256);
		mprintf((1,"RBA MCI:%s.\n", MCIErrorMsg));
		mprintf((0, "CD Stop command failed.\n"));
	} 
}


void RBASetStereoAudio(RBACHANNELCTL *channels)
{

}


void RBASetQuadAudio(RBACHANNELCTL *channels)
{

}


void RBAGetAudioInfo(RBACHANNELCTL *channels)
{

}


static unsigned rba_paused_head_loc = 0;

void RBAPause()
{
	int retval;
	MCI_GENERIC_PARMS mciGenParms;
	int frame, sec, min;


  if (!RedBookEnabled) return;

	rba_paused_head_loc = RBAGetHeadLoc(&min, &sec, &frame);	

	mciGenParms.dwCallback = GetLibraryWindow();
	retval = mciSendCommand(CDDeviceID, MCI_PAUSE, MCI_NOTIFY, 
		(DWORD)(LPVOID)&mciGenParms);
	if (retval)	{
		mprintf((1,"ERROR: Unable to pause CD.\n"));
	}
	else {
		mprintf((0, "Pausing CD...\n"));
		Playback_Pause_Time = timer_get_fixed_seconds();
	}

}


int RBAResume()
{
	int retval;
	MCI_PLAY_PARMS mciPlayParms;

	if (!RedBookEnabled) return 0;
   
	if (RBACheckMediaChange()) {
		RBARegisterCD();
		return RBA_MEDIA_CHANGED;
	}

	mciPlayParms.dwFrom = rba_paused_head_loc; // MCI_MAKE_TMSF(track, 0, 0, 0);
	mciPlayParms.dwTo = msf_add(CDTrackInfo[Playback_last_track-1].msf, 
									CDTrackInfo[Playback_last_track-1].length);
	retval = mciSendCommand(CDDeviceID, MCI_PLAY, MCI_FROM | MCI_TO, (DWORD)(LPVOID)&mciPlayParms);

	if (retval) {
		mprintf((1, "ERROR: Resume CD play failed.\n"));
	}
	else 
		Playback_Start_Time += timer_get_fixed_seconds() - Playback_Pause_Time;

	return 1;
}


void RBASetChannelVolume(int channel, int volume) 
{
	DWORD vol;
	
	volume = volume << 8;
	
	if (channel == 0) 
		vol = MAKELONG(0,volume);
	else if (channel == 1) 
		vol = MAKELONG(volume,0);

	auxSetVolume(AUXDevice, vol);
}


void RBASetVolume(int volume)
{
	DWORD vol;
	WORD wvol;

	wvol = (WORD)(volume) << 8;

	vol = MAKELONG(wvol, wvol);
	auxSetVolume(AUXDevice, vol);
}


long RBAGetHeadLoc(int *min, int *sec, int *frame)
{
	MCI_STATUS_PARMS mciStatusParms;
	int retval;

	if (!RedBookEnabled) return 0;

	if (RBACheckMediaChange())
		return RBA_MEDIA_CHANGED;

	mciStatusParms.dwItem = MCI_STATUS_POSITION;
	retval = mciSendCommand(CDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)(LPVOID)&mciStatusParms);
	
	if (retval) {
		mprintf((0, "Couldn't get location of CD head.\n"));
	}

	*min = MCI_MSF_MINUTE(mciStatusParms.dwReturn);
	*sec = MCI_MSF_SECOND(mciStatusParms.dwReturn);
	*frame = MCI_MSF_FRAME(mciStatusParms.dwReturn);

	return mciStatusParms.dwReturn;
}		


int RBAGetTrackNum()
{
	int track;
	int delta_time;	//in seconds

	if (!RBAPeekPlayStatus())
		return 0;

	delta_time = f2i(timer_get_fixed_seconds()-Playback_Start_Time+f1_0/2);

	for (track=Playback_first_track;track<=Playback_last_track && delta_time>0;track++) {

		delta_time -= get_track_time(track,track);

		if (delta_time < 0)
			break;
	}

	Assert(track <= Playback_last_track);

	return track;
}


int RBAPeekPlayStatus()
{
//@@	MCI_STATUS_PARMS mciStatusParms;
//@@	int retval;

	if (!RedBookEnabled) return 0;


	if ((timer_get_fixed_seconds()-Playback_Start_Time) > Playback_Length) return 0;
	else return 1; 


//@@	mciStatusParms.dwItem = MCI_STATUS_MODE;
//@@	retval = mciSendCommand(CDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)(LPVOID)&mciStatusParms);
//@@	if (retval) {
//@@		mprintf((0, "Unable to obtain current status of CD.\n"));
//@@	}
//@@	
//@@	if (mciStatusParms.dwReturn == MCI_MODE_PLAY) return 1;
//@@	else return 0;
}
	


int RBAEnabled()
{
	if (RedBookEnabled < 1) return 0;
	else return 1;
}


void RBAEnable()
{
	RedBookEnabled = 1;
	mprintf((0, "Enabling Redbook...\n"));
}


void RBADisable()
{
	RedBookEnabled = 0;
	mprintf((0, "Disabling Redbook...\n"));
}


int RBAGetNumberOfTracks(void)
{
	MCI_STATUS_PARMS	mciStatusParms;
	int retval;	

	if (!RedBookEnabled) return 0;

	if (RBACheckMediaChange())  
		RBARegisterCD();
		
	mciStatusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS; 
	retval = mciSendCommand(CDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, 
						(DWORD)(LPVOID)&mciStatusParms);

	if (retval) {
		mprintf((0, "Get number of CD tracks failed.\n"));
		return 0;
	}

	return mciStatusParms.dwReturn;
}


//	RB functions:  Internal to RBA library	-----------------------------

int RBACheckMediaChange()
{
	if (!RedBookEnabled) return 0;

	if (CDDiscID != GetCDDiscID() || !CDDiscID) return 1;
	else return 0;
}


//	Converts Logical Sector Number to Minutes Seconds Frame Format

unsigned long lsn_to_msf(unsigned long lsn)
{
   unsigned long min,sec,frame;
   lsn += 150;

	min   =  lsn / (60*75);
   lsn   =  lsn % (60*75);
	sec   =  lsn / 75;
   lsn   =  lsn % 75;
	frame =  lsn;

	return((min << 16) + (sec << 8) + (frame << 0));
}

// convert minutes seconds frame format to a logical sector number.

unsigned long msf_to_lsn(unsigned long msf)
{
  	unsigned long min,sec,frame;

	min   = (msf >> 16) & 0xFF;
   sec   = (msf >>  8) & 0xFF;
   frame = (msf >>  0) & 0xFF;

	return(  (min * 60*75) + (sec * 75) + frame - 150  );
}

unsigned long msf_add(unsigned long msf1, unsigned long msf2)
{
	uint min1, sec1, frame1;
	uint min2, sec2, frame2;

//	we don't take frames into account, which may not be right.
	min1 = MCI_MSF_MINUTE(msf1);
	sec1 = MCI_MSF_SECOND(msf1);
	frame1 = MCI_MSF_FRAME(msf1);

	frame2 = 0;
	min2 = 0;

	if ((sec1 + MCI_MSF_SECOND(msf2)) > 59) {
		sec2 = (sec1+MCI_MSF_SECOND(msf2)) - 60;
		min1++;
	}
	else sec2 = sec1 + MCI_MSF_SECOND(msf2);
	min2 = min1+MCI_MSF_MINUTE(msf2);
	frame2 = 0;
	
//	logentry("msf_add:(%d:%d)\n", min2, sec2);

	return MCI_MAKE_MSF(min2, sec2, 0);
} 


unsigned long msf_sub(unsigned long msf1, unsigned long msf2)
{
	int min1, sec1, frame1;
	int min2, sec2, frame2;

//	we don't take frames into account, which may not be right.
	min1 = MCI_MSF_MINUTE(msf1);
	sec1 = MCI_MSF_SECOND(msf1);
	frame1 = MCI_MSF_FRAME(msf1);

	frame2 = 0;
	min2 = 0;

	if ((sec1 - (int)MCI_MSF_SECOND(msf2)) < 0) {
		sec2 = 60 - ((int)MCI_MSF_SECOND(msf2)-sec1);
		min1--;					// This is a shortcut to min2 = min1-(MSF_MIN(msf2)-1)
	}
	else sec2 = sec1 - (int)MCI_MSF_SECOND(msf2);
	min2 = min1-(int)MCI_MSF_MINUTE(msf2);
	frame2 = 0;

//	logentry("msf_sub:(%d:%d)\n", min2, sec2);
	
	return MCI_MAKE_MSF(min2, sec2, 0);
} 


unsigned long msf_to_sec(unsigned long msf)
{
  	unsigned long min,sec,frame;

	min   = (msf >> 16) & 0xFF;
   sec   = (msf >>  8) & 0xFF;
   frame = (msf >>  0) & 0xFF;

	return (min*60) + sec;	
}


UINT GetCDDiscID()
{
	MCI_STATUS_PARMS	mciStatusParms;
	int retval;	
	unsigned long msflen, tracks, msftrack1;

	mciStatusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS; 
	if (retval = mciSendCommand(CDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, 
						(DWORD)(LPVOID)&mciStatusParms)) {
		mprintf((1, "RBA: Get number of CD tracks failed.\n"));
		return 0;
	}
	tracks = mciStatusParms.dwReturn;

	mciStatusParms.dwItem = MCI_STATUS_LENGTH;
	if (retval = mciSendCommand(CDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, 
						(DWORD)(LPVOID)&mciStatusParms)) {
		mprintf((1,	"RBA: Get media length failed.\n"));
		return 0;
	}
	msflen = mciStatusParms.dwReturn;

	mciStatusParms.dwTrack = 1;
	mciStatusParms.dwItem = MCI_STATUS_POSITION;
	if (retval = mciSendCommand(CDDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD)(LPVOID)&mciStatusParms)) {	
		mprintf((1, "Could not retrieve information on track %d.\n", 1));
		return 0;
	}
	msftrack1 = mciStatusParms.dwReturn;

	return MakeCDDiscID(tracks, msflen, msftrack1);
}


UINT MakeCDDiscID(int tracks, unsigned long msflen, unsigned long msftrack1)
{
	UINT code=0;
	
	code = (UINT)(msf_to_sec(msftrack1) << 19);
	code |= (UINT)(msf_to_sec(msflen) << 6);

	code |= (UINT)(tracks&0xffffffc0);

	return code;
}
	


