#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

#include "mveplay.h"
#include "error.h"
#include "u_mem.h"
#include "mvelib.h"
#include "mve_audio.h"
#include "gr.h"
#include "palette.h"
#include "fix.h"
#include "timer.h"

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

//#define DEBUG

static int g_spdFactorNum=0;
static int g_spdFactorDenom=10;

static short get_short(unsigned char *data)
{
    short value;
    value = data[0] | (data[1] << 8);
    return value;
}

static int get_int(unsigned char *data)
{
    int value;
    value = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    return value;
}

static int default_seg_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    Warning("mveplay: unknown chunk type %02x/%02x\n", major, minor);
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

/*
 * timer variables
 */
static fix fix_frame_delay = F0_0;
static int timer_started   = 0;
static fix timer_expire    = F0_0;

static int create_timer_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    long long temp;
    int micro_frame_delay = get_int(data) * (int)get_short(data+4);

    if (g_spdFactorNum != 0)
    {
        temp = micro_frame_delay;
        temp *= g_spdFactorNum;
        temp /= g_spdFactorDenom;
        micro_frame_delay = (int)temp;
    }
	fix_frame_delay = approx_usec_to_fsec(micro_frame_delay);

    return 1;
}

static void timer_stop(void)
{
	timer_expire = F0_0;
	timer_started = 0;
}

static void timer_start(void)
{
	timer_expire = timer_get_fixed_seconds();

	timer_expire += fix_frame_delay;

    timer_started=1;
}

static void do_timer_wait(void)
{
	fix ts, tv;

    if (! timer_started)
        return;

	tv = timer_get_fixed_seconds();

	if (tv > timer_expire)
		goto end;

	ts = timer_expire - tv;

	timer_delay(ts);

end:
	timer_expire += fix_frame_delay;
}

/*************************
 * audio handlers
 *************************/
static void mve_audio_callback(void *userdata, Uint8 *stream, int len);
static short *mve_audio_buffers[64];
static int    mve_audio_buflens[64];
static int    mve_audio_curbuf_curpos=0;
static int mve_audio_bufhead=0;
static int mve_audio_buftail=0;
static int mve_audio_playing=0;
static int mve_audio_canplay=0;
static int mve_audio_compressed=0;
static SDL_AudioSpec *mve_audio_spec=NULL;
static SDL_mutex *mve_audio_mutex = NULL;

static void mve_audio_callback(void *userdata, Uint8 *stream, int len)
{
    int total=0;
    int length;
    if (mve_audio_bufhead == mve_audio_buftail)
        return /* 0 */;

#ifdef DEBUG
	fprintf(stderr, "+ <%d (%d), %d, %d>\n", mve_audio_bufhead, mve_audio_curbuf_curpos, mve_audio_buftail, len);
#endif

    SDL_mutexP(mve_audio_mutex);

    while (mve_audio_bufhead != mve_audio_buftail                                       /* while we have more buffers  */
            &&  len > (mve_audio_buflens[mve_audio_bufhead]-mve_audio_curbuf_curpos))   /* and while we need more data */
    {
        length = mve_audio_buflens[mve_audio_bufhead]-mve_audio_curbuf_curpos;
        __builtin_memcpy(stream,                                                        /* cur output position */
                mve_audio_buffers[mve_audio_bufhead]+mve_audio_curbuf_curpos,           /* cur input position  */
                length);                                                                /* cur input length    */

        total += length;
        stream += length;                                                               /* advance output */
        len -= length;                                                                  /* decrement avail ospace */
        d_free(mve_audio_buffers[mve_audio_bufhead]);                                   /* free the buffer */
        mve_audio_buffers[mve_audio_bufhead]=NULL;                                      /* free the buffer */
        mve_audio_buflens[mve_audio_bufhead]=0;                                         /* free the buffer */

        if (++mve_audio_bufhead == 64)                                                  /* next buffer */
            mve_audio_bufhead = 0;
        mve_audio_curbuf_curpos = 0;
    }

#ifdef DEBUG
	fprintf(stderr, "= <%d (%d), %d, %d>: %d\n", mve_audio_bufhead, mve_audio_curbuf_curpos, mve_audio_buftail, len, total);
#endif
/*    return total; */

    if (len != 0                                                                        /* ospace remaining  */
            &&  mve_audio_bufhead != mve_audio_buftail)                                 /* buffers remaining */
    {
        __builtin_memcpy(stream,                                                        /* dest */
                mve_audio_buffers[mve_audio_bufhead] + mve_audio_curbuf_curpos,         /* src */
                len);                                                                   /* length */

        mve_audio_curbuf_curpos += len;                                                 /* advance input */
        stream += len;                                                                  /* advance output (unnecessary) */
        len -= len;                                                                     /* advance output (unnecessary) */

        if (mve_audio_curbuf_curpos >= mve_audio_buflens[mve_audio_bufhead])            /* if this ends the current chunk */
        {
            d_free(mve_audio_buffers[mve_audio_bufhead]);                               /* free buffer */
            mve_audio_buffers[mve_audio_bufhead]=NULL;
            mve_audio_buflens[mve_audio_bufhead]=0;

            if (++mve_audio_bufhead == 64)                                              /* next buffer */
                mve_audio_bufhead = 0;
            mve_audio_curbuf_curpos = 0;
        }
    }

#ifdef DEBUG
	fprintf(stderr, "- <%d (%d), %d, %d>\n", mve_audio_bufhead, mve_audio_curbuf_curpos, mve_audio_buftail, len);
#endif
    SDL_mutexV(mve_audio_mutex);
}

