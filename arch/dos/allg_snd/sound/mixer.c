/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *      By Shawn Hargreaves,
 *      1 Salisbury Road,
 *      Market Drayton,
 *      Shropshire,
 *      England, TF9 1AJ.
 *
 *      Sample mixing code.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <dir.h>

#ifdef DJGPP
#include <go32.h>
#include <sys/farptr.h>
#endif

#include "allegro.h"
#include "internal.h"


typedef struct MIXER_VOICE
{
   int playing;               /* are we active? */
   unsigned char *data8;      /* data for 8 bit samples */
   unsigned short *data16;    /* data for 16 bit samples */
   long pos;                  /* fixed point position in sample */
   long diff;                 /* fixed point speed of play */
   long len;                  /* fixed point sample length */
   long loop_start;           /* fixed point loop start position */
   long loop_end;             /* fixed point loop end position */
   int lvol;                  /* left channel volume */
   int rvol;                  /* right channel volume */
} MIXER_VOICE;


#define MIXER_VOLUME_LEVELS         32
#define MIXER_FIX_SHIFT             8

#define UPDATE_FREQ                 16


/* the samples currently being played */
static MIXER_VOICE mixer_voice[MIXER_MAX_SFX];

/* temporary sample mixing buffer */
static unsigned short *mix_buffer = NULL; 

/* lookup table for converting sample volumes */
typedef signed short MIXER_VOL_TABLE[256];
static MIXER_VOL_TABLE *mix_vol_table = NULL;

/* lookup table for amplifying and clipping samples */
static unsigned short *mix_clip_table = NULL;

#define MIX_RES_16      14
#define MIX_RES_8       10

/* flags for the mixing code */
static int mix_voices;
static int mix_size;
static int mix_freq;
static int mix_stereo;
static int mix_16bit;


static void mixer_lock_mem();



/* _mixer_init:
 *  Initialises the sample mixing code, returning 0 on success. You should
 *  pass it the number of samples you want it to mix each time the refill
 *  buffer routine is called, the sample rate to mix at, and two flags 
 *  indicating whether the mixing should be done in stereo or mono and with 
 *  eight or sixteen bits. The bufsize parameter is the number of samples,
 *  not bytes. It should take into account whether you are working in stereo 
 *  or not (eg. double it if in stereo), but it should not be affected by
 *  whether each sample is 8 or 16 bits.
 */
int _mixer_init(int bufsize, int freq, int stereo, int is16bit, int *voices)
{
   int i, j;
   int clip_size;
   int clip_scale;
   int clip_max;

   mix_voices = 1;
   while ((mix_voices < MIXER_MAX_SFX) && (mix_voices < *voices))
      mix_voices <<= 1;

   *voices = mix_voices;

   mix_size = bufsize;
   mix_freq = freq;
   mix_stereo = stereo;
   mix_16bit = is16bit;

   for (i=0; i<MIXER_MAX_SFX; i++) {
      mixer_voice[i].playing = FALSE;
      mixer_voice[i].data8 = NULL;
      mixer_voice[i].data16 = NULL;
   }

   /* temporary buffer for sample mixing */
   mix_buffer = malloc(mix_size*sizeof(short));
   if (!mix_buffer)
      return -1;

   _go32_dpmi_lock_data(mix_buffer, mix_size*sizeof(short));

   /* volume table for mixing samples into the temporary buffer */
   mix_vol_table = malloc(sizeof(MIXER_VOL_TABLE) * MIXER_VOLUME_LEVELS);
   if (!mix_vol_table) {
      free(mix_buffer);
      mix_buffer = NULL;
      return -1;
   }

   _go32_dpmi_lock_data(mix_vol_table, sizeof(MIXER_VOL_TABLE) * MIXER_VOLUME_LEVELS);

   for (j=0; j<MIXER_VOLUME_LEVELS; j++)
      for (i=0; i<256; i++)
	 mix_vol_table[j][i] = (i-128) * j * 256 / MIXER_VOLUME_LEVELS / mix_voices;

   /* lookup table for amplifying and clipping sample buffers */
   if (mix_16bit) {
      clip_size = 1 << MIX_RES_16;
      clip_scale = 18 - MIX_RES_16;
      clip_max = 0xFFFF;
   }
   else {
      clip_size = 1 << MIX_RES_8;
      clip_scale = 10 - MIX_RES_8;
      clip_max = 0xFF;
   }

   mix_clip_table = malloc(sizeof(short) * clip_size);
   if (!mix_clip_table) {
      free(mix_buffer);
      mix_buffer = NULL;
      free(mix_vol_table);
      mix_vol_table = NULL;
      return -1;
   }

   _go32_dpmi_lock_data(mix_clip_table, sizeof(short) * clip_size);

   if (mix_voices >= 8) {
      /* clip extremes of the sample range */
      for (i=0; i<clip_size*3/8; i++) {
	 mix_clip_table[i] = 0;
	 mix_clip_table[clip_size-1-i] = clip_max;
      }

      for (i=0; i<clip_size/4; i++)
	 mix_clip_table[clip_size*3/8 + i] = i<<clip_scale;
   }
   else {
      /* simple linear amplification */
      for (i=0; i<clip_size; i++)
	 mix_clip_table[i] = (i<<clip_scale)/4;
   }

   mixer_lock_mem();

   return 0;
}



