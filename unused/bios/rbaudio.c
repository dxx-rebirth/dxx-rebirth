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

char rbaudio_rcsid[] = "$Id: rbaudio.c,v 1.1.1.1 2001-01-19 03:30:14 bradleyb Exp $";


#include <dos.h>
#include <dpmi.h>
#include <string.h>

#include "mono.h"
#include "error.h"
#include "rbaudio.h"
#include "fix.h"
#include "timer.h"


#define HSG_CD_PLAYBACK 0
#define MSF_CD_PLAYBACK	1

#define RBERR_WRITEPROTVIOL	0
#define RBERR_UNKNOWNUNIT		1
#define RBERR_DRIVENOTREADY	2
#define RBERR_UNKNOWNCOMMAND	3
#define RBERR_CRCERROR			4
#define RBERR_BADDRIVEREQ		5
#define RBERR_SEEKERROR			6
#define RBERR_UNKNOWNMEDIA		7
#define RBERR_SECTORNOTFOUND	8
#define RBERR_WRITEFAULT		10
#define RBERR_READFAULT			12
#define RBERR_GENERALFAILURE	13
#define RBERR_BADDISKCHANGE	15

#define RBSTAT_ERROR				0x8000
#define RBSTAT_BUSY				0x0200
#define RBSTAT_DONE				0x0100

#pragma pack(1)


// Request Header	-----------------------------------------------------

typedef struct _REQHDR {
	unsigned char length;
	unsigned char subunit;
	unsigned char func;
	unsigned short status;
	char array[8];
} REQHDR;

typedef struct _IOCTL {
	char bpb;
	unsigned short buffer_realoff;
	unsigned short buffer_realseg;
	unsigned short xferlen;
	unsigned short sector;
	unsigned long id;
} IOCTL;


//	CD Information -----------------------------------------------------

static struct {
	char code;                       // Control Block Code
	char track;                      // Track Number
	long start;                      // Start of Track in MSF (Redbook)
	unsigned char info;              // Track Control Info
} CDTrackInfo = {11,2,0L,0};

static struct {
	REQHDR reqheader;
	IOCTL ioctl;
} TrackInfoREQ = {{13,0,3,0,""}, {0,0,0,0,0}};
  
static struct {
	unsigned char code;
	unsigned long devstat;
} DevStat = {6,0};

static struct {
	REQHDR	reqhdr;
	IOCTL	ioctl;
} DevStatREQ = {{13,0,3,0,""}, {0,0,0,0,0}};

static struct {
	unsigned char code;
	char byte;
} MedChg	= {9,0};

static struct {
	REQHDR 	reqhdr;
	IOCTL 	ioctl;
} MedChgREQ = {{13,0,3,0,""}, {0,0,0,0,0}};

static struct {
	REQHDR	reqhdr;
} StopREQ = {{13,0,133,0,""}};

static struct {
	REQHDR	reqhdr;
} ResumeREQ = {{13,0,136,0,""}};

static struct {
	unsigned char code;
	unsigned char first;
	unsigned char last;
	unsigned long lead_out;
} CDInfo = {10,0,0,0L};

static struct {
	REQHDR	reqhdr;
	IOCTL	ioctl;
} CDInfoREQ = {{13,0,3,0,""},{0,0,0,0,0}};

static struct {
	REQHDR  reqheader;
	char    mode;         				// Play in HSG or MSF(RedBook)
	unsigned long playbeg;
	unsigned long playlen;    	      // HSG Format
	char 		empty;						// Empty byte used so that all requests have same size
} PlayREQ = {{13,0,132,0,""},0,0,0,0};

static struct {
	unsigned char code;
	unsigned char out0in;
	unsigned char out0vol;
	unsigned char out1in;
	unsigned char out1vol;
	unsigned char out2in;
	unsigned char out2vol;
	unsigned char out3in;
	unsigned char out3vol;
} AUDChannelCtl = {3,0,0,0,0,0,0,0,0};

static struct {
	REQHDR	reqhdr;
	IOCTL		ioctl;
} AUDChannelCtlREQ = {{13,0,12,0,""},{0,0,0,0,0}};

static struct {
	unsigned char code;
	unsigned char out0in;
	unsigned char out0vol;
	unsigned char out1in;
	unsigned char out1vol;
	unsigned char out2in;
	unsigned char out2vol;
	unsigned char out3in;
	unsigned char out3vol;
} AUDChannelInfo = {4,0,0,0,0,0,0,0,0};

