/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

/*
 *
 * Functions for accessing arguments.
 *
 */

#include <string>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <SDL_stdinc.h>
#include "physfsx.h"
#include "args.h"
#include "u_mem.h"
#include "strutil.h"
#include "digi.h"
#include "game.h"
#include "gauges.h"
#include "console.h"
#include "mission.h"
#if DXX_USE_UDP
#include "net_udp.h"
#endif

#include "compiler-range_for.h"
#include "partial_range.h"

namespace dcx {
CArg CGameArg;
constexpr std::integral_constant<std::size_t, 1000> MAX_ARGS{};
typedef std::vector<std::string> Arglist;

namespace {

class ini_entry
{
	std::string m_filename;
public:
	ini_entry(std::string &&f) :
		m_filename(std::move(f))
	{
	}
	const std::string &filename() const
	{
		return m_filename;
	}
};

typedef std::vector<ini_entry> Inilist;

class argument_exception
{
public:
	const std::string arg;
	argument_exception(std::string &&a) :
		arg(std::move(a))
	{
	}
};

class missing_parameter : public argument_exception
{
public:
	missing_parameter(std::string &&a) :
		argument_exception(std::move(a))
	{
	}
};

class unhandled_argument : public argument_exception
{
public:
	unhandled_argument(std::string &&a) :
		argument_exception(std::move(a))
	{
	}
};

class conversion_failure : public argument_exception
{
public:
	const std::string value;
	conversion_failure(std::string &&a, std::string &&v) :
		argument_exception(std::move(a)), value(std::move(v))
	{
	}
};

class nesting_depth_exceeded
{
};

}

static void AppendIniArgs(const char *filename, Arglist &Args)
{
	if (auto f = PHYSFSX_openReadBuffered(filename))
	{
		PHYSFSX_gets_line_t<1024> line;
		while (Args.size() < MAX_ARGS && PHYSFSX_fgets(line, f))
		{
			const auto separator = " \t";
			for(char *token = strtok(line, separator); token != NULL; token = strtok(NULL, separator))
			{
				if (*token == ';')
					break;
				Args.push_back(token);
			}
		}
	}
}

static std::string &&arg_string(Arglist::iterator &pp, Arglist::const_iterator end)
{
	auto arg = pp;
	if (++pp == end)
		throw missing_parameter(std::move(*arg));
	return std::move(*pp);
}

static long arg_integer(Arglist::iterator &pp, Arglist::const_iterator end)
{
	auto arg = pp;
	auto &&value = arg_string(pp, end);
	char *p;
	auto i = strtol(value.c_str(), &p, 10);
	if (*p)
		throw conversion_failure(std::move(*arg), std::move(value));
	return i;
}

template<typename E> E arg_enum(Arglist::iterator &pp, Arglist::const_iterator end)
{
	return static_cast<E>(arg_integer(pp, end));
}

#if DXX_USE_UDP
static void arg_port_number(Arglist::iterator &pp, Arglist::const_iterator end, uint16_t &out, bool allow_privileged)
{
	auto port = arg_integer(pp, end);
	if (static_cast<uint16_t>(port) == port && (allow_privileged || port >= 1024))
		out = port;
}
#endif

static void InitGameArg()
{
	CGameArg.SysMaxFPS = MAXIMUM_FPS;
#if defined(DXX_BUILD_DESCENT_II)
	GameArg.SndDigiSampleRate = SAMPLE_RATE_22K;
#endif
#if DXX_USE_UDP
	CGameArg.MplUdpHostAddr = UDP_MANUAL_ADDR_DEFAULT;
#if DXX_USE_TRACKER
	CGameArg.MplTrackerAddr = TRACKER_ADDR_DEFAULT;
	CGameArg.MplTrackerPort = TRACKER_PORT_DEFAULT;
#endif
#endif
	CGameArg.DbgVerbose = CON_NORMAL;
	CGameArg.DbgBpp = 32;
#if DXX_USE_OGL
	CGameArg.OglSyncMethod = OGL_SYNC_METHOD_DEFAULT;
	CGameArg.OglSyncWait = OGL_SYNC_WAIT_DEFAULT;
	CGameArg.DbgGlIntensity4Ok 	= true;
	CGameArg.DbgGlLuminance4Alpha4Ok = true;
	CGameArg.DbgGlRGBA2Ok = true;
	CGameArg.DbgGlReadPixelsOk = true;
	CGameArg.DbgGlGetTexLevelParamOk = true;
#endif
}

}

