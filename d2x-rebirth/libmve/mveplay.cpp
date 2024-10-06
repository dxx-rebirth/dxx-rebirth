/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
//#define DEBUG

#include "dxxsconf.h"
#include <vector>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#ifdef _WIN32
# include <windows.h>
#else
# include <errno.h>
# include <fcntl.h>
# ifdef macintosh
#  include <types.h>
#  include <OSUtils.h>
# else
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <unistd.h>
# endif // macintosh
#endif // _WIN32

#include <SDL.h>
#if DXX_USE_SDLMIXER
#include <SDL_mixer.h>
#endif

#include "config.h"
#include "d_sdl_audio.h"
#include "mvelib.h"
#include "mve_audio.h"
#include "byteutil.h"
#include "decoders.h"
#include "libmve.h"
#include "args.h"
#include "console.h"
#include "u_mem.h"

namespace d2x {

int g_spdFactorNum=0;
static int g_spdFactorDenom=10;

static int16_t get_short(const unsigned char *data)
{
	short value;
	value = data[0] | (data[1] << 8);
	return value;
}

static uint16_t get_ushort(const unsigned char *data)
{
	unsigned short value;
	value = data[0] | (data[1] << 8);
	return value;
}

static int32_t get_int(const unsigned char *data)
{
	int value;
	value = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
	return value;
}

/*************************
 * general handlers
 *************************/
MVESTREAM::handle_result MVESTREAM::handle_mve_segment_endofstream()
{
	return MVESTREAM::handle_result::step_finished;
}

/*************************
 * timer handlers
 *************************/

/*
 * timer variables
 */
static int micro_frame_delay=0;
static int timer_started=0;
static struct timeval timer_expire = {0, 0};

}

#ifndef DXX_HAVE_STRUCT_TIMESPEC
struct timespec
{
	long int tv_sec;            /* Seconds.  */
	long int tv_nsec;           /* Nanoseconds.  */
};
#endif

#if defined(_WIN32) || defined(macintosh)
int gettimeofday(struct timeval *tv, void *)
{
	static int counter = 0;
#ifdef _WIN32
	DWORD now = GetTickCount();
#else
	long now = TickCount();
#endif
	counter++;

	tv->tv_sec = now / 1000;
	tv->tv_usec = (now % 1000) * 1000 + counter;

	return 0;
}
#endif //  defined(_WIN32) || defined(macintosh)

