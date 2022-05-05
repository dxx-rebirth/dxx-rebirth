/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 * This code handles HMP files. It can:
 * - Open/read/close HMP files
 * - Play HMP via Windows MIDI
 * - Convert HMP to MIDI for further use
 * Based on work of Arne de Bruijn and the JFFEE project
 */
#include <stdexcept>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "hmp.h"
#include "u_mem.h"
#include "console.h"
#include "timer.h"
#include "serial.h"

#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-range_for.h"
#include "d_range.h"
#include "d_underlying_value.h"
#include "partial_range.h"
#include <memory>

namespace dcx {

#define MIDIINT(x)	(words_bigendian ? (x) : (SWAPINT(x)))
#define MIDISHORT(x)	(words_bigendian ? (x) : (SWAPSHORT(x)))

#ifdef _WIN32
static int midi_volume;
static int channel_volume[16];
static void hmp_stop(hmp_file *hmp);
#endif

// READ/OPEN/CLOSE HMP

hmp_file::~hmp_file()
{
#ifdef _WIN32
	hmp_stop(this);
#endif
}

hmp_open_result hmp_open(const char *const filename)
{
	int tempo;
	const auto fpe = PHYSFSX_openReadBuffered(filename);

	if (!fpe.first)
		return {nullptr, hmp_open_error::physfs_open, fpe.second};
	auto &fp = fpe.first;

	char buf[8];
	const auto physfs_error = [](hmp_open_error e) {
		return hmp_open_result{nullptr, e, PHYSFS_getLastErrorCode()};
	};
	const auto data_error = [](hmp_open_error e) {
		return hmp_open_result{nullptr, e, PHYSFS_ErrorCode{}};
	};
	if (PHYSFS_read(fp, buf, 1, 8) != 8)
		return physfs_error(hmp_open_error::physfs_read_header);
	if (memcmp(buf, "HMIMIDIP", 8))
		return data_error(hmp_open_error::content_header);

	if (!PHYSFS_seek(fp, 0x30))
		return physfs_error(hmp_open_error::physfs_seek0);

	unsigned num_tracks;
	if (PHYSFS_read(fp, &num_tracks, 4, 1) != 1)
		return physfs_error(hmp_open_error::physfs_read_tracks);

	if ((num_tracks < 1) || (num_tracks > HMP_TRACKS))
		return data_error(hmp_open_error::track_count);
	std::unique_ptr<hmp_file> hmp(new hmp_file{});
	hmp->num_trks = num_tracks;

	if (!PHYSFS_seek(fp, 0x38))
		return physfs_error(hmp_open_error::physfs_seek1);
	if (PHYSFS_read(fp, &tempo, 4, 1) != 1)
		return physfs_error(hmp_open_error::physfs_read_tempo);
	hmp->tempo = INTEL_INT(tempo);

	if (!PHYSFS_seek(fp, 0x308))
		return physfs_error(hmp_open_error::physfs_seek2);

	range_for (auto &i, partial_range(hmp->trks, num_tracks))
	{
		std::array<int, 3> tdata;
		/* Discard tdata[0] and tdata[2].  They are read to avoid needing a
		 * seek operation to skip them.
		 */
		if (PHYSFS_read(fp, tdata.data(), sizeof(tdata[0]), tdata.size()) != tdata.size())
			return physfs_error(hmp_open_error::physfs_read_track_length);

		int data = tdata[1];
		data -= 12;
		i.len = data;
		i.data = std::make_unique<uint8_t[]>(data);
		/* finally, read track data */
		if (PHYSFS_read(fp, i.data.get(), data, 1) != 1)
			return physfs_error(hmp_open_error::physfs_read_track_data);
		i.loop_set = 0;
	}
	hmp->filesize = PHYSFS_fileLength(fp);
	return {std::move(hmp), hmp_open_error::None, PHYSFS_ErrorCode{}};
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
		midiOutUnprepareHeader(reinterpret_cast<HMIDIOUT>(hmp->hmidi), mhdr, sizeof(MIDIHDR));
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
	unsigned *value) {
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
	static const std::array<int, 7> cmdlen{{3,3,3,3,2,2,3}};
	unsigned long got;
	unsigned long mindelta, delta;
	int ev_num;
	hmp_track *fndtrk = nullptr;

	mindelta = INT_MAX;
	range_for (auto &rtrk, partial_range(hmp->trks, static_cast<unsigned>(hmp->num_trks)))
	{
		const auto trk = &rtrk;
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
	const auto trk = fndtrk;
	if (!trk)
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
			trk->left, &ev->datalen)))
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
	unsigned int *p = reinterpret_cast<unsigned int *>(mhdr->lpData + mhdr->dwBytesRecorded);
	unsigned int *pend = reinterpret_cast<unsigned int *>(mhdr->lpData + mhdr->dwBufferLength);
	unsigned int i;
	event ev{};

	while (p + 4 <= pend) {
		if (hmp->pending_size) {
			i = (p - pend) * 4;
			if (i > hmp->pending_size)
				i = hmp->pending_size;
			*(p++) = hmp->pending_event | i;
			*(p++) = 0;
			memcpy(reinterpret_cast<uint8_t *>(p), hmp->pending, i);
			hmp->pending_size -= i;
			p += (i + 3) / 4;
		} else {
			if ((i = get_event(hmp, &ev))) {
                                mhdr->dwBytesRecorded = (reinterpret_cast<uint8_t *>(p)) - (reinterpret_cast<uint8_t *>(mhdr->lpData));
				return i;
			}
			if (ev.datalen) {
				hmp->pending_size = ev.datalen;
				hmp->pending = ev.data;
				hmp->pending_event = ev.msg[0] << 24;
			} else {
				*(p++) = ev.delta;
				*(p++) = 0;
				*(p++) = (static_cast<DWORD>(MEVT_SHORTMSG) << 24) |
					static_cast<DWORD>(ev.msg[0]) |
					(static_cast<DWORD>(ev.msg[1]) << 8) |
					(static_cast<DWORD>(ev.msg[2]) << 16);
			}
		}
	}
        mhdr->dwBytesRecorded = (reinterpret_cast<uint8_t *>(p)) - (reinterpret_cast<uint8_t *>(mhdr->lpData));
	return 0;
}

