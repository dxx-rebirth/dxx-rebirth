/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

/*
 *
 * Segment Loading Stuff
 *
 */


#include "segment.h"
#include "physfsx.h"
#include "physfs-serial.h"

namespace dcx {

#if 0
DEFINE_SERIAL_UDT_TO_MESSAGE(wallnum_t, w, (w.value));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(wallnum_t, 2);
#endif
DEFINE_SERIAL_UDT_TO_MESSAGE(side, s, (s.wall_num, s.tmap_num, s.tmap_num2));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(side, 6);

void segment_side_wall_tmap_write(PHYSFS_File *fp, const side &side)
{
	PHYSFSX_serialize_write(fp, side);
}

}

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
/*
 * reads a segment2 structure from a PHYSFS_File
 */
void segment2_read(const vmsegptr_t s2, PHYSFS_File *fp)
{
	s2->special = PHYSFSX_readByte(fp);
	s2->matcen_num = PHYSFSX_readByte(fp);
	s2->value = PHYSFSX_readByte(fp);
	s2->s2_flags = PHYSFSX_readByte(fp);
	s2->static_light = PHYSFSX_readFix(fp);
}

/*
 * reads a delta_light structure from a PHYSFS_File
 */
void delta_light_read(delta_light *dl, PHYSFS_File *fp)
{
	dl->segnum = PHYSFSX_readShort(fp);
	dl->sidenum = PHYSFSX_readByte(fp);
	PHYSFSX_readByte(fp);
	dl->vert_light[0] = PHYSFSX_readByte(fp);
	dl->vert_light[1] = PHYSFSX_readByte(fp);
	dl->vert_light[2] = PHYSFSX_readByte(fp);
	dl->vert_light[3] = PHYSFSX_readByte(fp);
}


/*
 * reads a dl_index structure from a PHYSFS_File
 */
void dl_index_read(dl_index *di, PHYSFS_File *fp)
{
	di->segnum = PHYSFSX_readShort(fp);
	di->sidenum = PHYSFSX_readByte(fp);
	di->count = PHYSFSX_readByte(fp);
	di->index = PHYSFSX_readShort(fp);
}

void segment2_write(const vcsegptr_t s2, PHYSFS_File *fp)
{
	PHYSFSX_writeU8(fp, s2->special);
	PHYSFSX_writeU8(fp, s2->matcen_num);
	PHYSFSX_writeU8(fp, s2->value);
	PHYSFSX_writeU8(fp, s2->s2_flags);
	PHYSFSX_writeFix(fp, s2->static_light);
}

void delta_light_write(const delta_light *dl, PHYSFS_File *fp)
{
	PHYSFS_writeSLE16(fp, dl->segnum);
	PHYSFSX_writeU8(fp, dl->sidenum);
	PHYSFSX_writeU8(fp, 0);
	PHYSFSX_writeU8(fp, dl->vert_light[0]);
	PHYSFSX_writeU8(fp, dl->vert_light[1]);
	PHYSFSX_writeU8(fp, dl->vert_light[2]);
	PHYSFSX_writeU8(fp, dl->vert_light[3]);
}

void dl_index_write(const dl_index *di, PHYSFS_File *fp)
{
	PHYSFS_writeSLE16(fp, di->segnum);
	PHYSFSX_writeU8(fp, di->sidenum);
	PHYSFSX_writeU8(fp, di->count);
	PHYSFS_writeSLE16(fp, di->index);
}

}
#endif
