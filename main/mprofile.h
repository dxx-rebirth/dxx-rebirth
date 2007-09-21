//multiplayer profiles - Victor Rachels
void save_multi_profile(int multivalues[40], char *filename);
void get_multi_profile(int multivalues[40], char *filename);
int do_multi_profile(int multivalues[40]);

void putto_multivalues(int multivalues[40],netgame_info *temp_game, int *socket);
void putfrom_multivalues(int multivalues[40],netgame_info *temp_game, int *socket);
