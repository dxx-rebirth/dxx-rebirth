#ifndef _MVEPLAY_H
#define _MVEPLAY_H

#include "gr.h"
#include "mvelib.h"

void mveplay_initializeMovie(MVESTREAM *mve, grs_bitmap *mve_bitmap);
int  mveplay_stepMovie(MVESTREAM *mve);
void mveplay_restartTimer(MVESTREAM *mve);
void mveplay_shutdownMovie(MVESTREAM *mve);

#endif /* _MVEPLAY_H */
