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
 *      Sound setup routines and API framework functions.
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



/* dummy functions for the nosound drivers */
int  _dummy_detect() { return TRUE; }
int  _dummy_init(int voices) { return 0; }
void _dummy_exit() { }
int  _dummy_mixer_volume(int volume) { return 0; }
void _dummy_init_voice(int voice, SAMPLE *sample) { }
void _dummy_noop1(int p) { }
void _dummy_noop2(int p1, int p2) { }
void _dummy_noop3(int p1, int p2, int p3) { }
int  _dummy_get_position(int voice) { return -1; }
int  _dummy_get(int voice) { return 0; }
void _dummy_raw_midi(unsigned char data) { }
int  _dummy_load_patches(char *patches, char *drums) { return 0; }
void _dummy_adjust_patches(char *patches, char *drums) { }
void _dummy_key_on(int inst, int note, int bend, int vol, int pan) { }

/* put this after all the dummy functions, so they will all get locked */
END_OF_FUNCTION(_dummy_detect); 



DIGI_DRIVER digi_none =
{
   "No sound", "The sound of silence",
   0, 0, 0xFFFF, 0,
   _dummy_detect,
   _dummy_init,
   _dummy_exit,
   _dummy_mixer_volume,
   _dummy_init_voice,
   _dummy_noop1,
   _dummy_noop1,
   _dummy_noop1,
   _dummy_noop2,
   _dummy_get_position,
   _dummy_noop2,
   _dummy_get,
   _dummy_noop2,
   _dummy_noop3,
   _dummy_noop1,
   _dummy_get,
   _dummy_noop2,
   _dummy_noop3,
   _dummy_noop1,
   _dummy_get,
   _dummy_noop2,
   _dummy_noop3,
   _dummy_noop1,
   _dummy_noop3,
   _dummy_noop3,
   _dummy_noop3
};


MIDI_DRIVER midi_none =
{
   "No sound", "The sound of silence",
   0, 0, 0xFFFF, 0, -1, -1,
   _dummy_detect,
   _dummy_init,
   _dummy_exit,
   _dummy_mixer_volume,
   _dummy_raw_midi,
   _dummy_load_patches,
   _dummy_adjust_patches,
   _dummy_key_on,
   _dummy_noop1,
   _dummy_noop2,
   _dummy_noop3,
   _dummy_noop2,
   _dummy_noop2
};


int digi_card = DIGI_AUTODETECT;          /* current driver ID numbers */
int midi_card = MIDI_AUTODETECT;

DIGI_DRIVER *digi_driver = &digi_none;    /* these things do all the work */
MIDI_DRIVER *midi_driver = &midi_none;

static int sound_installed = FALSE;       /* are we installed? */

static int digi_reserve = -1;             /* how many voices to reserve */
static int midi_reserve = -1;

VOICE _voice[VIRTUAL_VOICES];             /* list of active samples */

PHYS_VOICE _phys_voice[DIGI_VOICES];      /* physical -> virtual voice map */

int _digi_volume = -1;                    /* current volume settings */
int _midi_volume = -1;

int _flip_pan = FALSE;                    /* reverse l/r sample panning? */

int _sb_freq = -1;                        /* hardware parameters */
int _sb_port = -1;
int _sb_dma = -1;
int _sb_irq = -1;
int _fm_port = -1;
int _mpu_port = -1;

#define SWEEP_FREQ   50

static void update_sweeps();
static void sound_lock_mem();

int (*_midi_init)() = NULL;
void (*_midi_exit)() = NULL;



/* read_sound_config:
 *  Helper for reading the sound hardware configuration data.
 */
static void read_sound_config()
{
   _flip_pan = get_config_int("sound", "flip_pan", FALSE);
   _sb_port = get_config_hex("sound", "sb_port", -1);
   _sb_dma = get_config_int("sound", "sb_dma", -1);
   _sb_irq = get_config_int("sound", "sb_irq", -1);
   _sb_freq = get_config_int("sound", "sb_freq", -1);
   _fm_port = get_config_hex("sound", "fm_port", -1);
   _mpu_port = get_config_hex("sound", "mpu_port", -1);
   _digi_volume = get_config_int("sound", "digi_volume", -1);
   _midi_volume = get_config_int("sound", "midi_volume", -1);
}



/* detect_digi_driver:
 *  Detects whether the specified digital sound driver is available,
 *  returning the maximum number of voices that it can provide, or
 *  zero if the device is not present. This function must be called
 *  _before_ install_sound().
 */
int detect_digi_driver(int driver_id)
{
   int i;

   if (sound_installed)
      return 0;

   read_sound_config();

   for (i=0; _digi_driver_list[i].driver_id; i++) {
      if (_digi_driver_list[i].driver_id == driver_id) {
	 digi_card = driver_id;
	 midi_card = MIDI_AUTODETECT;
	 if (_digi_driver_list[i].driver->detect())
	    return _digi_driver_list[i].driver->max_voices;
	 else
	    return 0;
      }
   }

   return digi_none.max_voices;
}



/* detect_midi_driver:
 *  Detects whether the specified midi sound driver is available,
 *  returning the maximum number of voices that it can provide, or
 *  zero if the device is not present. If this routine returns -1,
 *  it is a note-stealing MIDI driver, which shares voices with the
 *  current digital driver. In this situation you can use the 
 *  reserve_voices() function to specify how the available voices are
 *  divided between the digital and MIDI playback routines. This function
 *  must be called _before_ install_sound().
 */
