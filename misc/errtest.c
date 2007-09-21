
#define DEBUG_ON 1

#include <stdio.h>

#include "error.h"

main(int argc,char **argv)
{
	error_init("Thank-you for running ERRTEST!");

	Assert(argc!=0);
//	Assert(argc==5);

	Error("test of error, argc=%d, argv=%x",argc,argv);
}

ÿ