#ifndef __HMP_H
#define __HMP_H
#if !(defined(__APPLE__) && defined(__MACH__))
#include <physfs.h>
#else
#include <physfs/physfs.h>
#endif

#ifdef WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

#define HMP_TRACKS 32
#ifdef WIN32
#define HMP_BUFFERS 4
#define HMP_BUFSIZE 1024
#define HMP_INVALID_FILE -1
#define HMP_OUT_OF_MEM -2
#define HMP_MM_ERR -3
#define HMP_EOF 1
#endif

#ifdef WIN32
typedef struct event {
	unsigned int delta;
	unsigned char msg[3];
	unsigned char *data;
	unsigned int datalen;
} event;
#endif

typedef struct hmp_track {
	unsigned char *data;
	unsigned int len;
	unsigned char *cur;
	unsigned int left;
	unsigned int cur_time;
} hmp_track;

typedef struct hmp_file {
	int num_trks;
	hmp_track trks[HMP_TRACKS];
	unsigned int cur_time;
	int tempo;
#ifdef WIN32
	MIDIHDR *evbuf;
	HMIDISTRM hmidi;
	UINT devid;
#endif
	unsigned char *pending;
	unsigned int pending_size;
	unsigned int pending_event;
	int stop;
	int bufs_in_mm;
	int bLoop;
	unsigned int midi_division;
} hmp_file;

hmp_file *hmp_open(const char *filename);
void hmp_close(hmp_file *hmp);
void hmp2mid(char *hmp_name, char *mid_name);
#ifdef WIN32
int hmp_play(hmp_file *hmp, int bLoop);
#endif

#endif