int detect_midi_driver(int driver_id)
{
   int i;

   if (sound_installed)
      return 0;

   read_sound_config();

   for (i=0; _midi_driver_list[i].driver_id; i++) {
      if (_midi_driver_list[i].driver_id == driver_id) {
	 digi_card = DIGI_AUTODETECT;
	 midi_card = driver_id;
	 if (_midi_driver_list[i].driver->detect())
	    return _midi_driver_list[i].driver->max_voices;
	 else
	    return 0;
      }
   }

   return midi_none.max_voices;
}



/* reserve_voices:
 *  Reserves a number of voices for the digital and MIDI sound drivers
 *  respectively. This must be called _before_ install_sound(). If you 
 *  attempt to reserve too many voices, subsequent calls to install_sound()
 *  will fail. Note that depending on the driver you may actually get 
 *  more voices than you reserve: these values just specify the minimum
 *  that is appropriate for your application. Pass a negative reserve value 
 *  to use the default settings.
 */
void reserve_voices(int digi_voices, int midi_voices)
{
   digi_reserve = digi_voices;
   midi_reserve = midi_voices;
}



/* install_sound:
 *  Initialises the sound module, returning zero on success. The two card 
 *  parameters should use the DIGI_* and MIDI_* constants defined in 
 *  allegro.h. Pass DIGI_AUTODETECT and MIDI_AUTODETECT if you don't know 
 *  what the soundcard is.
 */
int install_sound(int digi, int midi, char *cfg_path)
{
   int digi_voices, midi_voices;
   int c;

   if (sound_installed)
      return 0;

   for (c=0; c<VIRTUAL_VOICES; c++) {
      _voice[c].sample = NULL;
      _voice[c].num = -1;
   }

   for (c=0; c<DIGI_VOICES; c++)
      _phys_voice[c].num = -1;

   /* initialise the midi file player */
   if (_midi_init)
      if (_midi_init() != 0)
	 return -1;

/*	 register_datafile_object(DAT_SAMPLE, NULL, (void (*)(void *))destroy_sample);*/

   digi_card = digi;
   midi_card = midi;

   /* read config information */
   if (digi_card == DIGI_AUTODETECT)
      digi_card = get_config_int("sound", "digi_card", DIGI_AUTODETECT);

   if (midi_card == MIDI_AUTODETECT)
      midi_card = get_config_int("sound", "midi_card", MIDI_AUTODETECT);

   if (digi_reserve < 0)
      digi_reserve = get_config_int("sound", "digi_voices", -1);

   if (midi_reserve < 0)
      midi_reserve = get_config_int("sound", "midi_voices", -1);

   read_sound_config();

   sound_lock_mem();
   _dma_lock_mem();

   digi_driver = NULL;

   /* search table for a specific digital driver */
   for (c=0; _digi_driver_list[c].driver; c++) { 
      if (_digi_driver_list[c].driver_id == digi_card) {
	 digi_driver = _digi_driver_list[c].driver;
	 if (!digi_driver->detect()) {
	    digi_driver = &digi_none; 
	    if (_midi_exit)
	       _midi_exit();
	    return -1;
	 }
	 break;
      }
   }

   /* autodetect digital driver */
   if (!digi_driver) {
      for (c=0; _digi_driver_list[c].driver; c++) {
	 digi_card = _digi_driver_list[c].driver_id;
	 if ((_digi_driver_list[c].autodetect) &&
	     (_digi_driver_list[c].driver->detect())) {
	    digi_driver = _digi_driver_list[c].driver;
	    break;
	 }
      }
   }

   midi_driver = NULL;

   /* search table for a specific MIDI driver */
   for (c=0; _midi_driver_list[c].driver; c++) { 
      if (_midi_driver_list[c].driver_id == midi_card) {
	 midi_driver = _midi_driver_list[c].driver;
	 if (!midi_driver->detect()) {
	    digi_driver = &digi_none; 
	    midi_driver = &midi_none; 
	    if (_midi_exit)
	       _midi_exit();
	    return -1;
	 }
	 break;
      }
   }

   /* autodetect MIDI driver */
   if (!midi_driver) {
      for (c=0; _midi_driver_list[c].driver; c++) {
	 midi_card = _midi_driver_list[c].driver_id;
	 if ((_midi_driver_list[c].autodetect) &&
	     (_midi_driver_list[c].driver->detect())) {
	    midi_driver = _midi_driver_list[c].driver;
	    break;
	 }
      }
   }

   /* work out how many voices to allocate for each driver */
   if (digi_reserve >= 0)
      digi_voices = digi_reserve;
   else
      digi_voices = digi_driver->def_voices;

   if (midi_driver->max_voices < 0) {
      /* MIDI driver steals voices from the digital player */
      if (midi_reserve >= 0)
	 midi_voices = midi_reserve;
      else
	 midi_voices = MID(0, digi_driver->max_voices - digi_voices, midi_driver->def_voices);

      digi_voices += midi_voices;
   }
   else {
      /* MIDI driver has voices of its own */
      if (midi_reserve >= 0)
	 midi_voices = midi_reserve;
      else
	 midi_voices = midi_driver->def_voices;
   }

   /* make sure this is a reasonable number of voices to use */
   if ((digi_voices > DIGI_VOICES) || (midi_voices > MIDI_VOICES)) {
      sprintf(allegro_error, "Insufficient %s voices available", 
		  (digi_voices > DIGI_VOICES) ? "digital" : "MIDI");
      digi_driver = &digi_none; 
      midi_driver = &midi_none; 
      if (_midi_exit)
	 _midi_exit();
      return -1;
   }

   /* initialise the digital sound driver */
   if (digi_driver->init(digi_voices) != 0) {
      digi_driver = &digi_none; 
      midi_driver = &midi_none; 
      if (_midi_exit)
	 _midi_exit();
      return -1;
   }

   /* initialise the midi driver */
   if (midi_driver->init(midi_voices) != 0) {
      digi_driver->exit();
      digi_driver = &digi_none; 
      midi_driver = &midi_none; 
      if (_midi_exit)
	 _midi_exit();
      return -1;
   }

   digi_driver->voices = MIN(digi_driver->voices, DIGI_VOICES);
   midi_driver->voices = MIN(midi_driver->voices, MIDI_VOICES);

   /* check that we actually got enough voices */
   if ((digi_driver->voices < digi_voices) ||
       ((midi_driver->voices < midi_voices) && (!midi_driver->raw_midi))) {
      sprintf(allegro_error, "Insufficient %s voices available", 
		  (digi_driver->voices < digi_voices) ? "digital" : "MIDI");
      midi_driver->exit();
      digi_driver->exit();
      digi_driver = &digi_none; 
      midi_driver = &midi_none; 
      if (_midi_exit)
	 _midi_exit();
      return -1;
   }

   /* adjust for note-stealing MIDI drivers */
   if (midi_driver->max_voices < 0) {
      midi_voices += (digi_driver->voices - digi_voices) * 3/4;
      digi_driver->voices -= midi_voices;
      midi_driver->basevoice = VIRTUAL_VOICES - midi_voices;
      midi_driver->voices = midi_voices;

      for (c=0; c<midi_voices; c++) {
	 _voice[midi_driver->basevoice+c].num = digi_driver->voices+c;
	 _phys_voice[digi_driver->voices+c].num = midi_driver->basevoice+c;
      }
   }

   /* simulate ramp/sweep effects for drivers that don't do it directly */
   if ((!digi_driver->ramp_volume) ||
       (!digi_driver->sweep_frequency) ||
       (!digi_driver->sweep_pan))
      install_int_ex(update_sweeps, BPS_TO_TIMER(SWEEP_FREQ));

   /* set the global sound volume */
   if ((_digi_volume >= 0) || (_midi_volume >= 0))
      set_volume(_digi_volume, _midi_volume);

   _add_exit_func(remove_sound);
   sound_installed = TRUE;
   return 0;
}



