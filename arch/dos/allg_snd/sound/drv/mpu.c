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
 *      Direct output to an MPU-401 MIDI interface. 
 *
 *      Marcel de Kogel fixed my original broken version, so that it now 
 *      actually works :-)
 *
 *      See readme.txt for copyright information.
 */


#ifndef DJGPP
#error This file should only be used by the djgpp version of Allegro
#endif

#include <stdlib.h>
#include <stdio.h>
#include <dos.h>

#include "allegro.h"
#include "internal.h"


/* external interface to the MPU-401 driver */
static int mpu_detect();
static int mpu_init(int voices);
static void mpu_exit();
static void mpu_output(unsigned char data);

static char mpu_desc[80] = "not initialised";


MIDI_DRIVER midi_mpu401 =
{
   "MPU-401", 
   mpu_desc,
   0, 0, 0xFFFF, 0, -1, -1,
   mpu_detect,
   mpu_init,
   mpu_exit,
   NULL,
   mpu_output,
   _dummy_load_patches,
   _dummy_adjust_patches,
   _dummy_key_on,
   _dummy_noop1,
   _dummy_noop2,
   _dummy_noop3,
   _dummy_noop2,
   _dummy_noop2
};



/* wait_for_mpu:
 *  Waits for the specified bit to clear on the specified port. Returns zero
 *  if it cleared, -1 if it timed out.
 */
static inline int wait_for_mpu(int flag, int port)
{
   int timeout;

   for (timeout=0; timeout<0x7FFF; timeout++)
      if (!(inportb(port) & flag))
	 return 0;

   return -1;
}



/* mpu_output:
 *  Writes a byte to the MPU-401 midi interface.
 */
static void mpu_output(unsigned char data)
{
   outportb(_mpu_port, data);
   inportb (_mpu_port);
   wait_for_mpu(0x40, _mpu_port+1);
}

static END_OF_FUNCTION(mpu_output);



/* mpu_detect:
 *  Detects the presence of an MPU-401 compatible midi interface.
 */
static int mpu_detect()
{
   char *blaster = getenv("BLASTER");

   /* parse BLASTER env */
   if ((blaster) && (_mpu_port < 0)) { 
      while (*blaster) {
	 while ((*blaster == ' ') || (*blaster == '\t'))
	    blaster++;

	 if ((*blaster == 'p') || (*blaster == 'P'))
	    _mpu_port = strtol(blaster+1, NULL, 16);

	 while ((*blaster) && (*blaster != ' ') && (*blaster != '\t'))
	    blaster++;
      }
   }

   /* if that didn't work, guess :-) */
   if (_mpu_port < 0)
      _mpu_port = 0x330;

   /* check whether the MPU is there */
   outportb(_mpu_port+1,0xFF);                  /* reset the mpu */
   inportb(_mpu_port);
   if (wait_for_mpu(0x40, _mpu_port+1) != 0) {  /* wait for ready */
      strcpy(allegro_error, "MPU-401 not found");
      return FALSE;
   }

   sprintf(mpu_desc, "MPU-401 MIDI interface on port %d", _mpu_port);
   return TRUE;
}



/* mpu_init:
 *  Initialises the MPU-401 midi interface.
 */
static int mpu_init(int voices)
{
   outportb(_mpu_port+1, 0x3F);                 /* put MPU in UART mode */
   inportb (_mpu_port);
   wait_for_mpu (0x80,_mpu_port+1);

   LOCK_VARIABLE(midi_mpu401);
   LOCK_VARIABLE(_mpu_port);
   LOCK_FUNCTION(mpu_output);

   return 0;
}



/* mpu_exit:
 *  Resets the MPU-401 midi interface when we are finished.
 */
static void mpu_exit()
{
   outportb(_mpu_port+1, 0xFF);
}


