#SConstruct

# TODO:
# - command-line parameter for datapath


print "D1X-Rebirth"

# needed imports
import sys
import os

# version number
D1XMAJOR = 0
D1XMINOR = 45

# place of descent data - please change if desired
D1XDATAPATH = '\\"\\"'

# command-line parms
debug = int(ARGUMENTS.get('debug', 0))
no_release = int(ARGUMENTS.get('no_release', 0))
sdl_only = int(ARGUMENTS.get('sdl_only', 0))
no_asm = int(ARGUMENTS.get('no_asm', 0))
editor = int(ARGUMENTS.get('editor', 0))

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
'cfile/cfile.c',
'iff/iff.c',
'main/ai.c',
'main/aipath.c',
'main/altsound.c',
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
'main/key.c',
'main/kmatrix.c',
'main/laser.c',
'main/lighting.c',
'main/m_inspak.c',
'main/menu.c',
'main/mglobal.c',
'main/mission.c',
'main/mlticntl.c',
'main/modem.c',
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
'misc/d_delay.c',
'misc/d_io.c',
'misc/d_slash.c',
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
'arch/linux/hmiplay.c',
'arch/linux/init.c',
'arch/linux/ipx_bsd.c',
'arch/linux/ipx_kali.c',
'arch/linux/linuxnet.c',
'arch/linux/mono.c',
'arch/linux/serial.c',
'arch/linux/timer.c',
'arch/linux/ukali.c',
'arch/sdl/clipboard.c',
'arch/sdl/digi.c',
'arch/sdl/event.c',
'arch/sdl/init.c',
'arch/sdl/joy.c',
'arch/sdl/joydefs.c',
'arch/sdl/key_arch.c',
'arch/sdl/mouse.c'
]

# for windows
arch_win32_sources = [
'arch/win32/arch_ip.cpp',
'arch/win32/hmpfile.c',
'arch/win32/init.c',
'arch/win32/ipx_win.c',
'arch/win32/midi.c',
'arch/win32/mono.c',
'arch/win32/serial.c',
'arch/win32/timer.c',
'arch/win32/winnet.c',
'arch/sdl/digi.c',
'arch/sdl/event.c',
'arch/sdl/init.c',
'arch/sdl/joy.c',
'arch/sdl/joydefs.c',
'arch/sdl/key_arch.c',
'arch/sdl/mouse.c'
]

# for opengl
arch_ogl_sources = [
'arch/ogl/gr.c',
'arch/ogl/ogl.c',
'arch/ogl/sdlgl.c',
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
env.Append(CPPFLAGS = '-O2 -Wall -funsigned-char ')
env.Append(CPPDEFINES = [('DESCENT_DATA_PATH', D1XDATAPATH)])
env.Append(CPPDEFINES = [('D1XMAJOR', '\\"' + str(D1XMAJOR) + '\\"'), ('D1XMINOR', '\\"' + str(D1XMINOR) + '\\"')])
env.Append(CPPPATH = ['include', 'main', 'arch/sdl/include'])
env.Append(CPPDEFINES = ['NMONO', 'NETWORK', 'HAVE_NETIPX_IPX_H', 'SUPPORTS_NET_IP', '__SDL__', 'SDL_INPUT', 'SDL_AUDIO', '_REENTRANT'])
sdllibs = ['SDL']

# windows or *nix?
if sys.platform == 'win32':
	print "compiling on Windows"
	osdef = '__WINDOWS__'
	env.Append(CPPDEFINES = ['__WINDOWS__'])
	env.Append(CPPPATH = ['arch/win32/include'])
	ogldefines = ['SDL_GL', 'OGL_RUNTIME_LOAD', 'OGL']
	common_sources = arch_win32_sources + common_sources
	ogllibs = ['']
	winlibs = ['glu32', 'wsock32', 'winmm']
	alllibs = winlibs + sdllibs
	lflags = '-mwindows'
else:
	print "compiling on *NIX"
	osdef = '__LINUX__'
	env.Append(CPPDEFINES = ['__LINUX__', 'WANT_AWE32'])
	env.Append(CPPPATH = ['arch/linux/include'])
	ogldefines = ['SDL_GL', 'OGL']
	common_sources = arch_linux_sources + common_sources
	ogllibs = ['GL', 'GLU']
	alllibs = sdllibs
	lflags = ' '

# sdl or opengl?
if (sdl_only == 1):
	print "building with SDL"
	target = 'd1x-rebirth-sdl'
	env.Append(CPPDEFINES = ['SDL_VIDEO'])
	env.Append(CPPPATH = ['arch/sdl/include'])
	common_sources = arch_sdl_sources + common_sources
else:
	print "building with OpenGL"
	target = 'd1x-rebirth-gl'
	env.Append(CPPDEFINES = ogldefines)
	env.Append(CPPPATH = ['arch/ogl/include'])
	common_sources = arch_ogl_sources + common_sources
	alllibs = ogllibs + sdllibs + alllibs

# debug?
if (debug == 1):
	print "including: DEBUG"
else:
	env.Append(CPPDEFINES = ['NDEBUG'])

# release?
if (no_release == 0):
	print "including: RELEASE"
	env.Append(CPPDEFINES = ['RELEASE'])

# assembler code?
if (no_asm == 0) and (sdl_only == 1):
	print "including: ASSEMBLER"
	env.Append(CPPDEFINES = ['ASM_VECMAT'])
	Object(['texmap/tmappent.S', 'texmap/tmapppro.S'], AS='gcc', ASFLAGS='-D' + str(osdef) + ' -c ')
	env.Replace(AS = 'nasm')
	env.Append(ASCOM = ' -f elf -d' + str(osdef) + ' -Itexmap/ ')
	common_sources = asm_sources + common_sources + ['texmap/tmappent.c', 'texmap/tmapppro.c']
else:
	env.Append(CPPDEFINES = ['NO_ASM'])
	common_sources = noasm_sources + common_sources

#editor build?
if (editor == 1):
	env.Append(CPPDEFINES = ['EDITOR'])
	env.Append(CPPPATH = ['include/editor'])
	common_sources = editor_sources + common_sources
	target = 'miner'
else:
	common_sources = ['main/loadrl2.c'] + common_sources

# finally building program...
env.Program(target=str(target), source = common_sources, LIBS = alllibs, LINKFLAGS = str(lflags))

# show some help when running scons -h
Help("""
	D1X-Rebirth, SConstruct file help:
	Type 'scons' to build the binary.
	Type 'scons -c' to clean up.
	Extra options (add them to command line, like 'scons extraoption=value'):
	'debug=1' build DEBUG binary which includes asserts, debugging output, cheats and more output
	'no_release=1' don't build RELEASE binary - gives debug/editor things
	'sdl_only=1' don't include OpenGL, use SDL-only instead
	'no_asm=1' don't use ASSEMBLER (only with sdl_only=1 - NOT recommended, slow)
	'editor=1' build editor !EXPERIMENTAL!
	""")

#EOF