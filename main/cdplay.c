/* cdplay.c by Victor Rachels for better management of cd player */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __DJGPP__
#include "bcd.h"
//edited on 01/03/99 by Matt Mueller - should be ifdef'd for SDL, since SDL isn't linux specific
//I changed all the other ones too, but didn't feel like commenting them all
#elif defined __SDL__
//end edit -MM
#include <SDL/SDL.h>
#endif

#include "args.h"

#define MAX_TRACKS 20

int playlist[MAX_TRACKS];
int cd_currenttrack = 1;
int cd_playing = 0;
int cd_used = 0;
int tracks_used = MAX_TRACKS;
#ifdef __SDL__
SDL_CD *cdrom;
#endif
int nocdaudio=0;

int cd_playtrack(int trackno)
{
 int track;
//added on 01/03/99 by Matt Mueller
  if (nocdaudio) return 0;
//end addition -MM

   if(trackno > MAX_TRACKS || trackno < 0)
    return 0;
  track = playlist[trackno];

#ifdef __DJGPP__
   if(bcd_play_track(&track))
#elif defined __SDL__
   if(CD_INDRIVE(SDL_CDStatus(cdrom)) && !SDL_CDPlayTracks(cdrom,track,0,1,0))
#else
   if(0)
#endif
    cd_playing = 1;
   else
    cd_playing = 0;

  cd_used = 1;
  return cd_playing;
}

void cd_stop()
{
#ifdef __DJGPP__
  bcd_stop();
#elif defined __SDL__
  SDL_CDStop(cdrom);
#endif
  cd_playing = 0;
}

int cd_resume()
{
//added on 01/03/99 by Matt Mueller
  if (nocdaudio) return 0;
//end addition -MM

#ifdef __DJGPP__
  if(bcd_resume())
#elif defined __SDL__
  if(CD_INDRIVE(SDL_CDStatus(cdrom)) && !SDL_CDResume(cdrom))
#else
  if(0)
#endif
   cd_playing = 1;
  else
   cd_playing = 0;

  cd_used = 1;
  return cd_playing;
}

void cd_playprev()
{
//added on 01/03/99 by Matt Mueller
  if (nocdaudio) return;
//end addition -MM

  cd_currenttrack--;
   if(cd_currenttrack < 0)
    cd_currenttrack = tracks_used-1;
  cd_playtrack(cd_currenttrack);
  cd_used = 1;
}

void cd_playnext()
{
//added on 01/03/99 by Matt Mueller
  if (nocdaudio) return;
//end addition -MM

	cd_currenttrack++;
   if(cd_currenttrack >= tracks_used)
    cd_currenttrack = 0;
  cd_playtrack(cd_currenttrack);
  cd_used = 1;
}

void cd_playtoggle()
{
//added on 01/03/99 by Matt Mueller
  if (nocdaudio) return;
//end addition -MM

#ifdef __DJGPP__
  if(bcd_now_playing())
#elif defined __SDL__
  if(SDL_CDStatus(cdrom)==CD_PLAYING)
#else
  if(0)
#endif
   {
     cd_stop();
     cd_playing=0;
   }
  else if(!cd_used)
   {
     cd_currenttrack = 0;
     cd_playtrack(cd_currenttrack);
   }
  else
   {
     cd_resume();
   }
 cd_used = 1;
}

void cd_cycle()
{
//added on 01/03/99 by Matt Mueller
  if (nocdaudio) return;
//end addition -MM

#ifdef __DJGPP__
  if(cd_playing && !bcd_now_playing())
#elif defined __SDL__
  if(cd_playing && SDL_CDStatus(cdrom)!=CD_PLAYING)
#else
  if(cd_playing && 0)
#endif
   cd_playnext();
}

void cd_playlist_reset()
{
 int i;

//added on 01/03/99 by Matt Mueller
  if (nocdaudio) return;
//end addition -MM

#ifdef __DJGPP__
  tracks_used = bcd_get_audio_info();
#elif defined __SDL__
  if(SDL_CDStatus(cdrom)>-1)
   tracks_used = cdrom->numtracks;
  else
   tracks_used = 0;
#endif
   for(i=0;i<tracks_used;i++)
    playlist[i]=i+1;
   for(i=tracks_used;i<MAX_TRACKS;i++)
    playlist[i]=1;
}

void cd_playlist_set(char *newlist)
{
 int i=0;
 char *p=newlist;

//added on 01/03/99 by Matt Mueller
  if (nocdaudio) return;
//end addition -MM

  while((p==newlist || p-1) && i < MAX_TRACKS)
    {
     playlist[i++]=atoi(p);
     p=strchr(p,' ')+1;
    }
  tracks_used = i;
}

void cd_init()
{
 int i;
  if ((nocdaudio=FindArg("-nocdaudio"))) {
    if (Inferno_verbose) printf( "cd audio disabled.\n");
  } else {
#ifdef __DJGPP__
    bcd_open();
#elif defined __SDL__
    if( SDL_Init(SDL_INIT_CDROM) >= 0 )
     {
      atexit(SDL_Quit);
      cdrom=SDL_CDOpen(0);
     }
#endif
    cd_playlist_reset();
     if((i=FindArg("-cdplaylist")))
      cd_playlist_set(Args[i+1]);
  }
}