/* _mixer_exit:
 *  Cleans up the sample mixer code when you are done with it.
 */
void _mixer_exit()
{
   free(mix_buffer);
   mix_buffer = NULL;

   free(mix_vol_table);
   mix_vol_table = NULL;

   free(mix_clip_table);
   mix_clip_table = NULL;
}



/* update_mixer_volume:
 *  Called whenever the voice volume or pan changes, to update the mixer 
 *  amplification table indexes.
 */
static inline void update_mixer_volume(MIXER_VOICE *mv, PHYS_VOICE *pv)
{
   int vol = pv->vol >> 12;
   int pan = pv->pan >> 12;
   int lvol, rvol;

   if (mix_stereo) {
      lvol = vol * (256-pan) * MIXER_VOLUME_LEVELS / 32768;
      rvol = vol * pan * MIXER_VOLUME_LEVELS / 32768;
   }
   else
      lvol = rvol = vol * MIXER_VOLUME_LEVELS / 256;

   mv->lvol = MID(0, lvol, MIXER_VOLUME_LEVELS-1);
   mv->rvol = MID(0, rvol, MIXER_VOLUME_LEVELS-1);
}



/* update_mixer_freq:
 *  Called whenever the voice frequency changes, to update the sample
 *  delta value.
 */
static inline void update_mixer_freq(MIXER_VOICE *mv, PHYS_VOICE *pv)
{
   mv->diff = (pv->freq >> (12 - MIXER_FIX_SHIFT)) / mix_freq;

   if (pv->playmode & PLAYMODE_BACKWARD)
      mv->diff = -mv->diff;
}



/* helper for constructing the body of a sample mixing routine */
#define MIXER()                                                              \
{                                                                            \
   if ((voice->playmode & PLAYMODE_LOOP) &&                                  \
       (spl->loop_start < spl->loop_end)) {                                  \
      restart:                                                               \
      if (voice->playmode & PLAYMODE_BACKWARD) {                             \
	 /* mix a backward looping sample */                                 \
	 while (len-- > 0) {                                                 \
	    MIX();                                                           \
	    spl->pos += spl->diff;                                           \
	    if (spl->pos < spl->loop_start) {                                \
	       if (voice->playmode & PLAYMODE_BIDIR) {                       \
		  spl->diff = -spl->diff;                                    \
		  spl->pos += spl->diff * 2;                                 \
		  voice->playmode ^= PLAYMODE_BACKWARD;                      \
		  goto restart;                                              \
	       }                                                             \
	       else                                                          \
		  spl->pos += (spl->loop_end - spl->loop_start);             \
	    }                                                                \
	    UPDATE();                                                        \
	 }                                                                   \
      }                                                                      \
      else {                                                                 \
	 /* mix a forward looping sample */                                  \
	 while (len-- > 0) {                                                 \
	    MIX();                                                           \
	    spl->pos += spl->diff;                                           \
	    if (spl->pos >= spl->loop_end) {                                 \
	       if (voice->playmode & PLAYMODE_BIDIR) {                       \
		  spl->diff = -spl->diff;                                    \
		  spl->pos += spl->diff * 2;                                 \
		  voice->playmode ^= PLAYMODE_BACKWARD;                      \
		  goto restart;                                              \
	       }                                                             \
	       else                                                          \
		  spl->pos -= (spl->loop_end - spl->loop_start);             \
	    }                                                                \
	    UPDATE();                                                        \
	 }                                                                   \
      }                                                                      \
   }                                                                         \
   else {                                                                    \
      /* mix a non-looping sample */                                         \
      while (len-- > 0) {                                                    \
	 MIX();                                                              \
	 spl->pos += spl->diff;                                              \
	 if ((unsigned long)spl->pos >= (unsigned long)spl->len) {           \
	    /* note: we don't need a different version for reverse play, */  \
	    /* as this will wrap and automatically do the Right Thing */     \
	    spl->playing = FALSE;                                            \
	    return;                                                          \
	 }                                                                   \
	 UPDATE();                                                           \
      }                                                                      \
   }                                                                         \
}



