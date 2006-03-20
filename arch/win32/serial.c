void com_done(void)
{
}

int com_init(void)
{
  return 0;
}

int com_read(char *buffer, int len, int timeout_value)
{
  return -1;
}

int com_write(char *buffer, int len)
{
  return -1;
}

void com_port_hangup()
{
}

int com_getdcd()
{
  return -1;
}

void com_flushbuffers()
{
}

void com_setbaudrate(int rate)
{
}

int com_readline(int timeout_value, char *input_buffer,int len)
{
 return 0;
}

