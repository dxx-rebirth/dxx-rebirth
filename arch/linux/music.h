#include <unistd.h>

#define SEQ_DEV "/dev/sequencer"

#define PLAYING 0
#define STOPPED 1

typedef struct
{
    int position;
    int status;
    int time;
} Track_info;

typedef struct
{
    signed short note;
    signed short channel;
} Voice_info;

void seqbuf_dump();
int seq_init();
void seq_close();
void set_program(int, int);
void start_note(int, int, int);
void stop_note(int, int, int);
void set_control(int, int, int);
void set_pitchbend(int, int);
void set_chn_pressure(int,int);
void set_key_pressure(int,int,int);
void play_hmi (void * arg);
void send_ipc(char *);
void kill_ipc();
