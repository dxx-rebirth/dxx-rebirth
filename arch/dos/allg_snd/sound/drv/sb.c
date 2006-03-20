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
 *      Soundblaster driver. Supports DMA driven sample playback (mixing 
 *      up to eight samples at a time) and raw note output to the SB MIDI 
 *      port. The Adlib (FM synth) MIDI playing code is in adlib.c.
 *
 *      See readme.txt for copyright information.
 */


#ifndef DJGPP
#error This file should only be used by the djgpp version of Allegro
#endif

#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <go32.h>
#include <dpmi.h>
#include <limits.h>
#include <sys/farptr.h>

#include "allegro.h"
#include "internal.h"


/* external interface to the digital SB driver */
static int sb_detect();
static int sb_init(int voices);
static void sb_exit();
static int sb_mixer_volume(int volume);

static char sb_desc[80] = "not initialised";


DIGI_DRIVER digi_sb =
{
   "Sound Blaster", 
   sb_desc,
   0, 0, MIXER_MAX_SFX, MIXER_DEF_SFX,
   sb_detect,
   sb_init,
   sb_exit,
   sb_mixer_volume,
   _mixer_init_voice,
   _mixer_release_voice,
   _mixer_start_voice,
   _mixer_stop_voice,
   _mixer_loop_voice,
   _mixer_get_position,
   _mixer_set_position,
   _mixer_get_volume,
   _mixer_set_volume,
   _mixer_ramp_volume,
   _mixer_stop_volume_ramp,
   _mixer_get_frequency,
   _mixer_set_frequency,
   _mixer_sweep_frequency,
   _mixer_stop_frequency_sweep,
   _mixer_get_pan,
   _mixer_set_pan,
   _mixer_sweep_pan,
   _mixer_stop_pan_sweep,
   _mixer_set_echo,
   _mixer_set_tremolo,
   _mixer_set_vibrato
};


/* external interface to the SB midi output driver */
static int sb_midi_init(int voices);
static void sb_midi_exit();
static void sb_midi_output(unsigned char data);

static char sb_midi_desc[80] = "not initialised";


MIDI_DRIVER midi_sb_out =
{
   "SB MIDI interface", 
   sb_midi_desc,
   0, 0, 0xFFFF, 0, -1, -1,
   sb_detect,
   sb_midi_init,
   sb_midi_exit,
   NULL,
   sb_midi_output,
   _dummy_load_patches,
   _dummy_adjust_patches,
   _dummy_key_on,
   _dummy_noop1,
   _dummy_noop2,
   _dummy_noop3,
   _dummy_noop2,
   _dummy_noop2
};


static int sb_in_use = FALSE;             /* is SB being used? */
static int sb_stereo = FALSE;             /* in stereo mode? */
static int sb_16bit = FALSE;              /* in 16 bit mode? */
static int sb_int = -1;                   /* interrupt vector */
static int sb_dsp_ver = -1;               /* SB DSP version */
/*static*/ int sb_hw_dsp_ver = -1;            /* as reported by autodetect */
static int sb_dma_size = -1;              /* size of dma transfer in bytes */
static int sb_dma_mix_size = -1;          /* number of samples to mix */
static int sb_dma_count = 0;              /* need to resync with dma? */
static volatile int sb_semaphore = FALSE; /* reentrant interrupt? */

static int sb_sel[2];                     /* selectors for the buffers */
static unsigned long sb_buf[2];           /* pointers to the two buffers */
static int sb_bufnum = 0;                 /* the one currently in use */

static unsigned char sb_default_pic1;     /* PIC mask flags to restore */
static unsigned char sb_default_pic2;

static int sb_master_vol;                 /* stored mixer settings */
static int sb_digi_vol;
static int sb_fm_vol;

static void sb_lock_mem();



/* sb_read_dsp:
 *  Reads a byte from the SB DSP chip. Returns -1 if it times out.
 */
static inline volatile int sb_read_dsp()
{
   int x;

   for (x=0; x<0xffff; x++)
      if (inportb(0x0E + _sb_port) & 0x80)
	 return inportb(0x0A+_sb_port);

   return -1; 
}



/* sb_write_dsp:
 *  Writes a byte to the SB DSP chip. Returns -1 if it times out.
 */
static inline volatile int sb_write_dsp(unsigned char byte)
{
   int x;

   for (x=0; x<0xffff; x++) {
      if (!(inportb(0x0C+_sb_port) & 0x80)) {
	 outportb(0x0C+_sb_port, byte);
	 return 0;
      }
   }
   return -1; 
}



/* sb_voice:
 *  Turns the SB speaker on or off.
 */
