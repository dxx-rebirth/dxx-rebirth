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



#ifndef _DS_H
#define _DS_H

#include <mmsystem.h>
#include "dsound.h"


typedef struct tagSSCaps {
	unsigned sample_rate;
	unsigned bits_per_sample;
} SSCaps;


typedef struct tagSSoundBuffer {
	char *data;
	long length;
	LPDIRECTSOUNDBUFFER obj; 
	LPDIRECTSOUNDBUFFER auxobj;
	LPDIRECTSOUNDBUFFER auxobj2;	
	unsigned vol;
	unsigned rate;
	unsigned short pan;
	unsigned short channels;
	unsigned short bits_per_sample;
	unsigned	short looping;
	int loop_start, loop_end, loop_length;
} SSoundBuffer;

typedef int SSChannel;


void SSGetCaps(SSCaps *caps);
BOOL SSInit(HWND hWnd, int channels, unsigned flags);
void SSDestroy();
void SSSetVolume(DWORD vol);
WORD SSGetVolume();
BOOL SSInitBuffer(SSoundBuffer *sample); 
void SSDestroyBuffer(SSoundBuffer *sample);
SSChannel SSInitChannel(SSoundBuffer *sample);
BOOL SSChannelPlaying(SSChannel channel);
BOOL SSStopChannel(SSChannel channel);
BOOL SSSetChannelPan(SSChannel channel, unsigned short pan);
BOOL SSSetChannelVolume(SSChannel channel, unsigned short vol);	

#endif