static int create_audiobuf_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    int flags;
    int sample_rate;
    int desired_buffer;

#ifdef DEBUG
	fprintf(stderr, "creating audio buffers\n");
#endif

    mve_audio_mutex = SDL_CreateMutex();

    flags = get_short(data + 2);
    sample_rate = get_short(data + 4);
    desired_buffer = get_int(data + 6);

#ifdef DEBUG
	fprintf(stderr, "stereo=%d 16bit=%d compressed=%d sample_rate=%d desired_buffer=%d\n",
        flags & 1, (flags >> 1) & 1, (flags >> 2) & 1, sample_rate, desired_buffer);
#endif

    mve_audio_compressed = flags & MVE_AUDIO_FLAGS_COMPRESSED;
    mve_audio_spec = (SDL_AudioSpec *)d_malloc(sizeof(SDL_AudioSpec));
    mve_audio_spec->freq = sample_rate;
#ifdef WORDS_BIGENDIAN
    mve_audio_spec->format = (flags & MVE_AUDIO_FLAGS_16BIT)?AUDIO_S16MSB:AUDIO_U8;
#else
    mve_audio_spec->format = (flags & MVE_AUDIO_FLAGS_16BIT)?AUDIO_S16LSB:AUDIO_U8;
#endif
    mve_audio_spec->channels = (flags & MVE_AUDIO_FLAGS_STEREO)?2:1;
    mve_audio_spec->samples = 32768;
    mve_audio_spec->callback = mve_audio_callback;
    mve_audio_spec->userdata = NULL;
    if (SDL_OpenAudio(mve_audio_spec, NULL) >= 0)
    {
#ifdef DEBUG
		fprintf(stderr, "   success\n");
#endif
        mve_audio_canplay = 1;
    }
    else
    {
#ifdef DEBUG
		fprintf(stderr, "   failure : %s\n", SDL_GetError());
#endif
		Warning("mveplay: failed to create audio buffers\n");
        mve_audio_canplay = 0;
    }

    memset(mve_audio_buffers, 0, sizeof(mve_audio_buffers));
    memset(mve_audio_buflens, 0, sizeof(mve_audio_buflens));

    return 1;
}

static int play_audio_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    if (mve_audio_canplay  &&  !mve_audio_playing  &&  mve_audio_bufhead != mve_audio_buftail)
    {
        SDL_PauseAudio(0);
        mve_audio_playing = 1;
    }

    return 1;
}