static void sb_voice(int state)
{
   if (state) {
      sb_write_dsp(0xD1);

      if (sb_hw_dsp_ver >= 0x300) {       /* set up the mixer */

	 outportb(_sb_port+4, 0x22);      /* store master volume */
	 sb_master_vol = inportb(_sb_port+5);

	 outportb(_sb_port+4, 4);         /* store DAC level */
	 sb_digi_vol = inportb(_sb_port+5);

	 outportb(_sb_port+4, 0x26);      /* store FM level */
	 sb_fm_vol = inportb(_sb_port+5);
      }
   }
   else {
      sb_write_dsp(0xD3);

      if (sb_hw_dsp_ver >= 0x300) {       /* reset previous mixer settings */

	 outportb(_sb_port+4, 0x22);      /* restore master volume */
	 outportb(_sb_port+5, sb_master_vol);

	 outportb(_sb_port+4, 4);         /* restore DAC level */
	 outportb(_sb_port+5, sb_digi_vol);

	 outportb(_sb_port+4, 0x26);      /* restore FM level */
	 outportb(_sb_port+5, sb_fm_vol);
      }
   }
}



/* _sb_set_mixer:
 *  Alters the SB-Pro hardware mixer.
 */
int _sb_set_mixer(int digi_volume, int midi_volume)
{
   if (sb_hw_dsp_ver < 0x300)
      return -1;

   if (digi_volume >= 0) {                   /* set DAC level */
      outportb(_sb_port+4, 4);
      outportb(_sb_port+5, (digi_volume & 0xF0) | (digi_volume >> 4));
   }

   if (midi_volume >= 0) {                   /* set FM level */
      outportb(_sb_port+4, 0x26);
      outportb(_sb_port+5, (midi_volume & 0xF0) | (midi_volume >> 4));
   }

   return 0;
}



/* sb_mixer_volume:
 *  Sets the SB mixer volume for playing digital samples.
 */
static int sb_mixer_volume(int volume)
{
   return _sb_set_mixer(volume, -1);
}



/* sb_stereo_mode:
 *  Enables or disables stereo output for SB-Pro.
 */
static void sb_stereo_mode(int enable)
{
   outportb(_sb_port+0x04, 0x0E); 
   outportb(_sb_port+0x05, (enable ? 2 : 0));
}



/* sb_set_sample_rate:
 *  The parameter is the rate to set in Hz (samples per second).
 */
static void sb_set_sample_rate(unsigned int rate)
{
   if (sb_16bit) {
      sb_write_dsp(0x41);
      sb_write_dsp(rate >> 8);
      sb_write_dsp(rate & 0xff);
   }
   else {
      if (sb_stereo)
	 rate *= 2;

      sb_write_dsp(0x40);
      sb_write_dsp((unsigned char)(256-1000000/rate));
   }
}



/* sb_reset_dsp:
 *  Resets the SB DSP chip, returning -1 on error.
 */
static int sb_reset_dsp()
{
   int x;

   outportb(0x06+_sb_port, 1);

   for (x=0; x<8; x++)
      inportb(0x06+_sb_port);

   outportb(0x06+_sb_port, 0);

   if (sb_read_dsp() != 0xAA)
      return -1;

   return 0;
}



/* _sb_read_dsp_version:
 *  Reads the version number of the SB DSP chip, returning -1 on error.
 */
int _sb_read_dsp_version()
{
   int x, y;

   if (sb_hw_dsp_ver > 0)
      return sb_hw_dsp_ver;

   if (_sb_port <= 0)
      _sb_port = 0x220;

   if (sb_reset_dsp() != 0)
      sb_hw_dsp_ver = -1;
   else {
      sb_write_dsp(0xE1);
      x = sb_read_dsp();
      y = sb_read_dsp();
      sb_hw_dsp_ver = ((x << 8) | y);
   }

   return sb_hw_dsp_ver;
}



/* sb_play_buffer:
 *  Starts a dma transfer of size bytes. On cards capable of it, the
 *  transfer will use auto-initialised dma, so there is no need to call
 *  this routine more than once. On older cards it must be called from
 *  the end-of-buffer handler to switch to the new buffer.
 */
