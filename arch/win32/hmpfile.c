/* This is HMP file playing code by Arne de Bruijn */
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include "hmpfile.h"

#if 0
#define CFILE FILE
#define cfopen fopen
#define cfseek fseek
#define cfread fread
#define cfclose fclose
#else
#include "cfile.h"
#endif

extern void PumpMessages(void);

hmp_file *hmp_open(const char *filename) {
	int i;
	char buf[256];
	long data;
	CFILE *fp;
	hmp_file *hmp;
	int num_tracks;
	unsigned char *p;

	if (!(fp = cfopen((char *)filename, "rb")))
		return NULL;

	hmp = malloc(sizeof(hmp_file));
	if (!hmp) {
		cfclose(fp);
		return NULL;
	}

	memset(hmp, 0, sizeof(*hmp));

	if ((cfread(buf, 1, 8, fp) != 8) || (memcmp(buf, "HMIMIDIP", 8)))
		goto err;

	if (cfseek(fp, 0x30, SEEK_SET))
		goto err;

	if (cfread(&num_tracks, 4, 1, fp) != 1)
		goto err;

	if ((num_tracks < 1) || (num_tracks > HMP_TRACKS))
		goto err;

	hmp->num_trks = num_tracks;
    hmp->tempo = 120;

	if (cfseek(fp, 0x308, SEEK_SET))
		goto err;

    for (i = 0; i < num_tracks; i++) {
		if ((cfseek(fp, 4, SEEK_CUR)) || (cfread(&data, 4, 1, fp) != 1))
			goto err;

		data -= 12;

#if 0
		if (i == 0)  /* track 0: reserve length for tempo */
		    data += sizeof(hmp_tempo);
#endif

		hmp->trks[i].len = data;

		if (!(p = hmp->trks[i].data = malloc(data)))
			goto err;

#if 0
		if (i == 0) { /* track 0: add tempo */
			memcpy(p, hmp_tempo, sizeof(hmp_tempo));
			p += sizeof(hmp_tempo);
			data -= sizeof(hmp_tempo);
		}
#endif
					     /* finally, read track data */
		if ((cfseek(fp, 4, SEEK_CUR)) || (cfread(p, data, 1, fp) != 1))
            goto err;
   }
   cfclose(fp);
   return hmp;

err:
   cfclose(fp);
   hmp_close(hmp);
   return NULL;
}

void hmp_stop(hmp_file *hmp) {
	MIDIHDR *mhdr;
	if (!hmp->stop) {
		hmp->stop = 1;
		//PumpMessages();
		midiStreamStop(hmp->hmidi);
                while (hmp->bufs_in_mm)
                 {
			//PumpMessages();
			Sleep(0);
                 }
	}
	while ((mhdr = hmp->evbuf)) {
		midiOutUnprepareHeader((HMIDIOUT)hmp->hmidi, mhdr, sizeof(MIDIHDR));
		hmp->evbuf = mhdr->lpNext;
		free(mhdr);
	}
	
	if (hmp->hmidi) {
		midiStreamClose(hmp->hmidi);
		hmp->hmidi = NULL;
	}
}

void hmp_close(hmp_file *hmp) {
	int i;

	hmp_stop(hmp);
	for (i = 0; i < hmp->num_trks; i++)
		if (hmp->trks[i].data)
			free(hmp->trks[i].data);
	free(hmp);
}

/*
 * read a HMI type variabele length number
 */
static int get_var_num_hmi(unsigned char *data, int datalen, unsigned long *value) {
	unsigned char *p;
	unsigned long v = 0;
	int shift = 0;

	p = data;
	while ((datalen > 0) && !(*p & 0x80)) {
		v += *(p++) << shift;
		shift += 7;
		datalen --;
    }
	if (!datalen)
		return 0;
    v += (*(p++) & 0x7f) << shift;
	if (value) *value = v;
    return p - data;
}

/*
 * read a MIDI type variabele length number
 */
static int get_var_num(unsigned char *data, int datalen,
 unsigned long *value) {
	unsigned char *orgdata = data;
	unsigned long v = 0;

	while ((datalen > 0) && (*data & 0x80))
		v = (v << 7) + (*(data++) & 0x7f);
	if (!datalen)
		return 0;
    v = (v << 7) + *(data++);
    if (value) *value = v;
    return data - orgdata;
}

