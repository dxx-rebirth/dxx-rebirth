/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#include <string.h> // for mem* functions
#if !defined(_WIN32) && !defined(macintosh)
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include "mvelib.h"
#include <memory>

static const char  MVE_HEADER[]  = "Interplay MVE File\x1A";
constexpr short MVE_HDRCONST1 = 0x001A;
constexpr short MVE_HDRCONST2 = 0x0100;
constexpr short MVE_HDRCONST3 = 0x1133;

/*
 * private utility functions
 */
static int16_t _mve_get_short(const unsigned char *data);
static uint16_t _mve_get_ushort(const unsigned char *data);

/*
 * private functions for mvefile
 */
static int _mvefile_open(MVEFILE *movie, void *stream);
static int  _mvefile_read_header(const MVEFILE *movie);
static void _mvefile_set_buffer_size(MVEFILE *movie, std::size_t buf_size);
static int _mvefile_fetch_next_chunk(MVEFILE *movie);

/*
 * private functions for mvestream
 */
static int _mvestream_open(MVESTREAM *movie, void *stream);
static void _mvestream_reset(MVESTREAM *movie);

/************************************************************
 * public MVEFILE functions
 ************************************************************/

/*
 * open an MVE file
 */
std::unique_ptr<MVEFILE> mvefile_open(void *stream)
{
    /* create the file */
	auto file = std::make_unique<MVEFILE>();
	if (! _mvefile_open(file.get(), stream))
    {
		return nullptr;
    }

    /* initialize the file */
	_mvefile_set_buffer_size(file.get(), 1024);

    /* verify the file's header */
	if (! _mvefile_read_header(file.get()))
    {
		return nullptr;
    }

    /* now, prefetch the next chunk */
	_mvefile_fetch_next_chunk(file.get());
    return file;
}

/*
 * reset a MVE file
 */
static void mvefile_reset(MVEFILE *file)
{
    /* initialize the file */
    _mvefile_set_buffer_size(file, 1024);

    /* verify the file's header */
    if (! _mvefile_read_header(file))
    {
		*file = {};
    }

    /* now, prefetch the next chunk */
    _mvefile_fetch_next_chunk(file);
}

static bool have_segment_header(const MVEFILE *movie)
{
	/* if nothing is cached, fail */
	if (movie->next_segment >= movie->cur_chunk.size())
		return false;
	/* if we don't have enough data to get a segment, fail */
	if (movie->cur_chunk.size() - movie->next_segment <= 4)
		return false;
	return true;
}

/*
 * get the size of the next segment
 */
int_fast32_t mvefile_get_next_segment_size(const MVEFILE *movie)
{
	if (!have_segment_header(movie))
        return -1;
    /* otherwise, get the data length */
    return _mve_get_short(&movie->cur_chunk[movie->next_segment]);
}

/*
 * get type of next segment in chunk (0xff if no more segments in chunk)
 */
unsigned char mvefile_get_next_segment_major(const MVEFILE *movie)
{
	if (!have_segment_header(movie))
        return 0xff;
    /* otherwise, get the data length */
    return movie->cur_chunk[movie->next_segment + 2];
}

/*
 * get subtype (version) of next segment in chunk (0xff if no more segments in
 * chunk)
 */
unsigned char mvefile_get_next_segment_minor(const MVEFILE *movie)
{
	if (!have_segment_header(movie))
        return 0xff;
    /* otherwise, get the data length */
    return movie->cur_chunk[movie->next_segment + 3];
}

/*
 * see next segment (return NULL if no next segment)
 */
const unsigned char *mvefile_get_next_segment(const MVEFILE *movie)
{
	if (!have_segment_header(movie))
        return NULL;

    /* otherwise, get the data length */
    return &movie->cur_chunk[movie->next_segment + 4];
}

/*
 * advance to next segment
 */
void mvefile_advance_segment(MVEFILE *movie)
{
    if (!have_segment_header(movie))
        return;
    /* else, advance to next segment */
    movie->next_segment +=
        (4 + _mve_get_ushort(&movie->cur_chunk[movie->next_segment]));
}

/*
 * fetch the next chunk (return 0 if at end of stream)
 */
int mvefile_fetch_next_chunk(MVEFILE *movie)
{
    return _mvefile_fetch_next_chunk(movie);
}

/************************************************************
 * public MVESTREAM functions
 ************************************************************/

/*
 * open an MVE stream
 */
MVESTREAM_ptr_t mve_open(void *stream)
{
    /* allocate */
	auto movie = std::make_unique<MVESTREAM>();

    /* open */
    if (! _mvestream_open(movie.get(), stream))
    {
        return nullptr;
    }
    return MVESTREAM_ptr_t(movie.release());
}

