/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 *
 * Routines to read/write pcx images.
 *
 */

#include <span>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gr.h"
#include "u_mem.h"
#include "pcx.h"
#include "physfsx.h"
#include "palette.h"

#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-poison.h"
#include "compiler-range_for.h"
#include "d_range.h"
#include "partial_range.h"
#include "console.h"

#if DXX_USE_SDLIMAGE
#include "physfsrwops.h"
#include <SDL_image.h>
#include "d_uspan.h"
#endif

namespace dcx {

namespace {

#if !DXX_USE_OGL && DXX_USE_SCREENSHOT_FORMAT_LEGACY
[[nodiscard]]
static int pcx_encode_byte(ubyte byt, ubyte cnt, PHYSFS_File *fid);
[[nodiscard]]
static int pcx_encode_line(std::span<const uint8_t> in, PHYSFS_File *fp);

/* PCX Header data type */
struct PCXHeader
{
	ubyte   Manufacturer;
	ubyte   Version;
	ubyte   Encoding;
	ubyte   BitsPerPixel;
	short   Xmin;
	short   Ymin;
	short   Xmax;
	short   Ymax;
	short   Hdpi;
	short   Vdpi;
	ubyte   ColorMap[16][3];
	ubyte   Reserved;
	ubyte   Nplanes;
	short   BytesPerLine;
	ubyte   filler[60];
} __pack__;
#endif

#if DXX_USE_SDLIMAGE
static pcx_result pcx_read_bitmap(const char *const filename, grs_main_bitmap &bmp, palette_array_t &palette, RWops_ptr rw)
{
	RAII_SDL_Surface surface(IMG_LoadPCX_RW(rw.get()));
	if (!surface.surface)
	{
		con_printf(CON_NORMAL, "%s:%u: failed to create surface from \"%s\"", __FILE__, __LINE__, filename);
		return pcx_result::ERROR_OPENING;
	}
	const auto &s = *surface.surface.get();
	const auto fmt{s.format};
	if (!fmt || fmt->BitsPerPixel != 8)
		return pcx_result::ERROR_WRONG_VERSION;
	const auto fpal = fmt->palette;
	if (!fpal || fpal->ncolors != palette.size())
		return pcx_result::ERROR_NO_PALETTE;
	const unsigned xsize = s.w;
	const unsigned ysize = s.h;
	if (xsize > 3840)
		return pcx_result::ERROR_MEMORY;
	if (ysize > 2400)
		return pcx_result::ERROR_MEMORY;
	DXX_CHECK_MEM_IS_DEFINED(std::span(static_cast<const std::byte *>(s.pixels), xsize * ysize));
	gr_init_bitmap_alloc(bmp, bm_mode::linear, 0, 0, xsize, ysize, xsize);
	std::copy_n(reinterpret_cast<const uint8_t *>(s.pixels), xsize * ysize, &bmp.get_bitmap_data()[0]);
	{
		const auto a = [](const SDL_Color &c) {
			return rgb_t{
				static_cast<uint8_t>(c.r >> 2),
				static_cast<uint8_t>(c.g >> 2),
				static_cast<uint8_t>(c.b >> 2)
			};
		};
		std::transform(fpal->colors, fpal->colors + palette.size(), palette.begin(), a);
	}
	return pcx_result::SUCCESS;
}
#endif

static pcx_result pcx_read_blank(grs_main_bitmap &bmp, palette_array_t &palette)
{
	constexpr unsigned xsize = 640;
	constexpr unsigned ysize = 480;
	gr_init_bitmap_alloc(bmp, bm_mode::linear, 0, 0, xsize, ysize, xsize);
	auto &bitmap_data = *reinterpret_cast<uint8_t (*)[ysize][xsize]>(bmp.get_bitmap_data());
	constexpr uint8_t border = 1;
	constexpr uint8_t body = 0;
	std::fill(&bitmap_data[0][0], &bitmap_data[2][0], border);
	std::fill(&bitmap_data[2][0], &bitmap_data[ysize - 2][0], body);
	std::fill(&bitmap_data[ysize - 2][0], &bitmap_data[ysize][0], border);
	for (auto &&y : xrange(2u, ysize - 2))
	{
		bitmap_data[y][0] = border;
		bitmap_data[y][1] = border;
		bitmap_data[y][xsize - 2] = border;
		bitmap_data[y][xsize - 1] = border;
	}
	palette = {};
	palette[border] = {63, 63, 63};
	return pcx_result::SUCCESS;
}

#if !DXX_USE_SDLIMAGE
static pcx_result pcx_support_not_compiled(const char *const filename, grs_main_bitmap &bmp, palette_array_t &palette)
{
	con_printf(CON_NORMAL, "%s:%u: PCX support disabled at compile time; cannot read file \"%s\"", __FILE__, __LINE__, filename);
	return pcx_read_blank(bmp, palette);
}
#endif

}

}

