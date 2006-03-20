#include "key.h"
#include "event.h"

void keyboard_handler(int button, ubyte state)
{
	generic_key_handler(button,state);
}

void arch_key_poll()
{
	event_poll();
}

void arch_key_close()
{
}

void arch_key_init()
{
	keyd_fake_repeat=1;
}

void arch_key_flush()
{
}