static int audio_data_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    static const int selected_chan=1;
    int chan;
    int nsamp;
    if (mve_audio_canplay)
    {
        if (mve_audio_playing)
            SDL_LockAudio();

        chan = get_short(data + 2);
        nsamp = get_short(data + 4);
        if (chan & selected_chan)
        {
            SDL_mutexP(mve_audio_mutex);
            mve_audio_buflens[mve_audio_buftail] = nsamp;
            mve_audio_buffers[mve_audio_buftail] = (short *)d_malloc(nsamp+4);
            if (major == 8)
                if (mve_audio_compressed)
                    mveaudio_uncompress(mve_audio_buffers[mve_audio_buftail], data, -1); /* XXX */
                else
                    memcpy(mve_audio_buffers[mve_audio_buftail], data + 6, nsamp);
            else
                memset(mve_audio_buffers[mve_audio_buftail], 0, nsamp); /* XXX */

            if (++mve_audio_buftail == 64)
                mve_audio_buftail = 0;

            if (mve_audio_buftail == mve_audio_bufhead)
                Warning("mveplay: d'oh!  buffer ring overrun (%d)\n", mve_audio_bufhead);
            SDL_mutexV(mve_audio_mutex);
        }

        if (mve_audio_playing)
            SDL_UnlockAudio();
    }

    return 1;
}

/*************************
 * video handlers
 *************************/
static grs_bitmap *g_screen;
static int g_screenWidth, g_screenHeight;
static int g_width, g_height;
static unsigned char g_palette[768];
static unsigned char *g_vBackBuf1, *g_vBackBuf2;
static unsigned char *g_pCurMap=NULL;
static int g_nMapLength=0;

static int create_videobuf_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    short w, h;
    w = get_short(data);
    h = get_short(data+2);
    g_width = w << 3;
    g_height = h << 3;
#ifdef DEBUG
	fprintf(stderr, "g_width, g_height: %d, %d\n", g_width, g_height);
#endif
    Assert((g_width == g_screen->bm_w) && (g_height == g_screen->bm_h));
    g_vBackBuf1 = d_malloc(g_width * g_height);
    g_vBackBuf2 = d_malloc(g_width * g_height);
    memset(g_vBackBuf1, 0, g_width * g_height);
    memset(g_vBackBuf2, 0, g_width * g_height);
    return 1;
}

static int display_video_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    gr_palette_load(g_palette);

    memcpy(g_screen->bm_data, g_vBackBuf1, g_width * g_height);

    return 1;
}

static int init_video_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    short width, height;
    width = get_short(data);
    height = get_short(data+2);
    g_screenWidth = width;
    g_screenHeight = height;
    memset(g_palette, 0, 765);
	memset(g_palette + 765, 63, 3); // 255 should be white

    return 1;
}

static int video_palette_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    short start, count;
    start = get_short(data);
    count = get_short(data+2);
    memcpy(g_palette + 3*start, data+4, 3*count);
    return 1;
}

static int video_codemap_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    g_pCurMap = data;
    g_nMapLength = len;
    return 1;
}

static void decodeFrame(unsigned char *pFrame, unsigned char *pMap, int mapRemain, unsigned char *pData, int dataRemain);

static int video_data_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    short nFrameHot, nFrameCold;
    short nXoffset, nYoffset;
    short nXsize, nYsize;
    short nFlags;
    unsigned char *temp;

    nFrameHot  = get_short(data);
    nFrameCold = get_short(data+2);
    nXoffset   = get_short(data+4);
    nYoffset   = get_short(data+6);
    nXsize     = get_short(data+8);
    nYsize     = get_short(data+10);
    nFlags     = get_short(data+12);

    if (nFlags & 1)
    {
        temp = g_vBackBuf1;
        g_vBackBuf1 = g_vBackBuf2;
        g_vBackBuf2 = temp;
    }

    /* convert the frame */
    decodeFrame(g_vBackBuf1, g_pCurMap, g_nMapLength, data+14, len-14);

    return 1;
}

