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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Code for localizable text
 *
 */


#include <stdlib.h>
#include <string.h>

#include "physfsx.h"
#include "pstypes.h"
#include "dxxerror.h"
#include "text.h"
#include "strutil.h"
#include "args.h"
#include <memory>
#include "d_range.h"

#ifdef GENERATE_BUILTIN_TEXT_TABLE
#include <ctype.h>
#endif

#if DXX_BUILD_DESCENT == 1
#define IDX_TEXT_OVERWRITTEN	330
#elif DXX_BUILD_DESCENT == 2
#define IDX_TEXT_OVERWRITTEN	350
#define SHAREWARE_TEXTSIZE  14677
#endif

namespace dcx {

namespace {

static std::unique_ptr<char[]> text;
static std::unique_ptr<char[]> overwritten_text;

static constexpr uint8_t decode_char(const uint8_t c)
{
	constexpr uint8_t BITMAP_TBL_XOR{0xd3};
	const uint8_t c2 = std::rotl(c, 1) ^ BITMAP_TBL_XOR;
	return {std::rotl(c2, 1)};
}

static std::pair<PHYSFS_sint64, std::unique_ptr<char[]>> load_blob_from_file(const char *const filename, const RAIIPHYSFS_File file)
{
	const auto len{PHYSFS_fileLength(file)};
	auto p{std::make_unique_for_overwrite<char[]>(len + 1)};
	const auto bytes_read{PHYSFSX_readBytes(file, p.get(), len)};
	if (bytes_read < 0)
		Error("Failed to read file %s: \"%s\"", filename, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	else if (bytes_read > len)
		/* This should never happen. */
		Error("Failed to read file %s: read more bytes than requested", filename);
	else
		return {bytes_read, std::move(p)};
}

static std::unique_ptr<char[]> parse_blob_as_text_strings(const char *const filename, std::unique_ptr<char[]> loaded_blob, const bool have_binary, const PHYSFS_sint64 bytes_read)
{
	char *tptr = loaded_blob.get();
#if DXX_BUILD_DESCENT == 1
	static const char *const extra_strings[] = {
		"done",
		"I am a",
		"CHEATER!",
		"Loading Data",
		"ALT-F2\t  Save Game",
		"ALT-F3\t  Load Game",
		"Only in Registered version!",
		"Concussion",
		"Homing",
		"ProxBomb",
		"Smart",
		"Mega",
		"Mission '%s' not found.\nYou must have this mission\nfile in order to playback\nthis demo.",
		"All player callsigns on screen",
		"There is already a game\nin progress with that name",
		"This mission is designated\nAnarchy-only",
		"Force level start",
		"Quitting now means ending the\nentire netgame\n\nAre you sure?",
		"The mission for that netgame\nis not installed on your\nsystem.  Cannot join.",
		"Start Multiplayer Game\n\nSelect mission",
		"Error loading mission file",
		"Custom (return to set)",
		"Base address (in Hex)",
		"IRQ Number",
		"Reset to Default",
		"Valid IRQ values are 2-7",
		"No UART was detected\nat those settings",
		"You will pay dearly for that!",
		"Revenge is mine!!",
		"Man I'm good!",
		"Its almost too easy!",
		"   Mission:",
		"+/- Changes viewing distance",
		"Alternate exit found!\n\nProceeding to Secret Level!",
		"Show all players on automap",
		"Killed by a robot",
		"Baud",
		"A consistency error has been\ndetected in your network connection.\nCheck you hardware and re-join",
		"Press any key to continue (Print Screen to save screenshot)",
		"An error occured while writing\ndemo.  Demo is likely corrupted.\nEnter demo name now or\npress ESC to delete demo.",
		"The main reactor is invulnerable for",
		"The level being loaded is not\navailable in Destination Saturn.\nUnable to continue demo playback.\n\nPress any key to continue.",
		"Reactor life",
		"min",
		"Current IPX Socket is default",
		"This program requires MS-DOS 5.0 or higher.\nYou are using MS-DOS",
		"You can use the -nodoscheck command line\nswitch to override this check, but it\nmay have unpredictable results, namely\nwith DOS file error handling.\n",
		"Not enough file handles!",
		"of the necessary file handles\nthat Descent requires to execute properly.  You will\nneed to increase the FILES=n line in your config.sys.",
		"If you are running with a clean boot, then you will need\nto create a CONFIG.SYS file in your root directory, with\nthe line FILES=15 in it.  If you need help with this,\ncontact Interplay technical support.",
		"You may also run with the -nofilecheck command line option\nthat will disable this check, but you might get errors\nwhen loading saved games or playing demos.",
		"Available memory",
		"more bytes of DOS memory needed!",
		"more bytes of virtual memory needed.  Reconfigure VMM.",
		"more bytes of extended/expanded memory needed!",
		"Or else you you need to use virtual memory (See README.TXT)",
		"more bytes of physical memory needed!",
		"Check to see that your virtual memory settings allow\nyou to use all of your physical memory (See README.TXT)",
		"Initializing DPMI services",
		"Initializing critical error handler",
		"Enables Virtual I/O Iglasses! stereo display",
		"Enables Iglasses! head tracking via COM port",
		"Enables Kasan's 3dMax stereo display in low res.",
		"3DBios must be installed for 3dMax operation.",
		"Enables Kasan's 3dMax stereo display in high res",
		"Press any key for more options...",
		"Enables dynamic socket changing",
		"Disables the file handles check",
		"Getting settings from DESCENT.CFG...",
		"Initializing timer system...",
		"Initializing keyboard handler...",
		"Initializing mouse handler...",
		"Mouse support disabled...",
		"Initializing joystick handler...",
		"Slow joystick reading enabled...",
		"Polled joystick reading enabled...",
		"BIOS joystick reading enabled...",
		"Joystick support disabled...",
		"Initializing divide by zero handler...",
		"Initializing network...",
		"Using IPX network support on channel",
		"No IPX compatible network found.",
		"Error opening socket",
		"Not enough low memory for IPX buffers.",
		"Error initializing IPX.  Error code:",
		"Network support disabled...",
		"Initializing graphics system...",
		"SOUND: Error opening",
		"SOUND: Error locking down instruments",
		"SOUND: (HMI)",
		"SOUND: Error locking down drums",
		"SOUND: Error locking midi track map!",
		"SOUND: Error locking midi callback function!",
		"Using external control:",
		"Invalid serial port parameter for -itrak!",
		"Initializing i-glasses! head tracking on serial port %d",
		"Make sure the glasses are turned on!",
		"Press ESC to abort",
		"Failed to open serial port.  Status =",
		"Message",
		"Macro",
		"Error locking serial interrupt routine!",
		"Error locking serial port data!",
		"Robots are normal",
		"Robots move fast, fire seldom",
		"Robot painting OFF",
		"Robot painting with texture %d"
	};
	(void)bytes_read;
#endif

	for (const auto i : xrange(N_TEXT_STRINGS))
	{
#if DXX_BUILD_DESCENT == 1
		if (!tptr)
		{
			if (i >= N_TEXT_STRINGS_MIN)	// account for non-registered 1.4/1.5 text files
			{
				Text_string[i - 1] = extra_strings[i - N_TEXT_STRINGS_MIN - 1];
				Text_string[i] = extra_strings[i - N_TEXT_STRINGS_MIN];
				continue;
			}
			else
			{
				Error("Not enough strings in text file - expecting %d (or at least %d), found %d", N_TEXT_STRINGS, N_TEXT_STRINGS_MIN, i);
			}
		}
#endif
		Text_string[i] = tptr;
		char *ts = tptr;

		tptr = strchr(tptr,'\n');

#if DXX_BUILD_DESCENT == 2
		if (!tptr)
		{
			if (i == 644)	/* older datafiles */
			{
				if (bytes_read == SHAREWARE_TEXTSIZE)
				{
					Text_string[173] = Text_string[172];
					Text_string[172] = Text_string[171];
					Text_string[171] = Text_string[170];
					Text_string[170] = Text_string[169];
					Text_string[169] = "Windows Joystick";
				}

				Text_string[644] = "Z1";
				Text_string[645] = "UN";
				Text_string[646] = "P1";
				Text_string[647] = "R1";
				Text_string[648] = "Y1";
				break;
			}

			Error("Not enough strings in text file - expecting %d, found %d\n", N_TEXT_STRINGS, i);
		}
#endif
		if (tptr)
			*tptr++ = 0;

		if (have_binary)
			decode_text_line(ts);

		//scan for special chars (like \n)
		if (auto p = strchr(ts, '\\'))
		{
			for (auto q = p; assert(*p == '\\'), *p;)
			{
				char newchar;

				if (p[1] == 'n') newchar = '\n';
				else if (p[1] == 't') newchar = '\t';
				else if (p[1] == '\\') newchar = '\\';
				else
					Error("Unsupported key sequence <\\%c> on line %d of file <%s>", p[1], i + 1, filename);

				*q++ = newchar;
				p += 2;
				char *r = strchr(p, '\\');
				if (!r)
				{
					r = tptr;
					assert(!r[-1]);
					assert(r == 1 + p + strlen(p));
					memmove(q, p, r - p);
					break;
				}
				memmove(q, p, r - p);
				q += (r - p);
				p = r;
			}
		}

		switch(i) {
#if DXX_BUILD_DESCENT == 1
			case 116:
				if (!d_stricmp(ts, "SPREADFIRE")) // This string is too long to fit in the cockpit-box
				{
					ts[6] = 0;
				}
				break;
#endif
				  
			  case IDX_TEXT_OVERWRITTEN:
				{
					static const char extra[] = "\n<Ctrl-C> converts format\nIntel <-> PowerPC";
					std::size_t l = strlen(ts);
					overwritten_text = std::make_unique<char[]>(l + sizeof(extra));
					char *o = overwritten_text.get();
					std::copy_n(extra, sizeof(extra), std::copy_n(ts, l, o));
					Text_string[i] = o;
					break;
				}
          }
	}
	return loaded_blob;
}

static std::unique_ptr<char[]> load_text_from_texfile(const char *const filename, RAIIPHYSFS_File texfile)
{
	auto &&[bytes_read, p] = load_blob_from_file(filename, std::move(texfile));
	const auto pg = p.get();
	const auto iter_remove = std::remove(pg, std::next(pg, bytes_read), '\r');
	*iter_remove = 0;
	return parse_blob_as_text_strings(filename, std::move(p), false, bytes_read);
}

static std::unique_ptr<char[]> load_text_from_txbfile(const char *const filename, RAIIPHYSFS_File txbfile)
{
	auto &&[bytes_read, p] = load_blob_from_file(filename, std::move(txbfile));
	p[bytes_read] = 0;
	return parse_blob_as_text_strings(filename, std::move(p), true, bytes_read);
}

static std::unique_ptr<char[]> load_text_from_file(const char *const texfilename)
{
	if (auto &&[texfile, physfserr] = PHYSFSX_openReadBuffered(texfilename); texfile)
		return load_text_from_texfile(texfilename, std::move(texfile));
#define DXX_TXB_FILENAME	"descent.txb"
	else if (auto &&[txbfile, physfserr2] = PHYSFSX_openReadBuffered(DXX_TXB_FILENAME); txbfile)
		return load_text_from_txbfile(DXX_TXB_FILENAME, std::move(txbfile));
	else
		Error("Failed to open files %s, " DXX_TXB_FILENAME ": \"%s\", \"%s\"", texfilename, PHYSFS_getErrorByCode(physfserr), PHYSFS_getErrorByCode(physfserr2));
#undef DXX_TXB_FILENAME
}

}

}

//decode an encoded line of text of bitmaps.tbl
void decode_text_line(char *p)
{
	for (; const char c = *p; p++)
		*p = decode_char(c);
}

// decode buffer of text, preserves newlines
void decode_text(std::span<char> ptr)
{
	for (auto &c : ptr)
	{
		if (c != '\n')
			c = decode_char(c);
	}
}

//load all the text strings for Descent
namespace dsx {

#ifdef USE_BUILTIN_ENGLISH_TEXT_STRINGS
static
#endif
std::array<const char *, N_TEXT_STRINGS> Text_string;

void load_text()
{
	auto &&local_text = load_text_from_file(CGameArg.DbgAltTex.empty() ? "descent.tex" : CGameArg.DbgAltTex.c_str());
#ifdef GENERATE_BUILTIN_TEXT_TABLE
	for (unsigned u = 0; u < std::size(Text_string); ++u)
	{
		printf("\t%u\t\"", u);
		for (char *px = Text_string[u]; *px; ++px) {
			unsigned x = (unsigned)*px;
			if (isprint(x))
				putchar(x);
			else if (x == '\t')
				printf("\\t");
			else if (x == '\r')
				printf("\\r");
			else if (x == '\n')
				printf("\\n");
			else
				printf("\\x%.2x", x);
		}
		printf("\"\n");
	}	
#endif
	text = std::move(local_text);
}
}