/* remove_sound:
 *  Sound module cleanup routine.
 */
void remove_sound()
{
   int c;

   if (sound_installed) {
      remove_int(update_sweeps);

      for (c=0; c<VIRTUAL_VOICES; c++)
	 if (_voice[c].sample)
	    deallocate_voice(c);

      if (_midi_exit)
	 _midi_exit();

      midi_driver->exit();
      midi_driver = &midi_none; 

      digi_driver->exit();
      digi_driver = &digi_none; 

      _remove_exit_func(remove_sound);
      sound_installed = FALSE;
   }
}



/* set_volume:
 *  Alters the global sound output volume. Specify volumes for both digital
 *  samples and MIDI playback, as integers from 0 to 255. If possible this
 *  routine will use a hardware mixer to control the volume, otherwise it
 *  will tell the sample mixer and MIDI player to simulate a mixer in
 *  software.
 */
void set_volume(int digi_volume, int midi_volume)
{
   if (digi_volume >= 0) {
      digi_volume = MID(0, digi_volume, 255);

      if ((digi_driver->mixer_volume) && 
	  (digi_driver->mixer_volume(digi_volume) == 0))
	 _digi_volume = -1;
      else
	 _digi_volume = digi_volume;
   }

   if (midi_volume >= 0) {
      midi_volume = MID(0, midi_volume, 255);

      if ((midi_driver->mixer_volume) && 
	  (midi_driver->mixer_volume(midi_volume) == 0))
	 _midi_volume = -1;
      else
	 _midi_volume = midi_volume;
   }
}



/* lock_sample:
 *  Locks a SAMPLE struct into physical memory. Pretty important, since 
 *  they are mostly accessed inside interrupt handlers.
 */
void lock_sample(SAMPLE *spl)
{
   #ifdef DJGPP
      _go32_dpmi_lock_data(spl, sizeof(SAMPLE));
      _go32_dpmi_lock_data(spl->data, spl->len*spl->bits/8);
   #endif
}



/* load_sample:
 *  Loads a sample from disk.
 */
SAMPLE *load_sample(char *filename)
{
   if (stricmp(get_extension(filename), "wav") == 0)
      return load_wav(filename);
   else if (stricmp(get_extension(filename), "voc") == 0)
      return load_voc(filename);
   else
      return NULL;
}



/* load_voc:
 *  Reads a mono 8 bit VOC format sample file, returning a SAMPLE structure, 
 *  or NULL on error.
 */
SAMPLE *load_voc(char *filename)
{
   PACKFILE *f;
   char buffer[30];
   int freq = 22050;
   int bits = 8;
   SAMPLE *spl = NULL;
   int len;
   int x;

   f = pack_fopen(filename, F_READ);
   if (!f) 
      return NULL;

   pack_fread(buffer, 0x16, f);

   if (memcmp(buffer, "Creative Voice File", 0x13))
      goto getout;

   if (pack_igetw(f) != 0x010A)        /* version: should be 0x010A */
      goto getout;

   if (pack_igetw(f) != 0x1129)        /* subversion: should be 0x1129 */
      goto getout;

   if (pack_getc(f) != 0x01)           /* sound data: should be 0x01 */
      goto getout;

   len = pack_igetw(f);                /* length is three bytes long: two */
   x = pack_getc(f);                   /* .. and one byte */
   x <<= 16;
   len += x-2;

   x = pack_getc(f);                   /* one byte of frequency */
   freq = 1000000 / (256-x);

   x = pack_getc(f);                   /* skip one byte */

   spl = malloc(sizeof(SAMPLE));

   if (spl) {
      spl->bits = bits;
      spl->freq = freq;
      spl->len = len;
      spl->priority = 255;
      spl->loop_start = 0;
      spl->loop_end = len;
      spl->param = -1;

      spl->data = malloc(len);
      if (!spl->data) {
	 free(spl);
	 spl = NULL;
      }
      else {
	 pack_fread(spl->data, len, f);
	 if (errno) {
	    free(spl->data);
	    free(spl);
	    spl = NULL;
	 }
      }
   }

   getout: 

   pack_fclose(f);

   if (spl)
      lock_sample(spl);

   return spl;
}



