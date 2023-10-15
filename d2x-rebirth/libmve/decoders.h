/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * INTERNAL header - not to be included outside of libmve
 *
 */

#pragma once
#include <cstdint>
#include <span>

namespace d2x {

extern int g_width, g_height;

void decodeFrame8(const uint8_t *vBackBuf2, std::size_t width, unsigned char *pFrame, std::span<const uint8_t> pMap, const unsigned char *pData, int dataRemain);
void decodeFrame16(const uint16_t *backBuf2, std::size_t width, unsigned char *pFrame, std::span<const uint8_t> pMap, const unsigned char *pData, int dataRemain);

}
