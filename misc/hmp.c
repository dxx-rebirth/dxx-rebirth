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

#include "hmp.h"
#include "u_mem.h"
#include "console.h"
#include "timer.h"

#ifdef WORDS_BIGENDIAN
#define MIDIINT(x) (x)
#define MIDISHORT(x) (x)
#else
#define MIDIINT(x) SWAPINT(x)
#define MIDISHORT(x) SWAPSHORT(x)
#endif

#ifdef _WIN32
int midi_volume;
int channel_volume[16];
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
	int i, data, num_tracks, tempo;
	char buf[256];
	PHYSFS_file *fp;
	hmp_file *hmp;
	unsigned char *p;

	if (!(fp = PHYSFSX_openReadBuffered((char *)filename)))
		return NULL;

	CALLOC(hmp, hmp_file, 1);
	if (!hmp) {
		PHYSFS_close(fp);
		return NULL;
	}

	if ((PHYSFS_read(fp, buf, 1, 8) != 8) || (memcmp(buf, "HMIMIDIP", 8)))
	{
		PHYSFS_close(fp);
		hmp_close(hmp);
		return NULL;
	}

	if (PHYSFSX_fseek(fp, 0x30, SEEK_SET))
	{
		PHYSFS_close(fp);
		hmp_close(hmp);
		return NULL;
	}

	if (PHYSFS_read(fp, &num_tracks, 4, 1) != 1)
	{
		PHYSFS_close(fp);
		hmp_close(hmp);
		return NULL;
	}

	if ((num_tracks < 1) || (num_tracks > HMP_TRACKS))
	{
		PHYSFS_close(fp);
		hmp_close(hmp);
		return NULL;
	}
	hmp->num_trks = num_tracks;

	if (PHYSFSX_fseek(fp, 0x38, SEEK_SET))
	{
		PHYSFS_close(fp);
		hmp_close(hmp);
		return NULL;
	}
	if (PHYSFS_read(fp, &tempo, 4, 1) != 1)
	{
		PHYSFS_close(fp);
		hmp_close(hmp);
		return NULL;
	}
	hmp->tempo = INTEL_INT(tempo);

	if (PHYSFSX_fseek(fp, 0x308, SEEK_SET))
	{
		PHYSFS_close(fp);
		hmp_close(hmp);
		return NULL;
	}

	for (i = 0; i < num_tracks; i++) {
		if ((PHYSFSX_fseek(fp, 4, SEEK_CUR)) || (PHYSFS_read(fp, &data, 4, 1) != 1))
		{
			PHYSFS_close(fp);
			hmp_close(hmp);
			return NULL;
		}

		data -= 12;
		hmp->trks[i].len = data;

		MALLOC(p, unsigned char, data);
		if (!(hmp->trks[i].data = p))
		{
			PHYSFS_close(fp);
			hmp_close(hmp);
			return NULL;
		}

		/* finally, read track data */
		if ((PHYSFSX_fseek(fp, 4, SEEK_CUR)) || (PHYSFS_read(fp, p, data, 1) != 1))
		{
			PHYSFS_close(fp);
			hmp_close(hmp);
			return NULL;
		}
		hmp->trks[i].loop_set = 0;
	}
	hmp->filesize = PHYSFS_fileLength(fp);
	PHYSFS_close(fp);
	return hmp;
}

#ifdef _WIN32
// PLAY HMP AS MIDI

