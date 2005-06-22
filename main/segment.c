/* $Id: segment.c,v 1.4 2005-06-22 09:08:21 chris Exp $ */

/*
 *
 * Segment Loading Stuff
 *
 */


#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "segment.h"
#include "cfile.h"

#ifdef RCS
static char rcsid[] = "$Id: segment.c,v 1.4 2005-06-22 09:08:21 chris Exp $";
#endif

#ifndef FAST_FILE_IO
/*
 * reads a segment2 structure from a CFILE
 */
void segment2_read(segment2 *s2, CFILE *fp)
{
	s2->special = cfile_read_byte(fp);
	s2->matcen_num = cfile_read_byte(fp);
	s2->value = cfile_read_byte(fp);
	s2->s2_flags = cfile_read_byte(fp);
	s2->static_light = cfile_read_fix(fp);
}

/*
 * reads a delta_light structure from a CFILE
 */
void delta_light_read(delta_light *dl, CFILE *fp)
{
	dl->segnum = cfile_read_short(fp);
	dl->sidenum = cfile_read_byte(fp);
	dl->dummy = cfile_read_byte(fp);
	dl->vert_light[0] = cfile_read_byte(fp);
	dl->vert_light[1] = cfile_read_byte(fp);
	dl->vert_light[2] = cfile_read_byte(fp);
	dl->vert_light[3] = cfile_read_byte(fp);
}


/*
 * reads a dl_index structure from a CFILE
 */
void dl_index_read(dl_index *di, CFILE *fp)
{
	di->segnum = cfile_read_short(fp);
	di->sidenum = cfile_read_byte(fp);
	di->count = cfile_read_byte(fp);
	di->index = cfile_read_short(fp);
}
#endif

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
