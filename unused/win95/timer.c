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
static char rcsid[] = "$Id: timer.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)

#include "types.h"
#include "fix.h"
#include "error.h"
#include "mono.h"

#define _WIN32
#define WIN32_LEAN_AND_MEAN
#include<windows.h>
#include<mmsystem.h>

#define TARGET_RESOLUTION 1

//@@ static UINT TimerResolution = 0;
//@@ static UINT GameTimer = 0;
//@@ #define GAME_TIMER_RESOLUTION (TimerResolution<<1)


int timer_initialized=0;

static UINT GameSystemTime = 0;
static UINT GameStartTime = 0;


void timer_normalize();

//@@ void PASCAL timer_callback(UINT wTimerID, UINT msg, DWORD dwUser, 
//@@					DWORD dw1, DWORD dw2) 
//@@{
//@@	static int last_time = 0;
//@@	UINT time;
//@@	
//@@	
//@@	time = timeGetTime()-GameStartTime;
//@@
//@@	if (timeGetTime() < GameStartTime) 
//@@		GameStartTime = GameStartTime - timeGetTime();
//@@
//@@}

void timer_normalize()
{
	DWORD new_time;

	new_time = timeGetTime();

	if (new_time < GameStartTime) {
		GameStartTime = new_time;
		return;
	}
	
	if ((new_time - GameStartTime) > 0x01808580)  	// 7 hours
		GameStartTime = new_time;
}


void timer_close();

void timer_init(int i)
{
	TIMECAPS tc;

	Assert(GameStartTime == 0);

	timer_initialized = 1;

	GameSystemTime = 0;
	GameStartTime = timeGetTime();

	atexit(timer_close);
}


void timer_close()
{
	Assert(GameStartTime > 0);

	timer_initialized = 0;

	GameSystemTime = 0;
	GameStartTime= 0;
}


fix timer_get_fixed_seconds()
{
	fix val;
	DWORD time;

	timer_normalize();

	time = timeGetTime() - GameStartTime;

	val = fixdiv(time,1000);

	return val;
}

fix timer_get_fixed_secondsX()
{
	fix val;
	DWORD time;

	timer_normalize();

	time = timeGetTime() - GameStartTime;

	val = fixdiv(time,1000);

	return val;
}

fix timer_get_approx_seconds()
{
	fix val;
	DWORD time;

	timer_normalize();

	time = timeGetTime() - GameStartTime;

	val = fixdiv(time,1000);

	return val;
}

void timer_set_function( void _far * function ) {}


void timer_delay(fix seconds)
{
	unsigned int numticks = timeGetTime() + (seconds>>16)*1000 + 
					(((seconds & 0x0000ffff)*1000)/65535);
	while (GetTickCount() < numticks);
}
