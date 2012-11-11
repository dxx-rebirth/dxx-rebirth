#ifndef INCLUDED_MVE_AUDIO_H
#define INCLUDED_MVE_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

void mveaudio_uncompress(short *buffer, unsigned char *data, int length);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_MVE_AUDIO_H */
