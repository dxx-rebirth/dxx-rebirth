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
#include <span>
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
#include "d_bitset.h"
#include "d_range.h"
#include "d_underlying_value.h"
#include "d_uspan.h"

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

enumerated_bitset<64, sound_channel> channels;

/* channel management */
static sound_channel digi_mixer_find_channel(const enumerated_bitset<64, sound_channel> &channels, const unsigned max_channels)
{
	uint8_t i{};
	for (; i < max_channels; ++i)
		if (!channels[(sound_channel{i})])
			break;
	return sound_channel{i};
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
unsigned digi_mixer_max_channels = std::size(channels);

void digi_mixer_free_channel(const int channel_num)
{
	if (const std::size_t u = channel_num; channels.valid_index(u))
		channels.reset(static_cast<sound_channel>(channel_num));
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
constexpr std::size_t FILTER_LEN = 51;
static constexpr std::array<int32_t, FILTER_LEN> coeffs_quarterband{{
		0, 0, -7, -25, -35, 0, 94, 200, 205, 0, -395, -751, -702, 0, 1178, 2127,
		1907, 0, -3050, -5490, -5011, 0, 9275, 20326, 29311, 32767, 29311,
		20326, 9275, 0, -5011, -5490, -3050, 0, 1907, 2127, 1178, 0, -702,
		-751, -395, 0, 205, 200, 94, 0, -35, -25, -7, 0, 0
}},
// Coefficient set for half-band (e.g. 22050 -> 44100)
coeffs_halfband{{
		0, 0, -11, 0, 49, 0, -133, 0, 290, 0, -558, 0, 992, 0, -1666, 0, 2697, 0,
		-4313, 0, 7086, 0, -13117, 0, 41452, 65535, 41452, 0, -13117, 0, 7086, 0,
		-4313, 0, 2697, 0, -1666, 0, 992, 0, -558, 0, 290, 0, -133, 0, 49, 0, -11,
		0, 0
}};

// Fixed-point FIR filtering
// Not optimal: consider optimization with 1/4, 1/2 band filters, and symmetric kernels
static auto filter_fir(const unique_span<int16_t> signal_storage, const std::span<const int32_t, FILTER_LEN> coeffs)
{
	const auto signal{signal_storage.span()};
	const std::size_t outsize = signal.size();
	unique_span<int16_t> result(outsize);
	const auto output{result.span()};
	// A FIR filter is just a 1D convolution
	// Keep only signalLen samples
	for (const auto nn : xrange(outsize))
	{
		// Determine start/stop indices for convolved chunk
		constexpr std::size_t coeffsLen = FILTER_LEN;
		/* Avoid use of `std::max` here, since `nn + 1 < coeffsLen` would cause
		 * unsigned subtraction to underflow.
		 */
		const std::size_t min_idx = (nn + 1 > coeffsLen ? nn + 1 - coeffsLen : 0u);
		const std::size_t max_idx = nn;
		if (min_idx > max_idx)
			continue;

		int32_t cur_output = 0;  // Increase bit size for fixed point expansion
		// Sum over each sample * coefficient in this column
		for (const auto kk : xrange(min_idx, max_idx + 1))
		{
			const auto product = int32_t{signal[kk]} * coeffs[nn - kk];
			cur_output = cur_output + product;
		}

		// Save and fit back into int16
		output[nn] = int16_t(cur_output >> 16);  // Arithmetic shift
	}
	return result;
}

static auto upsample(const std::span<const uint8_t> input, const std::size_t upsampledLen, const std::size_t factor)
{
	/* Caution: `output` is sparsely initialized, so the value-initialization
	 * from `make_unique` is necessary.  This site cannot be converted to
	 * `make_unique_for_overwrite`.
	 */
	unique_span<int16_t> result(upsampledLen);
	const auto output{result.span()};
	for (const auto ii : xrange(input.size()))
	{
		// Save input sample, convert to signed, and scale
		output[ii*factor] = 256 * (int16_t(input[ii]) - 128);
	}
	return result;
}

static auto replicateChannel(const unique_span<int16_t> input_storage, const std::size_t outsize, const std::size_t chFactor)
{
	const auto input{input_storage.span()};
	auto result = std::make_unique<Uint8[]>(outsize);
	const auto output = reinterpret_cast<int16_t *>(result.get());
	for (const auto ii : xrange(input.size()))
	{
		// Duplicate and interleave as many channels as needed
		std::fill_n(std::next(output, ii * chFactor), chFactor, input[ii]);
	}
	return result;
}

static std::unique_ptr<Uint8[]> convert_audio(const std::span<const uint8_t> input, const std::size_t outsize, const int upFactor, const std::size_t chFactor)
{
	const auto upsampledLen = input.size() * upFactor;

	// We expect a 4x upscaling 11025 -> 44100
	// But maybe 2x for d2x in some cases
	auto &coeffs = upFactor ? coeffs_halfband : coeffs_quarterband;

	return replicateChannel(
	// First upsample
	// Apply LPF filter to smooth out upscaled points
	// There will be some uniform amplitude loss here, but less than -3dB
		filter_fir(upsample(input, upsampledLen, upFactor), coeffs),
		outsize, chFactor);
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
	const auto data = GameSounds[i].span();
	int out_freq;
	int out_channels;
#if defined(DXX_BUILD_DESCENT_I)
	out_freq = digi_sample_rate;
	out_channels = MIX_OUTPUT_CHANNELS;
	const auto freq = GameSounds[i].freq;
#elif defined(DXX_BUILD_DESCENT_II)
	Uint16 out_format;
	Mix_QuerySpec(&out_freq, &out_format, &out_channels); // get current output settings
	const auto freq = GameArg.SndDigiSampleRate;
#endif

	if (!data.empty())
	{
		// Create output memory
		int upFactor = out_freq / freq;  // Should be integer, 2 or 4
		int formatFactor = 2;  // U8 -> S16 is two bytes
		int convertedSize = data.size() * upFactor * out_channels * formatFactor;

		auto cvtbuf = convert_audio(data, convertedSize, upFactor, out_channels);

		SoundChunks[i].abuf = cvtbuf.release();
		SoundChunks[i].alen = convertedSize;
		SoundChunks[i].allocated = 1;
		SoundChunks[i].volume = 128; // Max volume = 128
	}
}

}

// Volume 0-F1_0
sound_channel digi_mixer_start_sound(short soundnum, const fix volume, const sound_pan pan, const int looping, const int loop_start, const int loop_end, sound_object *)
{
	if (!digi_initialised)
		return sound_channel::None;

	if (soundnum < 0)
		return sound_channel::None;

	const unsigned max_channels = digi_mixer_max_channels;
	if (max_channels > channels.size())
		return sound_channel::None;
	const auto c = digi_mixer_find_channel(channels, max_channels);
	const auto channel = underlying_value(c);
	if (channel >= max_channels)
		return sound_channel::None;

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
	channels.set(c);
	return c;
}

}

namespace dcx {

void digi_mixer_set_channel_volume(const sound_channel channel, const int volume)
{
	if (!digi_initialised) return;
	Mix_SetDistance(underlying_value(channel), UINT8_MAX - fix2byte(volume));
}

void digi_mixer_set_channel_pan(const sound_channel channel, const sound_pan pan)
{
	int mix_pan = fix2byte(static_cast<fix>(pan));
	Mix_SetPanning(underlying_value(channel), 255 - mix_pan, mix_pan);
}

void digi_mixer_stop_sound(const sound_channel channel)
{
	if (!digi_initialised) return;
	const auto c = underlying_value(channel);
#if MIX_DIGI_DEBUG
	con_printf(CON_DEBUG, "%s:%u: %d", __FUNCTION__, __LINE__, c);
#endif
	Mix_HaltChannel(c);
	channels.reset(channel);
}

void digi_mixer_end_sound(const sound_channel channel)
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

int digi_mixer_is_channel_playing(const sound_channel c)
{
	return channels[c];
}

void digi_mixer_stop_all_channels()
{
	channels = {};
	Mix_HaltChannel(-1);
}

}
