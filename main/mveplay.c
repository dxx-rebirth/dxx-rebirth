#include "mvelib.h"                                             /* next buffer */
#include "mve_audio.h"

#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <SDL/SDL.h>
#include <pthread.h>

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

int doPlay(const char *filename);

void usage(void)
{
    fprintf(stderr, "usage: mveplay filename\n");
    exit(1);
}

int g_spdFactorNum=0;
int g_spdFactorDenom=10;

void initializeMovie(MVESTREAM *mve);
void playMovie(MVESTREAM *mve);
void shutdownMovie(MVESTREAM *mve);

short get_short(unsigned char *data)
{
    short value;
    value = data[0] | (data[1] << 8);
    return value;
}

int get_int(unsigned char *data)
{
    int value;
    value = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    return value;
}

int default_seg_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    fprintf(stderr, "unknown chunk type %02x/%02x\n", major, minor);
    return 1;
}

/*************************
 * general handlers
 *************************/
int end_movie_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    return 0;
}

/*************************
 * timer handlers
 *************************/

/*
 * timer variables
 */
int micro_frame_delay=0;
int timer_started=0;
struct timeval timer_expire = {0, 0};

int create_timer_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    long long temp;
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

void timer_start(void)
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

void do_timer_wait(void)
{
    int nsec=0;
    struct timespec ts, tsRem;
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
    if (nanosleep(&ts, &tsRem) == -1  &&  errno == EINTR)
        exit(1);

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
void mve_audio_callback(void *userdata, Uint8 *stream, int len); 
short *mve_audio_buffers[64];
int    mve_audio_buflens[64];
int    mve_audio_curbuf_curpos=0;
int mve_audio_bufhead=0;
int mve_audio_buftail=0;
int mve_audio_playing=0;
int mve_audio_canplay=0;
SDL_AudioSpec *mve_audio_spec=NULL;
pthread_mutex_t mve_audio_mutex = PTHREAD_MUTEX_INITIALIZER;

void mve_audio_callback(void *userdata, Uint8 *stream, int len)
{
    int total=0;
    int length;
    if (mve_audio_bufhead == mve_audio_buftail)
        return /* 0 */;

fprintf(stderr, "+ <%d (%d), %d, %d>\n", mve_audio_bufhead, mve_audio_curbuf_curpos, mve_audio_buftail, len);

    pthread_mutex_lock(&mve_audio_mutex);

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
        free(mve_audio_buffers[mve_audio_bufhead]);                                     /* free the buffer */
        mve_audio_buffers[mve_audio_bufhead]=NULL;                                      /* free the buffer */
        mve_audio_buflens[mve_audio_bufhead]=0;                                         /* free the buffer */

        if (++mve_audio_bufhead == 64)                                                  /* next buffer */
            mve_audio_bufhead = 0;
        mve_audio_curbuf_curpos = 0;
    }

fprintf(stderr, "= <%d (%d), %d, %d>: %d\n", mve_audio_bufhead, mve_audio_curbuf_curpos, mve_audio_buftail, len, total);
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
            free(mve_audio_buffers[mve_audio_bufhead]);                                 /* free buffer */
            mve_audio_buffers[mve_audio_bufhead]=NULL;
            mve_audio_buflens[mve_audio_bufhead]=0;

            if (++mve_audio_bufhead == 64)                                              /* next buffer */
                mve_audio_bufhead = 0;
            mve_audio_curbuf_curpos = 0;
        }
    }

fprintf(stderr, "- <%d (%d), %d, %d>\n", mve_audio_bufhead, mve_audio_curbuf_curpos, mve_audio_buftail, len);
    pthread_mutex_unlock(&mve_audio_mutex);
}

int create_audiobuf_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    int sample_rate;
    int desired_buffer;

fprintf(stderr, "creating audio buffers\n");

    sample_rate = get_short(data + 4);
    desired_buffer = get_int(data + 6);
    mve_audio_spec = (SDL_AudioSpec *)malloc(sizeof(SDL_AudioSpec));
    mve_audio_spec->freq = sample_rate;
    mve_audio_spec->format = AUDIO_S16LSB;
    mve_audio_spec->channels = 2;
    mve_audio_spec->samples = 32768;
    mve_audio_spec->callback = mve_audio_callback;
    mve_audio_spec->userdata = NULL;
    if (SDL_OpenAudio(mve_audio_spec, NULL) >= 0)
    {
fprintf(stderr, "   success\n");
        mve_audio_canplay = 1;
    }
    else
    {
fprintf(stderr, "   failure : %s\n", SDL_GetError());
        mve_audio_canplay = 0;
    }

    memset(mve_audio_buffers, 0, sizeof(mve_audio_buffers));
    memset(mve_audio_buflens, 0, sizeof(mve_audio_buflens));

    return 1;
}

