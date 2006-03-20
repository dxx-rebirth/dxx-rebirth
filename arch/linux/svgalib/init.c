#include "args.h"

extern void d_mouse_init();

void arch_svgalib_init()
{
 if (!FindArg("-nomouse"))
	d_mouse_init();
}
