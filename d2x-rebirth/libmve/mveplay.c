//#define DEBUG

#include <string.h>
#ifdef _WIN32
# include <windows.h>
#else
# include <errno.h>
# include <time.h>
# include <fcntl.h>
# ifdef macintosh
#  include <types.h>
#  include <OSUtils.h>
# else
#  include <sys/time.h>
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <unistd.h>
# endif // macintosh
#endif // _WIN32

#include <SDL/SDL.h>
#ifdef USE_SDLMIXER
#if !(defined(__APPLE__) && defined(__MACH__))
#include <SDL/SDL_mixer.h>
#else
#include <SDL_mixer/SDL_mixer.h>
#endif
#endif

#include "digi.h"
#include "mvelib.h"
#include "mve_audio.h"
#include "decoders.h"
#include "libmve.h"
#include "args.h"
#include "console.h"

#define MVE_OPCODE_ENDOFSTREAM          0x00
#define MVE_OPCODE_ENDOFCHUNK           0x01
#define MVE_OPCODE_CREATETIMER          0x02
#define MVE_OPCODE_INITAUDIOBUFFERS     0x03
#define MVE_OPCODE_STARTSTOPAUDIO       0x04
#define MVE_OPCODE_INITVIDEOBUFFERS     0x05

#define MVE_OPCODE_DISPLAYVIDEO         0x07
#define MVE_OPCODE_AUDIOFRAMEDATA       0x08
#define MVE_OPCODE_AUDIOFRAMESILENCE    0x09
#define MVE_OPCODE_INITVIDEOMODE        0x0A

#define MVE_OPCODE_SETPALETTE           0x0C
#define MVE_OPCODE_SETPALETTECOMPRESSED 0x0D

#define MVE_OPCODE_SETDECODINGMAP       0x0F

#define MVE_OPCODE_VIDEODATA            0x11

#define MVE_AUDIO_FLAGS_STEREO     1
#define MVE_AUDIO_FLAGS_16BIT      2
#define MVE_AUDIO_FLAGS_COMPRESSED 4

int g_spdFactorNum=0;
static int g_spdFactorDenom=10;
static int g_frameUpdated = 0;

static short get_short(unsigned char *data)
{
	short value;
	value = data[0] | (data[1] << 8);
	return value;
}

static unsigned short get_ushort(unsigned char *data)
{
	unsigned short value;
	value = data[0] | (data[1] << 8);
	return value;
}

static int get_int(unsigned char *data)
{
	int value;
	value = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
	return value;
}

static unsigned int unhandled_chunks[32*256];

static int default_seg_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	unhandled_chunks[major<<8|minor]++;
	//con_printf(CON_CRITICAL, "unknown chunk type %02x/%02x\n", major, minor);
	return 1;
}


/*************************
 * general handlers
 *************************/
static int end_movie_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	return 0;
}

/*************************
 * timer handlers
 *************************/

#if !defined(HAVE_STRUCT_TIMEVAL) || !HAVE_STRUCT_TIMEVAL
struct timeval
{
	long tv_sec;
	long tv_usec;
};
#endif

/*
 * timer variables
 */
static int timer_created = 0;
static int micro_frame_delay=0;
static int timer_started=0;
static struct timeval timer_expire = {0, 0};

#if !defined(HAVE_STRUCT_TIMESPEC) || !HAVE_STRUCT_TIMESPEC
struct timespec
{
	long int tv_sec;            /* Seconds.  */
	long int tv_nsec;           /* Nanoseconds.  */
};
#endif

#if defined(_WIN32) || defined(macintosh)
int gettimeofday(struct timeval *tv, void *tz)
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