static int get_event(hmp_file *hmp, event *ev) {
    static int cmdlen[7]={3,3,3,3,2,2,3};
	unsigned long got;
	unsigned long mindelta, delta;
	int i, ev_num;
	hmp_track *trk, *fndtrk;

	mindelta = INT_MAX;
	fndtrk = NULL;
	for (trk = hmp->trks, i = hmp->num_trks; (i--) > 0; trk++) {
		if (!trk->left)
			continue;
		if (!(got = get_var_num_hmi(trk->cur, trk->left, &delta)))
			return HMP_INVALID_FILE;
		if (trk->left > got + 2 && *(trk->cur + got) == 0xff
			&& *(trk->cur + got + 1) == 0x2f) {/* end of track */
			trk->left = 0;
			continue;
		}
        delta += trk->cur_time - hmp->cur_time;
		if (delta < mindelta) {
			mindelta = delta;
			fndtrk = trk;
		}
	}
	if (!(trk = fndtrk))
			return HMP_EOF;

	got = get_var_num_hmi(trk->cur, trk->left, &delta);

	trk->cur_time += delta;
	ev->delta = trk->cur_time - hmp->cur_time;
	hmp->cur_time = trk->cur_time;

	if ((trk->left -= got) < 3)
			return HMP_INVALID_FILE;
	trk->cur += got;
	/*memset(ev, 0, sizeof(*ev));*/ev->datalen = 0;
	ev->msg[0] = ev_num = *(trk->cur++);
	trk->left--;
	if (ev_num < 0x80)
	    return HMP_INVALID_FILE; /* invalid command */
	if (ev_num < 0xf0) {
		ev->msg[1] = *(trk->cur++);
		trk->left--;
		if (cmdlen[((ev_num) >> 4) - 8] == 3) {
			ev->msg[2] = *(trk->cur++);
			trk->left--;
		}
	} else if (ev_num == 0xff) {
		ev->msg[1] = *(trk->cur++);
		trk->left--;
		if (!(got = get_var_num(ev->data = trk->cur, 
			trk->left, (unsigned long *)&ev->datalen)))
			return HMP_INVALID_FILE;
	    trk->cur += ev->datalen;
		if (trk->left <= ev->datalen)
			return HMP_INVALID_FILE;
		trk->left -= ev->datalen;
	} else /* sysex -> error */
	    return HMP_INVALID_FILE;
	return 0;
}

static int fill_buffer(hmp_file *hmp) {
	MIDIHDR *mhdr = hmp->evbuf;
	unsigned int *p = (unsigned int *)(mhdr->lpData + mhdr->dwBytesRecorded);
	unsigned int *pend = (unsigned int *)(mhdr->lpData + mhdr->dwBufferLength);
	unsigned int i;
	event ev;

	while (p + 4 <= pend) {
		if (hmp->pending_size) {
			i = (p - pend) * 4;
			if (i > hmp->pending_size)
				i = hmp->pending_size;
			*(p++) = hmp->pending_event | i;
			*(p++) = 0;
			memcpy((unsigned char *)p, hmp->pending, i);
			hmp->pending_size -= i;
			p += (i + 3) / 4;
		} else {
			if ((i = get_event(hmp, &ev))) {
                                mhdr->dwBytesRecorded = ((unsigned char *)p) - ((unsigned char *)mhdr->lpData);
				return i;
			}
			if (ev.datalen) {
				hmp->pending_size = ev.datalen;
				hmp->pending = ev.data;
				hmp->pending_event = ev.msg[0] << 24;
			} else {
				*(p++) = ev.delta;
				*(p++) = 0;
				*(p++) = (((DWORD)MEVT_SHORTMSG) << 24) |
					((DWORD)ev.msg[0]) |
					(((DWORD)ev.msg[1]) << 8) |
					(((DWORD)ev.msg[2]) << 16);
			}
		}
	}
        mhdr->dwBytesRecorded = ((unsigned char *)p) - ((unsigned char *)mhdr->lpData);
	return 0;
}

static int setup_buffers(hmp_file *hmp) {
	int i;
	MIDIHDR *buf, *lastbuf;

	lastbuf = NULL;
	for (i = 0; i < HMP_BUFFERS; i++) {
		if (!(buf = malloc(HMP_BUFSIZE + sizeof(MIDIHDR))))
			return HMP_OUT_OF_MEM;
		memset(buf, 0, sizeof(MIDIHDR));
		buf->lpData = (unsigned char *)buf + sizeof(MIDIHDR);
		buf->dwBufferLength = HMP_BUFSIZE;
		buf->dwUser = (DWORD)hmp;
		buf->lpNext = lastbuf;
		lastbuf = buf;
	}
	hmp->evbuf = lastbuf;
	return 0;
}

