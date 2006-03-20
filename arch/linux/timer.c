#include <sys/time.h>
#include <stdio.h>
#include "maths.h"

static struct timeval tv_old;

fix timer_get_fixed_seconds(void)
{
	fix x;
	struct timeval tv_now;
	gettimeofday(&tv_now, NULL);
	x=i2f(tv_now.tv_sec - tv_old.tv_sec) + fixdiv(i2f((tv_now.tv_usec - tv_old.tv_usec)/1000), i2f(1000));
	return x;
}

void timer_init(void)
{
	gettimeofday(&tv_old, NULL);
}