#if DXX_BUILD_DESCENT == 1
namespace dsx {

namespace {

#if DXX_USE_SDLIMAGE
static auto load_physfs_blob(const char *const filename)
{
	unique_span<uint8_t> result;
	if (auto &&[file, physfserr]{PHYSFSX_openReadBuffered(filename)}; !file)
	{
		con_printf(CON_VERBOSE, "%s:%u: failed to open \"%s\": %s", __FILE__, __LINE__, filename, PHYSFS_getErrorByCode(physfserr));
	}
	else if (const std::size_t fsize = PHYSFS_fileLength(file); fsize > 0x100000u)
	{
		con_printf(CON_VERBOSE, "%s:%u: file too large \"%s\"", __FILE__, __LINE__, filename);
	}
	else if (PHYSFSX_readBytes(file, (result = {fsize}).get(), fsize) != fsize)
	{
		result.reset();
		con_printf(CON_VERBOSE, "%s:%u: failed to read \"%s\"", __FILE__, __LINE__, filename);
	}
	return result;
}

static auto load_decoded_physfs_blob(const char *const filename)
{
	unique_span<uint8_t> decoded_buffer;
	auto encoded_blob{load_physfs_blob(filename)};
	const auto b{encoded_blob.get()};
	if (!b)
		return decoded_buffer;
	const std::size_t data_size = encoded_blob.size() - 1;
	auto &&transform_predicate{[xor_value = b[data_size]](const uint8_t c) mutable {
		return c ^ --xor_value;
	}};
	decoded_buffer = {data_size};
	std::transform(std::make_reverse_iterator(std::next(b, data_size)), std::make_reverse_iterator(b), decoded_buffer.get(), transform_predicate);
	return decoded_buffer;
}
#endif

}

pcx_result bald_guy_load(const char *const filename, grs_main_bitmap &bmp, palette_array_t &palette)
{
#if DXX_USE_SDLIMAGE
	const auto &&bguy_data{load_decoded_physfs_blob(filename)};
	const auto b{bguy_data.get()};
	if (!b)
		return pcx_result::ERROR_OPENING;

	RWops_ptr rw(SDL_RWFromConstMem(b, bguy_data.size()));
	return pcx_read_bitmap(filename, bmp, palette, std::move(rw));
#else
	return pcx_support_not_compiled(filename, bmp, palette);
#endif
}

}
#endif

