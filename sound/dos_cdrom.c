/* DPH: This is the file where all the stub functions go. The aim is to have nothing in here ,eventually */
#include <conf.h>
#ifdef __ENV_DJGPP__
#include <stdio.h>
#include <stdlib.h>
#include "pstypes.h"
#include "error.h"
#include "args.h"


extern int Redbook_playing;
static int initialised = 0;

void RBAExit()
{
}

void RBAInit()
{

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
return 0; 
}

void RBAStop()
{
}

void RBASetVolume(int a)
{

}

void RBAPause()
{
}

void RBAResume()
{
}

int RBAGetNumberOfTracks()
{
return 0;
}

int RBAPlayTracks(int tracknum,int something)
{
return -1;
}

int RBAGetTrackNum()
{
return -1;
}

int RBAPeekPlayStatus()
{
 return -1;
}

int CD_blast_mixer()
{
 return 0;
}

#endif // __ENV_DJGPP__
