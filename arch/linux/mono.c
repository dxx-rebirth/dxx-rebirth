#include <stdio.h>
#include <stdarg.h>

#define MONO_IS_STDERR
//added 05/17/99 Matt Mueller - needed for editor build
int minit(void){return -1;}
//end addition -MM
void mopen( short n, short row, short col, short width, short height, char * title ) {}
void mclose(short n) {}

void _mprintf( short n, char * format, ... )
#ifdef MONO_IS_STDERR
{
	va_list args;
	va_start(args, format );
	vfprintf(stderr, format, args);
}
#else
{}
#endif
