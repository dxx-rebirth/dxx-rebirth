/*
 * This code handles HMP files. It can:
 * - Open/read/close HMP files
 * - Play HMP via Windows MIDI
 * - Convert HMP to MIDI for further use
 * Based on work of Arne de Bruijn and the JFFEE project
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#if !(defined(__APPLE__) && defined(__MACH__))
#include <physfs.h>
#else
#include <physfs/physfs.h>
#endif
#include "hmp.h"
#include "u_mem.h"
#include "cfile.h"

#ifdef WORDS_BIGENDIAN
#define MIDIINT(x) (x)
#define MIDISHORT(x) (x)
#else
#define MIDIINT(x) SWAPINT(x)
#define MIDISHORT(x) SWAPSHORT(x)
#endif

#ifdef _WIN32
void hmp_stop(hmp_file *hmp);
#endif

// READ/OPEN/CLOSE HMP

void hmp_close(hmp_file *hmp)
{
	int i;

#ifdef _WIN32
	hmp_stop(hmp);
#endif
	for (i = 0; i < hmp->num_trks; i++)
		if (hmp->trks[i].data)
			d_free(hmp->trks[i].data);
	d_free(hmp);
}

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

	hmp = d_malloc(sizeof(hmp_file));
	if (!hmp) {
		cfclose(fp);
		return NULL;
	}

	memset(hmp, 0, sizeof(*hmp));

	if ((cfread(buf, 1, 8, fp) != 8) || (memcmp(buf, "HMIMIDIP", 8)))
	{
		cfclose(fp);
		hmp_close(hmp);
		return NULL;
	}

	if (cfseek(fp, 0x30, SEEK_SET))
	{
		cfclose(fp);
		hmp_close(hmp);
		return NULL;
	}

	if (cfread(&num_tracks, 4, 1, fp) != 1)
	{
		cfclose(fp);
		hmp_close(hmp);
		return NULL;
	}

	if ((num_tracks < 1) || (num_tracks > HMP_TRACKS))
	{
		cfclose(fp);
		hmp_close(hmp);
		return NULL;
	}

	hmp->num_trks = num_tracks;
	hmp->tempo = 120;

	if (cfseek(fp, 0x308, SEEK_SET))
	{
		cfclose(fp);
		hmp_close(hmp);
		return NULL;
	}

	for (i = 0; i < num_tracks; i++) {
		if ((cfseek(fp, 4, SEEK_CUR)) || (cfread(&data, 4, 1, fp) != 1))
		{
			cfclose(fp);
			hmp_close(hmp);
			return NULL;
		}

		data -= 12;
		hmp->trks[i].len = data;

		if (!(p = hmp->trks[i].data = d_malloc(data)))
		{
			cfclose(fp);
			hmp_close(hmp);
			return NULL;
		}

		/* finally, read track data */
		if ((cfseek(fp, 4, SEEK_CUR)) || (cfread(p, data, 1, fp) != 1))
		{
			cfclose(fp);
			hmp_close(hmp);
			return NULL;
		}
	}
	cfclose(fp);
	return hmp;
}

#ifdef WIN32
// PLAY HMP AS MIDI