#define UPDATE()



/* mix_mono_8_samples:
 *  Mixes from an eight bit sample into a mono buffer, until either len 
 *  samples have been mixed or until the end of the sample is reached.
 */
static void mix_mono_8_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   signed short *vol = (short *)(mix_vol_table + spl->lvol);

   #define MIX()                                                             \
      *(buf++) += vol[spl->data8[spl->pos>>MIXER_FIX_SHIFT]];

   MIXER();

   #undef MIX
}

static END_OF_FUNCTION(mix_mono_8_samples);



/* mix_stereo_8_samples:
 *  Mixes from an eight bit sample into a stereo buffer, until either len 
 *  samples have been mixed or until the end of the sample is reached.
 */
static void mix_stereo_8_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   signed short *lvol = (short *)(mix_vol_table + spl->lvol);
   signed short *rvol = (short *)(mix_vol_table + spl->rvol);

   len >>= 1;

   #define MIX()                                                             \
      *(buf++) += lvol[spl->data8[spl->pos>>MIXER_FIX_SHIFT]];               \
      *(buf++) += rvol[spl->data8[spl->pos>>MIXER_FIX_SHIFT]];

   MIXER();

   #undef MIX
}

static END_OF_FUNCTION(mix_stereo_8_samples);



/* mix_mono_16_samples:
 *  Mixes from a 16 bit sample into a mono buffer, until either len samples 
 *  have been mixed or until the end of the sample is reached.
 */
static void mix_mono_16_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   signed short *vol = (short *)(mix_vol_table + spl->lvol);

   #define MIX()                                                             \
      *(buf++) += vol[(spl->data16[spl->pos>>MIXER_FIX_SHIFT])>>8];

   MIXER();

   #undef MIX
}

static END_OF_FUNCTION(mix_mono_16_samples);



/* mix_stereo_16_samples:
 *  Mixes from a 16 bit sample into a stereo buffer, until either len samples 
 *  have been mixed or until the end of the sample is reached.
 */
static void mix_stereo_16_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   signed short *lvol = (short *)(mix_vol_table + spl->lvol);
   signed short *rvol = (short *)(mix_vol_table + spl->rvol);

   len >>= 1;

   #define MIX()                                                             \
      *(buf++) += lvol[(spl->data16[spl->pos>>MIXER_FIX_SHIFT])>>8];         \
      *(buf++) += rvol[(spl->data16[spl->pos>>MIXER_FIX_SHIFT])>>8];

   MIXER();

   #undef MIX
}

static END_OF_FUNCTION(mix_stereo_16_samples);



#undef UPDATE


/* helper for updating the volume ramp and pitch/pan sweep status */
#define UPDATE()                                                             \
{                                                                            \
   if ((len & (UPDATE_FREQ-1)) == 0) {                                       \
      /* update volume ramp */                                               \
      if (voice->dvol) {                                                     \
	 voice->vol += voice->dvol;                                          \
	 if (((voice->dvol > 0) && (voice->vol >= voice->target_vol)) ||     \
	     ((voice->dvol < 0) && (voice->vol <= voice->target_vol))) {     \
	    voice->vol = voice->target_vol;                                  \
	    voice->dvol = 0;                                                 \
	 }                                                                   \
      }                                                                      \
									     \
      /* update frequency sweep */                                           \
      if (voice->dfreq) {                                                    \
	 voice->freq += voice->dfreq;                                        \
	 if (((voice->dfreq > 0) && (voice->freq >= voice->target_freq)) ||  \
	     ((voice->dfreq < 0) && (voice->freq <= voice->target_freq))) {  \
	    voice->freq = voice->target_freq;                                \
	    voice->dfreq = 0;                                                \
	 }                                                                   \
      }                                                                      \
									     \
      /* update pan sweep */                                                 \
      if (voice->dpan) {                                                     \
	 voice->pan += voice->dpan;                                          \
	 if (((voice->dpan > 0) && (voice->pan >= voice->target_pan)) ||     \
	     ((voice->dpan < 0) && (voice->pan <= voice->target_pan))) {     \
	    voice->pan = voice->target_pan;                                  \
	    voice->dpan = 0;                                                 \
	 }                                                                   \
      }                                                                      \
									     \
      update_mixer_volume(spl, voice);                                       \
      update_mixer_freq(spl, voice);                                         \
   }                                                                         \
}