static void sb_play_buffer(int size)
{
   if (sb_dsp_ver <= 0x200) {                /* 8 bit single-shot */
      sb_write_dsp(0x14);
      sb_write_dsp((size-1) & 0xFF);
      sb_write_dsp((size-1) >> 8);
   }
   else if (sb_dsp_ver < 0x400) {            /* 8 bit auto-initialised */
      sb_write_dsp(0x48);
      sb_write_dsp((size-1) & 0xff);
      sb_write_dsp((size-1) >> 8);
      sb_write_dsp(0x90);
   }
   else {                                    /* 16 bit */
      size /= 2;
      sb_write_dsp(0xB6);
      sb_write_dsp(0x20);
      sb_write_dsp((size-1) & 0xFF);
      sb_write_dsp((size-1) >> 8);
   }
}

static END_OF_FUNCTION(sb_play_buffer);



/* sb_interrupt:
 *  The SB end-of-buffer interrupt handler. Swaps to the other buffer 
 *  if the card doesn't have auto-initialised dma, and then refills the
 *  buffer that just finished playing.
 */
static int sb_interrupt()
{
   if (sb_dsp_ver <= 0x200) {                /* not auto-initialised */
      _dma_start(_sb_dma, sb_buf[1-sb_bufnum], sb_dma_size, FALSE);
      sb_play_buffer(sb_dma_size);
   }
   else {                                    /* poll dma position */
      sb_dma_count++;
      if (sb_dma_count > 16) {
	 sb_bufnum = (_dma_todo(_sb_dma) > (unsigned)sb_dma_size) ? 1 : 0;
	 sb_dma_count = 0;
      }
   }

   if (!sb_semaphore) {
      sb_semaphore = TRUE;

      ENABLE();                              /* mix some more samples */
      _mix_some_samples(sb_buf[sb_bufnum], _dos_ds, FALSE);
      DISABLE();

      sb_semaphore = FALSE;
   } 

   sb_bufnum = 1 - sb_bufnum; 

   if (sb_16bit)                             /* acknowlege SB */
      inportb(_sb_port+0x0F);
   else
      inportb(_sb_port+0x0E);

   outportb(0x20, 0x20);                     /* acknowledge interrupt */
   outportb(0xA0, 0x20);

   return 0;
}

static END_OF_FUNCTION(sb_interrupt);



/* sb_detect:
 *  SB detection routine. Uses the BLASTER environment variable,
 *  or 'sensible' guesses if that doesn't exist.
 */
