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

typedef unsigned int (*mve_cb_Read)(void *stream,
                                    void *buffer,
                                    unsigned int count);

typedef void *(*mve_cb_Alloc)(size_t size);
typedef void (*mve_cb_Free)(void *ptr);

typedef void (*mve_cb_ShowFrame)(unsigned char *buffer, int dstx, int dsty, int bufw, int bufh, int sw, int sh);

typedef void (*mve_cb_SetPalette)(const unsigned char *p,
                                  unsigned int start, unsigned int count);

void MVE_ioCallbacks(mve_cb_Read io_read);
void MVE_memCallbacks(mve_cb_Alloc mem_alloc, mve_cb_Free mem_free);
void MVE_sfCallbacks(mve_cb_ShowFrame showframe);
void MVE_palCallbacks(mve_cb_SetPalette setpalette);

#endif

#endif /* _LIBMVE_H */
