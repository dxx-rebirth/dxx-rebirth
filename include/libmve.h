#ifndef _LIBMVE_H
#define _LIBMVE_H

#define MVE_ERR_EOF 1

int  MVE_rmPrepMovie(int filehandle, int x, int y, int track);
int  MVE_rmStepMovie();
void MVE_rmHoldMovie();
void MVE_rmEndMovie();

void MVE_sndInit(int x);

#endif /* _LIBMVE_H */