namespace d2x {

MVESTREAM::handle_result MVESTREAM::handle_mve_segment_createtimer(const unsigned char *data)
{

#if !defined(_WIN32) && !defined(macintosh) // FIXME
	__extension__ long long temp;
#else
	long temp;
#endif

	if (timer_created)
		return MVESTREAM::handle_result::step_again;
	else
		timer_created = 1;

	micro_frame_delay = get_int(data) * static_cast<int>(get_short(data+4));
	if (g_spdFactorNum != 0)
	{
		temp = micro_frame_delay;
		temp *= g_spdFactorNum;
		temp /= g_spdFactorDenom;
		micro_frame_delay = static_cast<int>(temp);
	}
	return MVESTREAM::handle_result::step_again;
}

static void timer_stop(void)
{
	timer_expire.tv_sec = 0;
	timer_expire.tv_usec = 0;
	timer_started = 0;
}

static void timer_start(void)
{
	int nsec=0;
	gettimeofday(&timer_expire, NULL);
	timer_expire.tv_usec += micro_frame_delay;
	if (timer_expire.tv_usec > 1000000)
	{
		nsec = timer_expire.tv_usec / 1000000;
		timer_expire.tv_sec += nsec;
		timer_expire.tv_usec -= nsec*1000000;
	}
	timer_started=1;
}

static void do_timer_wait(void)
{
	int nsec=0;
	struct timespec ts;
	struct timeval tv;
	if (! timer_started)
		return;

	gettimeofday(&tv, NULL);
	if (tv.tv_sec > timer_expire.tv_sec)
		goto end;
	else if (tv.tv_sec == timer_expire.tv_sec  &&  tv.tv_usec >= timer_expire.tv_usec)
		goto end;

	ts.tv_sec = timer_expire.tv_sec - tv.tv_sec;
	ts.tv_nsec = 1000 * (timer_expire.tv_usec - tv.tv_usec);
	if (ts.tv_nsec < 0)
	{
		ts.tv_nsec += 1000000000UL;
		--ts.tv_sec;
	}
#ifdef _WIN32
	Sleep(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#elif defined(macintosh)
	Delay(ts.tv_sec * 1000 + ts.tv_nsec / 1000000, NULL);
#else
	if (nanosleep(&ts, NULL) == -1  &&  errno == EINTR)
		exit(1);
#endif

 end:
	timer_expire.tv_usec += micro_frame_delay;
	if (timer_expire.tv_usec > 1000000)
	{
		nsec = timer_expire.tv_usec / 1000000;
		timer_expire.tv_sec += nsec;
		timer_expire.tv_usec -= nsec*1000000;
	}
}

/*************************
 * audio handlers
 *************************/

namespace {

enum : uint8_t
{
	MVE_AUDIO_FLAGS_STEREO = 1,
	MVE_AUDIO_FLAGS_16BIT = 2,
	MVE_AUDIO_FLAGS_COMPRESSED = 4,
};

template <typename T>
struct MVE_audio_clamp
{
	const unsigned scale;
	MVE_audio_clamp(const unsigned DigiVolume) :
		scale(DigiVolume)
	{
	}
	T operator()(const T &i) const
	{
		return (static_cast<int32_t>(i) * scale) / 8;
	}
};

}

static void mve_audio_callback(void *userdata, unsigned char *stream, int len);

static void mve_audio_callback(void *const vstream, unsigned char *stream, int len)
{
	auto &mvestream = *reinterpret_cast<MVESTREAM *>(vstream);
#ifdef DXX_REPORT_TOTAL_LENGTH
	int total=0;
#endif
	if (mvestream.mve_audio_bufhead == mvestream.mve_audio_buftail)
		return /* 0 */;

	//con_printf(CON_CRITICAL, "+ <%d (%d), %d, %d>", mvestream.mve_audio_bufhead, mvestream.mve_audio_curbuf_curpos, mvestream.mve_audio_buftail, len);

	std::span<const std::byte> audio_buffer;
	while (mvestream.mve_audio_bufhead != mvestream.mve_audio_buftail                                           /* while we have more buffers  */
		   && len > (audio_buffer = std::as_bytes(mvestream.mve_audio_buffers[mvestream.mve_audio_bufhead].span()).subspan(mvestream.mve_audio_curbuf_curpos)).size())        /* and while we need more data */
	{
		const auto length = audio_buffer.size();
		memcpy(stream,                                                                  /* cur output position */
		       audio_buffer.data(),           /* cur input position  */
		       length);                                                                 /* cur input length    */

#ifdef DXX_REPORT_TOTAL_LENGTH
		total += length;
#endif
		stream += length;                                                               /* advance output */
		len -= length;                                                                  /* decrement avail ospace */
		mvestream.mve_audio_buffers[mvestream.mve_audio_bufhead] = {};                                 /* free the buffer */

		if (++mvestream.mve_audio_bufhead == mvestream.mve_audio_buffers.size())                                 /* next buffer */
			mvestream.mve_audio_bufhead = 0;
		mvestream.mve_audio_curbuf_curpos = 0;
	}

#ifdef DXX_REPORT_TOTAL_LENGTH
	//con_printf(CON_CRITICAL, "= <%d (%d), %d, %d>: %d", mvestream.mve_audio_bufhead, mvestream.mve_audio_curbuf_curpos, mvestream.mve_audio_buftail, len, total);
#endif
	/*    return total; */

	if (len != 0                                                                        /* ospace remaining  */
		&&  mvestream.mve_audio_bufhead != mvestream.mve_audio_buftail)                                     /* buffers remaining */
	{
		memcpy(stream,                                                                  /* dest */
		       (reinterpret_cast<uint8_t *>(mvestream.mve_audio_buffers[mvestream.mve_audio_bufhead].get())) + mvestream.mve_audio_curbuf_curpos,         /* src */
			   len);                                                                    /* length */

		mvestream.mve_audio_curbuf_curpos += len;                                                 /* advance input */
		stream += len;                                                                  /* advance output (unnecessary) */
		len -= len;                                                                     /* advance output (unnecessary) */

		auto &a = mvestream.mve_audio_buffers[mvestream.mve_audio_bufhead];
		if (mvestream.mve_audio_curbuf_curpos >= a.span().size_bytes())            /* if this ends the current chunk */
		{
			a = {};                             /* free buffer */

			if (++mvestream.mve_audio_bufhead == mvestream.mve_audio_buffers.size())                             /* next buffer */
				mvestream.mve_audio_bufhead = 0;
			mvestream.mve_audio_curbuf_curpos = 0;
		}
	}

	//con_printf(CON_CRITICAL, "- <%d (%d), %d, %d>", mvestream.mve_audio_bufhead, mvestream.mve_audio_curbuf_curpos, mvestream.mve_audio_buftail, len);
}

MVESTREAM::handle_result MVESTREAM::handle_mve_segment_initaudiobuffers(unsigned char minor, const unsigned char *data)
{
	if (mve_audio_enabled == MVE_play_sounds::silent)
		return MVESTREAM::handle_result::step_again;

	if (mve_audio_spec)
		return MVESTREAM::handle_result::step_again;

	auto flags{get_ushort(&data[2])};
	const auto sample_rate{get_ushort(&data[4])};

	const auto stereo{flags & MVE_AUDIO_FLAGS_STEREO};
	const auto bitsize{flags & MVE_AUDIO_FLAGS_16BIT};
	if (!minor)
		flags &= ~MVE_AUDIO_FLAGS_COMPRESSED;
	mve_audio_flags = flags;

	const unsigned format = (bitsize)
		? (words_bigendian ? AUDIO_S16MSB : AUDIO_S16LSB)
		: AUDIO_U8;

	if (CGameArg.SndDisableSdlMixer)
	{
		con_puts(CON_CRITICAL, "creating audio buffers:");
		const auto desired_buffer{get_int(&data[6])};
		const auto compressed{flags & MVE_AUDIO_FLAGS_COMPRESSED};
		con_printf(CON_CRITICAL, "sample rate = %d, desired buffer = %d, stereo = %d, bitsize = %d, compressed = %d",
				sample_rate, desired_buffer, stereo ? 1 : 0, bitsize ? 16 : 8, compressed ? 1 : 0);
	}

	mve_audio_spec = std::make_unique<SDL_AudioSpec>();
	{
		auto &s = *mve_audio_spec;
		s.freq = sample_rate;
		s.format = format;
		s.channels = (stereo) ? 2 : 1;
		s.samples = 4096;
		s.callback = mve_audio_callback;
		s.userdata = this;

	// MD2211: if using SDL_Mixer, we never reinit the sound system
	if (CGameArg.SndDisableSdlMixer)
	{
		if (SDL_OpenAudio(&s, NULL) >= 0) {
			con_puts(CON_CRITICAL, "   success");
		}
		else {
			con_printf(CON_CRITICAL, "   failure : %s", SDL_GetError());
			mve_audio_spec = {};
		}
	}

#if DXX_USE_SDLMIXER
	else {
		// MD2211: using the same old SDL audio callback as a postmixer in SDL_mixer
		Mix_SetPostMix(s.callback, s.userdata);
	}
#endif
	}

	mve_audio_buffers = {};
	return MVESTREAM::handle_result::step_again;
}

MVESTREAM::handle_result MVESTREAM::handle_mve_segment_startstopaudio()
{
	if (mve_audio_spec && !mve_audio_playing && mve_audio_bufhead != mve_audio_buftail)
	{
		if (CGameArg.SndDisableSdlMixer)
			SDL_PauseAudio(0);
#if DXX_USE_SDLMIXER
		else
			Mix_Pause(0);
#endif
		mve_audio_playing = 1;
	}
	return MVESTREAM::handle_result::step_again;
}

/* Caution: the data length argument is unnamed here because it is sometimes
 * wrong.  The first movie segment of the opening video of the game is
 * ill-formed:
 * - Its segment size according to mvelib is 2932.
 * - It has a 10 byte header:
 *   - 2 bytes skipped
 *   - 2 bytes of `chan`
 *   - 2 bytes of `nsamp`
 *   - 2 bytes of "current offset 0"
 *   - 2 bytes of "current offset 1"
 * - It has an internal size of 2924, which is 2 bytes too long.
 *
 * If `std::span` is used to describe this segment, then the historically used
 * audio processing will read past the end of the `std::span`, which is
 * undefined and may trigger an assertion failure.  Therefore, do not use
 * std::span to describe this block of memory.
 */
MVESTREAM::handle_result MVESTREAM::handle_mve_segment_audioframedata(const mve_opcode major, const unsigned char *data)
{
	static const int selected_chan=1;
	if (const auto mve_audio_spec = this->mve_audio_spec.get())
	{
		std::optional<RAII_SDL_LockAudio> lock_audio{
			mve_audio_playing ? std::optional<RAII_SDL_LockAudio>(std::in_place) : std::nullopt
		};

		const auto chan = get_ushort(data + 2);
		unsigned nsamp = get_ushort(data + 4);
		if (chan & selected_chan)
		{
			decltype(mve_audio_buffers)::value_type p;
			const auto DigiVolume{CGameCfg.DigiVolume};
			/* At volume 0 (minimum), no sound is wanted. */
			if (DigiVolume && major == mve_opcode::audioframedata) {
				const auto flags{mve_audio_flags};
				if (flags & MVE_AUDIO_FLAGS_COMPRESSED) {
					/* HACK: +4 mveaudio_uncompress adds 4 more bytes */
					nsamp += 4;

					p = {nsamp / 2};
					mveaudio_uncompress(p.span(), data);
				} else {
					nsamp -= 8;
					data += 8;

					p = {nsamp / 2};
					memcpy(p.get(), data, nsamp);
				}
				if (DigiVolume < 8)
				{
					/* At volume 8 (maximum), no scaling is needed. */
					if (flags & MVE_AUDIO_FLAGS_16BIT)
					{
						int16_t *const p16 = p.get();
						std::transform(p16, reinterpret_cast<int16_t *>(reinterpret_cast<uint8_t *>(p16) + nsamp), p16, MVE_audio_clamp<int16_t>(DigiVolume));
					}
					else
					{
						int8_t *const p8 = reinterpret_cast<int8_t *>(p.get());
						std::transform(p8, p8 + nsamp, p8, MVE_audio_clamp<int8_t>(DigiVolume));
					}
				}
			} else {
				/* unique_span will value-initialize the buffer, so no explicit
				 * initialization is necessary
				 */
				p = {nsamp / 2};
			}

			// MD2211: the following block does on-the-fly audio conversion for SDL_mixer
#if DXX_USE_SDLMIXER
			if (!CGameArg.SndDisableSdlMixer) {
				// build converter: in = MVE format, out = SDL_mixer output
				int out_freq;
				Uint16 out_format;
				int out_channels;
				Mix_QuerySpec(&out_freq, &out_format, &out_channels); // get current output settings

				SDL_AudioCVT cvt{};
				SDL_BuildAudioCVT(&cvt, mve_audio_spec->format, mve_audio_spec->channels, mve_audio_spec->freq,
					out_format, out_channels, out_freq);

				const auto cvtbuf = std::make_unique<uint8_t[]>(nsamp * cvt.len_mult);
				cvt.buf = cvtbuf.get();
				cvt.len = nsamp;

				// read the audio buffer into the conversion buffer
				memcpy(cvt.buf, p.get(), nsamp);

				// do the conversion
				if (SDL_ConvertAudio(&cvt))
					con_printf(CON_URGENT, "%s:%u: SDL_ConvertAudio failed: nsamp=%u out_format=%i out_channels=%i out_freq=%i", __FILE__, __LINE__, nsamp, out_format, out_channels, out_freq);
				else
				{
				// copy back to the audio buffer
					const std::size_t converted_buffer_size = cvt.len_cvt;
					p = {}; // free the old audio buffer
					p = {converted_buffer_size / 2};
					memcpy(p.get(), cvt.buf, converted_buffer_size);
				}
			}
#endif
			mve_audio_buffers[mve_audio_buftail] = std::move(p);

			if (++mve_audio_buftail == mve_audio_buffers.size())
				mve_audio_buftail = 0;

			if (mve_audio_buftail == mve_audio_bufhead)
				con_printf(CON_CRITICAL, "d'oh!  buffer ring overrun (%d)", mve_audio_bufhead);
		}
	}

	return MVESTREAM::handle_result::step_again;
}

/*************************
 * video handlers
 *************************/

MVESTREAM::handle_result MVESTREAM::handle_mve_segment_initvideobuffers(unsigned char minor, const unsigned char *data)
{
	short w, h;
#ifdef DEBUG
	short count;
#endif

	if (videobuf_created)
		return MVESTREAM::handle_result::step_again;
	else
		videobuf_created = 1;

	w = get_short(data);
	h = get_short(data+2);

#ifdef DEBUG
	if (minor > 0) {
		count = get_short(data+4);
	} else {
		count = 1;
	}
#endif

	if (minor > 1) {
		truecolor = !!get_short(data + 6);
	} else {
		truecolor = 0;
	}

	width = w << 3;
	height = h << 3;

	/* TODO: * 4 causes crashes on some files */
	/* only malloc once */
	vBuffers.assign(width * height * 8, 0);
	vBackBuf1 = vBuffers.data();
	if (truecolor) {
		vBackBuf2 = reinterpret_cast<uint8_t *>(reinterpret_cast<uint16_t *>(vBackBuf1) + (width * height));
	} else {
		vBackBuf2 = (vBackBuf1 + (width * height));
	}
#ifdef DEBUG
	con_printf(CON_CRITICAL, "DEBUG: w,h=%d,%d count=%d, tc=%d", w, h, count, truecolor);
#endif

	return MVESTREAM::handle_result::step_again;
}

MVESTREAM::handle_result MVESTREAM::handle_mve_segment_displayvideo()
{
	MovieShowFrame(vBackBuf1, destX, destY, width, height, screenWidth, screenHeight);

	frameUpdated = 1;

	return MVESTREAM::handle_result::step_again;
}

MVESTREAM::handle_result MVESTREAM::handle_mve_segment_initvideomode(const unsigned char *data)
{
	short width, height;

	if (video_initialized)
		return MVESTREAM::handle_result::step_again; /* maybe we actually need to change width/height here? */
	else
		video_initialized = 1;

	width = get_short(data);
	height = get_short(data+2);
	screenWidth = width;
	screenHeight = height;

	return MVESTREAM::handle_result::step_again;
}

MVESTREAM::handle_result MVESTREAM::handle_mve_segment_setpalette(const unsigned char *data)
{
	short start, count;
	start = get_short(data);
	count = get_short(data+2);

	auto p = data + 4;
	MovieSetPalette(p - 3*start, start, count);
	return MVESTREAM::handle_result::step_again;
}

MVESTREAM::handle_result MVESTREAM::handle_mve_segment_setdecodingmap(const unsigned char *data, int len)
{
	pCurMap = std::span<const uint8_t>(data, len);
	return MVESTREAM::handle_result::step_again;
}

MVESTREAM::handle_result MVESTREAM::handle_mve_segment_videodata(const unsigned char *data, int len)
{
	unsigned short nFlags;

// don't need those but kept for further reference
// 	nFrameHot  = get_short(data);
// 	nFrameCold = get_short(data+2);
// 	nXoffset   = get_short(data+4);
// 	nYoffset   = get_short(data+6);
// 	nXsize     = get_short(data+8);
// 	nYsize     = get_short(data+10);
	nFlags     = get_ushort(data+12);

	if (nFlags & 1)
	{
		std::swap(vBackBuf1, vBackBuf2);
	}

	/* convert the frame */
	if (truecolor) {
		decodeFrame16(reinterpret_cast<const uint16_t *>(vBackBuf2), width, height, vBackBuf1, pCurMap, data+14, len-14);
	} else {
		decodeFrame8(vBackBuf2, width, height, vBackBuf1, pCurMap, data+14, len-14);
	}

	return MVESTREAM::handle_result::step_again;
}

MVESTREAM::handle_result MVESTREAM::handle_mve_segment_endofchunk()
{
	pCurMap = {};
	return MVESTREAM::handle_result::step_again;
}

MVESTREAM_ptr_t MVE_rmPrepMovie(const MVE_play_sounds audio_enabled, const int x, const int y, RWops_ptr src)
{
	MVESTREAM_ptr_t pMovie{mve_open(audio_enabled, x, y, std::move(src))};
	if (!pMovie)
		return pMovie;

	auto &mve = *pMovie.get();

	mve_play_next_chunk(mve); /* video initialization chunk */
	mve_play_next_chunk(mve); /* audio initialization chunk */
	return pMovie;
}

MVE_StepStatus MVE_rmStepMovie(MVESTREAM &mve)
{
	static int init_timer=0;

	if (!timer_started)
		timer_start();

	for (;;)
	{
		// make a "step" be a frame, not a chunk...
		if (mve_play_next_chunk(mve) == MVESTREAM::handle_result::step_finished)
		{
			mve.frameUpdated = 0;
			return MVE_StepStatus::EndOfFile;
		}
		if (mve.frameUpdated)
			break;
	}
	mve.frameUpdated = 0;

	if (micro_frame_delay  && !init_timer) {
		timer_start();
		init_timer = 1;
	}

	do_timer_wait();

	return MVE_StepStatus::Continue;
}

void MVE_rmEndMovie(std::unique_ptr<MVESTREAM> stream)
{
	timer_stop();

	if (stream->mve_audio_spec) {
		// MD2211: if using SDL_Mixer, we never reinit sound, hence never close it
		if (CGameArg.SndDisableSdlMixer)
		{
			SDL_CloseAudio();
		}
#if DXX_USE_SDLMIXER
		else
			Mix_SetPostMix(nullptr, nullptr);
#endif
	}
}


void MVE_rmHoldMovie()
{
	timer_started = 0;
}

}
