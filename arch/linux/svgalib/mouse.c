#include <string.h>
#include <vga.h>
#include <vgamouse.h>
#include "fix.h"
#include "timer.h"
#include "event.h"
#include "mouse.h"

ubyte installed = 0;

struct mousebutton {
 ubyte pressed;
 fix time_went_down;
 fix time_held_down;
 uint num_downs;
 uint num_ups;
};

static struct mouseinfo {
 struct mousebutton buttons[MOUSE_MAX_BUTTONS];
//added on 10/17/98 by Hans de Goede for mouse functionality
 int min_x, min_y;
 int max_x, max_y;
 int delta_x, delta_y;
 int x,y;
//end this section addition - Hans
} Mouse;

void mouse_handler (int button, int dx, int dy, int dz, int drx, int dry, int drz)
{
 int i;
 for (i = 0; i < 8; i++)
 { 
 if (button & (1 << i))
  {
   Mouse.buttons[i].pressed = 1;
   Mouse.buttons[i].time_went_down = timer_get_fixed_seconds();
   Mouse.buttons[i].num_downs++;
  }
  else
  {
   Mouse.buttons[i].pressed = 0;
   Mouse.buttons[i].time_held_down += timer_get_fixed_seconds() - Mouse.buttons[i].time_went_down;
   Mouse.buttons[i].num_ups++;
  }
 }
 Mouse.delta_x += dx;
 Mouse.delta_y += dy;
 Mouse.x += dx;
 Mouse.y += dy;
 if      (Mouse.x > Mouse.max_x) Mouse.x = Mouse.max_x;
 else if (Mouse.x < Mouse.min_x) Mouse.x = Mouse.min_x;
 if      (Mouse.y > Mouse.max_y) Mouse.y = Mouse.max_y;
 else if (Mouse.y < Mouse.min_y) Mouse.y = Mouse.min_y;
}

void d_mouse_close(void)
{
 if (installed)
	vga_setmousesupport(0);
 installed = 0;
}

void d_mouse_init(void)
{
 memset(&Mouse,0,sizeof(Mouse));
 vga_setmousesupport(1);
 if (!installed)
	atexit(d_mouse_close);
 installed = 1;
}

int mouse_set_limits( int x1, int y1, int x2, int y2 )
{
 event_poll();
 Mouse.min_x = x1;
 Mouse.min_y = y1;
 Mouse.max_x = x2;
 Mouse.max_y = y2;
 return 0;
}

void mouse_flush()	// clears all mice events...
{
 int i;
 fix current_time;

 event_poll();

 current_time = timer_get_fixed_seconds();
 for (i=0; i<MOUSE_MAX_BUTTONS; i++)
 {
   Mouse.buttons[i].pressed=0;
   Mouse.buttons[i].time_went_down=current_time;
   Mouse.buttons[i].time_held_down=0;
   Mouse.buttons[i].num_ups=0;
   Mouse.buttons[i].num_downs=0;
 }
//added on 10/17/98 by Hans de Goede for mouse functionality 
 Mouse.delta_x = 0;
 Mouse.delta_y = 0;
 Mouse.x = 0;
 Mouse.y = 0;
//end this section addition - Hans
}

//========================================================================
void mouse_get_pos( int *x, int *y)
{
  event_poll();
  *x = Mouse.x;
  *y = Mouse.y;
}

void mouse_get_delta( int *dx, int *dy )
{
 event_poll();
 *dx = Mouse.delta_x;
 *dy = Mouse.delta_y;
 Mouse.delta_x = 0;
 Mouse.delta_y = 0;
}

int mouse_get_btns()
{
	int i;
        uint flag=1;
	int status = 0;

        event_poll();
	
	for (i=0; i<MOUSE_MAX_BUTTONS; i++ )
        {
		if (Mouse.buttons[i].pressed)
			status |= flag;
		flag <<= 1;
	}
	return status;
}

void mouse_set_pos( int x, int y)
{
 event_poll();
 Mouse.x=x;
 Mouse.y=y;
 if      (Mouse.x > Mouse.max_x) Mouse.x = Mouse.max_x;
 else if (Mouse.x < Mouse.min_x) Mouse.x = Mouse.min_x;
 if      (Mouse.y > Mouse.max_y) Mouse.y = Mouse.max_y;
 else if (Mouse.y < Mouse.min_y) Mouse.y = Mouse.min_y;
//end this section change - Hans
}

void mouse_get_cyberman_pos( int *x, int *y )
{
 // Shrug...
 event_poll();
}

// Returns how long this button has been down since last call.
fix mouse_button_down_time(int button)
{
 fix time_down, time;

 event_poll();

 if (!Mouse.buttons[button].pressed) {
   time_down = Mouse.buttons[button].time_held_down;
   Mouse.buttons[button].time_held_down = 0;
 } else {
   time = timer_get_fixed_seconds();
   time_down = time - Mouse.buttons[button].time_held_down;
   Mouse.buttons[button].time_held_down = time;
 }
 return time_down;
}

// Returns how many times this button has went down since last call
int mouse_button_down_count(int button)
{
  int count;

  event_poll();

  count = Mouse.buttons[button].num_downs;
  Mouse.buttons[button].num_downs = 0;

  return count;
}

// Returns 1 if this button is currently down
int mouse_button_state(int button)
{
  event_poll();
  return Mouse.buttons[button].pressed;
}


