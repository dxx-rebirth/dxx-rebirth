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

#ifdef __cplusplus
#include <cstdint>
#include <vector>
#include "dxxsconf.h"
#include <array>

/*
 * structure for maintaining info on a MVEFILE stream
 */
struct MVEFILE
{
	MVEFILE();
	~MVEFILE();
	void *stream = nullptr;
	std::vector<uint8_t> cur_chunk;
	std::size_t next_segment = 0;
};

/*
 * open a .MVE file
 */
std::unique_ptr<MVEFILE> mvefile_open(void *stream);

/*
 * get size of next segment in chunk (-1 if no more segments in chunk)
 */
int_fast32_t mvefile_get_next_segment_size(const MVEFILE *movie);

/*
 * get type of next segment in chunk (0xff if no more segments in chunk)
 */
unsigned char mvefile_get_next_segment_major(const MVEFILE *movie);

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
typedef int (*MVESEGMENTHANDLER)(unsigned char major, unsigned char minor, const unsigned char *data, int len, void *context);

/*
 * structure for maintaining an MVE stream
 */
struct MVESTREAM
{
	MVESTREAM();
	~MVESTREAM();
	std::unique_ptr<MVEFILE> movie;
	void *context = nullptr;
	std::array<MVESEGMENTHANDLER, 32> handlers = {};
};

struct MVESTREAM_deleter_t
{
	void operator()(MVESTREAM *p) const
	{
		MVE_rmEndMovie(std::unique_ptr<MVESTREAM>(p));
	}
};

typedef std::unique_ptr<MVESTREAM, MVESTREAM_deleter_t> MVESTREAM_ptr_t;
int  MVE_rmPrepMovie(MVESTREAM_ptr_t &, void *stream, int x, int y, int track);

/*
 * open an MVE stream
 */
MVESTREAM_ptr_t mve_open(void *stream);

/*
 * reset an MVE stream
 */
void mve_reset(MVESTREAM *movie);

/*
 * set segment type handler
 */
void mve_set_handler(MVESTREAM &movie, unsigned char major, MVESEGMENTHANDLER handler);

/*
 * set segment handler context
 */
void mve_set_handler_context(MVESTREAM *movie, void *context);

/*
 * play next chunk
 */
int mve_play_next_chunk(MVESTREAM &movie);

#endif
