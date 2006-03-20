/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *
 *       Stub GUS driver. This is just a placeholder: the real code isn't
 *       written yet.
 */


#ifndef DJGPP
#error This file should only be used by the djgpp version of Allegro
#endif

#include <stdlib.h>
#include <stdio.h>

#include "allegro.h"
#include "internal.h"


static char gus_desc[80] = "not initialised";

static int gus_detect();

static int gus_digi_init();
static void gus_digi_exit();


DIGI_DRIVER digi_gus =        /* GUS driver for playing digital sfx */
{
   "Gravis Ultrasound", 
   gus_desc,
   0, 0, MIXER_MAX_SFX, MIXER_DEF_SFX,
   gus_detect,
   gus_digi_init,
   gus_digi_exit,
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


static int gus_midi_init();
static void gus_midi_exit();
static int gus_load_patches(char *patches, char *drums);
static void gus_key_on(int inst, int note, int bend, int vol, int pan);
static void gus_key_off(int voice);
static void gus_set_volume(int voice, int vol);
static void gus_set_pitch(int voice, int note, int bend);


MIDI_DRIVER midi_gus =        /* GUS driver for playing MIDI music */
{
   "Gravis Ultrasound", 
   gus_desc,
   0, 0, -1, 24, -1, -1,
   gus_detect,
   gus_midi_init,
   gus_midi_exit,
   NULL,
   NULL,
   gus_load_patches,
   _dummy_adjust_patches,
   gus_key_on,
   gus_key_off,
   gus_set_volume,
   gus_set_pitch,
   _dummy_noop2,
   _dummy_noop2
};


static void gus_lock_data();



/* gus_detect:
 *  GUS detection routine. Returns TRUE if a GUS exists, otherwise FALSE.
 *  It shouldn't do anything that drastically alters the state of the
 *  card, since it is likely to get called more than once.
 */
static int gus_detect()
{
   strcpy(allegro_error, "GUS driver not written yet");

   /* if (no GUS present) */
      return FALSE;

   sprintf(gus_desc, "Some info about the GUS hardware");
   return TRUE;
}



/* gus_digi_init:
 *  Called once at startup to initialise the digital sample playing code.
 */
static int gus_digi_init()
{
   gus_lock_data();

#if GOT_SOME_GUS_CODE_TO_PUT_IN_HERE
   /* Allocate conventional memory buffers for the DMA transfer. Pass the
    * size of the buffer in bytes, an int pointer which will be set to the
    * protected mode selector to free the buffer with, and a pointer to a
    * long which will be set to the linear address of the buffer in 
    * conventional memory.
    */
   if (_dma_allocate_mem(buffer_size, &(int)selector, &(long)address) != 0)
      return -1;

   /* Initialise the sample mixing module. Pass the size of each buffer
    * (in samples, not bytes: if you pass 1024 for this parameter you would
    * need a _dma_allocate_mem of 1024 bytes if you are using 8 bit data, but 
    * of 2048 if in 16 bit mode. Don't double the size for stereo modes 
    * though: in an 8 bit stereo mode passing 1024 will still cause it to
    * mix 1024 bytes into each buffer, just the buffer will only last for
    * half as long...). Also pass the sample frequency the GUS is running at,
    * and whether you want stereo and/or 16 bit data.
    */
   if (_mixer_init(buffer_size, gus_frequency, stereo, 16bit) != 0)
      return -1;

   /*
   Apart from those calls, do whatever GUS-type stuff you need to do, set
   the dma transfer going (_dma_start(channel, addr, size, auto_init_flag)),
   and whenever a buffer needs filling with some more samples, call
   _mix_some_samples(linear_address_of_buffer_in_conventional_memory);
   btw. the 16 bit sample mixing and 16 bit DMA transfer modes are not
   tested, since I only have an 8 bit SB. If they don't work, it might well
   be my fault :-)
   */
#endif

   return 0;
}



/* gus_digi_exit:
 *  gus driver cleanup routine, removes ints, stops dma, frees buffers, etc.
 */
static void gus_digi_exit()
{
#if GOT_SOME_GUS_CODE_TO_PUT_IN_HERE

   _dma_stop(whatever_dma_channel_you_are_using);
   __dpmi_free_dos_memory(the selector returned by _dma_allocate_mem());
   _mixer_exit();

   etc.

#endif
}



/* gus_midi_init:
 *  Called once at startup to setup the GUS MIDI driver. 
 */
static int gus_midi_init()
{
   gus_lock_data();

   /* etc */

   return 0;
}



/* gus_midi_exit:
 *  Cleanup the MIDI driver. I ought to have turned off all the active
 *  voices by the time this gets called, but it wouldn't hurt to reset
 *  them all anyway.
 */
static void gus_midi_exit()
{
}



/* gus_load_patches:
 *  Called before starting to play a MIDI file, to load all the patches it
 *  uses into GUS ram. No MIDI voices will be active when this is called,
 *  so you are free to unload whatever old data you want. Patches points
 *  to an array of 128 flags indicating which GM sounds are used in the
 *  piece, and drums to an array of 128 flags indicating which drum sounds
 *  are used (the GM standard only defines drum sounds 35 (base drum) to
 *  81 (open triangle), so flags outside that range are guarranteed to be
 *  zero). Should return 0 for success, -1 on failure.
 *
 *  What should we do if there isn't enough GUS ram to load all the samples?
 *  Return an error code and not play the midi file? Or replace some patches
 *  with others? And if so, should this routine do those replacements, or
 *  should you just return an error code to me, and me call you again with
 *  a reduced list of requirements? If we are going to do patch replacements,
 *  it would seem sensible to do it at a higher level than the GUS driver,
 *  so at a later date any other wavetable drivers could use the same code.
 *  What do you think?
 */
static int gus_load_patches(char *patches, char *drums)
{
   return 0;
}

static END_OF_FUNCTION(gus_load_patches);



/* gus_key_on:
 *  Triggers the specified MIDI voice. The instrument is specified as a 
 *  GM patch number. Drums are indicated by passing an instrument greater 
 *  than 128, in which case you should trigger the GM percussion sound 
 *  normally mapped to drum key (inst-128). Drum sounds can ignore the 
 *  pitch, bend, and pan parameters, but should respond to volume. The 
 *  pitch is a midi note number (60=middle C, I think...). Pan and 
 *  volume are from 0 (left/min) to 127 (right/max). The bend isn't a midi 
 *  pitch bend value: it ranges from 0 (normal pitch) to 0xFFF (just a 
 *  fraction flatter than a semitone up).
 */
static void gus_key_on(int inst, int note, int bend, int vol, int pan)
{
   int voice = _midi_allocate_voice(-1, -1);
   if (voice < 0)
      return;

   /* after you set the sample to play and trigger the note, this is pretty 
    * much equivelant to:
    */
   gus_set_pitch(voice, note, bend);
   gus_set_volume(voice, vol);
}

static END_OF_FUNCTION(gus_key_on);



/* gus_key_off:
 *  Hey, guess what this does :-)
 */
static void gus_key_off(int voice)
{
   /* do something */
}

static END_OF_FUNCTION(gus_key_off);



/* gus_set_volume:
 *  Alters the volume of the specified voice (vol range 0-127). Should only
 *  ever be called while the voice is playing...
 */
static void gus_set_volume(int voice, int vol)
{
   /* do something */
}

static END_OF_FUNCTION(gus_set_volume);



/* gus_set_pitch:
 *  Alters the pitch of the specified voice. Should only be called while
 *  the voice is playing...
 */
static void gus_set_pitch(int voice, int note, int bend)
{
   /* do something */
}

static END_OF_FUNCTION(gus_set_pitch);



/*
 *  Locks all the memory touched by parts of the GUS code that are executed
 *  in an interrupt context.
 */
static void gus_lock_data()
{
   LOCK_VARIABLE(digi_gus);
   LOCK_VARIABLE(midi_gus);
   LOCK_FUNCTION(gus_load_patches);
   LOCK_FUNCTION(gus_key_on);
   LOCK_FUNCTION(gus_key_off);
   LOCK_FUNCTION(gus_set_volume);
   LOCK_FUNCTION(gus_set_pitch);
}