static int setup_buffers(hmp_file *hmp) {
	MIDIHDR *buf, *lastbuf;

	lastbuf = NULL;
	for (int i = 0; i < HMP_BUFFERS; i++) {
		if (!(buf = reinterpret_cast<MIDIHDR *>(d_malloc(HMP_BUFSIZE + sizeof(MIDIHDR)))))
			return HMP_OUT_OF_MEM;
		memset(buf, 0, sizeof(MIDIHDR));
		buf->lpData = reinterpret_cast<char *>(buf) + sizeof(MIDIHDR);
		buf->dwBufferLength = HMP_BUFSIZE;
		buf->dwUser = reinterpret_cast<uintptr_t>(hmp);
		buf->lpNext = lastbuf;
		lastbuf = buf;
	}
	hmp->evbuf = lastbuf;
	return 0;
}

static void reset_tracks(struct hmp_file *hmp)
{
	if (hmp->num_trks > 0)
	range_for (auto &i, partial_range(hmp->trks, static_cast<unsigned>(hmp->num_trks)))
	{
		if (i.loop_set)
			i.cur = i.loop;
		else
			i.cur = i.data.get();
		i.left = i.len;
		i.cur_time = 0;
	}
	hmp->cur_time = 0;
}

static void _stdcall midi_callback(HMIDISTRM, UINT uMsg, DWORD, DWORD_PTR dw1, DWORD)
{
	hmp_file *hmp;
	int rc;

	if (uMsg != MOM_DONE)
		return;

	const auto mhdr = reinterpret_cast<MIDIHDR *>(dw1);
	mhdr->dwBytesRecorded = 0;
	hmp = reinterpret_cast<hmp_file *>(mhdr->dwUser);
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
	unsigned int *p = reinterpret_cast<unsigned int *>(mhdr->lpData + mhdr->dwBytesRecorded);
	*(p++) = 0;
	*(p++) = 0;
	*(p++) = ((static_cast<DWORD>(MEVT_TEMPO))<<24) | tempo;
	mhdr->dwBytesRecorded += 12;
}

