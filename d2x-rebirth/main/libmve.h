/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#ifndef _LIBMVE_H
#define _LIBMVE_H

#define MVE_ERR_EOF 1

#ifdef __cplusplus
#include <memory>

enum class MVE_StepStatus
{
	Continue = 0,
	EndOfFile = 1,
};

struct MVESTREAM;

struct MVE_videoSpec {
	int screenWidth;
	int screenHeight;
	int width;
	int height;
	int truecolor;
};

MVE_StepStatus MVE_rmStepMovie(MVESTREAM &mve);
void MVE_rmHoldMovie();
void MVE_rmEndMovie(std::unique_ptr<MVESTREAM> mve);

void MVE_getVideoSpec(MVE_videoSpec *vSpec);

void MVE_sndInit(int x);

typedef void *(*mve_cb_Alloc)(size_t size);
typedef void (*mve_cb_Free)(void *ptr);

unsigned int MovieFileRead(void *handle, void *buf, unsigned int count);
void MVE_memCallbacks(mve_cb_Alloc mem_alloc, mve_cb_Free mem_free);
void MovieShowFrame(const uint8_t *buf, int dstx, int dsty, int bufw, int bufh, int sw, int sh);
void MovieSetPalette(const unsigned char *p, unsigned start, unsigned count);

#endif

#endif /* _LIBMVE_H */
