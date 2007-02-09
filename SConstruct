#SConstruct

# needed imports
import sys
import os

PROGRAM_NAME = 'D2X-Rebirth'

# version number
D2XMAJOR = 0
D2XMINOR = 5
D2XMICRO = 1

VERSION = str(D2XMAJOR) + '.' + str(D2XMINOR) + str(D2XMICRO)

print '\n===== ' + PROGRAM_NAME + ' v' + VERSION + ' =====\n'

# installation path
PREFIX = '/usr/local/'
BIN_SUBDIR = 'games/'
DATA_SUBDIR = 'share/games/d2x-rebirth'
BIN_DIR = PREFIX + BIN_SUBDIR
DATA_DIR = PREFIX + DATA_SUBDIR

# command-line parms
sharepath = str(ARGUMENTS.get('sharepath', DATA_DIR))
debug = int(ARGUMENTS.get('debug', 0))
profiler = int(ARGUMENTS.get('profiler', 0))
sdl_only = int(ARGUMENTS.get('sdl_only', 0))
no_asm = int(ARGUMENTS.get('no_asm', 0))
editor = int(ARGUMENTS.get('editor', 0))
console = int(ARGUMENTS.get('console',0))
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
'arch/sdl/rbaudio.c',
'arch/sdl/timer.c',
'iff/iff.c',
'libmve/decoder8.c',
'libmve/decoder16.c',
'libmve/mve_audio.c',
'libmve/mvelib.c',
'libmve/mveplay.c',
'main/ai.c',
'main/ai2.c',
'main/aipath.c',
'main/digiobj.c',
'main/automap.c',
'main/bm.c',
'main/cntrlcen.c',
'main/collide.c',
'main/config.c',
'main/console.c',
'main/controls.c',
'main/credits.c',
'main/crypt.c',
'main/dumpmine.c',
'main/effects.c',
'main/endlevel.c',
'main/escort.c',
'main/fireball.c',
'main/fuelcen.c',
'main/fvi.c',
'main/game.c',
'main/gamecntl.c',
'main/gamefont.c',
'main/gamemine.c',
'main/gamepal.c',
'main/gamerend.c',
'main/gamesave.c',
'main/gameseg.c',
'main/gameseq.c',
'main/gauges.c',
'main/hostage.c',
'main/hud.c',
'main/inferno.c',
'main/kconfig.c',
'main/kludge.c',
'main/kmatrix.c',
'main/laser.c',
'main/lighting.c',
'main/menu.c',
'main/mglobal.c',
'main/mission.c',
'main/morph.c',
'main/movie.c',
'main/multi.c',
'main/multibot.c',
'main/netmisc.c',
'main/network.c',
'main/newdemo.c',
'main/newmenu.c',
'main/object.c',
'main/paging.c',
'main/physics.c',
'main/piggy.c',
'main/player.c',
'main/playsave.c',
'main/polyobj.c',
'main/powerup.c',
'main/render.c',
'main/robot.c',
'main/scores.c',
'main/segment.c',
'main/slew.c',
'main/songs.c',
'main/state.c',
'main/switch.c',
'main/terrain.c',
'main/texmerge.c',
'main/text.c',
'main/titles.c',
'main/vclip.c',
'main/wall.c',
'main/weapon.c',
'mem/mem.c',
'misc/args.c',
'misc/error.c',
'misc/hash.c',
'misc/ignorecase.c',
'misc/physfsrwops.c',
'misc/strio.c',
'misc/strutil.c',
'texmap/ntmap.c',
'texmap/scanline.c'
]

# for editor
editor_sources = [
'main/bmread.c',
'main/editor/centers.c',
'main/editor/curves.c',
'main/editor/autosave.c',
'main/editor/eglobal.c',
'main/editor/ehostage.c',
'main/editor/elight.c',
'main/editor/eobject.c',
'main/editor/eswitch.c',
'main/editor/fixseg.c',
'main/editor/func.c',
'main/editor/group.c',
'main/editor/info.c',
'main/editor/kbuild.c',
'main/editor/kcurve.c',
'main/editor/kfuncs.c',
'main/editor/kgame.c',
'main/editor/khelp.c',
'main/editor/kmine.c',
'main/editor/ksegmove.c',
'main/editor/ksegsel.c',
'main/editor/ksegsize.c',
'main/editor/ktmap.c',
'main/editor/kview.c',
'main/editor/med.c',
'main/editor/meddraw.c',
'main/editor/medmisc.c',
'main/editor/medrobot.c',
'main/editor/medsel.c',
'main/editor/medwall.c',
'main/editor/mine.c',
'main/editor/objpage.c',
'main/editor/segment.c',
'main/editor/seguvs.c',
'main/editor/texpage.c',
'main/editor/texture.c',
'ui/button.c',
'ui/checkbox.c',
'ui/file.c',
'ui/gadget.c',
'ui/icon.c',
'ui/inputbox.c',
'ui/keypad.c',
'ui/keypress.c',
'ui/keytrap.c',
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
'arch/linux/init.c',
'arch/linux/ipx_bsd.c',
'arch/linux/ipx_kali.c',
'arch/linux/ipx_mcast4.c',
'arch/linux/ipx_udp.c',
'arch/linux/linuxnet.c',
'arch/linux/mono.c',
'arch/linux/ukali.c',
]

