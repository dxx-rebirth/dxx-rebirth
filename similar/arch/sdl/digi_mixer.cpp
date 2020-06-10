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

static int fix2byte(const fix f)
{
	return (f / 256) % 256;
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

}

namespace dsx {

/*
 * Play-time conversion. Performs output conversion only once per sound effect used.
 * Once the sound sample has been converted, it is cached in SoundChunks[]
 */
static void mixdigi_convert_sound(const unsigned i)
{
	if (SoundChunks[i].abuf)
		//proceed only if not converted yet
		return;
	SDL_AudioCVT cvt;
	Uint8 *data = GameSounds[i].data;
	Uint32 dlen = GameSounds[i].length;
	int freq;
	int out_freq;
	Uint16 out_format;
	int out_channels;
#if defined(DXX_BUILD_DESCENT_I)
	out_freq = digi_sample_rate;
	out_format = MIX_OUTPUT_FORMAT;
	out_channels = MIX_OUTPUT_CHANNELS;
	freq = GameSounds[i].freq;
#elif defined(DXX_BUILD_DESCENT_II)
	Mix_QuerySpec(&out_freq, &out_format, &out_channels); // get current output settings
	freq = GameArg.SndDigiSampleRate;
#endif

	if (data)
	{
		if (SDL_BuildAudioCVT(&cvt, AUDIO_U8, 1, freq, out_format, out_channels, out_freq) == -1)
		{
			con_printf(CON_URGENT, "%s:%u: SDL_BuildAudioCVT failed: sound=%i dlen=%u freq=%i out_format=%i out_channels=%i out_freq=%i", __FILE__, __LINE__, i, dlen, freq, out_format, out_channels, out_freq);
			return;
		}

		auto cvtbuf = std::make_unique<Uint8[]>(dlen * cvt.len_mult);
		cvt.buf = cvtbuf.get();
		cvt.len = dlen;
		memcpy(cvt.buf, data, dlen);
		if (SDL_ConvertAudio(&cvt))
		{
			con_printf(CON_URGENT, "%s:%u: SDL_ConvertAudio failed: sound=%i dlen=%u freq=%i out_format=%i out_channels=%i out_freq=%i", __FILE__, __LINE__, i, dlen, freq, out_format, out_channels, out_freq);
			return;
		}

		SoundChunks[i].abuf = cvtbuf.release();
		SoundChunks[i].alen = cvt.len_cvt;
		SoundChunks[i].allocated = 1;
		SoundChunks[i].volume = 128; // Max volume = 128
	}
}

// Volume 0-F1_0
int digi_mixer_start_sound(short soundnum, fix volume, int pan, int looping, int loop_start, int loop_end, sound_object *)
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

	const int mix_vol = fix2byte(fixmul(digi_volume, volume));
	const uint8_t mix_distance = (volume > F1_0) ? 0 : UINT8_MAX - mix_vol;
	const int mix_pan = fix2byte(pan);
#if MIX_DIGI_DEBUG
	con_printf(CON_DEBUG, "digi_start_sound %d, volume %d, pan %d (start=%d, end=%d)", soundnum, mix_vol, mix_pan, loop_start, loop_end);
#else
	(void)loop_start;
	(void)loop_end;
#endif

	const int mix_loop = looping * -1;
	Mix_PlayChannel(channel, &(SoundChunks[soundnum]), mix_loop);
	Mix_SetPanning(channel, 255-mix_pan, mix_pan);
	Mix_SetDistance(channel, mix_distance);
	channels.set(channel);

	return channel;
}

}

namespace dcx {

void digi_mixer_set_channel_volume(int channel, int volume)
{
	int mix_vol = fix2byte(volume);
	if (!digi_initialised) return;
	Mix_SetDistance(channel, 255-mix_vol);
}

void digi_mixer_set_channel_pan(int channel, int pan)
{
	int mix_pan = fix2byte(pan);
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

void digi_mixer_reset() {}
void digi_mixer_stop_all_channels()
{
	channels = {};
	Mix_HaltChannel(-1);
}

}