namespace dsx {

Arg GameArg;

static void ReadCmdArgs(Inilist &ini, Arglist &Args);

static void ReadIniArgs(Inilist &ini)
{
	Arglist Args;
	AppendIniArgs(ini.back().filename().c_str(), Args);
	ReadCmdArgs(ini, Args);
	ini.pop_back();
}

static void ReadCmdArgs(Inilist &ini, Arglist &Args)
{
	for (Arglist::iterator pp = Args.begin(), end = Args.end(); pp != end; ++pp)
	{
		const char *p = pp->c_str();
	// System Options

		if (!d_stricmp(p, "-help") || !d_stricmp(p, "-h") || !d_stricmp(p, "-?") || !d_stricmp(p, "?"))
			CGameArg.SysShowCmdHelp = true;
		else if (!d_stricmp(p, "-nonicefps"))
			CGameArg.SysNoNiceFPS = true;
		else if (!d_stricmp(p, "-maxfps"))
			CGameArg.SysMaxFPS = arg_integer(pp, end);
		else if (!d_stricmp(p, "-hogdir"))
			CGameArg.SysHogDir = arg_string(pp, end);
#if PHYSFS_VER_MAJOR >= 2
		else if (!d_stricmp(p, "-add-missions-dir"))
			CGameArg.SysMissionDir = arg_string(pp, end);
#endif
		else if (!d_stricmp(p, "-nohogdir"))
		{
			/* No effect on non-Unix.  Ignore it so that players can
			 * pass it via a cross-platform ini.
			 */
#if defined(__unix__)
			CGameArg.SysNoHogDir = true;
#endif
		}
		else if (!d_stricmp(p, "-use_players_dir"))
			CGameArg.SysUsePlayersDir = static_cast<int8_t>(- (sizeof(PLAYER_DIRECTORY_TEXT) - 1));
		else if (!d_stricmp(p, "-lowmem"))
			CGameArg.SysLowMem = true;
		else if (!d_stricmp(p, "-pilot"))
			CGameArg.SysPilot = arg_string(pp, end);
		else if (!d_stricmp(p, "-record-demo-format"))
			CGameArg.SysRecordDemoNameTemplate = arg_string(pp, end);
		else if (!d_stricmp(p, "-auto-record-demo"))
			CGameArg.SysAutoRecordDemo = true;
		else if (!d_stricmp(p, "-window"))
			CGameArg.SysWindow = true;
		else if (!d_stricmp(p, "-noborders"))
			CGameArg.SysNoBorders = true;
		else if (!d_stricmp(p, "-notitles"))
			CGameArg.SysNoTitles = true;
#if defined(DXX_BUILD_DESCENT_II)
		else if (!d_stricmp(p, "-nomovies"))
			GameArg.SysNoMovies 		= 1;
#endif
		else if (!d_stricmp(p, "-autodemo"))
			CGameArg.SysAutoDemo = true;

	// Control Options

		else if (!d_stricmp(p, "-nocursor"))
			CGameArg.CtlNoCursor 		= true;
		else if (!d_stricmp(p, "-nomouse"))
			CGameArg.CtlNoMouse 		= true;
		else if (!d_stricmp(p, "-nojoystick"))
		{
#if DXX_MAX_JOYSTICKS
			CGameArg.CtlNoJoystick		= 1;
#endif
		}
		else if (!d_stricmp(p, "-nostickykeys"))
			CGameArg.CtlNoStickyKeys	= true;

	// Sound Options

		else if (!d_stricmp(p, "-nosound"))
			CGameArg.SndNoSound		= 1;
		else if (!d_stricmp(p, "-nomusic"))
			CGameArg.SndNoMusic = true;
#if defined(DXX_BUILD_DESCENT_II)
		else if (!d_stricmp(p, "-sound11k"))
			GameArg.SndDigiSampleRate 		= SAMPLE_RATE_11K;
#endif
		else if (!d_stricmp(p, "-nosdlmixer"))
		{
#if DXX_USE_SDLMIXER
			CGameArg.SndDisableSdlMixer = true;
#endif
		}

	// Graphics Options

		else if (!d_stricmp(p, "-lowresfont"))
			CGameArg.GfxSkipHiresFNT = true;
#if defined(DXX_BUILD_DESCENT_II)
		else if (!d_stricmp(p, "-lowresgraphics"))
			GameArg.GfxSkipHiresGFX	= 1;
		else if (!d_stricmp(p, "-lowresmovies"))
			GameArg.GfxSkipHiresMovie 		= 1;
#endif
#if DXX_USE_OGL
	// OpenGL Options

		else if (!d_stricmp(p, "-gl_fixedfont"))
			CGameArg.OglFixedFont = true;
		else if (!d_stricmp(p, "-gl_syncmethod"))
			CGameArg.OglSyncMethod = arg_enum<SyncGLMethod>(pp, end);
		else if (!d_stricmp(p, "-gl_syncwait"))
			CGameArg.OglSyncWait = arg_integer(pp, end);
		else if (!d_stricmp(p, "-gl_darkedges"))
			CGameArg.OglDarkEdges = true;
#endif

	// Multiplayer Options

#if DXX_USE_UDP
		else if (!d_stricmp(p, "-udp_hostaddr"))
			CGameArg.MplUdpHostAddr = arg_string(pp, end);
		else if (!d_stricmp(p, "-udp_hostport"))
			/* Peers use -udp_myport to change, so peer cannot set a
			 * privileged port.
			 */
			arg_port_number(pp, end, CGameArg.MplUdpHostPort, false);
		else if (!d_stricmp(p, "-udp_myport"))
		{
			arg_port_number(pp, end, CGameArg.MplUdpMyPort, false);
		}
		else if (!d_stricmp(p, "-no-tracker"))
		{
			/* Always recognized.  No-op if tracker support compiled
			 * out. */
#if DXX_USE_TRACKER
			CGameArg.MplTrackerAddr.clear();
#endif
		}
#if DXX_USE_TRACKER
		else if (!d_stricmp(p, "-tracker_hostaddr"))
		{
			CGameArg.MplTrackerAddr = arg_string(pp, end);
		}
		else if (!d_stricmp(p, "-tracker_hostport"))
			arg_port_number(pp, end, CGameArg.MplTrackerPort, true);
#endif
#endif

#if defined(DXX_BUILD_DESCENT_I)
		else if (!d_stricmp(p, "-nobm"))
			GameArg.EdiNoBm 		= 1;
#elif defined(DXX_BUILD_DESCENT_II)
#if DXX_USE_EDITOR
	// Editor Options

		else if (!d_stricmp(p, "-autoload"))
			GameArg.EdiAutoLoad = arg_string(pp, end);
		else if (!d_stricmp(p, "-macdata"))
			GameArg.EdiMacData 		= 1;
		else if (!d_stricmp(p, "-hoarddata"))
			GameArg.EdiSaveHoardData 	= 1;
#endif
#endif

	// Debug Options

		else if (!d_stricmp(p, "-debug"))
			CGameArg.DbgVerbose 	= CON_DEBUG;
		else if (!d_stricmp(p, "-verbose"))
			CGameArg.DbgVerbose 	= CON_VERBOSE;

		else if (!d_stricmp(p, "-no-grab"))
			CGameArg.DbgForbidConsoleGrab = true;
		else if (!d_stricmp(p, "-safelog"))
			CGameArg.DbgSafelog = true;
		else if (!d_stricmp(p, "-norun"))
			CGameArg.DbgNoRun = true;
		else if (!d_stricmp(p, "-renderstats"))
			CGameArg.DbgRenderStats = true;
		else if (!d_stricmp(p, "-text"))
			CGameArg.DbgAltTex = arg_string(pp, end);
		else if (!d_stricmp(p, "-showmeminfo"))
			CGameArg.DbgShowMemInfo 		= 1;
		else if (!d_stricmp(p, "-nodoublebuffer"))
			CGameArg.DbgNoDoubleBuffer = true;
		else if (!d_stricmp(p, "-bigpig"))
			CGameArg.DbgNoCompressPigBitmap = true;
		else if (!d_stricmp(p, "-16bpp"))
			CGameArg.DbgBpp		= 16;

#if DXX_USE_OGL
		else if (!d_stricmp(p, "-gl_oldtexmerge"))
			CGameArg.DbgUseOldTextureMerge = true;
		else if (!d_stricmp(p, "-gl_intensity4_ok"))
			CGameArg.DbgGlIntensity4Ok = arg_integer(pp, end);
		else if (!d_stricmp(p, "-gl_luminance4_alpha4_ok"))
			CGameArg.DbgGlLuminance4Alpha4Ok = arg_integer(pp, end);
		else if (!d_stricmp(p, "-gl_rgba2_ok"))
			CGameArg.DbgGlRGBA2Ok = arg_integer(pp, end);
		else if (!d_stricmp(p, "-gl_readpixels_ok"))
			CGameArg.DbgGlReadPixelsOk = arg_integer(pp, end);
		else if (!d_stricmp(p, "-gl_gettexlevelparam_ok"))
			CGameArg.DbgGlGetTexLevelParamOk = arg_integer(pp, end);
#else
		else if (!d_stricmp(p, "-tmap"))
			CGameArg.DbgTexMap = arg_string(pp, end);
		else if (!d_stricmp(p, "-hwsurface"))
			CGameArg.DbgSdlHWSurface = true;
		else if (!d_stricmp(p, "-asyncblit"))
			CGameArg.DbgSdlASyncBlit = true;
#endif
		else if (!d_stricmp(p, "-ini"))
		{
			ini.emplace_back(arg_string(pp, end));
			if (ini.size() > 10)
				throw nesting_depth_exceeded();
			ReadIniArgs(ini);
		}
		else if (!strncmp(p, "-psn", 4))
		{
			//do nothing/gobble it up
		}
		else
			throw unhandled_argument(std::move(*pp));
	}
}

}