static int sb_detect()
{
   char *blaster = getenv("BLASTER");
   char *msg;
   int dma8 = 1;
   int dma16 = 5;
   int max_freq;
   int default_freq;

   /* what breed of SB are we looking for? */
   switch (digi_card) {

      case DIGI_SB10:
	 sb_dsp_ver = 0x100;
	 break;

      case DIGI_SB15:
	 sb_dsp_ver = 0x200;
	 break;

      case DIGI_SB20:
	 sb_dsp_ver = 0x201;
	 break;

      case DIGI_SBPRO:
	 sb_dsp_ver = 0x300;
	 break;

      case DIGI_SB16:
	 sb_dsp_ver = 0x400;
	 break;

      default:
	 sb_dsp_ver = -1;
	 break;
   } 

   /* parse BLASTER env */
   if (blaster) { 
      while (*blaster) {
	 while ((*blaster == ' ') || (*blaster == '\t'))
	    blaster++;

	 if (*blaster) {
	    switch (*blaster) {
	       case 'a': case 'A':
		  if (_sb_port < 0)
		     _sb_port = strtol(blaster+1, NULL, 16);
		  break;

	       case 'i': case 'I':
		  if (_sb_irq < 0)
		     _sb_irq = strtol(blaster+1, NULL, 10);
                  //added on 11/5/98 by Victor Rachels to hardwire irq 2->9
                      if (_sb_irq == 0x02)
                       _sb_irq=0x09;
                  //end this section addition
		  break;

	       case 'd': case 'D':
		  dma8 = strtol(blaster+1, NULL, 10);
		  break;

	       case 'h': case 'H':
		  dma16 = strtol(blaster+1, NULL, 10);
		  break;
	    }

	    while ((*blaster) && (*blaster != ' ') && (*blaster != '\t'))
	       blaster++;
	 }
      }
   }

   if (_sb_port < 0)
      _sb_port = 0x220;

   if (_sb_irq < 0)
      _sb_irq = 7;

   /* make sure we got a good port address */
   if (sb_reset_dsp() != 0) { 
      static int bases[] = { 0x210, 0x220, 0x230, 0x240, 0x250, 0x260, 0 };
      int i;

      for (i=0; bases[i]; i++) {
	 _sb_port = bases[i];
	 if (sb_reset_dsp() == 0)
	    break;
      }
   }

   /* check if the card really exists */
   _sb_read_dsp_version();
   if (sb_hw_dsp_ver < 0) {
      strcpy(allegro_error, "Sound Blaster not found");
      return FALSE;
   }

   if (sb_dsp_ver < 0)
      sb_dsp_ver = sb_hw_dsp_ver;
   else {
      if (sb_dsp_ver > sb_hw_dsp_ver) {
	 sb_hw_dsp_ver = sb_dsp_ver = -1;
	 strcpy(allegro_error, "Older SB version detected");
	 return FALSE;
      }
   }

   /* figure out the hardware interrupt number */
   if (_sb_irq > 7)
      sb_int = _sb_irq + 104;
   else
      sb_int = _sb_irq + 8;

   /* what breed of SB? */
   if (sb_dsp_ver >= 0x400) {
      msg = "SB 16";
      max_freq = 45454;
      default_freq = 22727;
   }
   else if (sb_dsp_ver >= 0x300) {
      msg = "SB Pro";
      max_freq = 22727;
      default_freq = 22727;
   }
   else if (sb_dsp_ver >= 0x201) {
      msg = "SB 2.0";
      max_freq = 45454;
      default_freq = 22727;
   }
   else if (sb_dsp_ver >= 0x200) {
      msg = "SB 1.5";
      max_freq = 16129;
      default_freq = 16129;
   }
   else {
      msg = "SB 1.0";
      max_freq = 16129;
      default_freq = 16129;
   }

   /* set up the playback frequency */
   if (_sb_freq <= 0)
      _sb_freq = default_freq;

   if (_sb_freq < 15000) {
      _sb_freq = 11906;
      sb_dma_size = 128;
   }
   else if (MIN(_sb_freq, max_freq) < 20000) {
      _sb_freq = 16129;
      sb_dma_size = 128;
   }
   else if (MIN(_sb_freq, max_freq) < 40000) {
      _sb_freq = 22727;
      sb_dma_size = 256;
   }
   else {
      _sb_freq = 45454;
      sb_dma_size = 512;
   }

   if (sb_dsp_ver <= 0x200)
      sb_dma_size *= 4;

   sb_dma_mix_size = sb_dma_size;

   /* can we handle 16 bit playback? */
   if (sb_dsp_ver >= 0x400) { 
      if (_sb_dma < 0)
	 _sb_dma = dma16;
      sb_16bit = TRUE;
      sb_dma_size <<= 1;
   }
   else { 
      if (_sb_dma < 0)
	 _sb_dma = dma8;
      sb_16bit = FALSE;
   }

   /* can we handle stereo? */
   if (sb_dsp_ver >= 0x300) {
      sb_stereo = TRUE;
      sb_dma_size <<= 1;
      sb_dma_mix_size <<= 1;
   }
   else
      sb_stereo = FALSE;

   /* set up the card description */
   sprintf(sb_desc, "%s (%d hz) on port %X, using IRQ %d and DMA channel %d",
			msg, _sb_freq, _sb_port, _sb_irq, _sb_dma);

   return TRUE;
}



/* sb_init:
 *  SB init routine: returns zero on success, -1 on failure.
 */
static int sb_init(int voices)
{
   if (sb_in_use) {
      strcpy(allegro_error, "Can't use SB MIDI interface and DSP at the same time");
      return -1;
   }

   if ((digi_card == DIGI_SB) || (digi_card == DIGI_AUTODETECT)) {
      if (sb_dsp_ver <= 0x100)
	 digi_card = DIGI_SB10;
      else if (sb_dsp_ver <= 0x200)
	 digi_card = DIGI_SB15;
      else if (sb_dsp_ver < 0x300)
	 digi_card = DIGI_SB20;
      else if (sb_dsp_ver < 0x400)
	 digi_card = DIGI_SBPRO;
      else
	 digi_card = DIGI_SB16;
   }

   if (sb_dsp_ver <= 0x200) {       /* two conventional mem buffers */
      if ((_dma_allocate_mem(sb_dma_size, &sb_sel[0], &sb_buf[0]) != 0) ||
	  (_dma_allocate_mem(sb_dma_size, &sb_sel[1], &sb_buf[1]) != 0))
	 return -1;
   }
   else {                           /* auto-init dma, one big buffer */
      if (_dma_allocate_mem(sb_dma_size*2, &sb_sel[0], &sb_buf[0]) != 0)
	 return -1;

      sb_sel[1] = sb_sel[0];
      sb_buf[1] = sb_buf[0] + sb_dma_size;
   }

   sb_lock_mem();

   digi_sb.voices = voices;

   if (_mixer_init(sb_dma_mix_size, _sb_freq, sb_stereo, sb_16bit, &digi_sb.voices) != 0)
      return -1;

   _mix_some_samples(sb_buf[0], _dos_ds, FALSE);
   _mix_some_samples(sb_buf[1], _dos_ds, FALSE);
   sb_bufnum = 0;

   sb_default_pic1 = inportb(0x21);
   sb_default_pic2 = inportb(0xA1);

   if (_sb_irq > 7) {               /* enable irq2 and PIC-2 irq */
      outportb(0x21, sb_default_pic1 & 0xFB); 
      outportb(0xA1, sb_default_pic2 & (~(1<<(_sb_irq-8))));
   }
   else                             /* enable PIC-1 irq */
      outportb(0x21, sb_default_pic1 & (~(1<<_sb_irq)));

   _install_irq(sb_int, sb_interrupt);

   sb_voice(1);
   sb_set_sample_rate(_sb_freq);

   if ((sb_hw_dsp_ver >= 0x300) && (sb_dsp_ver < 0x400))
      sb_stereo_mode(sb_stereo);

   if (sb_dsp_ver <= 0x200)
      _dma_start(_sb_dma, sb_buf[0], sb_dma_size, FALSE);
   else
      _dma_start(_sb_dma, sb_buf[0], sb_dma_size*2, TRUE);

   sb_play_buffer(sb_dma_size);

   sb_in_use = TRUE;
   return 0;
}



