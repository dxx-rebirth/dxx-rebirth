#ifndef MUTE_H
#define MUTE_H

#define MAX_MUTE 8

void addmute(char * mutename);
void addmute_by_number(int mutenum);
void erasemute(char * mutename);
void erasemute_by_number(int mutenum);
int checkmute(char * mutename);
void clearmute(void);
void listmute(void);

#endif //mute_h