static int create_timer_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{

#if !defined(_WIN32) && !defined(macintosh) // FIXME
	__extension__ long long temp;
#else
	long temp;
#endif

	if (timer_created)
		return 1;
	else
		timer_created = 1;

	micro_frame_delay = get_int(data) * (int)get_short(data+4);
	if (g_spdFactorNum != 0)
	{
		temp = micro_frame_delay;
		temp *= g_spdFactorNum;
		temp /= g_spdFactorDenom;
		micro_frame_delay = (int)temp;
	}

	return 1;
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
#define TOTAL_AUDIO_BUFFERS 64

static int audiobuf_created = 0;
static void mve_audio_callback(void *userdata, unsigned char *stream, int len);
static short *mve_audio_buffers[TOTAL_AUDIO_BUFFERS];
static int    mve_audio_buflens[TOTAL_AUDIO_BUFFERS];
static int    mve_audio_curbuf_curpos=0;
static int mve_audio_bufhead=0;
static int mve_audio_buftail=0;
static int mve_audio_playing=0;
static int mve_audio_canplay=0;
static int mve_audio_compressed=0;
static int mve_audio_enabled = 1;
static SDL_AudioSpec *mve_audio_spec=NULL;

static void mve_audio_callback(void *userdata, unsigned char *stream, int len)
{
	int total=0;
	int length;
	if (mve_audio_bufhead == mve_audio_buftail)
		return /* 0 */;

	//con_printf(CON_CRITICAL, "+ <%d (%d), %d, %d>\n", mve_audio_bufhead, mve_audio_curbuf_curpos, mve_audio_buftail, len);

	while (mve_audio_bufhead != mve_audio_buftail                                           /* while we have more buffers  */
		   &&  len > (mve_audio_buflens[mve_audio_bufhead]-mve_audio_curbuf_curpos))        /* and while we need more data */
	{
		length = mve_audio_buflens[mve_audio_bufhead]-mve_audio_curbuf_curpos;
		memcpy(stream,                                                                  /* cur output position */
		       ((unsigned char *)mve_audio_buffers[mve_audio_bufhead])+mve_audio_curbuf_curpos,           /* cur input position  */
		       length);                                                                 /* cur input length    */

		total += length;
		stream += length;                                                               /* advance output */
		len -= length;                                                                  /* decrement avail ospace */
		mve_free(mve_audio_buffers[mve_audio_bufhead]);                                 /* free the buffer */
		mve_audio_buffers[mve_audio_bufhead]=NULL;                                      /* free the buffer */
		mve_audio_buflens[mve_audio_bufhead]=0;                                         /* free the buffer */

		if (++mve_audio_bufhead == TOTAL_AUDIO_BUFFERS)                                 /* next buffer */
			mve_audio_bufhead = 0;
		mve_audio_curbuf_curpos = 0;
	}

	//con_printf(CON_CRITICAL, "= <%d (%d), %d, %d>: %d\n", mve_audio_bufhead, mve_audio_curbuf_curpos, mve_audio_buftail, len, total);
	/*    return total; */

	if (len != 0                                                                        /* ospace remaining  */
		&&  mve_audio_bufhead != mve_audio_buftail)                                     /* buffers remaining */
	{
		memcpy(stream,                                                                  /* dest */
			   ((unsigned char *)mve_audio_buffers[mve_audio_bufhead]) + mve_audio_curbuf_curpos,         /* src */
			   len);                                                                    /* length */

		mve_audio_curbuf_curpos += len;                                                 /* advance input */
		stream += len;                                                                  /* advance output (unnecessary) */
		len -= len;                                                                     /* advance output (unnecessary) */

		if (mve_audio_curbuf_curpos >= mve_audio_buflens[mve_audio_bufhead])            /* if this ends the current chunk */
		{
			mve_free(mve_audio_buffers[mve_audio_bufhead]);                             /* free buffer */
			mve_audio_buffers[mve_audio_bufhead]=NULL;
			mve_audio_buflens[mve_audio_bufhead]=0;

			if (++mve_audio_bufhead == TOTAL_AUDIO_BUFFERS)                             /* next buffer */
				mve_audio_bufhead = 0;
			mve_audio_curbuf_curpos = 0;
		}
	}

	//con_printf(CON_CRITICAL, "- <%d (%d), %d, %d>\n", mve_audio_bufhead, mve_audio_curbuf_curpos, mve_audio_buftail, len);
}

static int create_audiobuf_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	int flags;
	int sample_rate;
	int desired_buffer;

	int stereo;
	int bitsize;
	int compressed;

	int format;

	if (!mve_audio_enabled)
		return 1;

	if (audiobuf_created)
		return 1;
	else
		audiobuf_created = 1;

	flags = get_ushort(data + 2);
	sample_rate = get_ushort(data + 4);
	desired_buffer = get_int(data + 6);

	stereo = (flags & MVE_AUDIO_FLAGS_STEREO) ? 1 : 0;
	bitsize = (flags & MVE_AUDIO_FLAGS_16BIT) ? 1 : 0;

	if (minor > 0) {
		compressed = flags & MVE_AUDIO_FLAGS_COMPRESSED ? 1 : 0;
	} else {
		compressed = 0;
	}

	mve_audio_compressed = compressed;

	if (bitsize == 1) {
#ifdef WORDS_BIGENDIAN
		format = AUDIO_S16MSB;
#else
		format = AUDIO_S16LSB;
#endif
	} else {
		format = AUDIO_U8;
	}

#ifdef USE_SDLMIXER
	if (GameArg.SndDisableSdlMixer)
#endif
	{
		con_printf(CON_CRITICAL, "creating audio buffers:\n");
		con_printf(CON_CRITICAL, "sample rate = %d, desired buffer = %d, stereo = %d, bitsize = %d, compressed = %d\n",
				sample_rate, desired_buffer, stereo, bitsize ? 16 : 8, compressed);
	}

	mve_audio_spec = (SDL_AudioSpec *)mve_alloc(sizeof(SDL_AudioSpec));
	mve_audio_spec->freq = sample_rate;
	mve_audio_spec->format = format;
	mve_audio_spec->channels = (stereo) ? 2 : 1;
	mve_audio_spec->samples = 4096;
	mve_audio_spec->callback = mve_audio_callback;
	mve_audio_spec->userdata = NULL;

	// MD2211: if using SDL_Mixer, we never reinit the sound system
#ifdef USE_SDLMIXER
	if (GameArg.SndDisableSdlMixer)
#endif
	{
		if (SDL_OpenAudio(mve_audio_spec, NULL) >= 0) {
			con_printf(CON_CRITICAL, "   success\n");
			mve_audio_canplay = 1;
		}
		else {
			con_printf(CON_CRITICAL, "   failure : %s\n", SDL_GetError());
			mve_audio_canplay = 0;
		}
	}

#ifdef USE_SDLMIXER
	else {
		// MD2211: using the same old SDL audio callback as a postmixer in SDL_mixer
		Mix_SetPostMix(mve_audio_spec->callback, mve_audio_spec->userdata);
		mve_audio_canplay = 1;
	}
#endif

	memset(mve_audio_buffers, 0, sizeof(mve_audio_buffers));
	memset(mve_audio_buflens, 0, sizeof(mve_audio_buflens));

	return 1;
}