namespace dcx {

static void PostProcessGameArg()
{
	if (CGameArg.SysMaxFPS < MINIMUM_FPS)
		CGameArg.SysMaxFPS = MINIMUM_FPS;
	else if (CGameArg.SysMaxFPS > MAXIMUM_FPS)
		CGameArg.SysMaxFPS = MAXIMUM_FPS;
#if PHYSFS_VER_MAJOR >= 2
	if (!CGameArg.SysMissionDir.empty())
		PHYSFS_mount(CGameArg.SysMissionDir.c_str(), MISSION_DIR, 1);
#endif

#if SDL_MAJOR_VERSION == 1
	static char sdl_disable_lock_keys[] = "SDL_DISABLE_LOCK_KEYS=0";
	if (CGameArg.CtlNoStickyKeys) // Must happen before SDL_Init!
		sdl_disable_lock_keys[sizeof(sdl_disable_lock_keys) - 2] = '1';
	SDL_putenv(sdl_disable_lock_keys);
#endif
}

static std::string ConstructIniStackExplanation(const Inilist &ini)
{
	Inilist::const_reverse_iterator i = ini.rbegin(), e = ini.rend();
	if (i == e)
		return " while processing <command line>";
	std::string result;
	result.reserve(ini.size() * 128);
	result += " while processing \"";
	for (;;)
	{
		result += i->filename();
		if (++ i == e)
			return result += "\"";
		result += "\"\n    included from \"";
	}
}

}