static struct {
	REQHDR	reqhdr;
	IOCTL		ioctl;
} AUDChannelInfoREQ = {{13,0,3,0,""},{0,0,0,0,0}};

static struct {
	unsigned char code;
	unsigned char mode;
	unsigned long headloc;
} HeadInfo = {1,MSF_CD_PLAYBACK,0L};

static struct {
	REQHDR	reqhdr;
	IOCTL		ioctl;
} HeadInfoREQ = {{13,0,3,0,""},{0,0,0,0,0}};

//@@static struct {
//@@	ubyte code;
//@@	ubyte adr;
//@@	ubyte	tno;
//@@	ubyte	point;
//@@	ubyte	min;
//@@	ubyte	sec;
//@@	ubyte	frame;
//@@	ubyte	zero;
//@@	ubyte	pmin;
//@@	ubyte	psec;
//@@	ubyte	pframe;
//@@} QChannelInfo = {12, MSF_CD_PLAYBACK, 2,0,0,0,0,0,0,0,0	};
//@@
//@@static struct {
//@@	REQHDR	reqhdr;
//@@	IOCTL		ioctl;
//@@} QChannelInfoREQ = {{13, 0, 3, 0, ""}, {0,0,0,0,0}};


//	Translation from RealMode->Protected mode buffer

typedef struct _REQ_xlatbuf {			// Buffer for Real Mode Callback
	REQHDR reqheader;
	char ioctl[10];
} REQ_xlatbuf;



//	Other Data ---------------------------------------------------------

static unsigned char CDDriveLetter;
static unsigned long CDNumTracks,CDTrackStart[101];
static int RedBookEnabled = 0;
static int RedBookPlaybackType = MSF_CD_PLAYBACK;
static fix Playback_Start_Time = 0;
static fix Playback_Pause_Time = 0;
static fix Playback_Length = 0;
static int Playback_first_track,Playback_last_track;
							 

//	Prototypes ---------------------------------------------------------

int RBSendRequest(char *request, char *xferptr, int xferlen);	
unsigned long msf_to_lsn(unsigned long msf);
unsigned long lsn_to_msf(unsigned long lsn);


//	--------------------------------------------------------------------

void RBAInit(ubyte cd_drive_num)	//drive a == 0, drive b == 1
{
	union REGS regs;

//	Detect presence of CD-ROM
	regs.x.eax = 0x1500;
	regs.x.ebx = 0;
	int386(0x2f, &regs, &regs);
	
	if (regs.x.ebx == 0) 
		RBADisable();						// Disable RedBook
	//	Error("RBA Error: MSCDEX.EXE compatible driver couldn't be found.");
	else {
		CDDriveLetter = cd_drive_num;
//		CDDriveLetter = (unsigned char)regs.x.ecx;

		RBAEnable();
		RBACheckMediaChange();

		RBSendRequest((char*)&DevStatREQ, (char*)&DevStat, sizeof(DevStat));
		
	// If Door drive open, or no disc in CD-ROM, then Redbook is Disabled.
		if ((DevStat.devstat&2048) || (DevStat.devstat&1) || RBAEnabled() == 0) 
			RBADisable();

//@@		if (DevStat.devstat&4) 
//@@			mprintf((0, "supports cooked and raw reading.\n"));
//@@		else 
//@@			mprintf((0, "supports only cooked reading.\n"));
//@@
//@@		if (DevStat.devstat&256) 
//@@			mprintf((0, "audio channel manipulation.\n"));
//@@		else 
//@@			mprintf((0, "no audio channel manipulation.\n"));
	
		if (DevStat.devstat&512) {
//			RedBookPlaybackType = MSF_CD_PLAYBACK;
			mprintf((0, "supports HSG and RedBook addressing mode.\n"));
		}
		else {
//			RedBookPlaybackType = HSG_CD_PLAYBACK;
			mprintf((0, "supports HSG addressing only.\n"));
		}
			
		RedBookPlaybackType = MSF_CD_PLAYBACK;

	}
}