static int end_chunk_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    g_pCurMap=NULL;
    return 1;
}

/************************************************************
 * public mveplay functions
 ************************************************************/

void mveplay_initializeMovie(MVESTREAM *mve, grs_bitmap *mve_bitmap)
{
    g_screen = mve_bitmap;

    mve_set_handler(mve, 0x00, end_movie_handler);
    mve_set_handler(mve, 0x01, end_chunk_handler);
    mve_set_handler(mve, 0x02, create_timer_handler);
    mve_set_handler(mve, 0x03, create_audiobuf_handler);
    mve_set_handler(mve, 0x04, play_audio_handler);
    mve_set_handler(mve, 0x05, create_videobuf_handler);
    mve_set_handler(mve, 0x06, default_seg_handler);
    mve_set_handler(mve, 0x07, display_video_handler);
    mve_set_handler(mve, 0x08, audio_data_handler);
    mve_set_handler(mve, 0x09, audio_data_handler);
    mve_set_handler(mve, 0x0a, init_video_handler);
    mve_set_handler(mve, 0x0b, default_seg_handler);
    mve_set_handler(mve, 0x0c, video_palette_handler);
    mve_set_handler(mve, 0x0d, default_seg_handler);
    mve_set_handler(mve, 0x0e, default_seg_handler);
    mve_set_handler(mve, 0x0f, video_codemap_handler);
    mve_set_handler(mve, 0x10, default_seg_handler);
    mve_set_handler(mve, 0x11, video_data_handler);
    mve_set_handler(mve, 0x12, default_seg_handler);
    //mve_set_handler(mve, 0x13, default_seg_handler);
    mve_set_handler(mve, 0x14, default_seg_handler);
    //mve_set_handler(mve, 0x15, default_seg_handler);
}

int mveplay_stepMovie(MVESTREAM *mve)
{
    static int init_timer=0;
    int cont=1;

	if (!timer_started)
		timer_start();

	cont = mve_play_next_chunk(mve);
	if (fix_frame_delay  && !init_timer) {
		timer_start();
		init_timer = 1;
	}

	do_timer_wait();

	return cont;
}

void mveplay_restartTimer(MVESTREAM *mve)
{
	timer_start();
}

void mveplay_shutdownMovie(MVESTREAM *mve)
{
    int i;

	timer_stop();
    SDL_CloseAudio();
    for (i = 0; i < 64; i++)
        if (mve_audio_buffers[i] != NULL)
            d_free(mve_audio_buffers[i]);
    memset(mve_audio_buffers, 0, sizeof(mve_audio_buffers));
    memset(mve_audio_buflens, 0, sizeof(mve_audio_buflens));
	mve_audio_curbuf_curpos=0;
	mve_audio_bufhead=0;
	mve_audio_buftail=0;
	mve_audio_playing=0;
	mve_audio_canplay=0;
	mve_audio_compressed=0;
	mve_audio_mutex = NULL;
	if (mve_audio_spec)
		d_free(mve_audio_spec);
	mve_audio_spec=NULL;

    d_free(g_vBackBuf1);
	g_vBackBuf1 = NULL;
    d_free(g_vBackBuf2);
	g_vBackBuf2 = NULL;
	g_pCurMap=NULL;
	g_nMapLength=0;
}


/************************************************************
 * internal decoding functions
 ************************************************************/

static void dispatchDecoder(unsigned char **pFrame, unsigned char codeType, unsigned char **pData, int *pDataRemain, int *curXb, int *curYb);