/* load_wav:
 *  Reads a mono RIFF WAV format sample file, returning a SAMPLE structure, 
 *  or NULL on error.
 */
SAMPLE *load_wav(char *filename)
{
   int trashtmp;
   PACKFILE *f;
   char buffer[25];
   int i;
   int length, len;
   int freq = 22050;
   int bits = 8;
   signed short s;
   SAMPLE *spl = NULL;

   f = pack_fopen(filename, F_READ);
   if (!f)
      return NULL;

   pack_fread(buffer, 12, f);          /* check RIFF header */
   if (memcmp(buffer, "RIFF", 4) || memcmp(buffer+8, "WAVE", 4))
      goto getout;

   while (!pack_feof(f)) {
      if (pack_fread(buffer, 4, f) != 4)
	 break;

      length = pack_igetl(f);          /* read chunk length */

      if (memcmp(buffer, "fmt ", 4) == 0) {
	 i = pack_igetw(f);            /* should be 1 for PCM data */
	 length -= 2;
	 if (i != 1) 
	    goto getout;

	 i = pack_igetw(f);            /* should be 1 for mono data */
	 length -= 2;
	 if (i != 1)
	    goto getout;

	 freq = pack_igetl(f);         /* sample frequency */
	 length -= 4;

         trashtmp=pack_igetl(f);                /* skip six bytes */
         trashtmp=pack_igetw(f);
	 length -= 6;

	 bits = pack_igetw(f);         /* 8 or 16 bit data? */
	 length -= 2;
	 if ((bits != 8) && (bits != 16))
	    goto getout;
      }
      else if (memcmp(buffer, "data", 4) == 0) {
	 len = length;
	 if (bits == 16)
	    len /= 2;

	 spl = malloc(sizeof(SAMPLE)); 

	 if (spl) {                    /* initialise the sample struct */
	    spl->bits = bits;
	    spl->freq = freq;
	    spl->len = len;
	    spl->priority = 255;
	    spl->loop_start = 0;
	    spl->loop_end = len;
	    spl->param = -1;

	    spl->data = malloc(length);

	    if (!spl->data) {
	       free(spl);
	       spl = NULL;
	    }
	    else {                     /* read the actual sample data */
	       if (bits == 8) {
		  pack_fread(spl->data, length, f);
	       }
	       else {
		  for (i=0; i<len; i++) {
		     s = pack_igetw(f);
		     ((signed short *)spl->data)[i] = s^0x8000;
		  }
	       }

	       length -= len;

	       if (errno) {
		  free(spl->data);
		  free(spl);
		  spl = NULL;
	       }
	    }
	 }
      }

      while (length > 0) {             /* skip the remainder of the chunk */
	 if (pack_getc(f) == EOF)
	    break;

	 length--;
      }
   }

   getout:

   pack_fclose(f);

   if (spl)
      lock_sample(spl);

   return spl;
}



/* destroy_sample:
 *  Frees a SAMPLE struct, checking whether the sample is currently playing, 
 *  and stopping it if it is.
 */
void destroy_sample(SAMPLE *spl)
{
   if (spl) {
      stop_sample(spl);

      if (spl->data) {
	 _unlock_dpmi_data(spl->data, spl->len*spl->bits/8);
	 free(spl->data);
      }

      _unlock_dpmi_data(spl, sizeof(SAMPLE));
      free(spl);
   }
}



/* absolute_freq:
 *  Converts a pitch from the relative 1000 = original format to an absolute
 *  value in hz.
 */
static inline int absolute_freq(int freq, SAMPLE *spl)
{
   if (freq == 1000)
      return spl->freq;
   else
      return (spl->freq * freq) / 1000;
}



/* play_sample:
 *  Triggers a sample at the specified volume, pan position, and frequency.
 *  The volume and pan range from 0 (min/left) to 255 (max/right), although
 *  the resolution actually used by the playback routines is likely to be
 *  less than this. Frequency is relative rather than absolute: 1000 
 *  represents the frequency that the sample was recorded at, 2000 is 
 *  twice this, etc. If loop is true the sample will repeat until you call 
 *  stop_sample(), and can be manipulated while it is playing by calling
 *  adjust_sample().
 */
int play_sample(SAMPLE *spl, int vol, int pan, int freq, int loop)
{
   int voice = allocate_voice(spl);

   if (voice >= 0) {
      voice_set_volume(voice, vol);
      voice_set_pan(voice, pan);
      voice_set_frequency(voice, absolute_freq(freq, spl));
      voice_set_playmode(voice, (loop ? PLAYMODE_LOOP : PLAYMODE_PLAY));
      voice_start(voice);
      release_voice(voice);
   }

   return voice;
}

END_OF_FUNCTION(play_sample);



/* adjust_sample:
 *  Alters the parameters of a sample while it is playing, useful for
 *  manipulating looped sounds. You can alter the volume, pan, and
 *  frequency, and can also remove the looping flag, which will stop
 *  the sample when it next reaches the end of its loop. If there are
 *  several copies of the same sample playing, this will adjust the
 *  first one it comes across. If the sample is not playing it has no
 *  effect.
 */