void hmp_stop(hmp_file *hmp)
{
	MIDIHDR *mhdr;
	if (!hmp->stop) {
		hmp->stop = 1;
		//PumpMessages();
		midiStreamStop(hmp->hmidi);
		while (hmp->bufs_in_mm)
			Sleep(0);
	}
	while ((mhdr = hmp->evbuf)) {
		midiOutUnprepareHeader((HMIDIOUT)hmp->hmidi, mhdr, sizeof(MIDIHDR));
		hmp->evbuf = mhdr->lpNext;
		d_free(mhdr);
	}

	if (hmp->hmidi) {
		midiStreamClose(hmp->hmidi);
		hmp->hmidi = NULL;
	}
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
		if (!(buf = d_malloc(HMP_BUFSIZE + sizeof(MIDIHDR))))
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

static void reset_tracks(struct hmp_file *hmp)
{
	int i;

	for (i = 0; i < hmp->num_trks; i++) {
		hmp->trks [i].cur = hmp->trks [i].data;
		hmp->trks [i].left = hmp->trks [i].len;
		hmp->trks [i].cur_time = 0;
	}
	hmp->cur_time=0;
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
		while (fill_buffer(hmp) == HMP_EOF) {
			if (hmp->bLoop)
				hmp->stop = 0;
			else
				hmp->stop = 1;

			reset_tracks(hmp);
		}
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

int hmp_play(hmp_file *hmp, int bLoop)
{
	int rc;
	MIDIPROPTIMEDIV mptd;
#if 1
	unsigned int    numdevs;
	int i=0; // ZICO - LOOP HERE

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
	if (hmp->devid == -1)
#endif
		hmp->bLoop = bLoop;
	hmp->devid = MIDI_MAPPER;

	if ((rc = setup_buffers(hmp)))
		return rc;
	if ((midiStreamOpen(&hmp->hmidi, &hmp->devid,1, (DWORD) (size_t) midi_callback, 0, CALLBACK_FUNCTION)) != MMSYSERR_NOERROR) {
		hmp->hmidi = NULL;
		return HMP_MM_ERR;
	}
	mptd.cbStruct  = sizeof(mptd);
	mptd.dwTimeDiv = hmp->tempo;
	if ((midiStreamProperty(hmp->hmidi,(LPBYTE)&mptd,MIDIPROP_SET|MIDIPROP_TIMEDIV)) != MMSYSERR_NOERROR) {
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
 		if ((rc = midiOutPrepareHeader((HMIDIOUT)hmp->hmidi, hmp->evbuf,
			sizeof(MIDIHDR))) != MMSYSERR_NOERROR) {
			return HMP_MM_ERR;
		}
		if ((rc = midiStreamOut(hmp->hmidi, hmp->evbuf,
			sizeof(MIDIHDR))) != MMSYSERR_NOERROR) {
			return HMP_MM_ERR;
		}
		hmp->evbuf = hmp->evbuf->lpNext;
		hmp->bufs_in_mm++;
	}
	midiStreamRestart(hmp->hmidi);
	return 0;
}
#endif

// CONVERSION FROM HMP TO MIDI

static int hmptrk2mid(ubyte* data, int size, PHYSFS_file *mid)
{
	ubyte *dptr = data;
	ubyte lc1 = 0,last_com = 0;
	uint t = 0, d;
	int n1, n2;
	int offset = cftell(mid);

	while (data < dptr + size)
	{
		if (data[0] & 0x80) {
			ubyte b = (data[0] & 0x7F);
			PHYSFS_write(mid, &b, sizeof (b), 1);
			t+=b;
		}
		else {
			d = (data[0] & 0x7F);
			n1 = 0;
			while ((data[n1] & 0x80) == 0) {
				n1++;
				d += (data[n1] & 0x7F) << (n1 * 7);
				}
			t += d;
			n1 = 1;
			while ((data[n1] & 0x80) == 0) {
				n1++;
				if (n1 == 4)
					return 0;
				}
			for(n2 = 0; n2 <= n1; n2++) {
				ubyte b = (data[n1 - n2] & 0x7F);

				if (n2 != n1)
					b |= 0x80;
				PHYSFS_write(mid, &b, sizeof(b), 1);
				}
			data += n1;
		}
		data++;
		if (*data == 0xFF) { //meta?
			PHYSFS_write(mid, data, 3 + data [2], 1);
			if (data[1] == 0x2F)
				break;
		}
		else {
			lc1=data[0] ;
			if ((lc1&0x80) == 0)
				return 0;
			switch (lc1 & 0xF0) {
				case 0x80:
				case 0x90:
				case 0xA0:
				case 0xB0:
				case 0xE0:
					if (lc1 != last_com)
						PHYSFS_write(mid, &lc1, sizeof (lc1), 1);
					PHYSFS_write(mid, data + 1, 2, 1);
					data += 3;
					break;
				case 0xC0:
				case 0xD0:
					if (lc1 != last_com)
						PHYSFS_write(mid, &lc1, sizeof (lc1), 1);
					PHYSFS_write(mid, data + 1, 1, 1);
					data += 2;
					break;
				default:
					return 0;
				}
			last_com = lc1;
		}
	}
	return (cftell(mid) - offset);
}

ubyte tempo [19] = {'M','T','r','k',0,0,0,11,0,0xFF,0x51,0x03,0x18,0x80,0x00,0,0xFF,0x2F,0};

void hmp2mid(char *hmp_name, char *mid_name)
{
	PHYSFS_file *mid=NULL;
	int mi, i, loc;
	short ms;
	hmp_file *hmp=NULL;

	hmp = hmp_open(hmp_name);
	if (hmp == NULL)
		return;
	mid = PHYSFSX_openWriteBuffered(mid_name);
	if (mid == NULL)
	{
		hmp_close(hmp);
		return;
	// write MIDI-header
	PHYSFS_write(mid, "MThd", 4, 1);
	mi = MIDIINT(6);
	PHYSFS_write(mid, &mi, sizeof(mi), 1);
	ms = MIDISHORT(1);
	PHYSFS_write(mid, &ms, sizeof(mi), 1);
	ms = MIDISHORT(hmp->num_trks);
	PHYSFS_write(mid, &ms, sizeof(ms), 1);
	ms = MIDISHORT((short) 0xC0);
	PHYSFS_write(mid, &ms, sizeof(ms), 1);
	PHYSFS_write(mid, tempo, sizeof(tempo), 1);

	// tracks
	for (i = 1; i < hmp->num_trks; i++)
	{
		PHYSFS_write(mid, "MTrk", 4, 1);
		loc = cftell(mid);
		mi = 0;
		PHYSFS_write(mid, &mi, sizeof(mi), 1);
		mi = hmptrk2mid(hmp->trks[i].data, hmp->trks[i].len, mid);
		mi = MIDIINT(mi);
		cfseek(mid, loc, SEEK_SET);
		PHYSFS_write(mid, &mi, sizeof(mi), 1);
		cfseek(mid, 0, SEEK_END);
	}

	hmp_close(hmp);
	cfclose(mid);
}