/* mix_mono_8_samples_slow:
 *  Mixes from an eight bit sample into a mono buffer, until either len 
 *  samples have been mixed or until the end of the sample is reached,
 *  using the slow mixing code that supports volume ramps and frequency/pan
 *  sweep effects.
 */
static void mix_mono_8_samples_slow(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   signed short *vol = (short *)(mix_vol_table + spl->lvol);

   #define MIX()                                                             \
      *(buf++) += vol[spl->data8[spl->pos>>MIXER_FIX_SHIFT]];

   MIXER();

   #undef MIX
}

static END_OF_FUNCTION(mix_mono_8_samples_slow);



/* mix_stereo_8_samples_slow:
 *  Mixes from an eight bit sample into a stereo buffer, until either len 
 *  samples have been mixed or until the end of the sample is reached,
 *  using the slow mixing code that supports volume ramps and frequency/pan
 *  sweep effects.
 */
static void mix_stereo_8_samples_slow(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   signed short *lvol = (short *)(mix_vol_table + spl->lvol);
   signed short *rvol = (short *)(mix_vol_table + spl->rvol);

   len >>= 1;

   #define MIX()                                                             \
      *(buf++) += lvol[spl->data8[spl->pos>>MIXER_FIX_SHIFT]];               \
      *(buf++) += rvol[spl->data8[spl->pos>>MIXER_FIX_SHIFT]];

   MIXER();

   #undef MIX
}

static END_OF_FUNCTION(mix_stereo_8_samples_slow);



/* mix_mono_16_samples_slow:
 *  Mixes from a 16 bit sample into a mono buffer, until either len samples 
 *  have been mixed or until the end of the sample is reached, using the 
 *  slow mixing code that supports volume ramps and frequency/pan sweep 
 *  effects.
 */
static void mix_mono_16_samples_slow(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   signed short *vol = (short *)(mix_vol_table + spl->lvol);

   #define MIX()                                                             \
      *(buf++) += vol[(spl->data16[spl->pos>>MIXER_FIX_SHIFT])>>8];

   MIXER();

   #undef MIX
}

static END_OF_FUNCTION(mix_mono_16_samples_slow);



/* mix_stereo_16_samples_slow:
 *  Mixes from a 16 bit sample into a stereo buffer, until either len samples 
 *  have been mixed or until the end of the sample is reached, using the 
 *  slow mixing code that supports volume ramps and frequency/pan sweep 
 *  effects.
 */
static void mix_stereo_16_samples_slow(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   signed short *lvol = (short *)(mix_vol_table + spl->lvol);
   signed short *rvol = (short *)(mix_vol_table + spl->rvol);

   len >>= 1;

   #define MIX()                                                             \
      *(buf++) += lvol[(spl->data16[spl->pos>>MIXER_FIX_SHIFT])>>8];         \
      *(buf++) += rvol[(spl->data16[spl->pos>>MIXER_FIX_SHIFT])>>8];

   MIXER();

   #undef MIX
}

static END_OF_FUNCTION(mix_stereo_16_samples_slow);



/* _mix_some_samples:
 *  Mixes samples into a buffer in conventional memory (the buf parameter
 *  should be a linear offset into the specified segment), using the buffer 
 *  size, sample frequency, etc, set when you called _mixer_init(). This 
 *  should be called by the hardware end-of-buffer interrupt routine to 
 *  get the next buffer full of samples to DMA to the card.
 */
