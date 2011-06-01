
/*
 *
 * Segment Loading Stuff
 *
 */


#include "segment.h"

/*
 * reads a segment2 structure from a PHYSFS_file
 */
void segment2_read(segment2 *s2, PHYSFS_file *fp)
{
	s2->special = PHYSFSX_readByte(fp);
	s2->matcen_num = PHYSFSX_readByte(fp);
	s2->value = PHYSFSX_readByte(fp);
	s2->s2_flags = PHYSFSX_readByte(fp);
	s2->static_light = PHYSFSX_readFix(fp);
}

/*
 * reads a delta_light structure from a PHYSFS_file
 */
void delta_light_read(delta_light *dl, PHYSFS_file *fp)
{
	dl->segnum = PHYSFSX_readShort(fp);
	dl->sidenum = PHYSFSX_readByte(fp);
	dl->dummy = PHYSFSX_readByte(fp);
	dl->vert_light[0] = PHYSFSX_readByte(fp);
	dl->vert_light[1] = PHYSFSX_readByte(fp);
	dl->vert_light[2] = PHYSFSX_readByte(fp);
	dl->vert_light[3] = PHYSFSX_readByte(fp);
}


/*
 * reads a dl_index structure from a PHYSFS_file
 */
void dl_index_read(dl_index *di, PHYSFS_file *fp)
{
	di->segnum = PHYSFSX_readShort(fp);
	di->sidenum = PHYSFSX_readByte(fp);
	di->count = PHYSFSX_readByte(fp);
	di->index = PHYSFSX_readShort(fp);
}

void segment2_write(segment2 *s2, PHYSFS_file *fp)
{
	PHYSFSX_writeU8(fp, s2->special);
	PHYSFSX_writeU8(fp, s2->matcen_num);
	PHYSFSX_writeU8(fp, s2->value);
	PHYSFSX_writeU8(fp, s2->s2_flags);
	PHYSFSX_writeFix(fp, s2->static_light);
}

void delta_light_write(delta_light *dl, PHYSFS_file *fp)
{
	PHYSFS_writeSLE16(fp, dl->segnum);
	PHYSFSX_writeU8(fp, dl->sidenum);
	PHYSFSX_writeU8(fp, dl->dummy);
	PHYSFSX_writeU8(fp, dl->vert_light[0]);
	PHYSFSX_writeU8(fp, dl->vert_light[1]);
	PHYSFSX_writeU8(fp, dl->vert_light[2]);
	PHYSFSX_writeU8(fp, dl->vert_light[3]);
}

void dl_index_write(dl_index *di, PHYSFS_file *fp)
{
	PHYSFS_writeSLE16(fp, di->segnum);
	PHYSFSX_writeU8(fp, di->sidenum);
	PHYSFSX_writeU8(fp, di->count);
	PHYSFS_writeSLE16(fp, di->index);
}