int play_audio_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    if (mve_audio_canplay  &&  !mve_audio_playing  &&  mve_audio_bufhead != mve_audio_buftail)
    {
        SDL_PauseAudio(0);
        mve_audio_playing = 1;
    }

    return 1;
}

int audio_data_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    const int selected_chan=1;
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
            pthread_mutex_lock(&mve_audio_mutex);
            mve_audio_buflens[mve_audio_buftail] = nsamp;
            mve_audio_buffers[mve_audio_buftail] = (short *)malloc(nsamp);
            if (major == 8)
                mveaudio_uncompress(mve_audio_buffers[mve_audio_buftail], data, -1); /* XXX */
            else
                memset(mve_audio_buffers[mve_audio_buftail], 0, nsamp); /* XXX */

            if (++mve_audio_buftail == 64)
                mve_audio_buftail = 0;

            if (mve_audio_buftail == mve_audio_bufhead)
                fprintf(stderr, "d'oh!  buffer ring overrun (%d)\n", mve_audio_bufhead);
            pthread_mutex_unlock(&mve_audio_mutex);
        }

        if (mve_audio_playing)
            SDL_UnlockAudio();
    }

    return 1;
}

/*************************
 * video handlers
 *************************/
SDL_Surface *g_screen;
int g_screenWidth, g_screenHeight;
int g_width, g_height;
unsigned char g_palette[768];
unsigned char *g_vBackBuf1, *g_vBackBuf2;
unsigned char *g_pCurMap=NULL;
int g_nMapLength=0;

int create_videobuf_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    short w, h;
    w = get_short(data);
    h = get_short(data+2);
    g_width = w << 3;
    g_height = h << 3;
    g_vBackBuf1 = malloc(g_width * g_height);
    g_vBackBuf2 = malloc(g_width * g_height);
    memset(g_vBackBuf1, 0, g_width * g_height);
    memset(g_vBackBuf2, 0, g_width * g_height);
    return 1;
}

int stupefaction=0;
int display_video_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    int i;
    unsigned char *pal = g_palette;
    unsigned char *pDest;
    unsigned char *pixels = g_vBackBuf1;
    SDL_Surface *screenSprite;
    SDL_Rect renderArea;
    int x, y;

    SDL_Surface *initSprite = SDL_CreateRGBSurface(SDL_SWSURFACE, g_width, g_height, 8, 0, 0, 0, 0);
    for(i = 0; i < 256; i++)
    {
        initSprite->format->palette->colors[i].r = (*pal++) << 2;
        initSprite->format->palette->colors[i].g = (*pal++) << 2;
        initSprite->format->palette->colors[i].b = (*pal++) << 2;
        initSprite->format->palette->colors[i].unused = 0;
    }

    pDest = initSprite->pixels;
    for (i=0; i<g_height; i++)
    {
        memcpy(pDest, pixels, g_width);
        pixels += g_width;
        pDest += initSprite->pitch;
    }

    screenSprite = SDL_DisplayFormat(initSprite);
    SDL_FreeSurface(initSprite);

    if (g_screenWidth > screenSprite->w) x = (g_screenWidth - screenSprite->w) >> 1;
    else x=0;
    if (g_screenHeight > screenSprite->h) y = (g_screenHeight - screenSprite->h) >> 1;
    else y=0;
    renderArea.x = x;
    renderArea.y = y;
    renderArea.w = MIN(g_screenWidth  - x, screenSprite->w);
    renderArea.h = MIN(g_screenHeight - y, screenSprite->h);
    SDL_BlitSurface(screenSprite, NULL, g_screen, &renderArea);
    if ( (g_screen->flags & SDL_DOUBLEBUF) == SDL_DOUBLEBUF )
        SDL_Flip(g_screen);
    else
        SDL_UpdateRects(g_screen, 1, &renderArea);
    SDL_FreeSurface(screenSprite);
    return 1;
}

int init_video_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    short width, height;
    width = get_short(data);
    height = get_short(data+2);
    g_screen = SDL_SetVideoMode(width, height, 16, SDL_ANYFORMAT);
    g_screenWidth = width;
    g_screenHeight = height;
    memset(g_palette, 0, 768);
    return 1;
}

int video_palette_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    short start, count;
    start = get_short(data);
    count = get_short(data+2);
    memcpy(g_palette + 3*start, data+4, 3*count);
    return 1;
}

