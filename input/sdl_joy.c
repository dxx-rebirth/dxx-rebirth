/*
 * $Source: /cvs/cvsroot/d2x/input/sdl_joy.c,v $
 * $Revision: 1.1 $
 * $Author: bradleyb $
 * $Date: 2001-10-10 03:01:29 $
 *
 * SDL joystick support
 *
 * $Log: not supported by cvs2svn $
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <SDL/SDL.h>

#include "joy.h"
#include "error.h"

char joy_present = 0;

int
joy_init()
{
  if (SDL_Init( SDL_INIT_JOYSTICK ) < 0)
    {
      Warning("SDL library initialisation failed: %s.", SDL_GetError());
      return 0;
    }



  return 1;
}

void
joy_close()
{
  return;
}

void
joy_get_pos(int *x, int *y)
{
  *x = *y = 0;
  return;
}

int
joy_get_btns()
{
  return 0;
}

int
joy_get_button_down_cnt(int btn)
{
  return 0;
}

fix
joy_get_button_down_time(int btn)
{
  return 0;
}

ubyte
joystick_read_raw_axis(ubyte mask, int *axis)
{
  return 0;
}

void
joy_flush()
{
  return;
}

int
joy_get_button_state(int btn)
{
  return 0;
}

void
joy_get_cal_vals(int *axis_min, int *axis_center, int *axis_max)
{
  return;
}

void
joy_set_cal_vals(int *axis_min, int *axis_center, int *axis_max)
{
  return;
}

int
joy_get_scaled_reading(int raw, int axn)
{
  return 0;
}

void
joy_set_slow_reading(int flag)
{
  return;
}
