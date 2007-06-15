#SConstruct

# needed imports
import sys
import os
import SCons.Util

PROGRAM_NAME = 'D1X-Rebirth'

# version number
D1XMAJOR = 0
D1XMINOR = 52

#SVN_REVISION = os.popen('echo -n `LANG=C svn info | grep ^Revision | cut -d\  -f2`').read()

# optional micro revision: set it to SVN_REVISION if available, zero otherwise.
D1XMICRO = 0
#D1XMICRO = int(SVN_REVISION)

VERSION_STRING = ' v' + str(D1XMAJOR) + '.' + str(D1XMINOR)
if (D1XMICRO):
	VERSION_STRING += '.' + str(D1XMICRO)

print '\n===== ' + PROGRAM_NAME + VERSION_STRING + ' =====\n'

# installation path
PREFIX = '/usr/local/'
BIN_SUBDIR = 'bin/'
DATA_SUBDIR = 'share/games/d1x-rebirth/'
BIN_DIR = PREFIX + BIN_SUBDIR
DATA_DIR = PREFIX + DATA_SUBDIR

# command-line parms
sharepath = str(ARGUMENTS.get('sharepath', DATA_DIR))
debug = int(ARGUMENTS.get('debug', 0))
profiler = int(ARGUMENTS.get('profiler', 0))
sdl_only = int(ARGUMENTS.get('sdl_only', 0))
no_asm = int(ARGUMENTS.get('no_asm', 0))
editor = int(ARGUMENTS.get('editor', 0))
shareware = int(ARGUMENTS.get('shareware', 0))
sdlmixer = int(ARGUMENTS.get('sdlmixer', 0))
arm = int(ARGUMENTS.get('arm', 0))
gp2x = int(ARGUMENTS.get('gp2x', 0))

# general source files
common_sources = [
'2d/2dsline.c',
'2d/bitblt.c',
'2d/bitmap.c',
'2d/box.c',
'2d/canvas.c',
'2d/circle.c',
'2d/disc.c',
'2d/font.c',
'2d/gpixel.c',
'2d/ibitblt.c',
'2d/line.c',
'2d/palette.c',
'2d/pcx.c',
'2d/pixel.c',
'2d/poly.c',
'2d/rect.c',
'2d/rle.c',
'2d/scalec.c',
'2d/tmerge.c',
'3d/clipper.c',
'3d/draw.c',
'3d/globvars.c',
'3d/instance.c',
'3d/interp.c',
'3d/matrix.c',
'3d/points.c',
'3d/rod.c',
'3d/setup.c',
'arch/sdl/event.c',
'arch/sdl/init.c',
'arch/sdl/joy.c',
'arch/sdl/joydefs.c',
'arch/sdl/key.c',
'arch/sdl/mouse.c',
'arch/sdl/timer.c',
'cfile/cfile.c',
'iff/iff.c',
'main/ai.c',
'main/aipath.c',
'main/digiobj.c',
'main/args.c',
'main/automap.c',
'main/ban.c',
'main/bm.c',
'main/bmread.c',
'main/cdplay.c',
'main/cntrlcen.c',
'main/collide.c',
'main/command.c',
'main/config.c',
'main/controls.c',
'main/credits.c',
'main/custom.c',
'main/d_conv.c',
'main/d_gamecv.c',
'main/dumpmine.c',
'main/effects.c',
'main/endlevel.c',
'main/fireball.c',
'main/fuelcen.c',
'main/fvi.c',
'main/game.c',
'main/gamefont.c',
'main/gameseg.c',
'main/gameseq.c',
'main/gauges.c',
'main/hash.c',
'main/hostage.c',
'main/hud.c',
'main/hudlog.c',
'main/ignore.c',
'main/inferno.c',
'main/ip_base.cpp',
'main/ipclienc.c',
'main/ipclient.cpp',
'main/ipx_drv.c',
'main/kconfig.c',
'main/kmatrix.c',
'main/laser.c',
'main/lighting.c',
'main/m_inspak.c',
'main/menu.c',
'main/mglobal.c',
'main/mission.c',
'main/mlticntl.c',
'main/morph.c',
'main/multi.c',
'main/multibot.c',
'main/multipow.c',
'main/multiver.c',
'main/mprofile.c',
'main/netlist.c',
'main/netmisc.c',
'main/netpkt.c',
'main/network.c',
'main/newdemo.c',
'main/newmenu.c',
'main/nncoms.c',
'main/nocomlib.c',
'main/object.c',
'main/paging.c',
'main/physics.c',
'main/piggy.c',
'main/pingstat.c',
'main/playsave.c',
'main/polyobj.c',
'main/powerup.c',
'main/radar.c',
'main/reconfig.c',
'main/reorder.c',
'main/render.c',
'main/robot.c',
'main/scores.c',
'main/slew.c',
'main/songs.c',
'main/state.c',
'main/switch.c',
'main/terrain.c',
'main/texmerge.c',
'main/text.c',
'main/titles.c',
'main/vclip.c',
'main/vlcnfire.c',
'main/wall.c',
'main/weapon.c',
'mem/mem.c',
'misc/compare.c',
'misc/d_glob.c',
'misc/d_io.c',
'misc/d_slash.c',
'misc/dl_list.c',
'misc/error.c',
'misc/strio.c',
'misc/strutil.c',
'texmap/ntmap.c',
'texmap/scanline.c'
]

