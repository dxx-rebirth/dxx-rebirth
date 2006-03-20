/* altsound.c created on 11/13/99 by Victor Rachels to use alternate sounds */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "strio.h"
#include "types.h"
#include "d_io.h"
#include "u_mem.h"
#include "error.h"
#include "sounds.h"
#include "digi.h"
#include "altsound.h"


int use_alt_sounds=0;
digi_sound altsound_list[MAX_SOUNDS];
int use_altsound[MAX_SOUNDS];

int digi_xlat_sound(int soundnum)
{
   if ( soundnum < 0 )
    return -1;

   if ( digi_lomem )
    {
     int i;
      i = AltSounds[soundnum];
       if ( i == 255 )
        return -1;
       else
        return i;
    }
   else
    return Sounds[soundnum];
}

digi_sound *Sounddat(int soundnum)
{
  if(use_altsound[soundnum] && altsound_list[soundnum].data)
   return &altsound_list[soundnum];
  else
   {
    int i;
     i = digi_xlat_sound(soundnum);
      if (i < 0)
       return NULL;
      else
       return &(GameSounds[i]);
   }
}

void load_alt_sounds(char *soundfile_list)
{
 FILE *list,*sound;
 char *line,word[256];
 int i,blockalign=0,datsize=0,bits=0,freq=0;

  memset(&use_altsound,0,sizeof(int) * MAX_SOUNDS);
  memset(&altsound_list,0,sizeof(digi_sound) * MAX_SOUNDS);

  list=fopen(soundfile_list,"rt");

   if(!list) return;

   while(!feof(list))
    {
      line=fsplitword(list,'\n');
       if((line) &&
          (line[0]!='#') &&
          (sscanf(line,"%i=%s\n",&i,word)==2) &&
          (i >= 0) &&
          (i <= MAX_SOUNDS) )
        {
          sound=fopen(word,"rb");
           if(sound)
            {
              fseek(sound,40,SEEK_SET);
              fread(&(datsize),sizeof(u_int32_t),1,sound);

              altsound_list[i].data = (void *)malloc(datsize);

               if(altsound_list[i].data)
                {
                  fseek(sound,34,SEEK_SET);
                  fread(&(bits),sizeof(u_int16_t),1,sound);

                  fseek(sound,24,SEEK_SET);
                  fread(&(freq),sizeof(u_int32_t),1,sound);

                  fseek(sound,32,SEEK_SET);
                  fread(&(blockalign),sizeof(u_int16_t),1,sound);

                  fseek(sound,44,SEEK_SET);
                  fread(altsound_list[i].data,datsize,1,sound);

                   if(!ferror(sound))
                    {
                      altsound_list[i].bits=bits;
                      altsound_list[i].freq=freq;
#ifdef ALLEGRO
                      altsound_list[i].len=datsize/blockalign;
                      altsound_list[i].loop_start=0;
                      altsound_list[i].loop_end = altsound_list[i].len;
                      altsound_list[i].priority=128;
                      altsound_list[i].param=-1;
#else
                      altsound_list[i].length=datsize;
#endif
                      use_alt_sounds++;
                      use_altsound[i]=1;
                    }
                }
              fclose(sound);
            }
          free(line);
        }
    }
  fclose(list);
}

void free_alt_sounds()
{
 int i;
   for(i=0;i<MAX_SOUNDS;i++)
    if(use_altsound[i] && altsound_list[i].data)
     {
       free(altsound_list[i].data);
       use_altsound[i]=0;
       use_alt_sounds--;
     }

   for(i=0;i<MAX_SOUNDS;i++)
    if(use_altsound[i] || altsound_list[i].data || use_alt_sounds)
     Error("Something screwed in freeing altsounds (num:%i)\n",i);
}