static void reset_tracks(struct hmp_file *hmp) {
	int i;

	for (i = 0; i < hmp->num_trks; i++) {
		hmp->trks[i].cur = hmp->trks[i].data;
		hmp->trks[i].left = hmp->trks[i].len;
	}
}

static void _stdcall midi_callback(HMIDISTRM hms, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2) {
	MIDIHDR *mhdr;
	hmp_file *hmp;
	int rc;

	if (uMsg != MOM_DONE)
		return;
	
	mhdr = ((MIDIHDR *)dw1);
	mhdr->dwBytesRecorded = 0;
	hmp = (hmp_file *)(mhdr->dwUser);
	mhdr->lpNext = hmp->evbuf;
	hmp->evbuf = mhdr;
	hmp->bufs_in_mm--;

	if (!hmp->stop) {
		while (fill_buffer(hmp) == HMP_EOF)
			reset_tracks(hmp);
		if ((rc = midiStreamOut(hmp->hmidi, hmp->evbuf, 
			sizeof(MIDIHDR))) != MMSYSERR_NOERROR) {
			/* ??? */
		} else {
			hmp->evbuf = hmp->evbuf->lpNext;
			hmp->bufs_in_mm++;
		}
	}

}

static void setup_tempo(hmp_file *hmp, unsigned long tempo) {
	MIDIHDR *mhdr = hmp->evbuf;
	unsigned int *p = (unsigned int *)(mhdr->lpData + mhdr->dwBytesRecorded);
	*(p++) = 0;
	*(p++) = 0;
	*(p++) = (((DWORD)MEVT_TEMPO)<<24) | tempo;
	mhdr->dwBytesRecorded += 12;
}

int hmp_play(hmp_file *hmp) {
	int rc;
	MIDIPROPTIMEDIV mptd;
#if 0
        unsigned int    numdevs;
        int i=0;

        numdevs=midiOutGetNumDevs();
        hmp->devid=-1;
        do
        {
         MIDIOUTCAPS devcaps;
         midiOutGetDevCaps(i,&devcaps,sizeof(MIDIOUTCAPS));
         if ((devcaps.wTechnology==MOD_FMSYNTH) || (devcaps.wTechnology==MOD_SYNTH))
             hmp->devid=i;
         i++;
        } while ((i<(int)numdevs) && (hmp->devid==-1));
#else
	hmp->devid = MIDI_MAPPER;
#endif

	if ((rc = setup_buffers(hmp)))
		return rc;
	if ((midiStreamOpen(&hmp->hmidi, &hmp->devid,1,(DWORD)midi_callback,
	 0, CALLBACK_FUNCTION)) != MMSYSERR_NOERROR) {
		hmp->hmidi = NULL;
		return HMP_MM_ERR;
	}
	mptd.cbStruct  = sizeof(mptd);
	mptd.dwTimeDiv = hmp->tempo;
	if ((midiStreamProperty(hmp->hmidi,
         (LPBYTE)&mptd,
         MIDIPROP_SET|MIDIPROP_TIMEDIV)) != MMSYSERR_NOERROR) {
		/* FIXME: cleanup... */
		return HMP_MM_ERR;
	}

	reset_tracks(hmp);
	setup_tempo(hmp, 0x0f4240);

	hmp->stop = 0;
	while (hmp->evbuf) {
		if ((rc = fill_buffer(hmp))) {
			if (rc == HMP_EOF) {
				reset_tracks(hmp);
				continue;
			} else
				return rc;
		}
#if 0
		{  FILE *f = fopen("dump","wb"); fwrite(hmp->evbuf->lpData, 
 hmp->evbuf->dwBytesRecorded,1,f); fclose(f); exit(1);}
#endif
 		if ((rc = midiOutPrepareHeader((HMIDIOUT)hmp->hmidi, hmp->evbuf, 
			sizeof(MIDIHDR))) != MMSYSERR_NOERROR) {
			/* FIXME: cleanup... */
			return HMP_MM_ERR;
		}
		if ((rc = midiStreamOut(hmp->hmidi, hmp->evbuf, 
			sizeof(MIDIHDR))) != MMSYSERR_NOERROR) {
			/* FIXME: cleanup... */
			return HMP_MM_ERR;
		}
		hmp->evbuf = hmp->evbuf->lpNext;
		hmp->bufs_in_mm++;
	}
	midiStreamRestart(hmp->hmidi);
	return 0;
}

