/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 * This is an alternate backend for the sound effect system.
 * It uses SDL_mixer to provide a more reliable playback,
 * and allow processing of multiple audio formats.
 *
 * This file is based on the original D1X arch/sdl/digi.c
 *
 *  -- MD2211 (2006-10-12)
 */

#include <bitset>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_mixer.h>

#include "pstypes.h"
#include "dxxerror.h"
#include "sounds.h"
#include "digi.h"
#include "digi_mixer.h"
#include "digi_mixer_music.h"
#include "console.h"
#include "config.h"
#include "args.h"

#include "maths.h"
#include "piggy.h"
#include "u_mem.h"
#include <memory>

#define MIX_DIGI_DEBUG 0
#define MIX_OUTPUT_FORMAT	AUDIO_S16
#define MIX_OUTPUT_CHANNELS	2

#if !((defined(__APPLE__) && defined(__MACH__)) || defined(macintosh))
#define SOUND_BUFFER_SIZE 2048
#else
#define SOUND_BUFFER_SIZE 1024
#endif

namespace dcx {

namespace {

/* channel management */
static unsigned digi_mixer_find_channel(const std::bitset<64> &channels, const unsigned max_channels)
{
	unsigned i = 0;
	for (; i < max_channels; ++i)
		if (!channels[i])
			break;
	return i;
}

struct RAIIMix_Chunk : public Mix_Chunk
{
	RAIIMix_Chunk() = default;
	~RAIIMix_Chunk()
	{
		delete [] abuf;
	}
	RAIIMix_Chunk(const RAIIMix_Chunk &) = delete;
	RAIIMix_Chunk &operator=(const RAIIMix_Chunk &) = delete;
};

static uint8_t fix2byte(const fix f)
{
	if (f >= UINT8_MAX << 8)
		/* Values greater than this would produce incorrect results if
		 * shifted and truncated.  As a special case, coerce such values
		 * to the largest representable return value.
		 */
		return UINT8_MAX;
	return f >> 8;
}

uint8_t digi_initialised;
std::bitset<64> channels;
unsigned digi_mixer_max_channels = channels.size();

void digi_mixer_free_channel(const int channel_num)
{
	channels.reset(channel_num);
}

}

}

namespace dsx {

static std::array<RAIIMix_Chunk, MAX_SOUNDS> SoundChunks;

/* Initialise audio */
int digi_mixer_init()
{
#if defined(DXX_BUILD_DESCENT_II)
	const unsigned
#endif
	digi_sample_rate = SAMPLE_RATE_44K;

#if MIX_DIGI_DEBUG
	con_printf(CON_DEBUG, "digi_init %u (SDL_Mixer)", MAX_SOUNDS.value);
#endif
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) Error("SDL audio initialisation failed: %s.", SDL_GetError());

	if (Mix_OpenAudio(digi_sample_rate, MIX_OUTPUT_FORMAT, MIX_OUTPUT_CHANNELS, SOUND_BUFFER_SIZE))
	{
		//edited on 10/05/98 by Matt Mueller - should keep running, just with no sound.
		con_printf(CON_URGENT,"\nError: Couldn't open audio: %s", SDL_GetError());
		CGameArg.SndNoSound = 1;
		return 1;
	}

	digi_mixer_max_channels = Mix_AllocateChannels(digi_mixer_max_channels);
	channels.reset();
	Mix_Pause(0);
	Mix_ChannelFinished(digi_mixer_free_channel);

	digi_initialised = 1;

	digi_mixer_set_digi_volume( (GameCfg.DigiVolume*32768)/8 );

	return 0;
}

}

