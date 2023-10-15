/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "libmve.h"

#include <array>
#include <cstdint>
#include <span>
#include <vector>
#include "dxxsconf.h"
#include <SDL.h>
#include "d_uspan.h"
#include "physfsrwops.h"

namespace d2x {

enum class mve_opcode : uint8_t
{
	endofstream = 0x00,
	endofchunk = 0x01,
	createtimer = 0x02,
	initaudiobuffers = 0x03,
	startstopaudio = 0x04,
	initvideobuffers = 0x05,

	displayvideo = 0x07,
	audioframedata = 0x08,
	audioframesilence = 0x09,
	initvideomode = 0x0A,

	setpalette = 0x0C,
	/* setpalettecompressed = 0x0D,	// unused */

	setdecodingmap = 0x0F,

	videodata = 0x11,
	None = 0xff,
};

/*
 * structure for maintaining info on a MVEFILE stream
 */
struct MVEFILE
{
	MVEFILE() = default;
	MVEFILE(RWops_ptr);
	MVEFILE(MVEFILE &&) = default;
	MVEFILE &operator=(MVEFILE &&) = default;
	~MVEFILE();
	RWops_ptr stream{};
	std::vector<uint8_t> cur_chunk;
	std::size_t next_segment = 0;
};

/*
 * get size of next segment in chunk (-1 if no more segments in chunk)
 */
int_fast32_t mvefile_get_next_segment_size(const MVEFILE *movie);

/*
 * get type of next segment in chunk (0xff if no more segments in chunk)
 */
mve_opcode mvefile_get_next_segment_major(const MVEFILE *movie);

/*
 * get subtype (version) of next segment in chunk (0xff if no more segments in
 * chunk)
 */
unsigned char mvefile_get_next_segment_minor(const MVEFILE *movie);

/*
 * see next segment (return NULL if no next segment)
 */
const unsigned char *mvefile_get_next_segment(const MVEFILE *movie);

/*
 * advance to next segment
 */
void mvefile_advance_segment(MVEFILE *movie);

/*
 * fetch the next chunk (return 0 if at end of stream)
 */
int mvefile_fetch_next_chunk(MVEFILE *movie);

/*
 * callback for segment type
 */
typedef int (*MVESEGMENTHANDLER)(mve_opcode major, unsigned char minor, const unsigned char *data, int len, void *context);

/*
 * structure for maintaining an MVE stream
 */
struct MVESTREAM
{
	enum class handle_result : bool
	{
		step_finished,
		step_again,
	};
	MVESTREAM();
	~MVESTREAM();
	std::unique_ptr<MVEFILE> movie;
	std::span<const uint8_t> pCurMap{};
	std::vector<unsigned char> vBuffers{};
	std::unique_ptr<SDL_AudioSpec> mve_audio_spec;
	bool mve_audio_playing{};
	uint8_t timer_created{};
	uint8_t truecolor{};
	uint8_t videobuf_created{};
	uint16_t screenWidth{};
	uint16_t screenHeight{};
	unsigned char *vBackBuf1{};
	unsigned char *vBackBuf2{};
	std::array<::dcx::unique_span<int16_t>, 64> mve_audio_buffers;

	handle_result handle_mve_segment_endofstream();
	handle_result handle_mve_segment_endofchunk();
	handle_result handle_mve_segment_createtimer(const unsigned char *data);
	handle_result handle_mve_segment_initaudiobuffers(unsigned char minor, const unsigned char *data);
	handle_result handle_mve_segment_startstopaudio();
	handle_result handle_mve_segment_initvideobuffers(unsigned char minor, const unsigned char *data);
	handle_result handle_mve_segment_displayvideo();
	handle_result handle_mve_segment_audioframedata(mve_opcode major, const unsigned char *data);
	handle_result handle_mve_segment_initvideomode(const unsigned char *data);
	handle_result handle_mve_segment_setpalette(const unsigned char *data);
	handle_result handle_mve_segment_setdecodingmap(const unsigned char *data, int len);
	handle_result handle_mve_segment_videodata(const unsigned char *data, int len);
};

struct MVESTREAM_deleter_t
{
	void operator()(MVESTREAM *p) const
	{
		MVE_rmEndMovie(std::unique_ptr<MVESTREAM>(p));
	}
};

typedef std::unique_ptr<MVESTREAM, MVESTREAM_deleter_t> MVESTREAM_ptr_t;
MVESTREAM_ptr_t MVE_rmPrepMovie(RWops_ptr stream, int x, int y);

/*
 * open an MVE stream
 */
MVESTREAM_ptr_t mve_open(RWops_ptr stream);

/*
 * reset an MVE stream
 */
void mve_reset(MVESTREAM *movie);

/*
 * play next chunk
 */
MVESTREAM::handle_result mve_play_next_chunk(MVESTREAM &movie);

unsigned MovieFileRead(SDL_RWops *handle, std::span<uint8_t> buf);

}
