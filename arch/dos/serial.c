// dummy serial code because of linux serial stuff
#include "serial.h"

void com_write(char *buffer, int len)
{
}

int com_read(char *buffer, int len, int timeout_value)
{
 return 0;
}

int com_getdcd()
{
 return 0;
}

int com_readline(int timeout, char *input_buffer, int len)
{
 return 0;
}