void hmp_stop(hmp_file *hmp)
{
	MIDIHDR *mhdr;
	if (!hmp->stop) {
		hmp->stop = 1;
		//PumpMessages();
		midiStreamStop(hmp->hmidi);
		while (hmp->bufs_in_mm)
			timer_delay(1);
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
 * read a HMI type variable length number
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
 * read a MIDI type variable length number
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
	static const int cmdlen[7]={3,3,3,3,2,2,3};
	unsigned long got;
	unsigned long mindelta, delta;
	int i, ev_num;
	hmp_track *trk, *fndtrk;

	mindelta = INT_MAX;
	fndtrk = NULL;
	for (trk = hmp->trks, i = hmp->num_trks; (i--) > 0; trk++) {
		if (!trk->left || (hmp->loop_start && hmp->looping && !trk->loop_set))
			continue;
		if (!(got = get_var_num_hmi(trk->cur, trk->left, &delta)))
			return HMP_INVALID_FILE;
		if (trk->left > got + 2 && *(trk->cur + got) == 0xff
			&& *(trk->cur + got + 1) == 0x2f) {/* end of track */
			trk->left = 0;
			continue;
		}

		if (hmp->loop_start && hmp->looping)
			if (trk->cur == trk->loop)
				delta = trk->loop_start - hmp->loop_start;

        delta += trk->cur_time - hmp->cur_time;
		if (delta < mindelta) {
			mindelta = delta;
			fndtrk = trk;
		}
	}
	if (!(trk = fndtrk))
			return HMP_EOF;

	got = get_var_num_hmi(trk->cur, trk->left, &delta);

	if (hmp->loop_start && hmp->looping)
		if (trk->cur == trk->loop)
			delta = trk->loop_start - hmp->loop_start;

	trk->cur_time += delta;

	if (!hmp->loop_start && *(trk->cur + got) >> 4 == MIDI_CONTROL_CHANGE && *(trk->cur + got + 1) == HMP_LOOP_START)
	{
	    hmp->loop_start = trk->cur_time;
	    if ((hmp->filesize == 86949) && (trk->cur_time == 29)) // special ugly HACK HACK HACK for Descent2-version of descent.hmp where loop at this point causes instruments not being reset properly. No track supporting HMP loop I know of meets the requirements for the condition below and even if so - it only disabled the HMP loop feature.
	    {
	        hmp->loop_start = 0;
	    }

	}

	if (!hmp->loop_end && *(trk->cur + got) >> 4 == MIDI_CONTROL_CHANGE && *(trk->cur + got + 1) == HMP_LOOP_END)
		hmp->loop_end = trk->cur_time;

	if (hmp->loop_start && !trk->loop_set && trk->cur_time >= hmp->loop_start)
	{
		trk->loop_start = trk->cur_time;
		trk->loop = trk->cur;
		trk->len = trk->left;
		trk->loop_set = 1;
	}

	if (hmp->loop_end && trk->cur_time > hmp->loop_end)
		return HMP_EOF;

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

			if (ev->msg[0] >> 4 == MIDI_CONTROL_CHANGE && ev->msg[1] == MIDI_VOLUME)
			{
				channel_volume[ev->msg[0] & 0xf] = ev->msg[2];

				ev->msg[2] = ev->msg[2] * midi_volume / MIDI_VOLUME_SCALE;
			}
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

	memset(&ev, 0, sizeof(event));

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
		buf->lpData = (char *)buf + sizeof(MIDIHDR);
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
		if (hmp->trks[i].loop_set)
			hmp->trks[i].cur = hmp->trks[i].loop;
		else
			hmp->trks[i].cur = hmp->trks[i].data;
		hmp->trks[i].left = hmp->trks[i].len;
		hmp->trks[i].cur_time = 0;
	}
	hmp->cur_time = 0;
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

			hmp->looping = 1;

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

void hmp_setvolume(hmp_file *hmp, int volume)
{
	int channel;

	if (hmp)
		for (channel = 0; channel < 16; channel++)
			midiOutShortMsg((HMIDIOUT)hmp->hmidi, (DWORD)(channel | MIDI_CONTROL_CHANGE << 4 | MIDI_VOLUME << 8 | (channel_volume[channel] * volume / MIDI_VOLUME_SCALE) << 16));

	midi_volume = volume;
}

int hmp_play(hmp_file *hmp, int bLoop)
{
	int rc;
	MIDIPROPTIMEDIV mptd;

	hmp->bLoop = bLoop;
	hmp->loop_start = 0;
	hmp->loop_end = 0;
	hmp->looping = 0;
	hmp->devid = MIDI_MAPPER;

	if ((rc = setup_buffers(hmp)))
		return rc;
	if ((midiStreamOpen(&hmp->hmidi, &hmp->devid,1, (DWORD_PTR) midi_callback, 0, CALLBACK_FUNCTION)) != MMSYSERR_NOERROR) {
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

void hmp_pause(hmp_file *hmp)
{
	if (hmp)
		midiStreamPause(hmp->hmidi);
}

void hmp_resume(hmp_file *hmp)
{
	if (hmp)
		midiStreamRestart(hmp->hmidi);
}

void hmp_reset()
{
	HMIDIOUT hmidi;
	MIDIHDR mhdr;
	MMRESULT rval;
	int channel;
	char GS_Reset[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };


	if ((rval = midiOutOpen(&hmidi, MIDI_MAPPER, 0, 0, 0)) != MMSYSERR_NOERROR)
	{
		switch (rval)
		{
			case MIDIERR_NODEVICE:
				con_printf(CON_DEBUG, "midiOutOpen Error: no MIDI port was found.\n");
				break;
			case MMSYSERR_ALLOCATED:
				con_printf(CON_DEBUG, "midiOutOpen Error: specified resource is already allocated.\n");
				break;
			case MMSYSERR_BADDEVICEID:
				con_printf(CON_DEBUG, "midiOutOpen Error: specified device identifier is out of range.\n");
				break;
			case MMSYSERR_INVALPARAM:
				con_printf(CON_DEBUG, "midiOutOpen Error: specified pointer or structure is invalid.\n");
				break;
			case MMSYSERR_NOMEM:
				con_printf(CON_DEBUG, "midiOutOpen Error: unable to allocate or lock memory.\n");
				break;
			default:
				con_printf(CON_DEBUG, "midiOutOpen Error code %i\n",rval);
				break;
		}
		return;
	}

	for (channel = 0; channel < 16; channel++)
	{
		midiOutShortMsg(hmidi, (DWORD)(channel | MIDI_CONTROL_CHANGE << 4 | MIDI_ALL_SOUNDS_OFF << 8 | 0 << 16));
		midiOutShortMsg(hmidi, (DWORD)(channel | MIDI_CONTROL_CHANGE << 4 | MIDI_RESET_ALL_CONTROLLERS << 8 | 0 << 16));
		midiOutShortMsg(hmidi, (DWORD)(channel | MIDI_CONTROL_CHANGE << 4 | MIDI_ALL_NOTES_OFF << 8 | 0 << 16));

		channel_volume[channel] = 100;
		midiOutShortMsg(hmidi, (DWORD)(channel | MIDI_CONTROL_CHANGE << 4 | MIDI_PANPOT << 8 | 64 << 16));
		midiOutShortMsg(hmidi, (DWORD)(channel | MIDI_CONTROL_CHANGE << 4 | MIDI_REVERB << 8 | 40 << 16));
		midiOutShortMsg(hmidi, (DWORD)(channel | MIDI_CONTROL_CHANGE << 4 | MIDI_CHORUS << 8 | 0 << 16));

		midiOutShortMsg(hmidi, (DWORD)(channel | MIDI_CONTROL_CHANGE << 4 | MIDI_BANK_SELECT_MSB << 8 | 0 << 16));
		midiOutShortMsg(hmidi, (DWORD)(channel | MIDI_CONTROL_CHANGE << 4 | MIDI_BANK_SELECT_LSB << 8 | 0 << 16));
		midiOutShortMsg(hmidi, (DWORD)(channel | MIDI_PROGRAM_CHANGE << 4 | 0 << 8));
	}

	mhdr.lpData = GS_Reset;
	mhdr.dwBufferLength = sizeof(GS_Reset);
	mhdr.dwFlags = 0;
	if ((rval = midiOutPrepareHeader(hmidi, &mhdr, sizeof(MIDIHDR))) == MMSYSERR_NOERROR)
	{
		if ((rval = midiOutLongMsg(hmidi, &mhdr, sizeof(MIDIHDR))) == MMSYSERR_NOERROR)
		{
			fix64 wait_done = timer_query();
			while (!(mhdr.dwFlags & MHDR_DONE))
			{
				timer_update();
				if (timer_query() >= wait_done + F1_0)
				{
					con_printf(CON_DEBUG, "hmp_reset: Timeout waiting for MHDR_DONE\n");
					break;
				}
			}
		}
		else
		{
			switch (rval)
			{
				case MIDIERR_NOTREADY:
					con_printf(CON_DEBUG, "midiOutLongMsg Error: the hardware is busy with other data.\n");
					break;
				case MIDIERR_UNPREPARED:
					con_printf(CON_DEBUG, "midiOutLongMsg Error: the buffer pointed to by lpMidiOutHdr has not been prepared.\n");
					break;
				case MMSYSERR_INVALHANDLE:
					con_printf(CON_DEBUG, "midiOutLongMsg Error: the specified device handle is invalid.\n");
					break;
				case MMSYSERR_INVALPARAM:
					con_printf(CON_DEBUG, "midiOutLongMsg Error: the specified pointer or structure is invalid.\n");
					break;
				default:
					con_printf(CON_DEBUG, "midiOutLongMsg Error code %i\n",rval);
					break;
			}
		}
		midiOutUnprepareHeader(hmidi, &mhdr, sizeof(MIDIHDR));

		timer_delay(F1_0/20);
	}
	else
	{
		switch (rval)
		{
			case MMSYSERR_INVALHANDLE:
				con_printf(CON_DEBUG, "midiOutPrepareHeader Error: The specified device handle is invalid.\n");
				break;
			case MMSYSERR_INVALPARAM:
				con_printf(CON_DEBUG, "midiOutPrepareHeader Error: The specified address is invalid or the given stream buffer is greater than 64K.\n");
				break;
			case MMSYSERR_NOMEM:
				con_printf(CON_DEBUG, "midiOutPrepareHeader Error: The system is unable to allocate or lock memory.\n");
				break;
			default:
				con_printf(CON_DEBUG, "midiOutPrepareHeader Error code %i\n",rval);
				break;
		}
	}

	for (channel = 0; channel < 16; channel++)
		midiOutShortMsg(hmidi, (DWORD)(channel | MIDI_CONTROL_CHANGE << 4 | MIDI_VOLUME << 8 | (100 * midi_volume / MIDI_VOLUME_SCALE) << 16));
	midiOutClose(hmidi);
}
#endif

// CONVERSION FROM HMP TO MIDI

static unsigned int hmptrk2mid(ubyte* data, int size, unsigned char **midbuf, unsigned int *midlen)
{
	ubyte *dptr = data;
	ubyte lc1 = 0,last_com = 0;
	uint d;
	int n1, n2;
	unsigned int offset = *midlen;

	while (data < dptr + size)
	{
		if (data[0] & 0x80) {
			ubyte b = (data[0] & 0x7F);
			*midbuf = (unsigned char *) d_realloc(*midbuf, *midlen + 1);
			memcpy(&(*midbuf)[*midlen], &b, 1);
			*midlen += 1;
		}
		else {
			d = (data[0] & 0x7F);
			n1 = 0;
			while ((data[n1] & 0x80) == 0) {
				n1++;
				d += (data[n1] & 0x7F) << (n1 * 7);
				}
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
				*midbuf = (unsigned char *) d_realloc(*midbuf, *midlen + 1);
				memcpy(&(*midbuf)[*midlen], &b, 1);
				*midlen += 1;
				}
			data += n1;
		}
		data++;
		if (*data == 0xFF) { //meta?
			*midbuf = (unsigned char *) d_realloc(*midbuf, *midlen + 3 + data[2]);
			memcpy(&(*midbuf)[*midlen], data, 3 + data[2]);
			*midlen += 3 + data[2];
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
					{
						*midbuf = (unsigned char *) d_realloc(*midbuf, *midlen + 1);
						memcpy(&(*midbuf)[*midlen], &lc1, 1);
						*midlen += 1;
					}
					*midbuf = (unsigned char *) d_realloc(*midbuf, *midlen + 2);
					memcpy(&(*midbuf)[*midlen], data + 1, 2);
					*midlen += 2;
					data += 3;
					break;
				case 0xC0:
				case 0xD0:
					if (lc1 != last_com)
					{
						*midbuf = (unsigned char *) d_realloc(*midbuf, *midlen + 1);
						memcpy(&(*midbuf)[*midlen], &lc1, 1);
						*midlen += 1;
					}
					*midbuf = (unsigned char *) d_realloc(*midbuf, *midlen + 1);
					memcpy(&(*midbuf)[*midlen], data + 1, 1);
					*midlen += 1;
					data += 2;
					break;
				default:
					return 0;
				}
			last_com = lc1;
		}
	}
	return (*midlen - offset);
}

ubyte tempo [19] = {'M','T','r','k',0,0,0,11,0,0xFF,0x51,0x03,0x18,0x80,0x00,0,0xFF,0x2F,0};

void hmp2mid(char *hmp_name, unsigned char **midbuf, unsigned int *midlen)
{
	int mi, i;
	short ms, time_div = 0xC0;
	hmp_file *hmp=NULL;

	hmp = hmp_open(hmp_name);
	if (hmp == NULL)
		return;

	*midlen = 0;
	time_div = hmp->tempo*1.6;

	// write MIDI-header
	*midbuf = (unsigned char *) d_realloc(*midbuf, *midlen + 4);
	memcpy(&(*midbuf)[*midlen], "MThd", 4);
	*midlen += 4;
	mi = MIDIINT(6);
	*midbuf = (unsigned char *) d_realloc(*midbuf, *midlen + sizeof(mi));
	memcpy(&(*midbuf)[*midlen], &mi, sizeof(mi));
	*midlen += sizeof(mi);
	ms = MIDISHORT(1);
	*midbuf = (unsigned char *) d_realloc(*midbuf, *midlen + sizeof(ms));
	memcpy(&(*midbuf)[*midlen], &ms, sizeof(ms));
	*midlen += sizeof(ms);
	ms = MIDISHORT(hmp->num_trks);
	*midbuf = (unsigned char *) d_realloc(*midbuf, *midlen + sizeof(ms));
	memcpy(&(*midbuf)[*midlen], &ms, sizeof(ms));
	*midlen += sizeof(ms);
	ms = MIDISHORT(time_div);
	*midbuf = (unsigned char *) d_realloc(*midbuf, *midlen + sizeof(ms));
	memcpy(&(*midbuf)[*midlen], &ms, sizeof(ms));
	*midlen += sizeof(ms);
	*midbuf = (unsigned char *) d_realloc(*midbuf, *midlen + sizeof(tempo));
	memcpy(&(*midbuf)[*midlen], &tempo, sizeof(tempo));
	*midlen += sizeof(tempo);

	// tracks
	for (i = 1; i < hmp->num_trks; i++)
	{
		int midtrklenpos = 0;

		*midbuf = (unsigned char *) d_realloc(*midbuf, *midlen + 4);
		memcpy(&(*midbuf)[*midlen], "MTrk", 4);
		*midlen += 4;
		midtrklenpos = *midlen;
		mi = 0;
		*midbuf = (unsigned char *) d_realloc(*midbuf, *midlen + sizeof(mi));
		*midlen += sizeof(mi);
		mi = hmptrk2mid(hmp->trks[i].data, hmp->trks[i].len, midbuf, midlen);
		mi = MIDIINT(mi);
		memcpy(&(*midbuf)[midtrklenpos], &mi, 4);
	}

	hmp_close(hmp);
}