void adjust_sample(SAMPLE *spl, int vol, int pan, int freq, int loop)
{
   int c;

   for (c=0; c<VIRTUAL_VOICES; c++) { 
      if (_voice[c].sample == spl) {
	 voice_set_volume(c, vol);
	 voice_set_pan(c, pan);
	 voice_set_frequency(c, absolute_freq(freq, spl));
	 voice_set_playmode(c, (loop ? PLAYMODE_LOOP : PLAYMODE_PLAY));
	 return;
      }
   }
}

END_OF_FUNCTION(adjust_sample);



/* stop_sample:
 *  Kills off a sample, which is required if you have set a sample going 
 *  in looped mode. If there are several copies of the sample playing,
 *  it will stop them all.
 */
void stop_sample(SAMPLE *spl)
{
   int c;

   for (c=0; c<VIRTUAL_VOICES; c++)
      if (_voice[c].sample == spl)
	 deallocate_voice(c);
}

END_OF_FUNCTION(stop_sample);



/* allocate_physical_voice:
 *  Allocates a physical voice, killing off others as required in order
 *  to make room for it.
 */
static inline int allocate_physical_voice(int priority)
{
   VOICE *voice;
   int best = -1;
   int best_score = 0;
   int score;
   int c;

   /* look for a free voice */
   for (c=0; c<digi_driver->voices; c++)
      if (_phys_voice[c].num < 0)
	 return c;

   /* look for an autokill voice that has stopped */
   for (c=0; c<digi_driver->voices; c++) {
      voice = _voice + _phys_voice[c].num;
      if ((voice->autokill) && (digi_driver->get_position(c) < 0)) {
	 digi_driver->release_voice(c);
	 voice->sample = NULL;
	 voice->num = -1;
	 _phys_voice[c].num = -1;
	 return c;
      }
   }

   /* ok, we're going to have to get rid of something to make room... */
   for (c=0; c<digi_driver->voices; c++) {
      voice = _voice + _phys_voice[c].num;

      /* sort by voice priorities */
      if (voice->priority <= priority) {
	 score = 65536 - voice->priority * 256;

	 /* bias with a least-recently-used counter */
	 score += MID(0, retrace_count - voice->time, 32768);

	 /* bias according to whether the voice is looping or not */
	 if (!(_phys_voice[c].playmode & PLAYMODE_LOOP))
	    score += 32768;

	 if (score > best_score) {
	    best = c;
	    best_score = score;
	 }
      }
   }

   if (best >= 0) {
      /* kill off the old voice */
      digi_driver->stop_voice(best);
      digi_driver->release_voice(best);
      _voice[_phys_voice[best].num].num = -1;
      _phys_voice[best].num = -1;
      return best;
   }

   return -1;
}



/* allocate_virtual_voice:
 *  Allocates a virtual voice. This doesn't need to worry about killing off 
 *  others to make room, as we allow up to 256 virtual voices to be used
 *  simultaneously.
 */
static inline int allocate_virtual_voice()
{
   int virt_voices, c;

   virt_voices = VIRTUAL_VOICES;
   if (midi_driver->max_voices < 0)
      virt_voices -= midi_driver->voices;

   /* look for a free voice */
   for (c=0; c<virt_voices; c++)
      if (!_voice[c].sample)
	 return c;

   /* look for a stopped autokill voice */
   for (c=0; c<virt_voices; c++) {
      if (_voice[c].autokill) {
	 if (_voice[c].num < 0) {
	    _voice[c].sample = NULL;
	    return c;
	 }
	 else {
	    if (digi_driver->get_position(_voice[c].num) < 0) {
	       digi_driver->release_voice(_voice[c].num);
	       _phys_voice[_voice[c].num].num = -1;
	       _voice[c].sample = NULL;
	       _voice[c].num = -1;
	       return c;
	    }
	 }
      }
   }

   return -1;
}



/* allocate_voice:
 *  Allocates a voice ready for playing the specified sample, returning
 *  the voice number (note this is not the same as the physical voice
 *  number used by the sound drivers, and must only be used with the other
 *  voice functions, _not_ passed directly to the driver routines).
 *  Returns -1 if there is no voice available (this should never happen,
 *  since there are 256 virtual voices and anyone who needs more than that
 *  needs some urgent repairs to their brain :-)
 */
int allocate_voice(SAMPLE *spl)
{
   int phys = allocate_physical_voice(spl->priority);
   int virt = allocate_virtual_voice();

   if (virt >= 0) {
      _voice[virt].sample = spl;
      _voice[virt].num = phys;
      _voice[virt].autokill = FALSE;
      _voice[virt].time = retrace_count;
      _voice[virt].priority = spl->priority;

      if (phys >= 0) {
	 _phys_voice[phys].num = virt;
	 _phys_voice[phys].playmode = 0;
	 _phys_voice[phys].vol = ((_digi_volume >= 0) ? _digi_volume : 255) << 12;
	 _phys_voice[phys].pan = 128 << 12;
	 _phys_voice[phys].freq = spl->freq << 12;
	 _phys_voice[phys].dvol = 0;
	 _phys_voice[phys].dpan = 0;
	 _phys_voice[phys].dfreq = 0;

	 digi_driver->init_voice(phys, spl);
      }
   }

   return virt;
}

END_OF_FUNCTION(allocate_voice);



/* deallocate_voice:
 *  Releases a voice that was previously returned by allocate_voice().
 */
