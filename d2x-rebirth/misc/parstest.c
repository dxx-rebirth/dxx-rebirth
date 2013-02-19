
#include <stdio.h>

#include "parsarg.h"


handle_arg(char *a)
{
	printf("%s\n",a);
}


main(int argc,char **argv)
{

	parse_args(argc-1,argv+1,handle_arg,PA_EXPAND);
}

ÿ