#ifndef _HMPFILE_H
#define _HMPFILE_H
#include <windows.h>
#include <mmsystem.h>

#define HMP_TRACKS 32
#define HMP_BUFFERS 4
#define HMP_BUFSIZE 1024

typedef struct event {
	unsigned int delta;
	unsigned char msg[3];
	unsigned char *data;
	unsigned int datalen;
} event;

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
	MIDIHDR *evbuf;
    HMIDISTRM hmidi;
	UINT devid;
	unsigned char *pending;
	unsigned int pending_size;
	unsigned int pending_event;
	int stop;		/* 1 -> don't send more data */
	int bufs_in_mm;	/* number of queued buffers */
} hmp_file;


#define HMP_INVALID_FILE -1
#define HMP_OUT_OF_MEM -2
#define HMP_MM_ERR -3
#define HMP_EOF 1

hmp_file *hmp_open(const char *filename);
int hmp_play(hmp_file *hmp);
void hmp_close(hmp_file *hmp);

#endif