//find out how many tracks on the CD, and their starting locations
void RBARegisterCD(void)
{
	int i;
	int error;

	//	Get CD Information First
	RBACheckMediaChange();

	error = RBSendRequest((char*)&CDInfoREQ, (char*)&CDInfo, sizeof(CDInfo));

	if (error & RBSTAT_ERROR) {		//error!
		CDNumTracks = 0;
		return;
	}
	
	CDNumTracks = (CDInfo.last-CDInfo.first)+1;

	// Define Track Starts
	for (i=CDInfo.first; i<=CDNumTracks; i++)
	{
		CDTrackInfo.track = i;
		RBSendRequest((char *)&TrackInfoREQ, (char*)&CDTrackInfo, sizeof(CDTrackInfo));
		CDTrackStart[i] = CDTrackInfo.start;
	}

	CDTrackStart[i] = CDInfo.lead_out;
} 


long RBAGetDeviceStatus()
{
	RBSendRequest((char*)&DevStatREQ, (char*)&DevStat, sizeof(DevStat));
	return (long)DevStat.devstat;
}


int RBAGetNumberOfTracks(void)
{
	//	Get CD Information
	if (RBACheckMediaChange())
		RBARegisterCD(); 

	return CDNumTracks;
}

//returns the length in seconds of tracks first through last, inclusive
int get_track_time(int first,int last)
{
	unsigned long playlen;
	int min, sec;

	playlen = msf_to_lsn(CDTrackStart[last+1]) - msf_to_lsn(CDTrackStart[first]);
	playlen = lsn_to_msf(playlen);
	min   = (int)((playlen >> 16) & 0xFF);
	sec   = (int)((playlen >>  8) & 0xFF);

	return (min*60+sec);
}

int RBAPlayTrack(int track)
{
	Playback_Start_Time = 0;
	Playback_Length = 0;

	if (track < CDInfo.first || track > CDInfo.last) {
		mprintf((0,"CD not Redbook Compatible. Will not play track.\n"));
	}
//		Error("RBA Error: Track %d doesn't exist on CD!!!", track);
	
	if (RBACheckMediaChange()) {
		mprintf((0, "Unable to play track due to CD change.\n"));
		return 0;
	}

	//	Play the track now!!!
	PlayREQ.mode = RedBookPlaybackType;
	PlayREQ.playbeg = CDTrackStart[track];

	if (RedBookPlaybackType == MSF_CD_PLAYBACK) {
		PlayREQ.playlen = 
			msf_to_lsn(CDTrackStart[track+1]) - msf_to_lsn(CDTrackStart[track]);
	}
	else {
		mprintf((1, "RBERR: No! No! No! This shouldn't happen (HSG_CD_PLAYBACK).\n"));
		Int3();
		PlayREQ.playlen = CDTrackStart[track+1] - CDTrackStart[track];
	}
	RBSendRequest((char *)&PlayREQ,NULL,0);

	Playback_Length = i2f(get_track_time(track,track));
	Playback_Start_Time = timer_get_fixed_seconds();

	Playback_first_track = Playback_last_track = track;

	mprintf( (0,"Playing Track %d (len: %d secs)\n", track, f2i(Playback_Length)) );

	return 1;
}

//plays tracks first through last, inclusive
int RBAPlayTracks(int first, int last)
{
	Playback_Start_Time = 0;
	Playback_Length = 0;

	if (first < CDInfo.first || last > CDInfo.last) {
		mprintf((0,"Invalid start or end track.\n"));
	}
//		Error("RBA Error: Track %d doesn't exist on CD!!!", track);
	
	if (RBACheckMediaChange()) {
		mprintf((0, "Unable to play track due to CD change.\n"));
		return 0;
	}

	//	Play the track now!!!
	PlayREQ.mode = RedBookPlaybackType;
	PlayREQ.playbeg = CDTrackStart[first];

	if (RedBookPlaybackType == MSF_CD_PLAYBACK) {
		PlayREQ.playlen = 
			msf_to_lsn(CDTrackStart[last+1]) - msf_to_lsn(CDTrackStart[first]);
	}
	else {
		mprintf((1, "RBERR: No! No! No! This shouldn't happen (HSG_CD_PLAYBACK).\n"));
		Int3();
		PlayREQ.playlen = CDTrackStart[last+1] - CDTrackStart[first];
	}
	RBSendRequest((char *)&PlayREQ,NULL,0);

	Playback_Length = i2f(get_track_time(first,last));
	Playback_Start_Time = timer_get_fixed_seconds();

	Playback_first_track = first;
	Playback_last_track = last;

	mprintf( (0,"Playing Tracks %d-%d (len: %d secs)\n", first,last,f2i(Playback_Length)) );

	return 1;
}


