/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
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
#include "d_underlying_value.h"

namespace dcx {

namespace {

struct composite_side
{
	const shared_side &sside;
	const unique_side &uside;
};

DEFINE_SERIAL_UDT_TO_MESSAGE(composite_side, s, (s.sside.wall_num, s.uside.tmap_num, s.uside.tmap_num2));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(composite_side, 6);

static imsegidx_t build_segnum_from_optional_segnum(std::optional<segnum_t> value)
{
	return value.value_or(segment_none);
}

static imsegidx_t build_segnum_from_untrusted_value(auto value)
{
	return build_segnum_from_optional_segnum(vmsegidx_t::check_nothrow_index(value));
}

}

std::optional<sidenum_t> build_sidenum_from_untrusted(const uint8_t untrusted)
{
	switch (untrusted)
	{
		case static_cast<uint8_t>(sidenum_t::WLEFT):
		case static_cast<uint8_t>(sidenum_t::WTOP):
		case static_cast<uint8_t>(sidenum_t::WRIGHT):
		case static_cast<uint8_t>(sidenum_t::WBOTTOM):
		case static_cast<uint8_t>(sidenum_t::WBACK):
		case static_cast<uint8_t>(sidenum_t::WFRONT):
			return sidenum_t{untrusted};
		default:
			return std::nullopt;
	}
}

void segment_side_wall_tmap_write(PHYSFS_File *fp, const shared_side &sside, const unique_side &uside)
{
	PHYSFSX_serialize_write(fp, composite_side{sside, uside});
}

imsegidx_t read_untrusted_segnum_le16(NamedPHYSFS_File fp)
{
	return build_segnum_from_untrusted_value(PHYSFSX_readULE16(fp));
}

imsegidx_t read_untrusted_segnum_le32(NamedPHYSFS_File fp)
{
	return build_segnum_from_untrusted_value(PHYSFSX_readULE32(fp));
}

imsegidx_t read_untrusted_segnum_xe16(NamedPHYSFS_File fp, const physfsx_endian swap)
{
	return build_segnum_from_untrusted_value(PHYSFSX_readUXE16(fp, swap));
}

imsegidx_t read_untrusted_segnum_xe32(NamedPHYSFS_File fp, const physfsx_endian swap)
{
	return build_segnum_from_untrusted_value(PHYSFSX_readUXE32(fp, swap));
}

}

#if DXX_BUILD_DESCENT == 2
namespace dsx {

static std::optional<delta_light_index> build_delta_light_index_from_untrusted(const uint16_t i)
{
	if (i < MAX_DELTA_LIGHTS)
		return delta_light_index{i};
	else
		return std::nullopt;
}

/*
 * reads a delta_light structure from a PHYSFS_File
 */
void delta_light_read(delta_light *dl, const NamedPHYSFS_File fp)
{
	dl->segnum = read_untrusted_segnum_le16(fp);
	dl->sidenum = build_sidenum_from_untrusted(PHYSFSX_readByte(fp)).value_or(sidenum_t::WLEFT);
	PHYSFSX_skipBytes<1>(fp);
	dl->vert_light[side_relative_vertnum::_0] = PHYSFSX_readByte(fp);
	dl->vert_light[side_relative_vertnum::_1] = PHYSFSX_readByte(fp);
	dl->vert_light[side_relative_vertnum::_2] = PHYSFSX_readByte(fp);
	dl->vert_light[side_relative_vertnum::_3] = PHYSFSX_readByte(fp);
}


/*
 * reads a dl_index structure from a PHYSFS_File
 */
void dl_index_read(dl_index *di, const NamedPHYSFS_File fp)
{
	di->segnum = read_untrusted_segnum_le16(fp);
	di->sidenum = build_sidenum_from_untrusted(PHYSFSX_readByte(fp)).value_or(sidenum_t::WLEFT);
	const auto count = PHYSFSX_readByte(fp);
	if (const auto i{build_delta_light_index_from_untrusted(PHYSFSX_readShort(fp))}; i)
	{
		di->count = count;
		di->index = *i;
	}
	else
	{
		di->count = 0;
		di->index = {};
	}
}

void segment2_write(const cscusegment s2, PHYSFS_File *fp)
{
	PHYSFSX_writeU8(fp, underlying_value(s2.s.special));
	PHYSFSX_writeU8(fp, underlying_value(s2.s.matcen_num));
	PHYSFSX_writeU8(fp, underlying_value(s2.s.station_idx));
	PHYSFSX_writeU8(fp, underlying_value(s2.s.s2_flags));
	PHYSFSX_writeFix(fp, s2.u.static_light);
}

void delta_light_write(const delta_light *dl, PHYSFS_File *fp)
{
	PHYSFS_writeSLE16(fp, dl->segnum);
	PHYSFSX_writeU8(fp, underlying_value(dl->sidenum));
	PHYSFSX_writeU8(fp, 0);
	PHYSFSX_writeU8(fp, dl->vert_light[side_relative_vertnum::_0]);
	PHYSFSX_writeU8(fp, dl->vert_light[side_relative_vertnum::_1]);
	PHYSFSX_writeU8(fp, dl->vert_light[side_relative_vertnum::_2]);
	PHYSFSX_writeU8(fp, dl->vert_light[side_relative_vertnum::_3]);
}

void dl_index_write(const dl_index *di, PHYSFS_File *fp)
{
	PHYSFS_writeSLE16(fp, di->segnum);
	PHYSFSX_writeU8(fp, underlying_value(di->sidenum));
	PHYSFSX_writeU8(fp, di->count);
	PHYSFS_writeSLE16(fp, underlying_value(di->index));
}

}
#endif
