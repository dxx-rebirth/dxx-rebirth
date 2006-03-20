/// SDL keyboard input support

#include <stdio.h>
#include <stdlib.h>

#include <vgakeyboard.h>

#include "error.h"
#include "event.h"
#include "key.h"

void key_handler(int scancode, int press)
{
	generic_key_handler(scancode,press);
}

void arch_key_close(void)
{
	keyboard_close();
}

void arch_key_init(void)
{
	if (keyboard_init())
		Error ("SVGAlib Keyboard Init Failed");

	keyboard_seteventhandler (key_handler);
	//keyd_fake_repeat = 1;
}

void arch_key_flush(void)
{
}

void arch_key_poll(void)
{
	event_poll();
}