namespace dsx {

bool InitArgs( int argc,char **argv )
{
	InitGameArg();

	Inilist ini;
	try {
		{
			assert(ini.empty());
#if defined(DXX_BUILD_DESCENT_I)
			const auto INI_FILENAME = "d1x.ini";
#elif defined(DXX_BUILD_DESCENT_II)
			const auto INI_FILENAME = "d2x.ini";
#endif
			ini.emplace_back(INI_FILENAME);
			ReadIniArgs(ini);
		}
		{
			Arglist Args;
			Args.reserve(argc);
			range_for (auto &i, unchecked_partial_range(argv, 1u, static_cast<unsigned>(argc)))
				Args.push_back(i);
			ReadCmdArgs(ini, Args);
		}
		PostProcessGameArg();
		return true;
	} catch(const missing_parameter& e) {
		UserError("Missing parameter for argument \"%s\"%s", e.arg.c_str(), ConstructIniStackExplanation(ini).c_str());
	} catch(const unhandled_argument& e) {
		UserError("Unhandled argument \"%s\"%s", e.arg.c_str(), ConstructIniStackExplanation(ini).c_str());
	} catch(const conversion_failure& e) {
		UserError("Failed to convert argument \"%s\" parameter \"%s\"%s", e.arg.c_str(), e.value.c_str(), ConstructIniStackExplanation(ini).c_str());
	} catch(const nesting_depth_exceeded &) {
		UserError("Nesting depth exceeded%s", ConstructIniStackExplanation(ini).c_str());
	}
	return false;
}

}