namespace dcx {

pcx_result pcx_read_bitmap(const char *const filename, grs_main_bitmap &bmp, palette_array_t &palette)
{
#if DXX_USE_SDLIMAGE
	/* Try to enable buffering on the PHYSFS file.  On Windows,
	 * unbuffered access to the file causes SDL_image to be slow enough
	 * for users to detect the latency and report it as an issue.  On
	 * Linux, unbuffered access is still fast.
	 */
	auto &&[rw, physfserr] = PHYSFSRWOPS_openReadBuffered(filename, 1024 * 1024);
	if (!rw)
	{
		con_printf(CON_NORMAL, "%s:%u: failed to open \"%s\": %s", __FILE__, __LINE__, filename, PHYSFS_getErrorByCode(physfserr));
		return pcx_result::ERROR_OPENING;
	}
	return pcx_read_bitmap(filename, bmp, palette, std::move(rw));
#else
	return pcx_support_not_compiled(filename, bmp, palette);
#endif
}

pcx_result pcx_read_bitmap_or_default(const char *const filename, grs_main_bitmap &bmp, palette_array_t &palette)
{
	const auto r = pcx_read_bitmap(filename, bmp, palette);
	if (r != pcx_result::SUCCESS)
		pcx_read_blank(bmp, palette);
	return r;
}

#if !DXX_USE_OGL && DXX_USE_SCREENSHOT_FORMAT_LEGACY
unsigned pcx_write_bitmap(PHYSFS_File *const PCXfile, const grs_bitmap *const bmp, palette_array_t &palette)
{
	static constexpr std::size_t PCXHEADER_SIZE{128};
	ubyte data;
	PCXHeader header{};

	header.Manufacturer = 10;
	header.Encoding = 1;
	header.Nplanes = 1;
	header.BitsPerPixel = 8;
	header.Version = 5;
	header.Xmax = bmp->bm_w-1;
	header.Ymax = bmp->bm_h-1;
	header.BytesPerLine = bmp->bm_w;

	if (PHYSFSX_writeBytes(PCXfile, &header, PCXHEADER_SIZE) != PCXHEADER_SIZE)
	{
		return 1;
	}

	{
		const uint_fast32_t bm_w = bmp->bm_w;
		const uint_fast32_t bm_rowsize = bmp->bm_rowsize;
		const auto bm_data = bmp->get_bitmap_data();
		const auto e = &bm_data[bm_rowsize * bmp->bm_h];
		for (auto i = &bm_data[0]; i != e; i += bm_rowsize)
		{
			if (!pcx_encode_line(std::span{i, bm_w}, PCXfile))
			{
				return 1;
			}
		}
	}

	// Mark an extended palette
	data = 12;
	if (PHYSFSX_writeBytes(PCXfile, &data, 1) != 1)
	{
		return 1;
	}

	const auto retval{PHYSFSX_writeBytes(PCXfile, palette.data(), palette.size())};
	if (retval != palette.size())
		return 1;
	return 0;
}

namespace {

// returns number of bytes written into outBuff, 0 if failed
int pcx_encode_line(const std::span<const uint8_t> in, PHYSFS_File *fp)
{
	int i;
	int total;
	ubyte runCount; 	// max single runlength is 63
	total = 0;
	auto last = in.front();
	runCount = 1;

	range_for (const auto ub, unchecked_partial_range(in.data(), 1u, in.size()))
	{
		if (ub == last)	{
			runCount++;			// it encodes
			if (runCount == 63)	{
				if (!(i=pcx_encode_byte(last, runCount, fp)))
					return(0);
				total += i;
				runCount = 0;
			}
		} else {   	// this != last
			if (runCount)	{
				if (!(i=pcx_encode_byte(last, runCount, fp)))
					return(0);
				total += i;
			}
			last = ub;
			runCount = 1;
		}
	}

	if (runCount)	{		// finish up
		if (!(i=pcx_encode_byte(last, runCount, fp)))
			return 0;
		return total + i;
	}
	return total;
}

static inline int PHYSFSX_putc(PHYSFS_File *file, uint8_t ch)
{
	if (PHYSFSX_writeBytes(file, &ch, 1) < 1)
		return -1;
	else
		return ch;
}

// subroutine for writing an encoded byte pair
// returns count of bytes written, 0 if error
int pcx_encode_byte(ubyte byt, ubyte cnt, PHYSFS_File *fid)
{
	if (cnt) {
		if ( (cnt==1) && (0xc0 != (0xc0 & byt)) )	{
			if(EOF == PHYSFSX_putc(fid, static_cast<int>(byt)))
				return 0; 	// disk write error (probably full)
			return 1;
		} else {
			if(EOF == PHYSFSX_putc(fid, 0xC0 | cnt))
				return 0; 	// disk write error
			if(EOF == PHYSFSX_putc(fid, static_cast<int>(byt)))
				return 0; 	// disk write error
			return 2;
		}
	}
	return 0;
}

}
#endif

//text for error messges
constexpr char pcx_error_messages[] = {
	"No error.\0"
	"Error opening file.\0"
	"Could not read PCX header.\0"
	"Unsupported PCX version.\0"
	"Error reading data.\0"
	"Could not find palette information.\0"
	"Error writing data.\0"
};


//function to return pointer to error message
const char *pcx_errormsg(const pcx_result r)
{
	const char *p = pcx_error_messages;

	unsigned error_number = static_cast<unsigned>(r);
	while (error_number--) {
		if (p == std::end(pcx_error_messages))
			return nullptr;
		p += strlen(p)+1;
	}

	return p;
}

}
