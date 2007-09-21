//Define the max number of players that can be ignored, and max hud message length.
#ifndef _IGNORE_H
#define _IGNORE_H

#define MAX_IGNORE 8
#define HUD_MAX_LEN 150

//Prototypes...
void addignore (char * ignorename);
void addignore_by_number (int ignorenum);
void eraseignore (char * ignorename);
void eraseignore_by_number (int ignorenum);
int checkignore (char * ignorename);
void clearignore (void);
void listignore (void);

#endif