# for editor
editor_sources = [
'editor/centers.c',
'editor/curves.c',
'editor/autosave.c',
'editor/eglobal.c',
'editor/ehostage.c',
'editor/elight.c',
'editor/eobject.c',
'editor/eswitch.c',
'editor/fixseg.c',
'editor/func.c',
'editor/group.c',
'editor/info.c',
'editor/kbuild.c',
'editor/kcurve.c',
'editor/kfuncs.c',
'editor/kgame.c',
'editor/khelp.c',
'editor/kmine.c',
'editor/ksegmove.c',
'editor/ksegsel.c',
'editor/ksegsize.c',
'editor/ktmap.c',
'editor/kview.c',
'editor/med.c',
'editor/meddraw.c',
'editor/medmisc.c',
'editor/medrobot.c',
'editor/medsel.c',
'editor/medwall.c',
'editor/mine.c',
'editor/objpage.c',
'editor/segment.c',
'editor/seguvs.c',
'editor/texpage.c',
'editor/texture.c',
'main/gamemine.c',
'main/gamesave.c',
'ui/button.c',
'ui/checkbox.c',
'ui/gadget.c',
'ui/icon.c',
'ui/inputbox.c',
'ui/keypad.c',
'ui/keypress.c',
'ui/keytrap.c',
'ui/lfile.c',
'ui/listbox.c',
'ui/menu.c',
'ui/menubar.c',
'ui/message.c',
'ui/mouse.c',
'ui/popup.c',
'ui/radio.c',
'ui/scroll.c',
'ui/ui.c',
'ui/uidraw.c',
'ui/userbox.c',
'ui/window.c'
]

# for linux
arch_linux_sources = [
'arch/linux/arch_ip.cpp',
'arch/linux/init.c',
'arch/linux/ipx_bsd.c',
'arch/linux/ipx_kali.c',
'arch/linux/linuxnet.c',
'arch/linux/mono.c',
'arch/linux/ukali.c',
]

# choosing a sound implementation for Linux
common_sound_hmp2mid = [ 'main/hmp2mid.c' ]
arch_linux_sound_sdlmixer = [ 'arch/sdl/mixdigi.c', 'arch/sdl/mixmusic.c', 'arch/sdl/jukebox.c' ]
arch_linux_sound_old = [ 'arch/sdl/digi.c' ]

if (sdlmixer == 1):
	common_sources += common_sound_hmp2mid
	arch_linux_sources += arch_linux_sound_sdlmixer
else:
	arch_linux_sources += arch_linux_sound_old

# for windows
arch_win32_sources = [
'arch/win32/arch_ip.cpp',
'arch/win32/hmpfile.c',
'arch/win32/init.c',
'arch/win32/ipx_win.c',
'arch/win32/midi.c',
'arch/win32/mono.c',
'arch/win32/winnet.c',
'arch/sdl/digi.c'
]

# for opengl
arch_ogl_sources = [
'arch/ogl/gr.c',
'arch/ogl/ogl.c',
'arch/ogl/sdlgl.c'
]

# for sdl
arch_sdl_sources = [
'arch/sdl/gr.c',
'texmap/tmapflat.c'
]