void RBAPause()
{
	RBSendRequest((char *)&StopREQ, NULL, 0);
	Playback_Pause_Time = timer_get_fixed_seconds();
}


int RBAResume()
{
	if (RBACheckMediaChange()) {
		RBARegisterCD();
		return RBA_MEDIA_CHANGED;
	}
	RBSendRequest((char *)&ResumeREQ, NULL, 0);

	Playback_Start_Time += timer_get_fixed_seconds() - Playback_Pause_Time;

	return 1;
}


void RBAStop()
{
	if (RBACheckMediaChange())
		RBARegisterCD();

	RBSendRequest((char *)&StopREQ,NULL,0);      
}


void RBASetStereoAudio(RBACHANNELCTL *channels)
{
	AUDChannelCtl.out0in = channels->out0in;
	AUDChannelCtl.out0vol = channels->out0vol;
	AUDChannelCtl.out1in = channels->out1in;
	AUDChannelCtl.out1vol = channels->out1vol;
	AUDChannelCtl.out2in = AUDChannelCtl.out2vol = 0;
	AUDChannelCtl.out3in = AUDChannelCtl.out3vol = 0;
	
	RBSendRequest((char*)&AUDChannelCtlREQ, (char*)&AUDChannelCtl, sizeof(AUDChannelCtl));
}


void RBASetQuadAudio(RBACHANNELCTL *channels)
{
	AUDChannelCtl.out0in = (unsigned char)channels->out0in;
	AUDChannelCtl.out0vol = (unsigned char)channels->out0vol;
	AUDChannelCtl.out1in = (unsigned char)channels->out1in;
	AUDChannelCtl.out1vol = (unsigned char)channels->out1vol;
	AUDChannelCtl.out2in = (unsigned char)channels->out2in;
	AUDChannelCtl.out2vol = (unsigned char)channels->out2vol;
	AUDChannelCtl.out3in = (unsigned char)channels->out3in;
	AUDChannelCtl.out3vol = (unsigned char)channels->out3vol;
	
	RBSendRequest((char*)&AUDChannelCtlREQ, (char*)&AUDChannelCtl, sizeof(AUDChannelCtl));
}


void RBAGetAudioInfo(RBACHANNELCTL *channels)
{
	RBSendRequest((char*)&AUDChannelInfoREQ, (char*)&AUDChannelInfo, sizeof(AUDChannelInfo));
	
	channels->out0in = (int)AUDChannelInfo.out0in;
	channels->out0vol = (int)AUDChannelInfo.out0vol;
	channels->out1in = (int)AUDChannelInfo.out1in;
	channels->out1vol = (int)AUDChannelInfo.out1vol;
	channels->out2in = (int)AUDChannelInfo.out2in;
	channels->out2vol = (int)AUDChannelInfo.out2vol;
	channels->out3in = (int)AUDChannelInfo.out3in;
	channels->out3vol = (int)AUDChannelInfo.out3vol;

}


void RBASetChannelVolume(int channel, int volume) 
{
	//RBACHANNELCTL channels;
	
	RBSendRequest((char*)&AUDChannelInfoREQ, (char*)&AUDChannelInfo, sizeof(AUDChannelInfo));

	AUDChannelCtl.out0in = AUDChannelInfo.out0in;
	AUDChannelCtl.out0vol = AUDChannelInfo.out0vol;
	AUDChannelCtl.out1in = AUDChannelInfo.out1in;
	AUDChannelCtl.out1vol = AUDChannelInfo.out1vol;
	AUDChannelCtl.out2in = AUDChannelInfo.out2in;
	AUDChannelCtl.out2vol = AUDChannelInfo.out2vol;
	AUDChannelCtl.out3in = AUDChannelInfo.out3in;
	AUDChannelCtl.out3vol = AUDChannelInfo.out3vol;

	if (channel == 0) AUDChannelCtl.out0vol = (unsigned char)volume;
	if (channel == 1) AUDChannelCtl.out1vol = (unsigned char)volume;
	if (channel == 2) AUDChannelCtl.out2vol = (unsigned char)volume;
	if (channel == 3) AUDChannelCtl.out3vol = (unsigned char)volume;
	
	RBSendRequest((char*)&AUDChannelCtlREQ,(char*)&AUDChannelCtl, sizeof(AUDChannelCtl));

}