void _mix_some_samples(unsigned long buf, unsigned short seg, int issigned)
{
   int i;
   unsigned short *p = mix_buffer;
   unsigned long *l = (unsigned long *)p;

   /* clear mixing buffer */
   for (i=0; i<mix_size/2; i++)
      *(l++) = 0x80008000;

   /* mix the samples */
   if (mix_stereo) { 
      for (i=0; i<mix_voices; i++) {
	 if ((mixer_voice[i].playing) &&
	     ((_phys_voice[i].vol > 0) || (_phys_voice[i].dvol > 0)))  {
	    if ((_phys_voice[i].dvol) || (_phys_voice[i].dfreq) || (_phys_voice[i].dpan)) {
	       if (mixer_voice[i].data8)
		  mix_stereo_8_samples_slow(mixer_voice+i, _phys_voice+i, p, mix_size);
	       else
		  mix_stereo_16_samples_slow(mixer_voice+i, _phys_voice+i, p, mix_size);
	    }
	    else {
	       if (mixer_voice[i].data8)
		  mix_stereo_8_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
	       else
		  mix_stereo_16_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
	    }
	 }
      }
   }
   else {
      for (i=0; i<mix_voices; i++) {
	 if ((mixer_voice[i].playing) &&
	     ((_phys_voice[i].vol > 0) || (_phys_voice[i].dvol > 0)))  {
	    if ((_phys_voice[i].dvol) || (_phys_voice[i].dfreq) || (_phys_voice[i].dpan)) {
	       if (mixer_voice[i].data8)
		  mix_mono_8_samples_slow(mixer_voice+i, _phys_voice+i, p, mix_size);
	       else
		  mix_mono_16_samples_slow(mixer_voice+i, _phys_voice+i, p, mix_size);
	    }
	    else {
	       if (mixer_voice[i].data8)
		  mix_mono_8_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
	       else
		  mix_mono_16_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
	    }
	 }
      }
   }

   _farsetsel(seg);

   /* transfer to conventional memory buffer */
   if (mix_16bit) {
      if (issigned) {
	 for (i=0; i<mix_size; i++) {
	    _farnspokew(buf, mix_clip_table[*p >> (16-MIX_RES_16)] ^ 0x8000);
	    buf += 2;
	    p++;
	 }
      }
      else {
	 for (i=0; i<mix_size; i++) {
	    _farnspokew(buf, mix_clip_table[*p >> (16-MIX_RES_16)]);
	    buf += 2;
	    p++;
	 }
      }
   }
   else {
      for (i=0; i<mix_size; i++) {
	 _farnspokeb(buf, mix_clip_table[*p >> (16-MIX_RES_8)]);
	 buf++;
	 p++;
      }
   }
}

END_OF_FUNCTION(_mix_some_samples);



/* _mixer_init_voice:
 *  Initialises the specificed voice ready for playing a sample.
 */
void _mixer_init_voice(int voice, SAMPLE *sample)
{
   mixer_voice[voice].playing = FALSE;
   mixer_voice[voice].pos = 0;
   mixer_voice[voice].len = sample->len << MIXER_FIX_SHIFT;
   mixer_voice[voice].loop_start = sample->loop_start << MIXER_FIX_SHIFT;
   mixer_voice[voice].loop_end = sample->loop_end << MIXER_FIX_SHIFT;

   if (sample->bits == 8) {
      mixer_voice[voice].data8 = sample->data;
      mixer_voice[voice].data16 = NULL;
   }
   else {
      mixer_voice[voice].data8 = NULL;
      mixer_voice[voice].data16 = sample->data;
   }

   update_mixer_volume(mixer_voice+voice, _phys_voice+voice);
   update_mixer_freq(mixer_voice+voice, _phys_voice+voice);
}

END_OF_FUNCTION(_mixer_init_voice);



/* _mixer_release_voice:
 *  Releases a voice when it is no longer required.
 */
void _mixer_release_voice(int voice)
{
   mixer_voice[voice].playing = FALSE;
   mixer_voice[voice].data8 = NULL;
   mixer_voice[voice].data16 = NULL;
}

END_OF_FUNCTION(_mixer_release_voice);



/* _mixer_start_voice:
 *  Activates a voice, with the currently selected parameters.
 */
void _mixer_start_voice(int voice)
{
   if (mixer_voice[voice].pos >= mixer_voice[voice].len)
      mixer_voice[voice].pos = 0;

   mixer_voice[voice].playing = TRUE;
}