static int play_audio_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	if (mve_audio_canplay  &&  !mve_audio_playing  &&  mve_audio_bufhead != mve_audio_buftail)
	{
#ifdef USE_SDLMIXER
		if (GameArg.SndDisableSdlMixer)
#endif
			SDL_PauseAudio(0);
#ifdef USE_SDLMIXER
		else
			Mix_Pause(0);
#endif
		mve_audio_playing = 1;
	}
	return 1;
}

static int audio_data_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{

#ifdef USE_SDLMIXER
	// MD2211: for audio conversion
	SDL_AudioCVT cvt;
	int clen;
	int out_freq;
	Uint16 out_format;
	int out_channels;
	// end MD2211
#endif

	static const int selected_chan=1;
	int chan;
	int nsamp;
	if (mve_audio_canplay)
	{
		if (mve_audio_playing)
			SDL_LockAudio();

		chan = get_ushort(data + 2);
		nsamp = get_ushort(data + 4);
		if (chan & selected_chan)
		{
			/* HACK: +4 mveaudio_uncompress adds 4 more bytes */
			if (major == MVE_OPCODE_AUDIOFRAMEDATA) {
				if (mve_audio_compressed) {
					nsamp += 4;

					mve_audio_buflens[mve_audio_buftail] = nsamp;
					mve_audio_buffers[mve_audio_buftail] = (short *)mve_alloc(nsamp);
					mveaudio_uncompress(mve_audio_buffers[mve_audio_buftail], data, -1); /* XXX */
				} else {
					nsamp -= 8;
					data += 8;

					mve_audio_buflens[mve_audio_buftail] = nsamp;
					mve_audio_buffers[mve_audio_buftail] = (short *)mve_alloc(nsamp);
					memcpy(mve_audio_buffers[mve_audio_buftail], data, nsamp);
				}
			} else {
				mve_audio_buflens[mve_audio_buftail] = nsamp;
				mve_audio_buffers[mve_audio_buftail] = (short *)mve_alloc(nsamp);

				memset(mve_audio_buffers[mve_audio_buftail], 0, nsamp); /* XXX */
			}

			// MD2211: the following block does on-the-fly audio conversion for SDL_mixer
#ifdef USE_SDLMIXER
			if (!GameArg.SndDisableSdlMixer) {
				// build converter: in = MVE format, out = SDL_mixer output
				Mix_QuerySpec(&out_freq, &out_format, &out_channels); // get current output settings

				SDL_BuildAudioCVT(&cvt, mve_audio_spec->format, mve_audio_spec->channels, mve_audio_spec->freq,
					out_format, out_channels, out_freq);

				clen = nsamp * cvt.len_mult;
				cvt.buf = malloc(clen);
				cvt.len = nsamp;

				// read the audio buffer into the conversion buffer
				memcpy(cvt.buf, mve_audio_buffers[mve_audio_buftail], nsamp);

				// do the conversion
				if (SDL_ConvertAudio(&cvt)) con_printf(CON_DEBUG,"audio conversion failed!\n");

				// copy back to the audio buffer
				mve_free(mve_audio_buffers[mve_audio_buftail]); // free the old audio buffer
				mve_audio_buflens[mve_audio_buftail] = clen;
				mve_audio_buffers[mve_audio_buftail] = (short *)mve_alloc(clen);
				memcpy(mve_audio_buffers[mve_audio_buftail], cvt.buf, clen);
			}
#endif

			if (++mve_audio_buftail == TOTAL_AUDIO_BUFFERS)
				mve_audio_buftail = 0;

			if (mve_audio_buftail == mve_audio_bufhead)
				con_printf(CON_CRITICAL, "d'oh!  buffer ring overrun (%d)\n", mve_audio_bufhead);
		}

		if (mve_audio_playing)
			SDL_UnlockAudio();
	}

	return 1;
}

