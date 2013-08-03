#SConstruct

# needed imports
import sys
import os
import SCons.Util

class argumentIndirection:
	def __init__(self,prefix):
		self.prefix = prefix
		self.ARGUMENTS = ARGUMENTS
	def get(self,name,value=None):
		return self.ARGUMENTS.get('%s_%s' % (self.prefix, name), self.ARGUMENTS.get(name,value))

# endianess-checker
def checkEndian():
    import struct
    array = struct.pack('cccc', '\x01', '\x02', '\x03', '\x04')
    i = struct.unpack('i', array)
    if i == struct.unpack('<i', array):
        return "little"
    elif i == struct.unpack('>i', array):
        return "big"
    return "unknown"

class DXXCommon:
	__endian = checkEndian()
	class UserSettings:
		def __init__(self,ARGUMENTS):
			# Paths for the Videocore libs/includes on the Raspberry Pi
			self.RPI_DEFAULT_VC_PATH='/opt/vc'
			self.debug = int(ARGUMENTS.get('debug', 0))
			self.DESTDIR = ARGUMENTS.get('DESTDIR')
			self.profiler = int(ARGUMENTS.get('profiler', 0))
			self.opengl = int(ARGUMENTS.get('opengl', 1))
			self.asm = int(ARGUMENTS.get('asm', 0))
			self.editor = int(ARGUMENTS.get('editor', 0))
			self.extra_version = ARGUMENTS.get('extra_version', None)
			self.sdlmixer = int(ARGUMENTS.get('sdlmixer', 1))
			self.register_install_target = int(ARGUMENTS.get('register_install_target', 1))
			self.ipv6 = int(ARGUMENTS.get('ipv6', 0))
			self.program_name = ARGUMENTS.get('program_name')
			self.use_udp = int(ARGUMENTS.get('use_udp', 1))
			self.use_tracker = int(ARGUMENTS.get('use_tracker', 1))
			self.verbosebuild = int(ARGUMENTS.get('verbosebuild', 0))
			self.raspberrypi = int(ARGUMENTS.get('raspberrypi', 0))
			self.rpi_vc_path = str(ARGUMENTS.get('rpi_vc_path', self.RPI_DEFAULT_VC_PATH))
			self.default_opengles = 0
			self.default_OGLES_LIB = 'GLES_CM'
			# automatic setup for raspberrypi
			if (self.raspberrypi == 1):
				self.default_opengles=1
				self.default_OGLES_LIB='GLESv2'
			self.opengles = int(ARGUMENTS.get('opengles', self.default_opengles))
			self.opengles_lib = str(ARGUMENTS.get('opengles_lib', self.default_OGLES_LIB))
			builddir_prefix = ARGUMENTS.get('builddir_prefix', None)
			builddir_suffix = ARGUMENTS.get('builddir_suffix', None)
			default_builddir = builddir_prefix or ''
			if builddir_prefix is not None or builddir_suffix is not None:
				if os.environ.has_key('CC'):
					default_builddir += '%s-' % os.path.basename(os.environ['CC'])
				for a in (
					('debug', 'dbg'),
					('profiler', 'prf'),
					('editor', 'ed'),
					('opengl', 'ogl'),
					('opengles', 'es'),
				):
					if getattr(self, a[0]):
						default_builddir += a[1]
				if builddir_suffix is not None:
					default_builddir += builddir_prefix
			self.builddir = ARGUMENTS.get('builddir', default_builddir)
			if self.builddir != '' and self.builddir[-1:] != '/':
				self.builddir += '/'
	# Base class for platform-specific settings processing
	class _PlatformSettings:
		tools = None
		ogllibs = ''
		osasmdef = None
		platform_sources = []
		platform_objects = []
	# Settings to apply to mingw32 builds
	class Win32PlatformSettings(_PlatformSettings):
		tools = ['mingw']
		osdef = '_WIN32'
		osasmdef = 'win32'
		def __init__(self,user_settings):
			pass
		def adjust_environment(self,program,env):
			env.Append(CPPDEFINES = ['_WIN32', 'HAVE_STRUCT_TIMEVAL'])
	class DarwinPlatformSettings(_PlatformSettings):
		osdef = '__APPLE__'
		def __init__(self,user_settings):
			user_settings.asm = 0
			self.lflags = os.environ.get("LDFLAGS", '')
		def adjust_environment(self,program,env):
			VERSION = str(program.VERSION_MAJOR) + '.' + str(program.VERSION_MINOR)
			if (program.VERSION_MICRO):
				VERSION += '.' + str(program.VERSION_MICRO)
			env['VERSION_NUM'] = VERSION
			env['VERSION_NAME'] = program.PROGRAM_NAME + ' v' + VERSION
			env.Append(CPPDEFINES = ['HAVE_STRUCT_TIMESPEC', 'HAVE_STRUCT_TIMEVAL', '__unix__'])
			env.Append(CPPPATH = [os.path.join(program.srcdir, '../physfs'), os.path.join(os.getenv("HOME"), 'Library/Frameworks/SDL.framework/Headers'), '/Library/Frameworks/SDL.framework/Headers'])
			env.Append(FRAMEWORKS = ['ApplicationServices', 'Carbon', 'Cocoa', 'SDL'])
			env.Append(FRAMEWORKPATH = [os.path.join(os.getenv("HOME"), 'Library/Frameworks'), '/System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks'])
			self.libs = ['']
			env['LIBPATH'] = '../physfs/build/Debug'
	# Settings to apply to Linux builds
	class LinuxPlatformSettings(_PlatformSettings):
		osdef = '__LINUX__'
		osasmdef = 'elf'
		__opengl_libs = ['GL', 'GLU']
		__pkg_config_sdl = {}
		def __init__(self,user_settings):
			if (user_settings.opengles == 1):
				self.ogllibs = [ user_settings.opengles_lib, 'EGL']
			else:
				self.ogllibs = self.__opengl_libs
		def adjust_environment(self,program,env):
			env.Append(CPPDEFINES = ['__LINUX__', 'HAVE_STRUCT_TIMESPEC', 'HAVE_STRUCT_TIMEVAL'])
			try:
				pkgconfig = os.environ['PKG_CONFIG']
			except KeyError as e:
				try:
					pkgconfig = '%s-pkg-config' % os.environ['CHOST']
				except KeyError as e:
					pkgconfig = 'pkg-config'
			cmd = '%s --cflags --libs sdl' % pkgconfig
			try:
				flags = self.__pkg_config_sdl[cmd]
			except KeyError as e:
				if (program.user_settings.verbosebuild != 0):
					print "%s: reading SDL settings from `%s`" % (program.PROGRAM_NAME, cmd)
				self.__pkg_config_sdl[cmd] = env.backtick(cmd)
				flags = self.__pkg_config_sdl[cmd]
			env.MergeFlags(flags)

	def __init__(self):
		pass

	def prepare_environment(self):
		if self.user_settings.builddir != '':
			self.env.VariantDir(self.user_settings.builddir, '.', duplicate=0)

		# Prettier build messages......
		if (self.user_settings.verbosebuild == 0):
			self.env["CCCOMSTR"]     = "Compiling $SOURCE ..."
			self.env["CXXCOMSTR"]    = "Compiling $SOURCE ..."
			self.env["LINKCOMSTR"]   = "Linking $TARGET ..."
			self.env["ARCOMSTR"]     = "Archiving $TARGET ..."
			self.env["RANLIBCOMSTR"] = "Indexing $TARGET ..."

		self.env.Append(CCFLAGS = ['-Wall', '-funsigned-char', '-Werror=implicit-int', '-Werror=implicit-function-declaration', '-pthread'])
		self.env.Append(CFLAGS = ['-std=gnu99'])
		self.env.Append(CPPDEFINES = ['NETWORK'])
		# Get traditional compiler environment variables
		for cc in ['CC', 'CXX']:
			if os.environ.has_key(cc):
				self.env[cc] = os.environ[cc]
		for flags in ['CFLAGS', 'CXXFLAGS']:
			if os.environ.has_key(flags):
				self.env[flags] += SCons.Util.CLVar(os.environ[flags])

	def check_endian(self):
		# set endianess
		if (self.__endian == "big"):
			print "%s: BigEndian machine detected" % self.PROGRAM_NAME
			self.asm = 0
			self.env.Append(CPPDEFINES = ['WORDS_BIGENDIAN'])
		elif (self.__endian == "little"):
			print "%s: LittleEndian machine detected" % self.PROGRAM_NAME

	def check_platform(self):
		# windows or *nix?
		if sys.platform == 'win32':
			print "%s: compiling on Windows" % self.PROGRAM_NAME
			platform = self.Win32PlatformSettings
		elif sys.platform == 'darwin':
			print "%s: compiling on Mac OS X" % self.PROGRAM_NAME
			platform = self.DarwinPlatformSettings
		else:
			print "%s: compiling on *NIX" % self.PROGRAM_NAME
			platform = self.LinuxPlatformSettings
		self.platform_settings = platform(self.user_settings)
		# Acquire environment object...
		self.env = Environment(ENV = os.environ, tools = platform.tools)
		self.platform_settings.adjust_environment(self, self.env)
		self.platform_settings.libs += ['physfs', 'm']
		self.common_sources += self.platform_settings.platform_sources

	def process_user_settings(self):
		env = self.env
		# opengl or software renderer?
		if (self.user_settings.opengl == 1) or (self.user_settings.opengles == 1):
			if (self.user_settings.opengles == 1):
				print "%s: building with OpenGL ES" % self.PROGRAM_NAME
				env.Append(CPPDEFINES = ['OGLES'])
			else:
				print "%s: building with OpenGL" % self.PROGRAM_NAME
			env.Append(CPPDEFINES = ['OGL'])

		# assembler code?
		if (self.user_settings.asm == 1) and (self.user_settings.opengl == 0):
			print "%s: including: ASSEMBLER" % self.PROGRAM_NAME
			env.Replace(AS = 'nasm')
			env.Append(ASCOM = ' -f ' + str(self.platform_settings.osasmdef) + ' -d' + str(self.platform_settings.osdef) + ' -Itexmap/ ')
			self.common_sources += asm_sources
		else:
			env.Append(CPPDEFINES = ['NO_ASM'])

		# SDL_mixer support?
		if (self.user_settings.sdlmixer == 1):
			print "%s: including SDL_mixer" % self.PROGRAM_NAME
			env.Append(CPPDEFINES = ['USE_SDLMIXER'])

		# debug?
		if (self.user_settings.debug == 1):
			print "%s: including: DEBUG" % self.PROGRAM_NAME
			env.Append(CPPFLAGS = ['-g'])
		else:
			env.Append(CPPDEFINES = ['NDEBUG', 'RELEASE'])
			env.Append(CPPFLAGS = ['-O2'])

		# profiler?
		if (self.user_settings.profiler == 1):
			env.Append(CPPFLAGS = ['-pg'])

		#editor build?
		if (self.user_settings.editor == 1):
			env.Append(CPPDEFINES = ['EDITOR'])
			self.common_sources += self.editor_sources

		# IPv6 compability?
		if (self.user_settings.ipv6 == 1):
			env.Append(CPPDEFINES = ['IPv6'])

		# UDP support?
		if (self.user_settings.use_udp == 1):
			env.Append(CPPDEFINES = ['USE_UDP'])
			# Tracker support?  (Relies on UDP)
			if( self.user_settings.use_tracker == 1 ):
				env.Append( CPPDEFINES = [ 'USE_TRACKER' ] )

		# Raspberry Pi?
		if (self.user_settings.raspberrypi == 1):
			print "using Raspberry Pi vendor libs in %s" % self.user_settings.rpi_vc_path
			env.Append(CPPDEFINES = ['RPI', 'WORDS_NEED_ALIGNMENT'])
			env.Append(CPPPATH = [
				self.user_settings.rpi_vc_path+'/include',
				self.user_settings.rpi_vc_path+'/include/interface/vcos/pthreads',
				self.user_settings.rpi_vc_path+'/include/interface/vmcs_host/linux'])
			self.platform_settings.lflags += ' -L' + self.user_settings.rpi_vc_path + '/lib'
			self.platform_settings.libs += ['bcm_host']