void deallocate_voice(int voice)
{
   if (_voice[voice].num >= 0) {
      digi_driver->stop_voice(_voice[voice].num);
      digi_driver->release_voice(_voice[voice].num);
      _phys_voice[_voice[voice].num].num = -1;
      _voice[voice].num = -1;
   }

   _voice[voice].sample = NULL;
}

END_OF_FUNCTION(deallocate_voice);



/* reallocate_voice:
 *  Switches an already-allocated voice to use a different sample.
 */
void reallocate_voice(int voice, SAMPLE *spl)
{
   int phys = _voice[voice].num;

   if (phys >= 0) {
      digi_driver->stop_voice(phys);
      digi_driver->release_voice(phys);
   }

   _voice[voice].sample = spl;
   _voice[voice].autokill = FALSE;
   _voice[voice].time = retrace_count;
   _voice[voice].priority = spl->priority;

   if (phys >= 0) {
      _phys_voice[phys].playmode = 0;
      _phys_voice[phys].vol = ((_digi_volume >= 0) ? _digi_volume : 255) << 12;
      _phys_voice[phys].pan = 128 << 12;
      _phys_voice[phys].freq = spl->freq << 12;
      _phys_voice[phys].dvol = 0;
      _phys_voice[phys].dpan = 0;
      _phys_voice[phys].dfreq = 0;

      digi_driver->init_voice(phys, spl);
   }
}

END_OF_FUNCTION(reallocate_voice);



/* release_voice:
 *  Flags that a voice is no longer going to be updated, so it can 
 *  automatically be freed as soon as the sample finishes playing.
 */
void release_voice(int voice)
{
   _voice[voice].autokill = TRUE;
}

END_OF_FUNCTION(release_voice);



/* voice_start:
 *  Starts a voice playing.
 */
void voice_start(int voice)
{
   if (_voice[voice].num >= 0)
      digi_driver->start_voice(_voice[voice].num);

   _voice[voice].time = retrace_count;
}

END_OF_FUNCTION(voice_start);



/* voice_stop:
 *  Stops a voice from playing.
 */
void voice_stop(int voice)
{
   if (_voice[voice].num >= 0)
      digi_driver->stop_voice(_voice[voice].num);
}

END_OF_FUNCTION(voice_stop);



/* voice_set_priority:
 *  Adjusts the priority of a voice (0-255).
 */
void voice_set_priority(int voice, int priority)
{
   _voice[voice].priority = priority;
}

END_OF_FUNCTION(voice_set_priority);



/* voice_check:
 *  Checks whether a voice is playing, returning the sample if it is,
 *  or NULL if it has finished or been preempted by a different sound.
 */
SAMPLE *voice_check(int voice)
{
   if (_voice[voice].sample) {
      if (_voice[voice].num < 0)
	 return NULL;

      if (_voice[voice].autokill)
	 if (voice_get_position(voice) < 0)
	    return NULL;

      return _voice[voice].sample;
   }
   else
      return NULL;
}

END_OF_FUNCTION(voice_check);



/* voice_get_position:
 *  Returns the current play position of a voice, or -1 if that cannot
 *  be determined (because it has finished or been preempted by a
 *  different sound).
 */
int voice_get_position(int voice)
{
   if (_voice[voice].num >= 0)
      return digi_driver->get_position(_voice[voice].num);
   else
      return -1;
}

END_OF_FUNCTION(voice_get_position);



/* voice_set_position:
 *  Sets the play position of a voice.
 */
void voice_set_position(int voice, int position)
{
   if (_voice[voice].num >= 0)
      digi_driver->set_position(_voice[voice].num, position);
}

END_OF_FUNCTION(voice_set_position);



/* voice_set_playmode:
 *  Sets the loopmode of a voice.
 */
void voice_set_playmode(int voice, int playmode)
{
   if (_voice[voice].num >= 0) {
      _phys_voice[_voice[voice].num].playmode = playmode;
      digi_driver->loop_voice(_voice[voice].num, playmode);

      if (playmode & PLAYMODE_BACKWARD)
	 digi_driver->set_position(_voice[voice].num, _voice[voice].sample->len-1);
   }
}

END_OF_FUNCTION(voice_set_playmode);



/* voice_get_volume:
 *  Returns the current volume of a voice, or -1 if that cannot
 *  be determined (because it has finished or been preempted by a
 *  different sound).
 */
int voice_get_volume(int voice)
{
   int vol;

   if (_voice[voice].num >= 0)
      vol = digi_driver->get_volume(_voice[voice].num);
   else
      vol = -1;

   if ((vol >= 0) && (_digi_volume >= 0)) {
      if (_digi_volume > 0)
	 vol = MID(0, (vol * 255) / _digi_volume, 255);
      else
	 vol = 0;
   }

   return vol;
}

END_OF_FUNCTION(voice_get_volume);



/* voice_set_volume:
 *  Sets the current volume of a voice.
 */
void voice_set_volume(int voice, int volume)
{
   if (_digi_volume >= 0)
      volume = (volume * _digi_volume) / 255;

   if (_voice[voice].num >= 0) {
      _phys_voice[_voice[voice].num].vol = volume << 12;
      _phys_voice[_voice[voice].num].dvol = 0;

      digi_driver->set_volume(_voice[voice].num, volume);
   }
}

END_OF_FUNCTION(voice_set_volume);



/* voice_ramp_volume:
 *  Begins a volume ramp operation.
 */