# choosing a sound implementation for Linux
common_sound_hmp2mid = [ 'misc/hmp2mid.c' ]
arch_linux_sound_sdlmixer = [ 'arch/sdl/mixdigi.c', 'arch/sdl/mixmusic.c' ]
arch_linux_sound_old = [ 'arch/sdl/digi.c', 'arch/linux/hmiplay.c' ]

if (sdlmixer == 1):
	common_sources += common_sound_hmp2mid
	arch_linux_sources += arch_linux_sound_sdlmixer
else:
	arch_linux_sources += arch_linux_sound_old

# for windows
arch_win32_sources = [
'arch/win32/hmpfile.c',
'arch/win32/init.c',
'arch/win32/ipx_win.c',
'arch/win32/ipx_mcast4.c',
'arch/win32/ipx_udp.c',
'arch/win32/midi.c',
'arch/win32/mono.c',
'arch/win32/winnet.c',
'arch/sdl/digi.c',
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
env.Append(CPPDEFINES = [('VERSION', '\\"' + str(VERSION) + '\\"')])
env.Append(CPPDEFINES = [('USE_SDLMIXER', sdlmixer)])
env.Append(CPPDEFINES = ['NMONO', 'PIGGY_USE_PAGING', 'NETWORK', 'NATIVE_IPX', 'HAVE_NETIPX_IPX_H', 'NEWDEMO', 'SDL_INPUT', '_REENTRANT'])
env.Append(CPPPATH = ['include', 'main', 'arch/include'])
generic_libs = ['SDL', 'physfs']
sdlmixerlib = ['SDL_mixer']

# windows or *nix?
if sys.platform == 'win32':
	print "compiling on Windows"
	osdef = '_WIN32'
	osasmdef = 'win32'
	sharepath = ''
	env.Append(CPPDEFINES = ['_WIN32', 'HAVE_STRUCT_TIMEVAL'])
	env.Append(CPPPATH = ['arch/win32/include', '/msys/1.0/MinGW/include/SDL'])
	ogldefines = ['SDL_GL_VIDEO', 'OGL', 'OGL_RUNTIME_LOAD']
	common_sources += arch_win32_sources
	ogllibs = ''
	winlibs = ['glu32', 'wsock32', 'winmm', 'mingw32', 'SDLmain']
	libs = winlibs + generic_libs
	lflags = '-mwindows'
#elif sys.platform == 'darwin': #!!!TODO!!!#
else:
	print "compiling on *NIX"
	osdef = '__LINUX__'
	osasmdef = 'elf'
	sharepath += '/'
	env.Append(CPPDEFINES = ['__LINUX__', 'KALINIX', 'HAVE_STRUCT_TIMESPEC', 'HAVE_STRUCT_TIMEVAL'])
	env.Append(CPPPATH = ['arch/linux/include'])
	ogldefines = ['SDL_GL_VIDEO', 'OGL']
	common_sources += arch_linux_sources
	ogllibs = ['GL', 'GLU']
	libs = generic_libs
	lflags = '-L/usr/X11R6/lib'

# GP2X test env
if (gp2x == 1):
	sdl_only = 1
	arm = 1
	sharepath = ''
	env.Replace(CC = '/gp2xsdk/Tools/bin/arm-gp2x-linux-gcc')
	env.Replace(CXX = '/gp2xsdk/Tools/bin/arm-gp2x-linux-g++')
	env.Append(CPPDEFINES = ['GP2X'])
	env.Append(CPPPATH = ['/gp2xdev/Tools/arm-gp2x-linux/include'])
	#env.Append(CPPFLAGS = ' -ffast-math -fPIC -funroll-all-loops -fomit-frame-pointer -march=armv4t') # left for further optimisation/debugging
	common_sources += ['main/clock.c']
	libs += ['pthread']
	lflags = '-static'
	lpath = '/gp2xdev/Tools/arm-gp2x-linux/lib'
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
	target = 'd2x-rebirth-sdl'
	env.Append(CPPDEFINES = ['SDL_VIDEO'])
	env.Append(CPPPATH = ['arch/sdl/include'])
	common_sources += arch_sdl_sources
else:
	print "building with OpenGL"
	target = 'd2x-rebirth-gl'
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

#console?
if (console == 1):
	env.Append(CPPDEFINES = ['CONSOLE'])
	common_sources += ['main/cmd.c', 'console/CON_console.c']

print '\n'

env.Append(CPPDEFINES = [('SHAREPATH', '\\"' + str(sharepath) + '\\"')])
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
	
	'sharepath=DIR'	 (*NIX only) use DIR for shared game data. Must end with a slash.
	'sdl_only=1'	 don't include OpenGL, use SDL-only instead
	'sdlmixer=1'	 (*NIX only) use SDL_Mixer for sound (includes external music support)
	'no_asm=1'	 don't use ASSEMBLER (only with sdl_only=1)
	'debug=1' 	 build DEBUG binary which includes asserts, debugging output, cheats and more output
	'profiler=1' 	 do profiler build
	'console=1'	 build with console support !EXPERIMENTAL!
	'editor=1' 	 build editor !EXPERIMENTAL!
	'arm=1'		 compile for ARM architecture
	'gp2x=1'	 compile for GP2X handheld
	
	Default values:
	""" + ' sharepath = ' + DATA_DIR + '\n')

#EOF