class DXXProgram(DXXCommon):
	# version number
	VERSION_MAJOR = 0
	VERSION_MINOR = 58
	VERSION_MICRO = 1
	class UserSettings(DXXCommon.UserSettings):
		def __init__(self,ARGUMENTS,target):
			DXXCommon.UserSettings.__init__(self, ARGUMENTS.ARGUMENTS)
			# installation path
			PREFIX = str(ARGUMENTS.get('prefix', '/usr/local'))
			self.BIN_DIR = PREFIX + '/bin'
			self.DATA_DIR = PREFIX + '/share/games/' + target
			# command-line parms
			self.sharepath = str(ARGUMENTS.get('sharepath', self.DATA_DIR))
	# Settings to apply to mingw32 builds
	class Win32PlatformSettings(DXXCommon.Win32PlatformSettings):
		def __init__(self,user_settings):
			DXXCommon.Win32PlatformSettings.__init__(self,user_settings)
			user_settings.sharepath = ''
			self.lflags = '-mwindows'
			self.libs = ['glu32', 'wsock32', 'ws2_32', 'winmm', 'mingw32', 'SDLmain', 'SDL']
		def adjust_environment(self,program,env):
			DXXCommon.Win32PlatformSettings.adjust_environment(self, program, env)
			env.Append(CPPPATH = [os.path.join(program.srcdir, 'arch/win32/include')])
			self.platform_sources = [os.path.join(program.srcdir, 'arch/win32/messagebox.c')]
			rcbasename = 'arch/win32/%s' % program.target
			self.platform_objects = [env.RES(target='%s%s%s' % (program.user_settings.builddir, rcbasename, env["OBJSUFFIX"]), source='%s.rc' % rcbasename)]
	# Settings to apply to Apple builds
	# This appears to be unused.  The reference to sdl_only fails to
	# execute.
	class DarwinPlatformSettings(DXXCommon.DarwinPlatformSettings):
		def __init__(self,user_settings):
			DXXCommon.DarwinPlatformSettings.__init__(self)
			user_settings.sharepath = ''
		def adjust_environment(self,program,env):
			DXXCommon.DarwinPlatformSettings.adjust_environment(self, program, env)
			self.platform_sources = [os.path.join(program.srcdir, f) for f in ['arch/cocoa/SDLMain.m', 'arch/carbon/messagebox.c']]
	# Settings to apply to Linux builds
	class LinuxPlatformSettings(DXXCommon.LinuxPlatformSettings):
		def __init__(self,user_settings):
			DXXCommon.LinuxPlatformSettings.__init__(self,user_settings)
			user_settings.sharepath += '/'
			self.lflags = os.environ.get("LDFLAGS", '')
		def adjust_environment(self,program,env):
			DXXCommon.LinuxPlatformSettings.adjust_environment(self, program, env)
			self.libs = env['LIBS']
			env.Append(CPPPATH = [os.path.join(program.srcdir, 'arch/linux/include')])

	def __init__(self):
		DXXCommon.__init__(self)
		self.user_settings = self.UserSettings(self.ARGUMENTS, self.target)
		self.check_platform()
		self.prepare_environment()
		self.banner()
		self.check_endian()
		self.process_user_settings()
		self.register_program()

	def prepare_environment(self):
		DXXCommon.prepare_environment(self)
		self.VERSION_STRING = ' v' + str(self.VERSION_MAJOR) + '.' + str(self.VERSION_MINOR) + '.' + str(self.VERSION_MICRO)
		self.env.Append(CPPDEFINES = [('DXX_VERSION_MAJORi', str(self.VERSION_MAJOR)), ('DXX_VERSION_MINORi', str(self.VERSION_MINOR)), ('DXX_VERSION_MICROi', str(self.VERSION_MICRO))])

	def banner(self):
		print '\n===== ' + self.PROGRAM_NAME + self.VERSION_STRING + ' =====\n'

	def process_user_settings(self):
		DXXCommon.process_user_settings(self)
		env = self.env
		# opengl or software renderer?
		if (self.user_settings.opengl == 1) or (self.user_settings.opengles == 1):
			self.common_sources += self.arch_ogl_sources
			if (sys.platform != 'darwin'):
				self.platform_settings.libs += self.platform_settings.ogllibs
			else:
				env.Append(FRAMEWORKS = ['OpenGL'])
		else:
			print "%s: building with Software Renderer" % self.PROGRAM_NAME
			self.common_sources += self.arch_sdl_sources

		# SDL_mixer support?
		if (self.user_settings.sdlmixer == 1):
			self.common_sources += self.arch_sdlmixer
			if (sys.platform != 'darwin'):
				self.platform_settings.libs += ['SDL_mixer']
			else:
				env.Append(FRAMEWORKS = ['SDL_mixer'])

		# profiler?
		if (self.user_settings.profiler == 1):
			self.platform_settings.lflags += ' -pg'

		#editor build?
		if (self.user_settings.editor == 1):
			env.Append(CPPPATH = [os.path.join(self.srcdir, 'include/editor')])

		# UDP support?
		if (self.user_settings.use_udp == 1):
			self.common_sources += self.sources_use_udp

		env.Append(CPPDEFINES = [('SHAREPATH', '\\"' + str(self.user_settings.sharepath) + '\\"')])

	def _register_program(self,dxxstr,program_specific_objects=[]):
		env = self.env
		exe_target = os.path.join(self.srcdir, self.target)
		objects = [self.env.StaticObject(target='%s%s%s' % (self.user_settings.builddir, os.path.splitext(s)[0], self.env["OBJSUFFIX"]), source=s) for s in self.common_sources]
		objects.extend(self.platform_settings.platform_objects)
		objects.extend(program_specific_objects)
		versid_cppdefines=env['CPPDEFINES'][:]
		if self.user_settings.program_name:
			exe_target = self.user_settings.program_name
		if self.user_settings.extra_version:
			versid_cppdefines.append(('DESCENT_VERSION_EXTRA', '\\"%s\\"' % self.user_settings.extra_version))
		objects.append(self.env.StaticObject(target='%s%s%s' % (self.user_settings.builddir, 'main/vers_id', self.env["OBJSUFFIX"]), source='main/vers_id.c', CPPDEFINES=versid_cppdefines))
		# finally building program...
		env.Program(target='%s%s' % (self.user_settings.builddir, str(exe_target)), source = objects, LIBS = self.platform_settings.libs, LINKFLAGS = str(self.platform_settings.lflags))
		if (sys.platform != 'darwin'):
			if self.user_settings.register_install_target:
				install_dir = os.path.join(self.user_settings.DESTDIR or '', self.user_settings.BIN_DIR)
				env.Install(install_dir, str(exe_target))
				env.Alias('install', install_dir)
		else:
			sys.path += ['./arch/cocoa']
			import tool_bundle
			tool_bundle.TOOL_BUNDLE(env)
			env.MakeBundle(self.PROGRAM_NAME + '.app', exe_target,
					'free.%s-rebirth' % dxxstr, '%sgl-Info.plist' % dxxstr,
					typecode='APPL', creator='DCNT',
					icon_file='arch/cocoa/%s-rebirth.icns' % dxxstr,
					subst_dict={'%sgl' % dxxstr : exe_target},	# This is required; manually update version for Xcode compatibility
					resources=[['English.lproj/InfoPlist.strings', 'English.lproj/InfoPlist.strings']])

