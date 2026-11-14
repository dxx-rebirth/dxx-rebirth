/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <stdexcept>
#include <utility>
#include "physfsx.h"
#include "serial.h"

namespace dcx {

class PHYSFSX_short_read : public std::runtime_error
{
public:
	PHYSFSX_short_read(PHYSFS_File *) :
		runtime_error("short read in PHYSFS file")
	{
	}
};

class PHYSFSX_short_write : public std::runtime_error
{
public:
	PHYSFSX_short_write(PHYSFS_File *) :
		runtime_error("short write in PHYSFS file")
	{
	}
};

template <typename exception_type>
void PHYSFSX_serialize_read_bytes(const NamedPHYSFS_File fp, uint8_t *const buf, const std::size_t size)
{
	if (!std::cmp_equal(PHYSFSX_readBytes(fp, buf, size), size)) [[unlikely]]
		throw exception_type{fp};
}

template <typename exception_type>
void PHYSFSX_serialize_write_bytes(PHYSFS_File *fp, const uint8_t *const buf, const std::size_t size)
{
	if (!std::cmp_equal(PHYSFS_writeBytes(fp, buf, size), size)) [[unlikely]]
		throw exception_type{fp};
}

template <typename T, std::endian endian_value = std::endian::little, typename exception_type = PHYSFSX_short_read>
void PHYSFSX_serialize_read(const NamedPHYSFS_File fp, T &t)
{
	constexpr std::size_t maximum_size{serial::message_type<T>::maximum_size};
	uint8_t buf[maximum_size];
	PHYSFSX_serialize_read_bytes<exception_type>(fp, buf, maximum_size);
	serial::reader::bytebuffer<endian_value> b{buf};
	serial::process_buffer(b, t);
}

template <typename T, std::endian endian_value = std::endian::little, typename exception_type = PHYSFSX_short_write>
void PHYSFSX_serialize_write(PHYSFS_File *fp, const T &t)
{
	constexpr std::size_t maximum_size{serial::message_type<T>::maximum_size};
	uint8_t buf[maximum_size];
	serial::writer::bytebuffer<endian_value> b{buf};
	serial::process_buffer(b, t);
	PHYSFSX_serialize_write_bytes<exception_type>(fp, buf, maximum_size);
}

}