static void decodeFrame(unsigned char *pFrame, unsigned char *pMap, int mapRemain, unsigned char *pData, int dataRemain)
{
    int i, j;
    int xb, yb;

    xb = g_width >> 3;
    yb = g_height >> 3;
    for (j=0; j<yb; j++)
    {
        for (i=0; i<xb/2; i++)
        {
            dispatchDecoder(&pFrame, (*pMap) & 0xf, &pData, &dataRemain, &i, &j);
            if (pFrame < g_vBackBuf1)
                Warning("mveplay: danger!  pointing out of bounds below after dispatch decoder: %d, %d (1) [%x]\n", i, j, (*pMap) & 0xf);
            else if (pFrame >= g_vBackBuf1 + g_width*g_height)
                Warning("mveplay: danger!  pointing out of bounds above after dispatch decoder: %d, %d (1) [%x]\n", i, j, (*pMap) & 0xf);
            dispatchDecoder(&pFrame, (*pMap) >> 4, &pData, &dataRemain, &i, &j);
            if (pFrame < g_vBackBuf1)
                Warning("mveplay: danger!  pointing out of bounds below after dispatch decoder: %d, %d (2) [%x]\n", i, j, (*pMap) >> 4);
            else if (pFrame >= g_vBackBuf1 + g_width*g_height)
                Warning("mveplay: danger!  pointing out of bounds above after dispatch decoder: %d, %d (2) [%x]\n", i, j, (*pMap) >> 4);

            ++pMap;
            --mapRemain;
        }

        pFrame += 7*g_width;
    }
}

static void relClose(int i, int *x, int *y)
{
    int ma, mi;

    ma = i >> 4;
    mi = i & 0xf;

    *x = mi - 8;
    *y = ma - 8;
}

static void relFar(int i, int sign, int *x, int *y)
{
    if (i < 56)
    {
        *x = sign * (8 + (i % 7));
        *y = sign *      (i / 7);
    }
    else
    {
        *x = sign * (-14 + (i - 56) % 29);
        *y = sign *   (8 + (i - 56) / 29);
    }
}

static void copyFrame(unsigned char *pDest, unsigned char *pSrc)
{
    int i;

    for (i=0; i<8; i++)
    {
        memcpy(pDest, pSrc, 8);
        pDest += g_width;
        pSrc += g_width;
    }
}

static void patternRow4Pixels(unsigned char *pFrame,
                              unsigned char pat0, unsigned char pat1,
                              unsigned char *p)
{
    unsigned short mask=0x0003;
    unsigned short shift=0;
    unsigned short pattern = (pat1 << 8) | pat0;

    while (mask != 0)
    {
        *pFrame++ = p[(mask & pattern) >> shift];
        mask <<= 2;
        shift += 2;
    }
}

static void patternRow4Pixels2(unsigned char *pFrame,
                               unsigned char pat0,
                               unsigned char *p)
{
    unsigned char mask=0x03;
    unsigned char shift=0;
    unsigned char pel;
    int skip=1;

    while (mask != 0)
    {
        pel = p[(mask & pat0) >> shift];
        pFrame[0] = pel;
        pFrame[2] = pel;
        pFrame[g_width + 0] = pel;
        pFrame[g_width + 2] = pel;
        pFrame += skip;
        skip = 4 - skip;
        mask <<= 2;
        shift += 2;
    }
}

static void patternRow4Pixels2x1(unsigned char *pFrame, unsigned char pat, unsigned char *p)
{
    unsigned char mask=0x03;
    unsigned char shift=0;
    unsigned char pel;

    while (mask != 0)
    {
        pel = p[(mask & pat) >> shift];
        pFrame[0] = pel;
        pFrame[1] = pel;
        pFrame += 2;
        mask <<= 2;
        shift += 2;
    }
}

static void patternQuadrant4Pixels(unsigned char *pFrame, unsigned char pat0, unsigned char pat1, unsigned char pat2, unsigned char pat3, unsigned char *p)
{
    unsigned long mask = 0x00000003UL;
    int shift=0;
    int i;
    unsigned long pat = (pat3 << 24) | (pat2 << 16) | (pat1 << 8) | pat0;

    for (i=0; i<16; i++)
    {
        pFrame[i&3] = p[(pat & mask) >> shift];

        if ((i&3) == 3)
            pFrame += g_width;

        mask <<= 2;
        shift += 2;
    }
}