class D2XProgram(DXXProgram):
	PROGRAM_NAME = 'D2X-Rebirth'
	target = 'd2x-rebirth'
	srcdir = ''
	ARGUMENTS = argumentIndirection('d2x')
	def prepare_environment(self):
		DXXProgram.prepare_environment(self)
		# Flags and stuff for all platforms...
		self.env.Append(CPPPATH = [os.path.join(self.srcdir, f) for f in ['include', 'main', 'arch/include']])

	def __init__(self):
	# general source files
		self.common_sources = [os.path.join(self.srcdir, f) for f in [
'2d/2dsline.c',
'2d/bitblt.c',
'2d/bitmap.c',
'2d/box.c',
'2d/canvas.c',
'2d/circle.c',
'2d/disc.c',
'2d/font.c',
'2d/gpixel.c',
'2d/line.c',
'2d/palette.c',
'2d/pcx.c',
'2d/pixel.c',
'2d/rect.c',
'2d/rle.c',
'2d/scalec.c',
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
'arch/sdl/key.c',
'arch/sdl/mouse.c',
'arch/sdl/rbaudio.c',
'arch/sdl/timer.c',
'arch/sdl/window.c',
'arch/sdl/digi.c',
'arch/sdl/digi_audio.c',
'iff/iff.c',
'libmve/decoder8.c',
'libmve/decoder16.c',
'libmve/mve_audio.c',
'libmve/mvelib.c',
'libmve/mveplay.c',
'main/ai.c',
'main/ai2.c',
'main/aipath.c',
'main/automap.c',
'main/bm.c',
'main/cntrlcen.c',
'main/collide.c',
'main/config.c',
'main/console.c',
'main/controls.c',
'main/credits.c',
'main/digiobj.c',
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
'maths/fixc.c',
'maths/rand.c',
'maths/tables.c',
'maths/vecmat.c',
'mem/mem.c',
'misc/args.c',
'misc/error.c',
'misc/hash.c',
'misc/hmp.c',
'misc/ignorecase.c',
'misc/physfsrwops.c',
'misc/physfsx.c',
'misc/strio.c',
'misc/strutil.c',
'texmap/ntmap.c',
'texmap/scanline.c'
]
]

	# for editor
		self.editor_sources = [os.path.join(self.srcdir, f) for f in [
'editor/centers.c',
'editor/curves.c',
'editor/autosave.c',
'editor/eglobal.c',
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
'main/bmread.c',
'ui/button.c',
'ui/checkbox.c',
'ui/dialog.c',
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
'ui/popup.c',
'ui/radio.c',
'ui/scroll.c',
'ui/ui.c',
'ui/uidraw.c',
'ui/userbox.c'
]
]
		DXXProgram.__init__(self)

	sources_use_udp = [os.path.join(srcdir, 'main/net_udp.c')]

	# SDL_mixer sound implementation
	arch_sdlmixer = [os.path.join(srcdir, f) for f in [
'arch/sdl/digi_mixer.c',
'arch/sdl/digi_mixer_music.c',
'arch/sdl/jukebox.c'
]
]

	# for opengl
	arch_ogl_sources = [os.path.join(srcdir, f) for f in [
'arch/ogl/gr.c',
'arch/ogl/ogl.c',
]
]

	# for non-ogl
	arch_sdl_sources = [os.path.join(srcdir, f) for f in [
'arch/sdl/gr.c',
'texmap/tmapflat.c'
]
]

	# assembler related
	asm_sources = [os.path.join(srcdir, f) for f in [
'texmap/tmap_ll.asm',
'texmap/tmap_flt.asm',
'texmap/tmapfade.asm',
'texmap/tmap_lin.asm',
'texmap/tmap_per.asm'
]
]

	def register_program(self):
		self._register_program('d2x')

