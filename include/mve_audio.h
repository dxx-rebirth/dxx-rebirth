#ifndef INCLUDED_MVE_AUDIO_H
#define INCLUDED_MVE_AUDIO_H

#define MVE_AUDIO_FLAGS_STEREO     1
#define MVE_AUDIO_FLAGS_16BIT      2
#define MVE_AUDIO_FLAGS_COMPRESSED 4

void mveaudio_uncompress(short *buffer, unsigned char *data, int length);

#endif /* INCLUDED_MVE_AUDIO_H */