# assembler related
asm_sources = [
'2d/linear.asm',
'2d/tmerge_a.asm',
'maths/fix.asm',
'maths/rand.c',
'maths/vecmat.c',
'maths/vecmata.asm',
'texmap/tmap_ll.asm',
'texmap/tmap_flt.asm',
'texmap/tmapfade.asm',
'texmap/tmap_lin.asm',
'texmap/tmap_per.asm'
]

noasm_sources = [
'maths/fixc.c',
'maths/rand.c',
'maths/tables.c',
'maths/vecmat.c'
]

# flags and stuff for all platforms
env = Environment(ENV = os.environ)
env.ParseConfig('sdl-config --cflags')
env.ParseConfig('sdl-config --libs')
env.Append(CPPFLAGS = ['-Wall', '-funsigned-char'])
env.Append(CPPDEFINES = [('D1XMAJOR', '\\"' + str(D1XMAJOR) + '\\"'), ('D1XMINOR', '\\"' + str(D1XMINOR) + '\\"')])
env.Append(CPPDEFINES = ['NMONO', 'NETWORK', 'HAVE_NETIPX_IPX_H', 'SUPPORTS_NET_IP', '__SDL__', 'SDL_AUDIO', '_REENTRANT'])
env.Append(CPPPATH = ['include', 'main', 'arch/sdl/include'])
generic_libs = ['SDL']
sdlmixerlib = ['SDL_mixer']

if sdlmixer:
	env.Append(CPPDEFINES = ['USE_SDLMIXER'])

if (D1XMICRO):
	env.Append(CPPDEFINES = [('D1XMICRO', '\\"' + str(D1XMICRO) + '\\"')])

# Get traditional compiler environment variables
if os.environ.has_key('CC'):
	env['CC'] = os.environ['CC']
if os.environ.has_key('CFLAGS'):
	env['CCFLAGS'] += SCons.Util.CLVar(os.environ['CFLAGS'])
if os.environ.has_key('CXX'):
	env['CXX'] = os.environ['CXX']
if os.environ.has_key('CXXFLAGS'):
	env['CXXFLAGS'] += SCons.Util.CLVar(os.environ['CXXFLAGS'])
if os.environ.has_key('LDFLAGS'):
	env['LINKFLAGS'] += SCons.Util.CLVar(os.environ['LDFLAGS'])

# windows or *nix?
if sys.platform == 'win32':
	print "compiling on Windows"
	osdef = '__WINDOWS__'
	osasmdef = 'win32'
	sharepath = ''
	env.Append(CPPDEFINES = ['__WINDOWS__'])
	env.Append(CPPPATH = ['arch/win32/include'])
	ogldefines = ['SDL_GL', 'OGL_RUNTIME_LOAD', 'OGL']
	common_sources += arch_win32_sources
	ogllibs = ''
	winlibs = ['glu32', 'wsock32', 'winmm', 'mingw32', 'SDLmain']
	libs = winlibs + generic_libs
	lflags = '-mwindows'
else:
	print "compiling on *NIX"
	osdef = '__LINUX__'
	osasmdef = 'elf'
	sharepath += '/'
	env.Append(CPPDEFINES = ['__LINUX__'])
	env.Append(CPPPATH = ['arch/linux/include'])
	ogldefines = ['SDL_GL', 'OGL']
	common_sources += arch_linux_sources
	ogllibs = ['GL', 'GLU']
	libs = generic_libs
	lflags = '-L/usr/X11R6/lib'

# GP2X test env
if (gp2x == 1):
	sdl_only = 1
	arm = 1
	sharepath = ''
	env.Replace(CC = '/usr/local/gp2xdev/bin/gp2x-gcc')
	env.Replace(CXX = '/usr/local/gp2xdev/bin/gp2x-g++')
	env.Append(CPPDEFINES = ['GP2X'])
	env.Append(CPPPATH = ['/usr/local/gp2xdev/include'])
	#env.Append(CPPFLAGS = ' -ffast-math -fPIC -funroll-all-loops -fomit-frame-pointer -march=armv4t') # left for further optimisation/debugging
	common_sources += ['main/clock.c']
	libs += ['pthread']
	lflags = '-static'
	lpath = '/usr/local/gp2xdev/lib'
else:
	lpath = ''

# arm architecture?
if (arm == 1):
	no_asm = 1
	env.Append(CPPDEFINES = ['WORDS_NEED_ALIGNMENT'])
	env.Append(CPPFLAGS = ['-mstructure-size-boundary=8'])