program = D2XProgram()

# show some help when running scons -h
Help(program.PROGRAM_NAME + ', SConstruct file help:' +
	"""

	Type 'scons' to build the binary.
	Type 'scons install' to build (if it hasn't been done) and install.
	Type 'scons -c' to clean up.
	
	Extra options (add them to command line, like 'scons extraoption=value'):
	
	'sharepath=[DIR]'     (non-Mac OS *NIX only) use [DIR] for shared game data. [default: /usr/local/share/games/d2x-rebirth]
	'opengl=[0/1]'        build with OpenGL support [default: 1]
	'opengles=[0/1]'      build with OpenGL ES support [default: 0]
	'opengles_lib=[NAME]' specify the name of the OpenGL ES library to link against
	'sdlmixer=[0/1]'      build with SDL_Mixer support for sound and music (includes external music support) [default: 1]
	'asm=[0/1]'           build with ASSEMBLER code (only with opengl=0, requires NASM and x86) [default: 0]
	'debug=[0/1]'         build DEBUG binary which includes asserts, debugging output, cheats and more output [default: 0]
	'profiler=[0/1]'      profiler build [default: 0]
	'editor=[0/1]'        include editor into build (!EXPERIMENTAL!) [default: 0]
	'ipv6=[0/1]'          enable IPv6 compability [default: 0]
	'use_udp=[0/1]'       enable UDP support [default: 1]
	'use_tracker=[0/1]'   enable Tracker support (requires udp) [default :1]
	'verbosebuild=[0/1]'  print out all compiler/linker messages during building [default: 0]
	'raspberrypi=[0/1]'   build for Raspberry Pi (automatically sets opengles and opengles_lib) [default: 0]
	'rpi_vc_path=[DIR]'   use [DIR] to look for VideoCore libraries/header files (RPi only)

	Default values:
	""" + ' sharepath = ' + program.user_settings.DATA_DIR + """
	""" + ' opengles_lib = ' + program.user_settings.default_OGLES_LIB + """
	""" + ' rpi_vc_path = ' + program.user_settings.RPI_DEFAULT_VC_PATH + """

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