int video_codemap_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    g_pCurMap = data;
    g_nMapLength = len;
    return 1;
}

void decodeFrame(unsigned char *pFrame, unsigned char *pMap, int mapRemain, unsigned char *pData, int dataRemain);

int video_data_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
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

int end_chunk_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
    g_pCurMap=NULL;
    return 1;
}

void initializeMovie(MVESTREAM *mve)
{
    mve_set_handler(mve, 0x00, end_movie_handler);
    mve_set_handler(mve, 0x01, end_chunk_handler);
    mve_set_handler(mve, 0x02, create_timer_handler);
    mve_set_handler(mve, 0x03, create_audiobuf_handler);
    mve_set_handler(mve, 0x04, play_audio_handler);
    mve_set_handler(mve, 0x05, create_videobuf_handler);
    mve_set_handler(mve, 0x07, display_video_handler);
    mve_set_handler(mve, 0x08, audio_data_handler);
    mve_set_handler(mve, 0x09, audio_data_handler);
    mve_set_handler(mve, 0x0a, init_video_handler);
    mve_set_handler(mve, 0x0c, video_palette_handler);
    mve_set_handler(mve, 0x0f, video_codemap_handler);
    mve_set_handler(mve, 0x11, video_data_handler);
}

void playMovie(MVESTREAM *mve)
{
    int init_timer=0;
    int cont=1;
    while (cont)
    {
        cont = mve_play_next_chunk(mve);
        if (micro_frame_delay  &&  !init_timer)
        {
            timer_start();
            init_timer = 1;
        }

        do_timer_wait();
    }
}

void shutdownMovie(MVESTREAM *mve)
{
}

void dispatchDecoder(unsigned char **pFrame, unsigned char codeType, unsigned char **pData, int *pDataRemain, int *curXb, int *curYb);

void decodeFrame(unsigned char *pFrame, unsigned char *pMap, int mapRemain, unsigned char *pData, int dataRemain)
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
                fprintf(stderr, "danger!  pointing out of bounds below after dispatch decoder: %d, %d (1) [%x]\n", i, j, (*pMap) & 0xf);
            else if (pFrame >= g_vBackBuf1 + g_width*g_height)
                fprintf(stderr, "danger!  pointing out of bounds above after dispatch decoder: %d, %d (1) [%x]\n", i, j, (*pMap) & 0xf);
            dispatchDecoder(&pFrame, (*pMap) >> 4, &pData, &dataRemain, &i, &j);
            if (pFrame < g_vBackBuf1)
                fprintf(stderr, "danger!  pointing out of bounds below after dispatch decoder: %d, %d (2) [%x]\n", i, j, (*pMap) >> 4);
            else if (pFrame >= g_vBackBuf1 + g_width*g_height)
                fprintf(stderr, "danger!  pointing out of bounds above after dispatch decoder: %d, %d (2) [%x]\n", i, j, (*pMap) >> 4);

            ++pMap;
            --mapRemain;
        }

        pFrame += 7*g_width;
    }
}

void relClose(int i, int *x, int *y)
{
    int ma, mi;

    ma = i >> 4;
    mi = i & 0xf;

    *x = mi - 8;
    *y = ma - 8;
}

void relFar(int i, int sign, int *x, int *y)
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

void copyFrame(unsigned char *pDest, unsigned char *pSrc)
{
    int i;

    for (i=0; i<8; i++)
    {
        memcpy(pDest, pSrc, 8);
        pDest += g_width;
        pSrc += g_width;
    }
}

void patternRow4Pixels(unsigned char *pFrame,
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

void patternRow4Pixels2(unsigned char *pFrame,
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

void patternRow4Pixels2x1(unsigned char *pFrame, unsigned char pat, unsigned char *p)
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

void patternQuadrant4Pixels(unsigned char *pFrame, unsigned char pat0, unsigned char pat1, unsigned char pat2, unsigned char pat3, unsigned char *p)
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


void patternRow2Pixels(unsigned char *pFrame, unsigned char pat, unsigned char *p)
{
    unsigned char mask=0x01;

    while (mask != 0)
    {
        *pFrame++ = p[(mask & pat) ? 1 : 0];
        mask <<= 1;
    }
}

void patternRow2Pixels2(unsigned char *pFrame, unsigned char pat, unsigned char *p)
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

void patternQuadrant2Pixels(unsigned char *pFrame, unsigned char pat0, unsigned char pat1, unsigned char *p)
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

void dispatchDecoder(unsigned char **pFrame, unsigned char codeType, unsigned char **pData, int *pDataRemain, int *curXb, int *curYb)
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