static void patternRow2Pixels(unsigned char *pFrame, unsigned char pat, unsigned char *p)
{
    unsigned char mask=0x01;

    while (mask != 0)
    {
        *pFrame++ = p[(mask & pat) ? 1 : 0];
        mask <<= 1;
    }
}

static void patternRow2Pixels2(unsigned char *pFrame, unsigned char pat, unsigned char *p)
{
    unsigned char pel;
    unsigned char mask=0x1;
    int skip=1;

    while (mask != 0x10)
    {
        pel = p[(mask & pat) ? 1 : 0];
        pFrame[0] = pel;
        pFrame[2] = pel;
        pFrame[g_width + 0] = pel;
        pFrame[g_width + 2] = pel;
        pFrame += skip;
        skip = 4 - skip;
        mask <<= 1;
    }
}

static void patternQuadrant2Pixels(unsigned char *pFrame, unsigned char pat0, unsigned char pat1, unsigned char *p)
{
    unsigned short mask = 0x0001;
    int i;
    unsigned short pat = (pat1 << 8) | pat0;

    for (i=0; i<16; i++)
    {
        pFrame[i&3] = p[(pat & mask) ? 1 : 0];

        if ((i&3) == 3)
            pFrame += g_width;

        mask <<= 1;
    }
}

static void dispatchDecoder(unsigned char **pFrame, unsigned char codeType, unsigned char **pData, int *pDataRemain, int *curXb, int *curYb)
{
    unsigned char p[4];
    unsigned char pat[16];
    int i, j, k;
    int x, y;

    switch(codeType)
    {
        case 0x0:
                  copyFrame(*pFrame, *pFrame + (g_vBackBuf2 - g_vBackBuf1));
        case 0x1:
                  *pFrame += 8;
                  break;

        case 0x2:
                  relFar(*(*pData)++, 1, &x, &y);
                  copyFrame(*pFrame, *pFrame + x + y*g_width);
                  *pFrame += 8;
                  --*pDataRemain;
                  break;

        case 0x3:
                  relFar(*(*pData)++, -1, &x, &y);
                  copyFrame(*pFrame, *pFrame + x + y*g_width);
                  *pFrame += 8;
                  --*pDataRemain;
                  break;

        case 0x4:
                  relClose(*(*pData)++, &x, &y);
                  copyFrame(*pFrame, *pFrame + (g_vBackBuf2 - g_vBackBuf1) + x + y*g_width);
                  *pFrame += 8;
                  --*pDataRemain;
                  break;

        case 0x5:
                  x = (char)*(*pData)++;
                  y = (char)*(*pData)++;
                  copyFrame(*pFrame, *pFrame + (g_vBackBuf2 - g_vBackBuf1) + x + y*g_width);
                  *pFrame += 8;
                  *pDataRemain -= 2;
                  break;

        case 0x6:
                  for (i=0; i<2; i++)
                  {
                      *pFrame += 16;
                      if (++*curXb == (g_width >> 3))
                      {
                          *pFrame += 7*g_width;
                          *curXb = 0;
                          if (++*curYb == (g_height >> 3))
                              return;
                      }
                  }
                  break;

        case 0x7:
                  p[0] = *(*pData)++;
                  p[1] = *(*pData)++;
                  if (p[0] <= p[1])
                  {
                      for (i=0; i<8; i++)
                      {
                          patternRow2Pixels(*pFrame, *(*pData)++, p);
                          *pFrame += g_width;
                      }
                  }
                  else
                  {
                      for (i=0; i<2; i++)
                      {
                          patternRow2Pixels2(*pFrame, *(*pData) & 0xf, p);
                          *pFrame += 2*g_width;
                          patternRow2Pixels2(*pFrame, *(*pData)++ >> 4, p);
                          *pFrame += 2*g_width;
                      }
                  }
                  *pFrame -= (8*g_width - 8);
                  break;

        case 0x8:
                  if ( (*pData)[0] <= (*pData)[1])
                  {
                      for (i=0; i<4; i++)
                      {
                          p[0] = *(*pData)++;
                          p[1] = *(*pData)++;
                          pat[0] = *(*pData)++;
                          pat[1] = *(*pData)++;
                          patternQuadrant2Pixels(*pFrame, pat[0], pat[1], p);

                          if (i & 1)
                              *pFrame -= (4*g_width - 4);
                          else
                              *pFrame += 4*g_width;
                      }
                  }
                  else if ( (*pData)[6] <= (*pData)[7])
                  {
                      for (i=0; i<4; i++)
                      {
                          if ((i & 1) == 0)
                          {
                              p[0] = *(*pData)++;
                              p[1] = *(*pData)++;
                          }
                          pat[0] = *(*pData)++;
                          pat[1] = *(*pData)++;
                          patternQuadrant2Pixels(*pFrame, pat[0], pat[1], p);

                          if (i & 1)
                              *pFrame -= (4*g_width - 4);
                          else
                              *pFrame += 4*g_width;
                      }
                  }
                  else
                  {
                      for (i=0; i<8; i++)
                      {
                          if ((i & 3) == 0)
                          {
                              p[0] = *(*pData)++;
                              p[1] = *(*pData)++;
                          }
                          patternRow2Pixels(*pFrame, *(*pData)++, p);
                          *pFrame += g_width;
                      }
                      *pFrame -= (8*g_width - 8);
                  }
                  break;

        case 0x9:
                  if ( (*pData)[0] <= (*pData)[1])
                  {
                      if ( (*pData)[2] <= (*pData)[3])
                      {
                          p[0] = *(*pData)++;
                          p[1] = *(*pData)++;
                          p[2] = *(*pData)++;
                          p[3] = *(*pData)++;

                          for (i=0; i<8; i++)
                          {
                              pat[0] = *(*pData)++;
                              pat[1] = *(*pData)++;
                              patternRow4Pixels(*pFrame, pat[0], pat[1], p);
                              *pFrame += g_width;
                          }

                          *pFrame -= (8*g_width - 8);
                      }
                      else
                      {
                          p[0] = *(*pData)++;
                          p[1] = *(*pData)++;
                          p[2] = *(*pData)++;
                          p[3] = *(*pData)++;

                          patternRow4Pixels2(*pFrame, *(*pData)++, p);
                          *pFrame += 2*g_width;
                          patternRow4Pixels2(*pFrame, *(*pData)++, p);
                          *pFrame += 2*g_width;
                          patternRow4Pixels2(*pFrame, *(*pData)++, p);
                          *pFrame += 2*g_width;
                          patternRow4Pixels2(*pFrame, *(*pData)++, p);
                          *pFrame -= (6*g_width - 8);
                      }
                  }
                  else
                  {
                      if ( (*pData)[2] <= (*pData)[3])
                      {
                          p[0] = *(*pData)++;
                          p[1] = *(*pData)++;
                          p[2] = *(*pData)++;
                          p[3] = *(*pData)++;

                          for (i=0; i<8; i++)
                          {
                              pat[0] = *(*pData)++;
                              patternRow4Pixels2x1(*pFrame, pat[0], p);
                              *pFrame += g_width;
                          }

                          *pFrame -= (8*g_width - 8);
                      }
                      else
                      {
                          p[0] = *(*pData)++;
                          p[1] = *(*pData)++;
                          p[2] = *(*pData)++;
                          p[3] = *(*pData)++;

                          for (i=0; i<4; i++)
                          {
                              pat[0] = *(*pData)++;
                              pat[1] = *(*pData)++;
                              patternRow4Pixels(*pFrame, pat[0], pat[1], p);
                              *pFrame += g_width;
                              patternRow4Pixels(*pFrame, pat[0], pat[1], p);
                              *pFrame += g_width;
                          }

                          *pFrame -= (8*g_width - 8);
                      }
                  }
                  break;

        case 0xa:
                  if ( (*pData)[0] <= (*pData)[1])
                  {
                      for (i=0; i<4; i++)
                      {
                          p[0] = *(*pData)++;
                          p[1] = *(*pData)++;
                          p[2] = *(*pData)++;
                          p[3] = *(*pData)++;
                          pat[0] = *(*pData)++;
                          pat[1] = *(*pData)++;
                          pat[2] = *(*pData)++;
                          pat[3] = *(*pData)++;

                          patternQuadrant4Pixels(*pFrame, pat[0], pat[1], pat[2], pat[3], p);

                          if (i & 1)
                              *pFrame -= (4*g_width - 4);
                          else
                              *pFrame += 4*g_width;
                      }
                  }
                  else
                  {
                      if ( (*pData)[12] <= (*pData)[13])
                      {
                          for (i=0; i<4; i++)
                          {
                              if ((i&1) == 0)
                              {
                                  p[0] = *(*pData)++;
                                  p[1] = *(*pData)++;
                                  p[2] = *(*pData)++;
                                  p[3] = *(*pData)++;
                              }

                              pat[0] = *(*pData)++;
                              pat[1] = *(*pData)++;
                              pat[2] = *(*pData)++;
                              pat[3] = *(*pData)++;

                              patternQuadrant4Pixels(*pFrame, pat[0], pat[1], pat[2], pat[3], p);

                              if (i & 1)
                                  *pFrame -= (4*g_width - 4);
                              else
                                  *pFrame += 4*g_width;
                          }
                      }
                      else
                      {
                          for (i=0; i<8; i++)
                          {
                              if ((i&3) == 0)
                              {
                                  p[0] = *(*pData)++;
                                  p[1] = *(*pData)++;
                                  p[2] = *(*pData)++;
                                  p[3] = *(*pData)++;
                              }

                              pat[0] = *(*pData)++;
                              pat[1] = *(*pData)++;
                              patternRow4Pixels(*pFrame, pat[0], pat[1], p);
                              *pFrame += g_width;
                          }

                          *pFrame -= (8*g_width - 8);
                      }
                  }
                  break;

        case 0xb:
                  for (i=0; i<8; i++)
                  {
                      memcpy(*pFrame, *pData, 8);
                      *pFrame += g_width;
                      *pData += 8;
                      *pDataRemain -= 8;
                  }
                  *pFrame -= (8*g_width - 8);
                  break;

        case 0xc:
                  for (i=0; i<4; i++)
                  {
                      for (j=0; j<2; j++)
                      {
                          for (k=0; k<4; k++)
                          {
                              (*pFrame)[j+2*k] = (*pData)[k];
                              (*pFrame)[g_width+j+2*k] = (*pData)[k];
                          }
                          *pFrame += g_width;
                      }
                      *pData += 4;
                      *pDataRemain -= 4;
                  }
                  *pFrame -= (8*g_width - 8);
                  break;

        case 0xd:
                  for (i=0; i<2; i++)
                  {
                      for (j=0; j<4; j++)
                      {
                          for (k=0; k<4; k++)
                          {
                              (*pFrame)[k*g_width+j] = (*pData)[0];
                              (*pFrame)[k*g_width+j+4] = (*pData)[1];
                          }
                      }
                      *pFrame += 4*g_width;
                      *pData += 2;
                      *pDataRemain -= 2;
                  }
                  *pFrame -= (8*g_width - 8);
                  break;

        case 0xe:
                  for (i=0; i<8; i++)
                  {
                      memset(*pFrame, **pData, 8);
                      *pFrame += g_width;
                  }
                  ++*pData;
                  --*pDataRemain;
                  *pFrame -= (8*g_width - 8);
                  break;

        case 0xf:
                  for (i=0; i<8; i++)
                  {
                      for (j=0; j<8; j++)
                      {
                          (*pFrame)[j] = (*pData)[(i+j)&1];
                      }
                      *pFrame += g_width;
                  }
                  *pData += 2;
                  *pDataRemain -= 2;
                  *pFrame -= (8*g_width - 8);
                  break;

        default:
            break;
    }
}