END_OF_FUNCTION(_mixer_start_voice);



/* _mixer_stop_voice:
 *  Stops a voice from playing.
 */
void _mixer_stop_voice(int voice)
{
   mixer_voice[voice].playing = FALSE;
}

END_OF_FUNCTION(_mixer_stop_voice);



/* _mixer_loop_voice:
 *  Sets the loopmode for a voice.
 */
void _mixer_loop_voice(int voice, int loopmode)
{
   update_mixer_freq(mixer_voice+voice, _phys_voice+voice);
}

END_OF_FUNCTION(_mixer_loop_voice);



/* _mixer_get_position:
 *  Returns the current play position of a voice, or -1 if it has finished.
 */
int _mixer_get_position(int voice)
{
   if (mixer_voice[voice].pos >= mixer_voice[voice].len)
      return -1;

   return (mixer_voice[voice].pos >> MIXER_FIX_SHIFT);
}

END_OF_FUNCTION(_mixer_get_position);



/* _mixer_set_position:
 *  Sets the current play position of a voice.
 */
void _mixer_set_position(int voice, int position)
{
   mixer_voice[voice].pos = (position << MIXER_FIX_SHIFT);

   if (mixer_voice[voice].pos >= mixer_voice[voice].len)
      mixer_voice[voice].playing = FALSE;
}

END_OF_FUNCTION(_mixer_set_position);



/* _mixer_get_volume:
 *  Returns the current volume of a voice.
 */
int _mixer_get_volume(int voice)
{
   return (_phys_voice[voice].vol >> 12);
}

END_OF_FUNCTION(_mixer_get_volume);



/* _mixer_set_volume:
 *  Sets the volume of a voice.
 */
void _mixer_set_volume(int voice, int volume)
{
   update_mixer_volume(mixer_voice+voice, _phys_voice+voice);
}

END_OF_FUNCTION(_mixer_set_volume);



/* _mixer_ramp_volume:
 *  Starts a volume ramping operation.
 */
void _mixer_ramp_volume(int voice, int time, int endvol)
{
   int d = (endvol << 12) - _phys_voice[voice].vol;
   time = MAX(time * (mix_freq / UPDATE_FREQ) / 1000, 1);

   _phys_voice[voice].target_vol = endvol << 12;
   _phys_voice[voice].dvol = d / time;
}

END_OF_FUNCTION(_mixer_ramp_volume);



/* _mixer_stop_volume_ramp:
 *  Ends a volume ramp operation.
 */
void _mixer_stop_volume_ramp(int voice)
{
   _phys_voice[voice].dvol = 0;
}

END_OF_FUNCTION(_mixer_stop_volume_ramp);



/* _mixer_get_frequency:
 *  Returns the current frequency of a voice.
 */
int _mixer_get_frequency(int voice)
{
   return (_phys_voice[voice].freq >> 12);
}

END_OF_FUNCTION(_mixer_get_frequency);



/* _mixer_set_frequency:
 *  Sets the frequency of a voice.
 */
void _mixer_set_frequency(int voice, int frequency)
{
   update_mixer_freq(mixer_voice+voice, _phys_voice+voice);
}

END_OF_FUNCTION(_mixer_set_frequency);



/* _mixer_sweep_frequency:
 *  Starts a frequency sweep.
 */
void _mixer_sweep_frequency(int voice, int time, int endfreq)
{
   int d = (endfreq << 12) - _phys_voice[voice].freq;
   time = MAX(time * (mix_freq / UPDATE_FREQ) / 1000, 1);

   _phys_voice[voice].target_freq = endfreq << 12;
   _phys_voice[voice].dfreq = d / time;
}

END_OF_FUNCTION(_mixer_sweep_frequency);



/* _mixer_stop_frequency_sweep:
 *  Ends a frequency sweep.
 */
void _mixer_stop_frequency_sweep(int voice)
{
   _phys_voice[voice].dfreq = 0;
}

END_OF_FUNCTION(_mixer_stop_frequency_sweep);



/* _mixer_get_pan:
 *  Returns the current pan position of a voice.
 */
int _mixer_get_pan(int voice)
{
   return (_phys_voice[voice].pan >> 12);
}

END_OF_FUNCTION(_mixer_get_pan);



/* _mixer_set_pan:
 *  Sets the pan position of a voice.
 */