void hmp_setvolume(hmp_file *hmp, int volume)
{
	if (hmp)
		range_for (const int channel, xrange(16u))
			midiOutShortMsg(reinterpret_cast<HMIDIOUT>(hmp->hmidi), static_cast<DWORD>(channel | MIDI_CONTROL_CHANGE << 4 | MIDI_VOLUME << 8 | (channel_volume[channel] * volume / MIDI_VOLUME_SCALE) << 16));

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
	if ((midiStreamOpen(&hmp->hmidi, &hmp->devid, 1, reinterpret_cast<DWORD_PTR>(&midi_callback), 0, CALLBACK_FUNCTION)) != MMSYSERR_NOERROR)
	{
		hmp->hmidi = NULL;
		return HMP_MM_ERR;
	}
	mptd.cbStruct  = sizeof(mptd);
	mptd.dwTimeDiv = hmp->tempo;
	if ((midiStreamProperty(hmp->hmidi, reinterpret_cast<BYTE *>(&mptd), MIDIPROP_SET|MIDIPROP_TIMEDIV)) != MMSYSERR_NOERROR)
	{
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
 		if ((rc = midiOutPrepareHeader(reinterpret_cast<HMIDIOUT>(hmp->hmidi), hmp->evbuf,
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
	char GS_Reset[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };


	if ((rval = midiOutOpen(&hmidi, MIDI_MAPPER, 0, 0, 0)) != MMSYSERR_NOERROR)
	{
		switch (rval)
		{
			case MIDIERR_NODEVICE:
				con_printf(CON_DEBUG, "midiOutOpen Error: no MIDI port was found.");
				break;
			case MMSYSERR_ALLOCATED:
				con_printf(CON_DEBUG, "midiOutOpen Error: specified resource is already allocated.");
				break;
			case MMSYSERR_BADDEVICEID:
				con_printf(CON_DEBUG, "midiOutOpen Error: specified device identifier is out of range.");
				break;
			case MMSYSERR_INVALPARAM:
				con_printf(CON_DEBUG, "midiOutOpen Error: specified pointer or structure is invalid.");
				break;
			case MMSYSERR_NOMEM:
				con_printf(CON_DEBUG, "midiOutOpen Error: unable to allocate or lock memory.");
				break;
			default:
				con_printf(CON_DEBUG, "midiOutOpen Error code %i",rval);
				break;
		}
		return;
	}

	range_for (const int channel, xrange(16u))
	{
		midiOutShortMsg(hmidi, static_cast<DWORD>(channel | MIDI_CONTROL_CHANGE << 4 | MIDI_ALL_SOUNDS_OFF << 8 | 0 << 16));
		midiOutShortMsg(hmidi, static_cast<DWORD>(channel | MIDI_CONTROL_CHANGE << 4 | MIDI_RESET_ALL_CONTROLLERS << 8 | 0 << 16));
		midiOutShortMsg(hmidi, static_cast<DWORD>(channel | MIDI_CONTROL_CHANGE << 4 | MIDI_ALL_NOTES_OFF << 8 | 0 << 16));

		channel_volume[channel] = 100;
		midiOutShortMsg(hmidi, static_cast<DWORD>(channel | MIDI_CONTROL_CHANGE << 4 | MIDI_PANPOT << 8 | 64 << 16));
		midiOutShortMsg(hmidi, static_cast<DWORD>(channel | MIDI_CONTROL_CHANGE << 4 | MIDI_REVERB << 8 | 40 << 16));
		midiOutShortMsg(hmidi, static_cast<DWORD>(channel | MIDI_CONTROL_CHANGE << 4 | MIDI_CHORUS << 8 | 0 << 16));

		midiOutShortMsg(hmidi, static_cast<DWORD>(channel | MIDI_CONTROL_CHANGE << 4 | MIDI_BANK_SELECT_MSB << 8 | 0 << 16));
		midiOutShortMsg(hmidi, static_cast<DWORD>(channel | MIDI_CONTROL_CHANGE << 4 | MIDI_BANK_SELECT_LSB << 8 | 0 << 16));
		midiOutShortMsg(hmidi, static_cast<DWORD>(channel | MIDI_PROGRAM_CHANGE << 4 | 0 << 8));
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
				auto timer = timer_update();
				if (timer >= wait_done + F1_0)
				{
					con_printf(CON_DEBUG, "hmp_reset: Timeout waiting for MHDR_DONE");
					break;
				}
			}
		}
		else
		{
			switch (rval)
			{
				case MIDIERR_NOTREADY:
					con_printf(CON_DEBUG, "midiOutLongMsg Error: the hardware is busy with other data.");
					break;
				case MIDIERR_UNPREPARED:
					con_printf(CON_DEBUG, "midiOutLongMsg Error: the buffer pointed to by lpMidiOutHdr has not been prepared.");
					break;
				case MMSYSERR_INVALHANDLE:
					con_printf(CON_DEBUG, "midiOutLongMsg Error: the specified device handle is invalid.");
					break;
				case MMSYSERR_INVALPARAM:
					con_printf(CON_DEBUG, "midiOutLongMsg Error: the specified pointer or structure is invalid.");
					break;
				default:
					con_printf(CON_DEBUG, "midiOutLongMsg Error code %i",rval);
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
				con_printf(CON_DEBUG, "midiOutPrepareHeader Error: The specified device handle is invalid.");
				break;
			case MMSYSERR_INVALPARAM:
				con_printf(CON_DEBUG, "midiOutPrepareHeader Error: The specified address is invalid or the given stream buffer is greater than 64K.");
				break;
			case MMSYSERR_NOMEM:
				con_printf(CON_DEBUG, "midiOutPrepareHeader Error: The system is unable to allocate or lock memory.");
				break;
			default:
				con_printf(CON_DEBUG, "midiOutPrepareHeader Error code %i",rval);
				break;
		}
	}

	range_for (const int channel, xrange(16u))
		midiOutShortMsg(hmidi, static_cast<DWORD>(channel | MIDI_CONTROL_CHANGE << 4 | MIDI_VOLUME << 8 | (100 * midi_volume / MIDI_VOLUME_SCALE) << 16));
	midiOutClose(hmidi);
}
#endif

// CONVERSION FROM HMP TO MIDI

static void hmptrk2mid(ubyte* data, int size, std::vector<uint8_t> &midbuf)
{
	uint8_t *dptr = data;
	ubyte lc1 = 0,last_com = 0;
	int n1;

	while (data < dptr + size)
	{
		if (data[0] & 0x80) {
			ubyte b = (data[0] & 0x7F);
			midbuf.emplace_back(b);
		}
		else {
			n1 = 1;
			while ((data[n1] & 0x80) == 0) {
				n1++;
				if (n1 == 4)
					throw std::runtime_error("bad HMP");
				}
			for(int n2 = 0; n2 <= n1; n2++) {
				ubyte b = (data[n1 - n2] & 0x7F);

				if (n2 != n1)
					b |= 0x80;
				midbuf.emplace_back(b);
				}
			data += n1;
		}
		data++;
		if (*data == 0xFF) { //meta?
			midbuf.insert(midbuf.end(), data, data + 3 + data[2]);
			if (data[1] == 0x2F)
				break;
		}
		else {
			lc1=data[0] ;
			if ((lc1&0x80) == 0)
				throw std::runtime_error("bad HMP");
			switch (lc1 & 0xF0) {
				case 0x80:
				case 0x90:
				case 0xA0:
				case 0xB0:
				case 0xE0:
					if (lc1 != last_com)
					{
						midbuf.emplace_back(lc1);
					}
					midbuf.insert(midbuf.end(), data + 1, data + 3);
					data += 3;
					break;
				case 0xC0:
				case 0xD0:
					if (lc1 != last_com)
					{
						midbuf.emplace_back(lc1);
					}
					midbuf.emplace_back(data[1]);
					data += 2;
					break;
				default:
					throw std::runtime_error("bad HMP");
				}
			last_com = lc1;
		}
	}
}

struct be_bytebuffer_t : serial::writer::bytebuffer_t
{
	be_bytebuffer_t(pointer u) : bytebuffer_t(u) {}
	static constexpr bytebuffer_t::big_endian_type endian{};
};

const std::array<uint8_t, 10> magic_header{{
	'M', 'T', 'h', 'd',
	0, 0, 0, 6,
	0, 1,
}};
const std::array<uint8_t, 19> tempo{{'M','T','r','k',0,0,0,11,0,0xFF,0x51,0x03,0x18,0x80,0x00,0,0xFF,0x2F,0}};
const std::array<uint8_t, 8> track_header{{'M', 'T', 'r', 'k', 0, 0, 0, 0}};

struct midhdr
{
	int16_t num_trks;
	int16_t time_div;
	midhdr(hmp_file *hmp) :
		num_trks(hmp->num_trks), time_div(hmp->tempo*1.6)
	{
	}
};

DEFINE_SERIAL_CONST_UDT_TO_MESSAGE(midhdr, m, (magic_header, m.num_trks, m.time_div, tempo));

hmpmid_result hmp2mid(const char *hmp_name)
{
	auto &&[hmp, hoe, pec] = hmp_open(hmp_name);
	if (!hmp)
	{
		if (underlying_value(hoe) <= static_cast<unsigned>(hmp_open_error::physfs_read_track_data))
			con_printf(CON_CRITICAL, "Failed to read HMP music %s: hmp_open_error=%u; PHYSFS_ErrorCode=%u/%s", hmp_name, underlying_value(hoe), pec, PHYSFS_getErrorByCode(pec));
		else
			con_printf(CON_CRITICAL, "Failed to read HMP music %s: hmp_open_error=%u", hmp_name, underlying_value(hoe));
		return {std::vector<uint8_t>{}, hoe};
	}

	const midhdr mh(hmp.get());
	std::vector<uint8_t> midbuf;
	// write MIDI-header
	midbuf.resize(serial::message_type<decltype(mh)>::maximum_size);
	be_bytebuffer_t bb(&midbuf[0]);
	serial::process_buffer(bb, mh);

	// tracks
	for (int i = 1; i < hmp->num_trks; i++)
	{
		midbuf.insert(midbuf.end(), track_header.begin(), track_header.end());
		auto size_before = midbuf.size();
		auto midtrklenpos = midbuf.size() - 4;
		hmptrk2mid(hmp->trks[i].data.get(), hmp->trks[i].len, midbuf);
		auto size_after = midbuf.size();
		be_bytebuffer_t bbmi(&midbuf[midtrklenpos]);
		serial::process_buffer(bbmi, static_cast<int32_t>(size_after - size_before));
	}
	return {midbuf, hmp_open_error::None};
}

}
