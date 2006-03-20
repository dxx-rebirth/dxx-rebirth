#ifndef _MLTICNTL_H
#define _MLTICNTL_H

extern int restrict_mode;

void lamer_do_netgame_menu (void);
void lamer_network_menu_update (int , newmenu_item * , int * , int);
void lamer_do_restrict_alert (sequence_packet * their);
void lamer_do_restrict_frame (void);
void lamer_accept_joining_player (void);
void lamer_dump_joining_player (void);
void lamer_network_welcome_player_restricted (sequence_packet * their);

#endif