void RBASetVolume(int volume)
{
	RBASetChannelVolume(0,volume);
	RBASetChannelVolume(1,volume);
}


long RBAGetHeadLoc(int *min, int *sec, int *frame)
{
	unsigned long loc;

	if (RBACheckMediaChange())
		return RBA_MEDIA_CHANGED;

	HeadInfo.mode = RedBookPlaybackType;
	RBSendRequest((char*)&HeadInfoREQ, (char*)&HeadInfo, sizeof(HeadInfo));
	
	if (RedBookPlaybackType == MSF_CD_PLAYBACK)
		loc = HeadInfo.headloc;
	else 
		loc = lsn_to_msf(HeadInfo.headloc);

	*min   = (int)((loc >> 16) & 0xFF);
	*sec   = (int)((loc >>  8) & 0xFF);
	*frame = (int)((loc >>  0) & 0xFF);

	return loc;
}		

//return the track number currently playing.  Useful if RBAPlayTracks() 
//is called.  Returns 0 if no track playing, else track number
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
	if (RBACheckMediaChange()) {		//if media changed, then not playing
		RBARegisterCD(); 
		return 0;
	}

	if ((timer_get_fixed_seconds()-Playback_Start_Time) > Playback_Length) return 0;
	else return 1; 
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



//	RB functions:  Internal to RBA library	-----------------------------

//returns 1 if media has changed, else 0
int RBACheckMediaChange()
{
	RBSendRequest((char *)&MedChgREQ, (char*)&MedChg, sizeof(MedChg));

	if (MedChg.byte == 255 || MedChg.byte == 0) {
		// New media in drive (or maybe no media in drive)
		mprintf((0,"CD-ROM Media change detected\n"));
		return 1;
	}
	return 0;
}


//	Send Request

//returns 16-bit value.  Bit 15 means error.
//WE NEED SYMBOLOIC CONSTANTS FOR ALL RETURN CODES & STATUS BITS
int RBSendRequest(char *request, char *xferptr, int xferlen)
{
	//union REGS regs;
	dpmi_real_regs rregs;

	IOCTL *ioctl;
	REQ_xlatbuf *xlat_req;				// Translated Buffer Request 
	char *xlat_xferptr;					// Translated Buffer Transfer Buffer Ptr
	
	unsigned short status;
	
	if (!RedBookEnabled) return 0;	// Don't send request if no RBA

	memset(&rregs,0,sizeof(dpmi_real_regs));

// Get Temporary Real Mode Buffer for request translation
	xlat_req = (REQ_xlatbuf *)dpmi_get_temp_low_buffer(128+xferlen);
	memcpy(xlat_req, request, sizeof(REQHDR)+sizeof(IOCTL));
	ioctl = (IOCTL *)(((char*)xlat_req)+sizeof(REQHDR));

//	Set Transfer Buffer in IOCTL reqion of 'request'	
	if (xferlen && xferptr)	{
		xlat_xferptr = ((char*)xlat_req) + 128;
		memcpy(xlat_xferptr, xferptr, xferlen);
		
		ioctl->buffer_realoff = DPMI_real_offset(xlat_xferptr);
		ioctl->buffer_realseg = DPMI_real_segment(xlat_xferptr);
		ioctl->xferlen = xferlen;
	}

//	Setup and Send Request Packet
	rregs.eax = 0x1510;
	rregs.ecx = (unsigned)(CDDriveLetter);
	rregs.es  = DPMI_real_segment(xlat_req);
	rregs.ebx = DPMI_real_offset(xlat_req);
	dpmi_real_int386x(0x2f, &rregs);

//	Return Translate Buffer to Protected mode 'xferptr'
	if (xferlen && xferptr) {
		memcpy(xferptr, xlat_xferptr, xferlen);
	}
	memcpy(request, xlat_req, sizeof(REQHDR)+sizeof(IOCTL));
	
//	Check for Errors.
	status = ((REQHDR *)request)->status;

	if (status & RBSTAT_ERROR) {
		mprintf((0,"Error in SendRequest: %x.\n", status));
	}

	return status;
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

