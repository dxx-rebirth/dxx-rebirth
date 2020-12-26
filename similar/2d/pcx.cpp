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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gr.h"
#include "grdef.h"
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
#endif

namespace dcx {

namespace {

#if DXX_USE_SDLIMAGE
struct RAII_SDL_Surface
{
	struct deleter
	{
		void operator()(SDL_Surface *s) const
		{
			SDL_FreeSurface(s);
		}
	};
	std::unique_ptr<SDL_Surface, deleter> surface;
	explicit RAII_SDL_Surface(SDL_Surface *const s) :
		surface(s)
	{
	}
};
#endif

#if !DXX_USE_OGL && DXX_USE_SCREENSHOT_FORMAT_LEGACY
static int pcx_encode_byte(ubyte byt, ubyte cnt, PHYSFS_File *fid);
static int pcx_encode_line(const uint8_t *inBuff, uint_fast32_t inLen, PHYSFS_File *fp);

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

#define PCXHEADER_SIZE 128

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
	const auto fmt = s.format;
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
	DXX_CHECK_MEM_IS_DEFINED(s.pixels, xsize * ysize);
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
	pcx_read_blank(bmp, palette);
}
#endif

}

}

#if defined(DXX_BUILD_DESCENT_I)
namespace dsx {

namespace {

#if DXX_USE_SDLIMAGE
static std::pair<std::unique_ptr<uint8_t[]>, std::size_t> load_physfs_blob(const char *const filename)
{
	RAIIPHYSFS_File file(PHYSFSX_openReadBuffered(filename));
	if (!file)
	{
		con_printf(CON_VERBOSE, "%s:%u: failed to open \"%s\"", __FILE__, __LINE__, filename);
		return {};
	}
	const std::size_t fsize = PHYSFS_fileLength(file);
	if (fsize > 0x100000)
	{
		con_printf(CON_VERBOSE, "%s:%u: file too large \"%s\"", __FILE__, __LINE__, filename);
		return {};
	}
	auto encoded_buffer = std::make_unique<uint8_t[]>(fsize);
	if (PHYSFS_read(file, encoded_buffer.get(), 1, fsize) < fsize)
	{
		con_printf(CON_VERBOSE, "%s:%u: failed to read \"%s\"", __FILE__, __LINE__, filename);
		return {};
	}
	return {std::move(encoded_buffer), fsize};
}

static std::pair<std::unique_ptr<uint8_t[]>, std::size_t> load_decoded_physfs_blob(const char *const filename)
{
	auto encoded_blob = load_physfs_blob(filename);
	auto &&[encoded_buffer, fsize] = encoded_blob;
	if (!encoded_buffer)
		return encoded_blob;
	const std::size_t data_size = fsize - 1;
	auto &&transform_predicate = [xor_value = encoded_buffer[data_size]](uint8_t c) mutable {
		return c ^ --xor_value;
	};
	auto &&decoded_buffer = std::make_unique<uint8_t[]>(data_size);
	const auto b = encoded_buffer.get();
	std::transform(std::make_reverse_iterator(std::next(b, data_size)), std::make_reverse_iterator(b), decoded_buffer.get(), transform_predicate);
	return {std::move(decoded_buffer), data_size};
}
#endif

}

pcx_result bald_guy_load(const char *const filename, grs_main_bitmap &bmp, palette_array_t &palette)
{
#if DXX_USE_SDLIMAGE
	const auto &&[bguy_data, data_size] = load_decoded_physfs_blob(filename);
	if (!bguy_data)
		return pcx_result::ERROR_OPENING;

	RWops_ptr rw(SDL_RWFromConstMem(bguy_data.get(), data_size));
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
	auto rw = PHYSFSRWOPS_openReadBuffered(filename, 1024 * 1024);
	if (!rw)
	{
		con_printf(CON_NORMAL, "%s:%u: failed to open \"%s\"", __FILE__, __LINE__, filename);
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
pcx_result pcx_write_bitmap(PHYSFS_File *const PCXfile, const grs_bitmap *const bmp, palette_array_t &palette)
{
	int retval;
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

	if (PHYSFS_write(PCXfile, &header, PCXHEADER_SIZE, 1) != 1)
	{
		return pcx_result::ERROR_WRITING;
	}

	{
		const uint_fast32_t bm_w = bmp->bm_w;
		const uint_fast32_t bm_rowsize = bmp->bm_rowsize;
		const auto bm_data = bmp->get_bitmap_data();
		const auto e = &bm_data[bm_rowsize * bmp->bm_h];
		for (auto i = &bm_data[0]; i != e; i += bm_rowsize)
		{
			if (!pcx_encode_line(i, bm_w, PCXfile))
			{
			return pcx_result::ERROR_WRITING;
			}
		}
	}

	// Mark an extended palette
	data = 12;
	if (PHYSFS_write(PCXfile, &data, 1, 1) != 1)
	{
		return pcx_result::ERROR_WRITING;
	}

	retval = PHYSFS_write(PCXfile, &palette[0], sizeof(palette), 1);
	if (retval !=1)	{
		return pcx_result::ERROR_WRITING;
	}
	return pcx_result::SUCCESS;
}

// returns number of bytes written into outBuff, 0 if failed
int pcx_encode_line(const uint8_t *inBuff, uint_fast32_t inLen, PHYSFS_File *fp)
{
	ubyte last;
	int i;
	int total;
	ubyte runCount; 	// max single runlength is 63
	total = 0;
	last = *(inBuff);
	runCount = 1;

	range_for (const auto ub, unchecked_partial_range(inBuff, 1u, inLen))
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
	if (PHYSFS_write(file, &ch, 1, 1) < 1)
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
