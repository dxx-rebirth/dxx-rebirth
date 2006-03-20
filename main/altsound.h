/* altsound.h created on 11/13/99 by Victor Rachels to use alternate sounds */
#ifndef _ALTSOUND_H
#define _ALTSOUND_H

extern int use_alt_sounds;
extern int use_altsound[MAX_SOUNDS];
extern digi_sound altsound_list[MAX_SOUNDS];

void load_alt_sounds(char *fname);
void free_alt_sounds();

digi_sound *Sounddat(int soundnum);
int digi_xlat_sound(int soundnum);

#endif // _ALTSOUND_H
