#ifndef _SNDDECOM_H
#define _SNDDECOM_H

#ifdef __cplusplus
extern "C" {
#endif

void sound_decompress(unsigned char *data, int size, unsigned char *outp);

#ifdef __cplusplus
}
#endif

#endif