/* sb_exit:
 *  SB driver cleanup routine, removes ints, stops dma, frees buffers, etc.
 */
static void sb_exit()
{
   /* halt sound output */
   sb_voice(0);

   /* stop dma transfer */
   _dma_stop(_sb_dma);

   if (sb_dsp_ver <= 0x0200)
      sb_write_dsp(0xD0); 

   sb_reset_dsp();

   /* restore interrupts */
   _remove_irq(sb_int);

   /* reset PIC channels */
   outportb(0x21, sb_default_pic1);
   outportb(0xA1, sb_default_pic2);

   /* free conventional memory buffer */
   __dpmi_free_dos_memory(sb_sel[0]);
   if (sb_sel[1] != sb_sel[0])
      __dpmi_free_dos_memory(sb_sel[1]);

   _mixer_exit();

   sb_hw_dsp_ver = sb_dsp_ver = -1;
   sb_in_use = FALSE;
}



/* sb_midi_init:
 *  Initialises the SB midi interface, returning zero on success.
 */
static int sb_midi_init(int voices)
{
   if (sb_in_use) {
      strcpy(allegro_error, "Can't use SB MIDI interface and DSP at the same time");
      return -1;
   }

   sb_dsp_ver = -1;

   sb_lock_mem();

   sprintf(sb_midi_desc, "Sound Blaster MIDI interface on port %X", _sb_port);

   sb_in_use = TRUE;
   return 0;
}



/* sb_midi_exit:
 *  Resets the SB midi interface when we are finished.
 */
static void sb_midi_exit()
{
   sb_reset_dsp();
   sb_in_use = FALSE;
}



/* sb_midi_output:
 *  Writes a byte to the SB midi interface.
 */
static void sb_midi_output(unsigned char data)
{
   sb_write_dsp(0x38);
   sb_write_dsp(data);
}

static END_OF_FUNCTION(sb_midi_output);



/* sb_lock_mem:
 *  Locks all the memory touched by parts of the SB code that are executed
 *  in an interrupt context.
 */
static void sb_lock_mem()
{
   LOCK_VARIABLE(digi_sb);
   LOCK_VARIABLE(midi_sb_out);
   LOCK_VARIABLE(_sb_freq);
   LOCK_VARIABLE(_sb_port);
   LOCK_VARIABLE(_sb_dma);
   LOCK_VARIABLE(_sb_irq);
   LOCK_VARIABLE(sb_int);
   LOCK_VARIABLE(sb_in_use);
   LOCK_VARIABLE(sb_dsp_ver);
   LOCK_VARIABLE(sb_hw_dsp_ver);
   LOCK_VARIABLE(sb_dma_size);
   LOCK_VARIABLE(sb_dma_mix_size);
   LOCK_VARIABLE(sb_sel);
   LOCK_VARIABLE(sb_buf);
   LOCK_VARIABLE(sb_bufnum);
   LOCK_VARIABLE(sb_default_pic1);
   LOCK_VARIABLE(sb_default_pic2);
   LOCK_VARIABLE(sb_dma_count);
   LOCK_VARIABLE(sb_semaphore);
   LOCK_FUNCTION(sb_play_buffer);
   LOCK_FUNCTION(sb_interrupt);
   LOCK_FUNCTION(sb_midi_output);
}