namespace dcx {

/* Shut down audio */
void digi_mixer_close() {
#if MIX_DIGI_DEBUG
	con_printf(CON_DEBUG, "digi_close (SDL_Mixer)");
#endif
	if (!digi_initialised) return;
	digi_initialised = 0;
	Mix_CloseAudio();
}

namespace {

/*
 * Blackman windowed-sinc filter coefficients at 1/4 bandwidth of upsampled
 * frequency. Chosen for linear phase and approximates ~10th order IIR
 *
 * MATLAB/Octave code:

	N = 51;   % Num coeffs (odd)
	B = 0.25;  % 1/4 band
	half = (N-1)/2;
	n = (-half:half); % the sample index
	% Windowed sinc
	h_ideal = 2 * B .* sinc(B*n);
	h_win = blackman(N);
	b = h_win .* h_ideal.';
	% Convert to fix point to ultimately apply to signed 16-bit data
	b_s16 = int32(round(b * (2^16 -1)));  % coeffs!

 */
#define FILTER_LEN 51
static int32_t coeffs_quarterband[FILTER_LEN] =
{
		0, 0, -7, -25, -35, 0, 94, 200, 205, 0, -395, -751, -702, 0, 1178, 2127,
		1907, 0, -3050, -5490, -5011, 0, 9275, 20326, 29311, 32767, 29311,
		20326, 9275, 0, -5011, -5490, -3050, 0, 1907, 2127, 1178, 0, -702,
		-751, -395, 0, 205, 200, 94, 0, -35, -25, -7, 0, 0
};

// Coefficient set for half-band (e.g. 22050 -> 44100)
static int32_t coeffs_halfband[FILTER_LEN] =
{
		0, 0, -11, 0, 49, 0, -133, 0, 290, 0, -558, 0, 992, 0, -1666, 0, 2697, 0,
		-4313, 0, 7086, 0, -13117, 0, 41452, 65535, 41452, 0, -13117, 0, 7086, 0,
		-4313, 0, 2697, 0, -1666, 0, 992, 0, -558, 0, 290, 0, -133, 0, 49, 0, -11,
		0, 0
};

// Fixed-point FIR filtering
// Not optimal: consider optimization with 1/4, 1/2 band filters, and symmetric kernels
static std::unique_ptr<int16_t[]> filter_fir(int16_t *signal, int signalLen, const int32_t (&coeffs)[FILTER_LEN])
{
	auto output = std::make_unique<int16_t[]>(signalLen);
	// A FIR filter is just a 1D convolution
	// Keep only signalLen samples
	for(int nn = 0; nn < signalLen; nn++)
	{
		// Determine start/stop indices for convolved chunk
		int min_idx = std::max(0, nn - FILTER_LEN + 1);
		int max_idx = std::min(nn, signalLen-1);

		int32_t cur_output = 0;  // Increase bit size for fixed point expansion
		// Sum over each sample * coefficient in this column
		for(int kk = min_idx; kk <= max_idx; kk++)
		{
			int product = int32_t(signal[kk]) * coeffs[nn-kk];
			cur_output = cur_output + product;
		}

		// Save and fit back into int16
		output[nn] = int16_t(cur_output >> 16);  // Arithmetic shift
	}
	return output;
}

static std::unique_ptr<int16_t[]> upsample(uint8_t *input, const std::size_t upsampledLen, int inputLen, int factor)
{
	/* Caution: `output` is sparsely initialized, so the value-initialization
	 * from `make_unique` is necessary.  This site cannot be converted to
	 * `make_unique_for_overwrite`.
	 */
	auto output = std::make_unique<int16_t[]>(upsampledLen);
	for(int ii = 0; ii < inputLen; ii++)
	{
		// Save input sample, convert to signed, and scale
		output[ii*factor] = 256 * (int16_t(input[ii]) - 128);
	}
	return output;
}


static void replicateChannel(int16_t *input, int16_t *output, int inputLen, int chFactor)
{
	for(int ii = 0; ii < inputLen; ii++)
	{
		// Duplicate and interleave as many channels as needed
		for(int jj = 0; jj < chFactor; jj++)
		{
			int idx = (ii*chFactor) + jj;
			output[idx] = input[ii];
		}
	}
}


static void convert_audio(uint8_t *input, int16_t *output, int inputLen, int upFactor, int chFactor)
{
	// First upsample
	int upsampledLen = inputLen*upFactor;

	auto stage1 = upsample(input, upsampledLen, inputLen, upFactor);

	// We expect a 4x upscaling 11025 -> 44100
	// But maybe 2x for d2x in some cases
	auto &coeffs = upFactor ? coeffs_halfband : coeffs_quarterband;

	// Apply LPF filter to smooth out upscaled points
	// There will be some uniform amplitude loss here, but less than -3dB
	auto stage2 = filter_fir(stage1.get(), upsampledLen, coeffs);

	// Replicate channel
	replicateChannel(stage2.get(), output, upsampledLen, chFactor);
}

}

}