void voice_ramp_volume(int voice, int time, int endvol)
{
   if (_digi_volume >= 0)
      endvol = (endvol * _digi_volume) / 255;

   if (_voice[voice].num >= 0) {
      if (digi_driver->ramp_volume) {
	 digi_driver->ramp_volume(_voice[voice].num, time, endvol);
      }
      else {
	 int d = (endvol << 12) - _phys_voice[_voice[voice].num].vol;
	 time = MAX(time * SWEEP_FREQ / 1000, 1);
	 _phys_voice[_voice[voice].num].target_vol = endvol << 12;
	 _phys_voice[_voice[voice].num].dvol = d / time;
      }
   }
}

END_OF_FUNCTION(voice_ramp_volume);



/* voice_stop_volumeramp:
 *  Ends a volume ramp operation.
 */
void voice_stop_volumeramp(int voice)
{
   if (_voice[voice].num >= 0) {
      _phys_voice[_voice[voice].num].dvol = 0;

      if (digi_driver->stop_volume_ramp)
	 digi_driver->stop_volume_ramp(_voice[voice].num);
   }
}

END_OF_FUNCTION(voice_stop_volumeramp);



/* voice_get_frequency:
 *  Returns the current frequency of a voice, or -1 if that cannot
 *  be determined (because it has finished or been preempted by a
 *  different sound).
 */
int voice_get_frequency(int voice)
{
   if (_voice[voice].num >= 0)
      return digi_driver->get_frequency(_voice[voice].num);
   else
      return -1;
}

END_OF_FUNCTION(voice_get_frequency);



/* voice_set_frequency:
 *  Sets the pitch of a voice.
 */
void voice_set_frequency(int voice, int frequency)
{
   if (_voice[voice].num >= 0) {
      _phys_voice[_voice[voice].num].freq = frequency << 12;
      _phys_voice[_voice[voice].num].dfreq = 0;

      digi_driver->set_frequency(_voice[voice].num, frequency);
   }
}

END_OF_FUNCTION(voice_set_frequency);



/* voice_sweep_frequency:
 *  Begins a frequency sweep (glissando) operation.
 */
void voice_sweep_frequency(int voice, int time, int endfreq)
{
   if (_voice[voice].num >= 0) {
      if (digi_driver->sweep_frequency) {
	 digi_driver->sweep_frequency(_voice[voice].num, time, endfreq);
      }
      else {
	 int d = (endfreq << 12) - _phys_voice[_voice[voice].num].freq;
	 time = MAX(time * SWEEP_FREQ / 1000, 1);
	 _phys_voice[_voice[voice].num].target_freq = endfreq << 12;
	 _phys_voice[_voice[voice].num].dfreq = d / time;
      }
   }
}

END_OF_FUNCTION(voice_sweep_frequency);



/* voice_stop_frequency_sweep:
 *  Ends a frequency sweep.
 */
void voice_stop_frequency_sweep(int voice)
{
   if (_voice[voice].num >= 0) {
      _phys_voice[_voice[voice].num].dfreq = 0;

      if (digi_driver->stop_frequency_sweep)
	 digi_driver->stop_frequency_sweep(_voice[voice].num);
   }
}

END_OF_FUNCTION(voice_stop_frequency_sweep);



/* voice_get_pan:
 *  Returns the current pan position of a voice, or -1 if that cannot
 *  be determined (because it has finished or been preempted by a
 *  different sound).
 */
int voice_get_pan(int voice)
{
   int pan;

   if (_voice[voice].num >= 0)
      pan = digi_driver->get_pan(_voice[voice].num);
   else
      pan = -1;

   if ((pan >= 0) && (_flip_pan))
      pan = 255 - pan;

   return pan;
}

END_OF_FUNCTION(voice_get_pan);



/* voice_set_pan:
 *  Sets the pan position of a voice.
 */
void voice_set_pan(int voice, int pan)
{
   if (_flip_pan)
      pan = 255 - pan;

   if (_voice[voice].num >= 0) {
      _phys_voice[_voice[voice].num].pan = pan << 12;
      _phys_voice[_voice[voice].num].dpan = 0;

      digi_driver->set_pan(_voice[voice].num, pan);
   }
}

END_OF_FUNCTION(voice_set_pan);



/* voice_sweep_pan:
 *  Begins a pan sweep (left <-> right movement) operation.
 */
void voice_sweep_pan(int voice, int time, int endpan)
{
   if (_flip_pan)
      endpan = 255 - endpan;

   if (_voice[voice].num >= 0) {
      if (digi_driver->sweep_pan) {
	 digi_driver->sweep_pan(_voice[voice].num, time, endpan);
      }
      else {
	 int d = (endpan << 12) - _phys_voice[_voice[voice].num].pan;
	 time = MAX(time * SWEEP_FREQ / 1000, 1);
	 _phys_voice[_voice[voice].num].target_pan = endpan << 12;
	 _phys_voice[_voice[voice].num].dpan = d / time;
      }
   }
}

END_OF_FUNCTION(voice_sweep_pan);



/* voice_stop_pan_sweep:
 *  Ends a pan sweep.
 */
void voice_stop_pan_sweep(int voice)
{
   if (_voice[voice].num >= 0) {
      _phys_voice[_voice[voice].num].dpan = 0;

      if (digi_driver->stop_pan_sweep)
	 digi_driver->stop_pan_sweep(_voice[voice].num);
   }
}

END_OF_FUNCTION(voice_stop_pan_sweep);



/* voice_set_echo:
 *  Sets the echo parameters of a voice.
 */
void voice_set_echo(int voice, int strength, int delay)
{
   if ((_voice[voice].num >= 0) && (digi_driver->set_echo))
      digi_driver->set_echo(_voice[voice].num, strength, delay);
}

END_OF_FUNCTION(voice_set_echo);



