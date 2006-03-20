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
 *      List of available sound drivers, kept in a seperate file so that
 *      they can be overriden by user programs.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"



DECLARE_DIGI_DRIVER_LIST
(
   DIGI_DRIVER_GUS
   DIGI_DRIVER_SB
)


DECLARE_MIDI_DRIVER_LIST
(
   MIDI_DRIVER_AWE32
   MIDI_DRIVER_DIGMID
   MIDI_DRIVER_ADLIB
   MIDI_DRIVER_SB_OUT
   MIDI_DRIVER_MPU
)