namespace dsx {

namespace {

/*
 * Play-time conversion. Performs output conversion only once per sound effect used.
 * Once the sound sample has been converted, it is cached in SoundChunks[]
 */
static void mixdigi_convert_sound(const unsigned i)
{
	if (SoundChunks[i].abuf)
		//proceed only if not converted yet
		return;

	Uint8 *data = GameSounds[i].data;
	Uint32 dlen = GameSounds[i].length;
	int freq;
	int out_freq;
	int out_channels;
#if defined(DXX_BUILD_DESCENT_I)
	out_freq = digi_sample_rate;
	out_channels = MIX_OUTPUT_CHANNELS;
	freq = GameSounds[i].freq;
#elif defined(DXX_BUILD_DESCENT_II)
	Uint16 out_format;
	Mix_QuerySpec(&out_freq, &out_format, &out_channels); // get current output settings
	freq = GameArg.SndDigiSampleRate;
#endif

	if (data)
	{
		// Create output memory
		int upFactor = out_freq / freq;  // Should be integer, 2 or 4
		int formatFactor = 2;  // U8 -> S16 is two bytes
		int convertedSize = dlen * upFactor * out_channels * formatFactor;

		auto cvtbuf = std::make_unique<Uint8[]>(convertedSize);
		convert_audio(data, reinterpret_cast<int16_t*>(cvtbuf.get()), dlen, upFactor, out_channels);

		SoundChunks[i].abuf = cvtbuf.release();
		SoundChunks[i].alen = convertedSize;
		SoundChunks[i].allocated = 1;
		SoundChunks[i].volume = 128; // Max volume = 128
	}
}

}

// Volume 0-F1_0
int digi_mixer_start_sound(short soundnum, const fix volume, const sound_pan pan, const int looping, const int loop_start, const int loop_end, sound_object *)
{
	if (!digi_initialised) return -1;

	if (soundnum < 0)
		return -1;

	const unsigned max_channels = digi_mixer_max_channels;
	if (max_channels > channels.size())
		return -1;
	const auto channel = digi_mixer_find_channel(channels, max_channels);
	if (channel >= max_channels)
		return -1;

	Assert(GameSounds[soundnum].data != reinterpret_cast<void *>(-1));

	mixdigi_convert_sound(soundnum);

	const int mix_pan = fix2byte(static_cast<fix>(pan));
#if MIX_DIGI_DEBUG
	con_printf(CON_DEBUG, "digi_start_sound %d, volume=%d, pan %d (start=%d, end=%d)", soundnum, volume, mix_pan, loop_start, loop_end);
#else
	(void)loop_start;
	(void)loop_end;
#endif

	const int mix_loop = looping * -1;
	Mix_PlayChannel(channel, &(SoundChunks[soundnum]), mix_loop);
	Mix_SetPanning(channel, 255-mix_pan, mix_pan);
	Mix_SetDistance(channel, UINT8_MAX - fix2byte(volume));
	channels.set(channel);

	return channel;
}

}

namespace dcx {

void digi_mixer_set_channel_volume(int channel, int volume)
{
	if (!digi_initialised) return;
	Mix_SetDistance(channel, UINT8_MAX - fix2byte(volume));
}

void digi_mixer_set_channel_pan(int channel, const sound_pan pan)
{
	int mix_pan = fix2byte(static_cast<fix>(pan));
	Mix_SetPanning(channel, 255-mix_pan, mix_pan);
}

void digi_mixer_stop_sound(int channel) {
	if (!digi_initialised) return;
#if MIX_DIGI_DEBUG
	con_printf(CON_DEBUG, "digi_stop_sound %d", channel);
#endif
	Mix_HaltChannel(channel);
	channels.reset(channel);
}

void digi_mixer_end_sound(int channel)
{
	digi_mixer_stop_sound(channel);
	channels.reset(channel);
}

void digi_mixer_set_digi_volume( int dvolume )
{
	digi_volume = dvolume;
	if (!digi_initialised) return;
	Mix_Volume(-1, fix2byte(dvolume));
}

int digi_mixer_is_channel_playing(const int c)
{
	return channels[c];
}

void digi_mixer_stop_all_channels()
{
	channels = {};
	Mix_HaltChannel(-1);
}

}