/* voice_set_tremolo:
 *  Sets the tremolo parameters of a voice.
 */
void voice_set_tremolo(int voice, int rate, int depth)
{
   if ((_voice[voice].num >= 0) && (digi_driver->set_tremolo))
      digi_driver->set_tremolo(_voice[voice].num, rate, depth);
}

END_OF_FUNCTION(voice_set_tremolo);



/* voice_set_vibrato:
 *  Sets the vibrato parameters of a voice.
 */
void voice_set_vibrato(int voice, int rate, int depth)
{
   if ((_voice[voice].num >= 0) && (digi_driver->set_vibrato))
      digi_driver->set_vibrato(_voice[voice].num, rate, depth);
}

END_OF_FUNCTION(voice_set_vibrato);



/* update_sweeps:
 *  Timer callback routine used to implement volume/frequency/pan sweep 
 *  effects, for those drivers that can't do them directly.
 */
static void update_sweeps()
{
   int phys_voices, i;

   phys_voices = digi_driver->voices;
   if (midi_driver->max_voices < 0)
      phys_voices += midi_driver->voices;

   for (i=0; i<phys_voices; i++) {
      if (_phys_voice[i].num >= 0) {
	 /* update volume ramp */
	 if ((!digi_driver->ramp_volume) && (_phys_voice[i].dvol)) {
	    _phys_voice[i].vol += _phys_voice[i].dvol;

	    if (((_phys_voice[i].dvol > 0) && (_phys_voice[i].vol >= _phys_voice[i].target_vol)) ||
		((_phys_voice[i].dvol < 0) && (_phys_voice[i].vol <= _phys_voice[i].target_vol))) {
	       _phys_voice[i].vol = _phys_voice[i].target_vol;
	       _phys_voice[i].dvol = 0;
	    }

	    digi_driver->set_volume(i, _phys_voice[i].vol >> 12);
	 }

	 /* update frequency sweep */
	 if ((!digi_driver->sweep_frequency) && (_phys_voice[i].dfreq)) {
	    _phys_voice[i].freq += _phys_voice[i].dfreq;

	    if (((_phys_voice[i].dfreq > 0) && (_phys_voice[i].freq >= _phys_voice[i].target_freq)) ||
		((_phys_voice[i].dfreq < 0) && (_phys_voice[i].freq <= _phys_voice[i].target_freq))) {
	       _phys_voice[i].freq = _phys_voice[i].target_freq;
	       _phys_voice[i].dfreq = 0;
	    }

	    digi_driver->set_frequency(i, _phys_voice[i].freq >> 12);
	 }

	 /* update pan sweep */
	 if ((!digi_driver->sweep_pan) && (_phys_voice[i].dpan)) {
	    _phys_voice[i].pan += _phys_voice[i].dpan;

	    if (((_phys_voice[i].dpan > 0) && (_phys_voice[i].pan >= _phys_voice[i].target_pan)) ||
		((_phys_voice[i].dpan < 0) && (_phys_voice[i].pan <= _phys_voice[i].target_pan))) {
	       _phys_voice[i].pan = _phys_voice[i].target_pan;
	       _phys_voice[i].dpan = 0;
	    }

	    digi_driver->set_pan(i, _phys_voice[i].pan >> 12);
	 }
      }
   }
}

static END_OF_FUNCTION(update_sweeps);



/* sound_lock_mem:
 *  Locks memory used by the functions in this file.
 */
static void sound_lock_mem()
{
   LOCK_VARIABLE(digi_none);
   LOCK_VARIABLE(midi_none);
   LOCK_VARIABLE(digi_card);
   LOCK_VARIABLE(midi_card);
   LOCK_VARIABLE(digi_driver);
   LOCK_VARIABLE(midi_driver);
   LOCK_VARIABLE(_voice);
   LOCK_VARIABLE(_phys_voice);
   LOCK_VARIABLE(_digi_volume);
   LOCK_VARIABLE(_midi_volume);
   LOCK_VARIABLE(_flip_pan);
   LOCK_FUNCTION(_dummy_detect);
   LOCK_FUNCTION(play_sample);
   LOCK_FUNCTION(adjust_sample);
   LOCK_FUNCTION(stop_sample);
   LOCK_FUNCTION(allocate_voice);
   LOCK_FUNCTION(deallocate_voice);
   LOCK_FUNCTION(reallocate_voice);
   LOCK_FUNCTION(voice_start);
   LOCK_FUNCTION(voice_stop);
   LOCK_FUNCTION(voice_set_priority);
   LOCK_FUNCTION(voice_check);
   LOCK_FUNCTION(voice_get_position);
   LOCK_FUNCTION(voice_set_position);
   LOCK_FUNCTION(voice_set_playmode);
   LOCK_FUNCTION(voice_get_volume);
   LOCK_FUNCTION(voice_set_volume);
   LOCK_FUNCTION(voice_ramp_volume);
   LOCK_FUNCTION(voice_stop_volumeramp);
   LOCK_FUNCTION(voice_get_frequency);
   LOCK_FUNCTION(voice_set_frequency);
   LOCK_FUNCTION(voice_sweep_frequency);
   LOCK_FUNCTION(voice_stop_frequency_sweep);
   LOCK_FUNCTION(voice_get_pan);
   LOCK_FUNCTION(voice_set_pan);
   LOCK_FUNCTION(voice_sweep_pan);
   LOCK_FUNCTION(voice_stop_pan_sweep);
   LOCK_FUNCTION(voice_set_echo);
   LOCK_FUNCTION(voice_set_tremolo);
   LOCK_FUNCTION(voice_set_vibrato);
   LOCK_FUNCTION(update_sweeps);
}