/*************************
 * video handlers
 *************************/

static int videobuf_created = 0;
static int video_initialized = 0;
int g_width, g_height;
void *g_vBuffers = NULL, *g_vBackBuf1, *g_vBackBuf2;

static int g_destX, g_destY;
static int g_screenWidth, g_screenHeight;
static unsigned char *g_pCurMap=NULL;
static int g_nMapLength=0;
static int g_truecolor;

static int create_videobuf_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	short w, h,
#ifdef DEBUG
		count, 
#endif
		truecolor;

	if (videobuf_created)
		return 1;
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
		truecolor = get_short(data+6);
	} else {
		truecolor = 0;
	}

	g_width = w << 3;
	g_height = h << 3;

	/* TODO: * 4 causes crashes on some files */
	/* only malloc once */
	if (g_vBuffers == NULL)
		g_vBackBuf1 = g_vBuffers = mve_alloc(g_width * g_height * 8);
	if (truecolor) {
		g_vBackBuf2 = (unsigned short *)g_vBackBuf1 + (g_width * g_height);
	} else {
		g_vBackBuf2 = (unsigned char *)g_vBackBuf1 + (g_width * g_height);
	}

	memset(g_vBackBuf1, 0, g_width * g_height * 4);

#ifdef DEBUG
	con_printf(CON_CRITICAL, "DEBUG: w,h=%d,%d count=%d, tc=%d\n", w, h, count, truecolor);
#endif

	g_truecolor = truecolor;

	return 1;
}

static int display_video_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	mve_showframe(g_vBackBuf1, g_destX, g_destY, g_width, g_height, g_screenWidth, g_screenHeight);

	g_frameUpdated = 1;

	return 1;
}

static int init_video_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	short width, height;

	if (video_initialized)
		return 1; /* maybe we actually need to change width/height here? */
	else
		video_initialized = 1;

	width = get_short(data);
	height = get_short(data+2);
	g_screenWidth = width;
	g_screenHeight = height;

	return 1;
}

static int video_palette_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	short start, count;
	unsigned char *p;

	start = get_short(data);
	count = get_short(data+2);

	p = data + 4;

	mve_setpalette(p - 3*start, start, count);

	return 1;
}

static int video_codemap_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	g_pCurMap = data;
	g_nMapLength = len;
	return 1;
}

static int video_data_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	unsigned short nFlags;
	unsigned char *temp;

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
		temp = (unsigned char *)g_vBackBuf1;
		g_vBackBuf1 = g_vBackBuf2;
		g_vBackBuf2 = temp;
	}

	/* convert the frame */
	if (g_truecolor) {
		decodeFrame16((unsigned char *)g_vBackBuf1, g_pCurMap, g_nMapLength, data+14, len-14);
	} else {
		decodeFrame8(g_vBackBuf1, g_pCurMap, g_nMapLength, data+14, len-14);
	}

	return 1;
}

static int end_chunk_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	g_pCurMap=NULL;
	return 1;
}


static MVESTREAM *mve = NULL;

void MVE_ioCallbacks(mve_cb_Read io_read)
{
	mve_read = io_read;
}

void MVE_memCallbacks(mve_cb_Alloc mem_alloc, mve_cb_Free mem_free)
{
	mve_alloc = mem_alloc;
	mve_free = mem_free;
}

void MVE_sfCallbacks(mve_cb_ShowFrame showframe)
{
	mve_showframe = showframe;
}

