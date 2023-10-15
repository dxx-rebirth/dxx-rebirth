/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#ifndef _LIBMVE_H
#define _LIBMVE_H

#include <memory>
#include <SDL.h>

namespace d2x {

enum class MVE_StepStatus
{
	Continue = 0,
	EndOfFile = 1,
};

enum class MVE_play_sounds : bool
{
	silent,
	enabled,
};

struct MVESTREAM;

MVE_StepStatus MVE_rmStepMovie(MVESTREAM &mve);
void MVE_rmHoldMovie();
void MVE_rmEndMovie(std::unique_ptr<MVESTREAM> mve);

void MovieShowFrame(const uint8_t *buf, int dstx, int dsty, int bufw, int bufh, int sw, int sh);
void MovieSetPalette(const unsigned char *p, unsigned start, unsigned count);

}

#endif /* _LIBMVE_H */
