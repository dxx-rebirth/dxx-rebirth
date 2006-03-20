//hudlog.h added 11/01/98 Matthew Mueller - log messages and score grid to files.
void hud_log_check_multi_start(void);
void open_hud_log(void);
void close_hud_log(void);
void toggle_hud_log(void);
void kmatrix_log(int fhudonly);
void hud_log_setdir(char *dir);
void hud_log_message(char * message);

extern int HUD_log_messages;
extern int HUD_log_multi_autostart;
extern int HUD_log_autostart;