void MVE_palCallbacks(mve_cb_SetPalette setpalette)
{
	mve_setpalette = setpalette;
}

int MVE_rmPrepMovie(void *src, int x, int y, int track)
{
	int i;

	if (mve) {
		mve_reset(mve);
		return 0;
	}

	mve = mve_open(src);

	if (!mve)
		return 1;

	g_destX = x;
	g_destY = y;

	for (i = 0; i < 32; i++)
		mve_set_handler(mve, i, default_seg_handler);

	mve_set_handler(mve, MVE_OPCODE_ENDOFSTREAM,          end_movie_handler);
	mve_set_handler(mve, MVE_OPCODE_ENDOFCHUNK,           end_chunk_handler);
	mve_set_handler(mve, MVE_OPCODE_CREATETIMER,          create_timer_handler);
	mve_set_handler(mve, MVE_OPCODE_INITAUDIOBUFFERS,     create_audiobuf_handler);
	mve_set_handler(mve, MVE_OPCODE_STARTSTOPAUDIO,       play_audio_handler);
	mve_set_handler(mve, MVE_OPCODE_INITVIDEOBUFFERS,     create_videobuf_handler);

	mve_set_handler(mve, MVE_OPCODE_DISPLAYVIDEO,         display_video_handler);
	mve_set_handler(mve, MVE_OPCODE_AUDIOFRAMEDATA,       audio_data_handler);
	mve_set_handler(mve, MVE_OPCODE_AUDIOFRAMESILENCE,    audio_data_handler);
	mve_set_handler(mve, MVE_OPCODE_INITVIDEOMODE,        init_video_handler);

	mve_set_handler(mve, MVE_OPCODE_SETPALETTE,           video_palette_handler);
	mve_set_handler(mve, MVE_OPCODE_SETPALETTECOMPRESSED, default_seg_handler);

	mve_set_handler(mve, MVE_OPCODE_SETDECODINGMAP,       video_codemap_handler);

	mve_set_handler(mve, MVE_OPCODE_VIDEODATA,            video_data_handler);

	mve_play_next_chunk(mve); /* video initialization chunk */
	mve_play_next_chunk(mve); /* audio initialization chunk */

	return 0;
}


void MVE_getVideoSpec(MVE_videoSpec *vSpec)
{
	vSpec->screenWidth = g_screenWidth;
	vSpec->screenHeight = g_screenHeight;
	vSpec->width = g_width;
	vSpec->height = g_height;
	vSpec->truecolor = g_truecolor;
}


int MVE_rmStepMovie()
{
	static int init_timer=0;
	int cont=1;

	if (!timer_started)
		timer_start();

	while (cont && !g_frameUpdated) // make a "step" be a frame, not a chunk...
		cont = mve_play_next_chunk(mve);
	g_frameUpdated = 0;

	if (!cont)
		return MVE_ERR_EOF;

	if (micro_frame_delay  && !init_timer) {
		timer_start();
		init_timer = 1;
	}

	do_timer_wait();

	return 0;
}

void MVE_rmEndMovie()
{
	int i;

	timer_stop();
	timer_created = 0;

	if (mve_audio_canplay) {
		// MD2211: if using SDL_Mixer, we never reinit sound, hence never close it
#ifdef USE_SDLMIXER
		if (GameArg.SndDisableSdlMixer)
#endif
		{
			SDL_CloseAudio();
		}
		mve_audio_canplay = 0;
	}
	for (i = 0; i < TOTAL_AUDIO_BUFFERS; i++)
		if (mve_audio_buffers[i] != NULL)
			mve_free(mve_audio_buffers[i]);

	memset(mve_audio_buffers, 0, sizeof(mve_audio_buffers));
	memset(mve_audio_buflens, 0, sizeof(mve_audio_buflens));

	mve_audio_curbuf_curpos=0;
	mve_audio_bufhead=0;
	mve_audio_buftail=0;
	mve_audio_playing=0;
	mve_audio_canplay=0;
	mve_audio_compressed=0;

	if (mve_audio_spec)
		mve_free(mve_audio_spec);
	mve_audio_spec=NULL;
	audiobuf_created = 0;

	mve_free(g_vBuffers);
	g_vBuffers = NULL;
	g_pCurMap=NULL;
	g_nMapLength=0;
	videobuf_created = 0;
	video_initialized = 0;

	mve_close(mve);
	mve = NULL;
}


void MVE_rmHoldMovie()
{
	timer_started = 0;
}


void MVE_sndInit(int x)
{
	mve_audio_enabled = (x == -1 ? 0 : 1);
}
