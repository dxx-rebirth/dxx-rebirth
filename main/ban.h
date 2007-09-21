#ifndef _BAN_H
#define _BAN_H

#define MAX_BANS 20
#define HUD_MAX_LEN 150

extern int bansused;

void addnickban(char * banname);
void addipban(char * banname);
void addban_by_number(int bannum);
int checkban(ubyte banip[6]);
void unban(char * banname);
void unban_by_number(int bannum);
void clearbans();
void listbans();

int readbans();
int writebans();

#endif