/*
 * reset an MVE stream
 */
void mve_reset(MVESTREAM *movie)
{
	_mvestream_reset(movie);
}

/*
 * set segment type handler
 */
void mve_set_handler(MVESTREAM &movie, unsigned char major, MVESEGMENTHANDLER handler)
{
    if (major < 32)
        movie.handlers[major] = handler;
}

/*
 * set segment handler context
 */
void mve_set_handler_context(MVESTREAM *movie, void *context)
{
    movie->context = context;
}

/*
 * play next chunk
 */
int mve_play_next_chunk(MVESTREAM &movie)
{
	const auto m = movie.movie.get();
    /* loop over segments */
        /* advance to next segment */
	for (;; mvefile_advance_segment(m))
    {
		const auto major = mvefile_get_next_segment_major(m);
		if (major == 0xff)
			break;
		if (major >= movie.handlers.size())
			continue;
        /* check whether to handle the segment */
		if (const auto handler = movie.handlers[major])
        {
			const auto minor = mvefile_get_next_segment_minor(m);
			const auto len = mvefile_get_next_segment_size(m);
			const auto data = mvefile_get_next_segment(m);

            if (!handler(major, minor, data, len, movie.context))
                return 0;
        }
    }

	if (!mvefile_fetch_next_chunk(m))
        return 0;

    /* return status */
    return 1;
}

/************************************************************
 * private functions
 ************************************************************/

/*
 * allocate an MVEFILE
 */
MVEFILE::MVEFILE()
{
}

/*
 * free an MVE file
 */
MVEFILE::~MVEFILE()
{
}

/*
 * open the file stream in thie object
 */
static int _mvefile_open(MVEFILE *file, void *stream)
{
    file->stream = stream;
    if (! file->stream)
        return 0;

    return 1;
}

/*
 * read and verify the header of the recently opened file
 */
static int _mvefile_read_header(const MVEFILE *movie)
{
    unsigned char buffer[26];

    /* check the file is open */
    if (! movie->stream)
        return 0;

    /* check the file is long enough */
	if (!MovieFileRead(movie->stream, buffer, 26))
        return 0;

    /* check the signature */
    if (memcmp(buffer, MVE_HEADER, 20))
        return 0;

    /* check the hard-coded constants */
    if (_mve_get_short(buffer+20) != MVE_HDRCONST1)
        return 0;
    if (_mve_get_short(buffer+22) != MVE_HDRCONST2)
        return 0;
    if (_mve_get_short(buffer+24) != MVE_HDRCONST3)
        return 0;

    return 1;
}

static void _mvefile_set_buffer_size(MVEFILE *movie, std::size_t buf_size)
{
    /* check if this would be a redundant operation */
	if (buf_size <= movie->cur_chunk.size())
        return;

    /* allocate new buffer */
    /* copy old data */
    /* free old buffer */
    /* install new buffer */
	movie->cur_chunk.resize(100 + buf_size);
}

static int _mvefile_fetch_next_chunk(MVEFILE *movie)
{
    unsigned char buffer[4];
    unsigned short length;

    /* fail if not open */
    if (! movie->stream)
        return 0;

    /* fail if we can't read the next segment descriptor */
	if (!MovieFileRead(movie->stream, buffer, 4))
        return 0;

    /* pull out the next length */
    length = _mve_get_short(buffer);

    /* make sure we've got sufficient space */
    _mvefile_set_buffer_size(movie, length);

    /* read the chunk */
	if (!MovieFileRead(movie->stream, &movie->cur_chunk[0], length))
        return 0;
    movie->cur_chunk.resize(length);
    movie->next_segment = 0;

    return 1;
}

static int16_t _mve_get_short(const unsigned char *data)
{
    short value;
    value = data[0] | (data[1] << 8);
    return value;
}

static uint16_t _mve_get_ushort(const unsigned char *data)
{
    unsigned short value;
    value = data[0] | (data[1] << 8);
    return value;
}

/*
 * allocate an MVESTREAM
 */
MVESTREAM::MVESTREAM()
    /* allocate and zero-initialize everything */
{
}

MVESTREAM::~MVESTREAM()
{
}

/*
 * open an MVESTREAM object
 */
static int _mvestream_open(MVESTREAM *movie, void *stream)
{
    movie->movie = mvefile_open(stream);

    return (movie->movie == NULL) ? 0 : 1;
}

/*
 * reset an MVESTREAM
 */
static void _mvestream_reset(MVESTREAM *movie)
{
	mvefile_reset(movie->movie.get());
}