# sdl or opengl?
if (sdl_only == 1):
	print "building with SDL"
	target = 'd1x-rebirth-sdl'
	env.Append(CPPDEFINES = ['SDL_VIDEO'])
	env.Append(CPPPATH = ['arch/sdl/include'])
	common_sources += arch_sdl_sources
else:
	print "building with OpenGL"
	target = 'd1x-rebirth-gl'
	env.Append(CPPDEFINES = ogldefines)
	env.Append(CPPPATH = ['arch/ogl/include'])
	common_sources += arch_ogl_sources
	libs += ogllibs

# SDL_mixer for sound? (*NIX only)
if (sdlmixer == 1):
	print "including SDL_mixer"
	libs += sdlmixerlib

# debug?
if (debug == 1):
	print "including: DEBUG"
	env.Append(CPPFLAGS = ['-g'])
else:
	env.Append(CPPDEFINES = ['NDEBUG', 'RELEASE'])
	env.Append(CPPFLAGS = ['-O2'])

# profiler?
if (profiler == 1):
	env.Append(CPPFLAGS = ['-pg'])
	lflags += ' -pg'

# assembler code?
if (no_asm == 0) and (sdl_only == 1):
	print "including: ASSEMBLER"
	env.Append(CPPDEFINES = ['ASM_VECMAT'])
	Object(['texmap/tmappent.S', 'texmap/tmapppro.S'], AS='gcc', ASFLAGS='-D' + str(osdef) + ' -c ')
	env.Replace(AS = 'nasm')
	env.Append(ASCOM = ' -f ' + str(osasmdef) + ' -d' + str(osdef) + ' -Itexmap/ ')
	common_sources += asm_sources + ['texmap/tmappent.o', 'texmap/tmapppro.o']
else:
	env.Append(CPPDEFINES = ['NO_ASM'])
	common_sources += noasm_sources

#editor build?
if (editor == 1):
	env.Append(CPPDEFINES = ['EDITOR'])
	env.Append(CPPPATH = ['include/editor'])
	common_sources += editor_sources

#shareware build?
if (shareware == 1):
	env.Append(CPPDEFINES = ['SHAREWARE'])
	common_sources += ['main/snddecom.c']
	if (editor == 0):
		common_sources += ['main/gamesave.c', 'main/gamemine.c']

if (shareware == 0) and (editor == 0):
	common_sources += ['main/loadrl2.c']

print '\n'

env.Append(CPPDEFINES = [('DESCENT_DATA_PATH', '\\"' + str(sharepath) + '\\"')])
# finally building program...
env.Program(target=str(target), source = common_sources, LIBS = libs, LINKFLAGS = str(lflags), LIBPATH = str(lpath))
env.Install(BIN_DIR, str(target))
env.Alias('install', BIN_DIR)

# show some help when running scons -h
Help(PROGRAM_NAME + ', SConstruct file help:' +
	"""

	Type 'scons' to build the binary.
	Type 'scons install' to build (if it hasn't been done) and install.
	Type 'scons -c' to clean up.
	
	Extra options (add them to command line, like 'scons extraoption=value'):
	
	'sharepath=DIR'   (*NIX only) use DIR for shared game data. (default: /usr/local/share/games/d1x-rebirth)
	'sdl_only=1'      don't include OpenGL, use SDL-only instead
	'sdlmixer=1'      (*NIX only) use SDL_Mixer for sound (includes external music support)
	'shareware=1'     build SHAREWARE version
	'no_asm=1'        don't use ASSEMBLER (only with sdl_only=1)
	'debug=1'         build DEBUG binary which includes asserts, debugging output, cheats and more output
	'profiler=1'      do profiler build
	'editor=1'        build editor !EXPERIMENTAL!
	'arm=1'           compile for ARM architecture
	'gp2x=1'          compile for GP2X handheld
	
	Default values:
	""" + ' sharepath = ' + DATA_DIR + """

	Some influential environment variables:
	  CC          C compiler command
	  CFLAGS      C compiler flags
	  LDFLAGS     linker flags, e.g. -L<lib dir> if you have libraries in a
                      nonstandard directory <lib dir>
                      <include dir>
	  CXX         C++ compiler command
	  CXXFLAGS    C++ compiler flags
        """)

#EOF
