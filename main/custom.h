#ifndef _CUSTOM_H
#define _CUSTOM_H

/* from piggy.c */
#define DBM_FLAG_LARGE	128		// Flags added onto the flags struct in b
#define DBM_FLAG_ABM            64

extern int GameBitmapOffset[MAX_BITMAP_FILES];
extern ubyte GameBitmapFlags[MAX_BITMAP_FILES];
extern ubyte * Piggy_bitmap_cache_data;

/* from gamesave.c */
int read_int(CFILE *f);
short read_short(CFILE *f);
fix read_fix(CFILE *f);
byte read_byte(CFILE *f);

void load_custom_data(char *level_file);

void custom_close();

#endif
