//Begin updated nncoms.h

#ifndef _NNCOMS_H
#define _NNCOMS_H

void boot (char * bootname);
void boot_by_number (int bootnum);
void discon (char * disconname);
void discon_by_number (int disconnum);
void ghost (char * ghostname);
void ghost_by_number (int ghostnum);
void unghost (char * unghostname);
void unghost_by_number (int unghostnum);
void recon (char * reconname);
void recon_by_number (int reconnum);
void ping_by_name (char * pingname);
void ping_by_number (int pingnum);
//added 03/04/99 Matt Mueller - move it all into one func.  duplication==bad.
void ping_by_player_num (int pl,int noisy);
void ping_all (int noisy);
//end addition -MM
#endif