void _mixer_set_pan(int voice, int pan)
{
   update_mixer_volume(mixer_voice+voice, _phys_voice+voice);
}

END_OF_FUNCTION(_mixer_set_pan);



/* _mixer_sweep_pan:
 *  Starts a pan sweep.
 */
void _mixer_sweep_pan(int voice, int time, int endpan)
{
   int d = (endpan << 12) - _phys_voice[voice].pan;
   time = MAX(time * (mix_freq / UPDATE_FREQ) / 1000, 1);

   _phys_voice[voice].target_pan = endpan << 12;
   _phys_voice[voice].dpan = d / time;
}

END_OF_FUNCTION(_mixer_sweep_pan);



/* _mixer_stop_pan_sweep:
 *  Ends a pan sweep.
 */
void _mixer_stop_pan_sweep(int voice)
{
   _phys_voice[voice].dpan = 0;
}

END_OF_FUNCTION(_mixer_stop_pan_sweep);



/* _mixer_set_echo:
 *  Sets the echo parameters for a voice.
 */
void _mixer_set_echo(int voice, int strength, int delay)
{
   /* not implemented */
}

END_OF_FUNCTION(_mixer_set_echo);



/* _mixer_set_tremolo:
 *  Sets the tremolo parameters for a voice.
 */
void _mixer_set_tremolo(int voice, int rate, int depth)
{
   /* not implemented */
}

END_OF_FUNCTION(_mixer_set_tremolo);



/* _mixer_set_vibrato:
 *  Sets the amount of vibrato for a voice.
 */
void _mixer_set_vibrato(int voice, int rate, int depth)
{
   /* not implemented */
}

END_OF_FUNCTION(_mixer_set_vibrato);



/* mixer_lock_mem:
 *  Locks memory used by the functions in this file.
 */
static void mixer_lock_mem()
{
   LOCK_VARIABLE(mixer_voice);
   LOCK_VARIABLE(mix_buffer);
   LOCK_VARIABLE(mix_vol_table);
   LOCK_VARIABLE(mix_clip_table);
   LOCK_VARIABLE(mix_voices);
   LOCK_VARIABLE(mix_size);
   LOCK_VARIABLE(mix_freq);
   LOCK_VARIABLE(mix_stereo);
   LOCK_VARIABLE(mix_16bit);
   LOCK_FUNCTION(mix_mono_8_samples);
   LOCK_FUNCTION(mix_stereo_8_samples);
   LOCK_FUNCTION(mix_mono_16_samples);
   LOCK_FUNCTION(mix_stereo_16_samples);
   LOCK_FUNCTION(mix_mono_8_samples_slow);
   LOCK_FUNCTION(mix_stereo_8_samples_slow);
   LOCK_FUNCTION(mix_mono_16_samples_slow);
   LOCK_FUNCTION(mix_stereo_16_samples_slow);
   LOCK_FUNCTION(_mix_some_samples);
   LOCK_FUNCTION(_mixer_init_voice);
   LOCK_FUNCTION(_mixer_release_voice);
   LOCK_FUNCTION(_mixer_start_voice);
   LOCK_FUNCTION(_mixer_stop_voice);
   LOCK_FUNCTION(_mixer_loop_voice);
   LOCK_FUNCTION(_mixer_get_position);
   LOCK_FUNCTION(_mixer_set_position);
   LOCK_FUNCTION(_mixer_get_volume);
   LOCK_FUNCTION(_mixer_set_volume);
   LOCK_FUNCTION(_mixer_ramp_volume);
   LOCK_FUNCTION(_mixer_stop_volume_ramp);
   LOCK_FUNCTION(_mixer_get_frequency);
   LOCK_FUNCTION(_mixer_set_frequency);
   LOCK_FUNCTION(_mixer_sweep_frequency);
   LOCK_FUNCTION(_mixer_stop_frequency_sweep);
   LOCK_FUNCTION(_mixer_get_pan);
   LOCK_FUNCTION(_mixer_set_pan);
   LOCK_FUNCTION(_mixer_sweep_pan);
   LOCK_FUNCTION(_mixer_stop_pan_sweep);
   LOCK_FUNCTION(_mixer_set_echo);
   LOCK_FUNCTION(_mixer_set_tremolo);
   LOCK_FUNCTION(_mixer_set_vibrato);
}



