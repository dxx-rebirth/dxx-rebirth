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
		for p in self.prefix:
			try:
				return self.ARGUMENTS['%s_%s' % (p, name)]
			except KeyError:
				pass
		return self.ARGUMENTS.get(name,value)

# endianess-checker
def checkEndian():
    if ARGUMENTS.has_key('endian'):
        r = ARGUMENTS['endian']
        if r == "little" or r == "big":
            return r
        raise SCons.Errors.UserError("Unknown endian value: %s" % r)
    import struct
    array = struct.pack('cccc', '\x01', '\x02', '\x03', '\x04')
    i = struct.unpack('i', array)
    if i == struct.unpack('<i', array):
        return "little"
    elif i == struct.unpack('>i', array):
        return "big"
    return "unknown"

class LazyObjectConstructor:
	def __lazy_objects(self,name,source):
		try:
			return self.__lazy_object_cache[name]
		except KeyError as e:
			def __strip_extension(self,name):
				return os.path.splitext(name)[0]
			value = []
			for s in source:
				if isinstance(s, str):
					s = {'source': [s]}
				transform_target = s.get('transform_target', __strip_extension)
				for srcname in s['source']:
					t = transform_target(self, srcname)
					value.append(self.env.StaticObject(target='%s%s%s' % (self.user_settings.builddir, t, self.env["OBJSUFFIX"]), source=srcname))
			self.__lazy_object_cache[name] = value
			return value

	@staticmethod
	def create_lazy_object_getter(sources):
		name = repr(sources)
		return lambda s: s.__lazy_objects(name, sources)

	@classmethod
	def create_lazy_object_property(cls,sources):
		return property(cls.create_lazy_object_getter(sources))

	def __init__(self):
		self.__lazy_object_cache = {}

class DXXCommon(LazyObjectConstructor):
	__endian = checkEndian()
	# Settings which affect how the files are compiled
	class UserBuildSettings:
		# Paths for the Videocore libs/includes on the Raspberry Pi
		RPI_DEFAULT_VC_PATH='/opt/vc'
		default_OGLES_LIB = 'GLES_CM'
		def default_builddir(self):
			builddir_prefix = self.__arguments.get('builddir_prefix')
			builddir_suffix = self.__arguments.get('builddir_suffix')
			default_builddir = builddir_prefix or ''
			if builddir_prefix is not None or builddir_suffix is not None:
				if self.host_platform:
					default_builddir += '%s-' % self.host_platform
				if self.CC:
					default_builddir += '%s-' % os.path.basename(self.CC)
				if self.CFLAGS:
					# Mix in CRC of CFLAGS to get reasonable uniqueness
					# when flags are changed.  A full hash is
					# unnecessary here.
					import binascii
					default_builddir += '{:08x}-'.format(binascii.crc32(self.CFLAGS))
				for a in (
					('debug', 'dbg'),
					('profiler', 'prf'),
					('editor', 'ed'),
					('opengl', 'ogl'),
					('opengles', 'es'),
					('raspberrypi', 'rpi'),
				):
					if getattr(self, a[0]):
						default_builddir += a[1]
				if builddir_suffix is not None:
					default_builddir += builddir_prefix
			return default_builddir
		# automatic setup for raspberrypi
		def default_opengles(self):
			if self.raspberrypi:
				return 1
			return 0
		def selected_OGLES_LIB(self):
			if self.raspberrypi:
				return 'GLESv2'
			return self.default_OGLES_LIB
		options = (
			{
				# Process this early since it influences later checks.
				'type': int,
				'arguments': (
					('raspberrypi', 0),
				),
			},
			{
				'type': int,
				'arguments': (
					('debug', 0),
					('profiler', 0),
					('opengl', 1),
					('asm', 0),
					('editor', 0),
					('sdlmixer', 1),
					('ipv6', 0),
					('use_udp', 1),
					('use_tracker', 1),
					('verbosebuild', 0),
					('opengles', default_opengles),
				),
			},
			{
				'type': str,
				'arguments': (
					('CC', os.environ.get('CC')),
					('CXX', os.environ.get('CXX')),
					('CFLAGS', os.environ.get('CFLAGS')),
					('CXXFLAGS', os.environ.get('CXXFLAGS')),
					('rpi_vc_path', RPI_DEFAULT_VC_PATH),
					('opengles_lib', selected_OGLES_LIB),
				),
			},
			{
				'type': None,
				'arguments': (
					('extra_version',),
					('host_platform',),
					# This must be last so that default_builddir will
					# have access to other properties.
					('builddir', default_builddir),
				),
			},
		)
		def __init__(self,ARGUMENTS):
			self.__arguments = ARGUMENTS
			if ARGUMENTS is None:
				return
			for grp in self.options:
				wanted_type = grp.get('type')
				for o in grp['arguments']:
					name = o[0]
					value = ARGUMENTS.get(*o)
					if callable(value):
						value = value(self)
					if wanted_type is not None and value is not None:
						value = wanted_type(value)
					setattr(self, name, value)
			if self.builddir != '' and self.builddir[-1:] != '/':
				self.builddir += '/'
		def clone(self):
			clone = DXXCommon.UserBuildSettings(None)
			for grp in self.options:
				for o in grp['arguments']:
					name = o[0]
					value = getattr(self, name)
					setattr(clone, name, value)
			return clone
	class UserInstallSettings:
		def __init__(self,ARGUMENTS):
			self.DESTDIR = ARGUMENTS.get('DESTDIR')
			self.register_install_target = int(ARGUMENTS.get('register_install_target', 1))
			self.program_name = ARGUMENTS.get('program_name')
	class UserSettings(UserBuildSettings,UserInstallSettings):
		def __init__(self,ARGUMENTS):
			DXXCommon.UserBuildSettings.__init__(self, ARGUMENTS)
			DXXCommon.UserInstallSettings.__init__(self, ARGUMENTS)
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
			self.user_settings = user_settings
			user_settings.asm = 0
			self.lflags = os.environ.get("LDFLAGS", '')
		def adjust_environment(self,program,env):
			env.Append(CPPDEFINES = ['HAVE_STRUCT_TIMESPEC', 'HAVE_STRUCT_TIMEVAL', '__unix__'])
			env.Append(CPPPATH = [os.path.join(program.srcdir, '../physfs'), os.path.join(os.getenv("HOME"), 'Library/Frameworks/SDL.framework/Headers'), '/Library/Frameworks/SDL.framework/Headers'])
			env.Append(FRAMEWORKS = ['ApplicationServices', 'Carbon', 'Cocoa', 'SDL'])
			if (self.user_settings.opengl == 1) or (self.user_settings.opengles == 1):
				env.Append(FRAMEWORKS = ['OpenGL'])
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
		LazyObjectConstructor.__init__(self)
		self.sources = []

	def prepare_environment(self):
		if self.user_settings.builddir != '':
			self.env.VariantDir(self.user_settings.builddir, '.', duplicate=0)

		# Prettier build messages......
		if (self.user_settings.verbosebuild == 0):
			builddir = self.user_settings.builddir if self.user_settings.builddir != '' else '.'
			self.env["CCCOMSTR"]     = "Compiling %s %s $SOURCE" % (self.target, builddir)
			self.env["CXXCOMSTR"]    = "Compiling %s %s $SOURCE" % (self.target, builddir)
			self.env["LINKCOMSTR"]   = "Linking %s $TARGET" % self.target
			self.env["ARCOMSTR"]     = "Archiving $TARGET ..."
			self.env["RANLIBCOMSTR"] = "Indexing $TARGET ..."

		# Use -Wundef to catch when a shared source file includes a
		# shared header that misuses conditional compilation.  Use
		# -Werror=undef to make this fatal.  Both are needed, since
		# gcc 4.5 silently ignores -Werror=undef.  On gcc 4.5, misuse
		# produces a warning.  On gcc 4.7, misuse produces an error.
		self.env.Append(CCFLAGS = ['-Wall', '-Wundef', '-Werror=undef', '-funsigned-char', '-Werror=implicit-int', '-Werror=implicit-function-declaration', '-pthread'])
		self.env.Append(CFLAGS = ['-std=gnu99'])
		self.env.Append(CPPDEFINES = ['NETWORK'])
		self.env.Append(CPPPATH = ['common/include', 'common/main', '.'])
		if (self.user_settings.editor == 1):
			self.env.Append(CPPPATH = ['common/include/editor'])
		# Get traditional compiler environment variables
		for cc in ['CC', 'CXX']:
			value = getattr(self.user_settings, cc)
			if value is not None:
				self.env[cc] = value
		for flags in ['CFLAGS', 'CXXFLAGS']:
			value = getattr(self.user_settings, flags)
			if value is not None:
				self.env[flags] += SCons.Util.CLVar(value)
		self.sources += self.objects_common[:]

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
		platform_name = self.user_settings.host_platform or sys.platform
		print "%s: compiling on %s for %s" % (self.PROGRAM_NAME, sys.platform, platform_name)
		if platform_name == 'win32':
			platform = self.Win32PlatformSettings
		elif platform_name == 'darwin':
			platform = self.DarwinPlatformSettings
		else:
			platform = self.LinuxPlatformSettings
		self.platform_settings = platform(self.user_settings)
		# Acquire environment object...
		self.env = Environment(ENV = os.environ, tools = platform.tools)
		self.platform_settings.adjust_environment(self, self.env)
		self.sources += self.platform_settings.platform_sources

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
			self.sources += asm_sources
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

class DXXArchive(DXXCommon):
	srcdir = 'common'
	target = 'dxx-common'
	# Use a prefix of "common" since that is the source directory
	# governed by these arguments.
	ARGUMENTS = argumentIndirection(['common'])
	__objects_common = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'2d/2dsline.c',
'2d/bitblt.c',
'2d/bitmap.c',
'2d/box.c',
'2d/canvas.c',
'2d/circle.c',
'2d/disc.c',
'2d/gpixel.c',
'2d/line.c',
'2d/pixel.c',
'2d/poly.c',
'2d/rect.c',
'2d/rle.c',
'2d/scalec.c',
'3d/clipper.c',
'3d/draw.c',
'3d/globvars.c',
'3d/instance.c',
'3d/matrix.c',
'3d/points.c',
'3d/rod.c',
'3d/setup.c',
'arch/sdl/joy.c',
'arch/sdl/rbaudio.c',
'arch/sdl/window.c',
'maths/fixc.c',
'maths/rand.c',
'maths/tables.c',
'maths/vecmat.c',
'misc/dl_list.c',
'misc/error.c',
'misc/hmp.c',
'misc/ignorecase.c',
'misc/strio.c',
'misc/strutil.c',
'texmap/ntmap.c',
'texmap/scanline.c'
]
])
	objects_editor = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'editor/func.c',
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
])
	# for non-ogl
	objects_arch_sdl = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'texmap/tmapflat.c'
]
])
	objects_arch_sdlmixer = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'arch/sdl/digi_mixer_music.c',
]
])
	class Win32PlatformSettings(LazyObjectConstructor, DXXCommon.Win32PlatformSettings):
		platform_objects = LazyObjectConstructor.create_lazy_object_property([
'common/arch/win32/messagebox.c'
])
		def __init__(self,user_settings):
			LazyObjectConstructor.__init__(self)
			DXXCommon.Win32PlatformSettings.__init__(self, user_settings)
			self.user_settings = user_settings
		def adjust_environment(self,program,env):
			DXXCommon.Win32PlatformSettings.adjust_environment(self, program, env)
			self.env = env
	@property
	def objects_common(self):
		objects_common = self.__objects_common
		return objects_common + self.platform_settings.platform_objects
	def __init__(self,user_settings):
		self.PROGRAM_NAME = 'DXX-Archive'
		DXXCommon.__init__(self)
		self.user_settings = user_settings.clone()
		self.check_platform()
		self.prepare_environment()
		self.check_endian()
		self.process_user_settings()

class DXXProgram(DXXCommon):
	# version number
	VERSION_MAJOR = 0
	VERSION_MINOR = 57
	VERSION_MICRO = 3
	static_archive_construction = {}
	def _apply_target_name(self,name):
		return os.path.join(os.path.dirname(name), '.%s.%s' % (self.target, os.path.splitext(os.path.basename(name))[0]))
	objects_similar_arch_ogl = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join('similar', f) for f in [
'arch/ogl/gr.c',
'arch/ogl/ogl.c',
]
],
		'transform_target':_apply_target_name,
	}])
	objects_similar_arch_sdl = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join('similar', f) for f in [
'arch/sdl/gr.c',
]
],
		'transform_target':_apply_target_name,
	}])
	objects_similar_arch_sdlmixer = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join('similar', f) for f in [
'arch/sdl/digi_mixer.c',
'arch/sdl/jukebox.c'
]
],
		'transform_target':_apply_target_name,
	}])
	objects_common = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join('similar', f) for f in [
'2d/font.c',
'2d/palette.c',
'2d/pcx.c',
'3d/interp.c',
'arch/sdl/digi.c',
'arch/sdl/digi_audio.c',
'arch/sdl/event.c',
'arch/sdl/init.c',
'arch/sdl/key.c',
'arch/sdl/mouse.c',
'arch/sdl/timer.c',
'main/cntrlcen.c',
'main/config.c',
'main/console.c',
'main/controls.c',
'main/credits.c',
'main/digiobj.c',
'main/effects.c',
'main/game.c',
'main/gamecntl.c',
'main/gamefont.c',
'main/gamerend.c',
'main/gameseg.c',
'main/hostage.c',
'main/hud.c',
'main/inferno.c',
'main/kmatrix.c',
'main/lighting.c',
'main/mglobal.c',
'main/morph.c',
'main/multibot.c',
'main/newmenu.c',
'main/paging.c',
'main/physics.c',
'main/player.c',
'main/robot.c',
'main/scores.c',
'main/slew.c',
'main/state.c',
'main/terrain.c',
'main/texmerge.c',
'main/text.c',
'main/vclip.c',
'main/wall.c',
'mem/mem.c',
'misc/args.c',
'misc/hash.c',
'misc/physfsx.c',
]
],
		'transform_target':_apply_target_name,
	}])
	objects_editor = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join('similar', f) for f in [
'editor/autosave.c',
'editor/centers.c',
'editor/curves.c',
'editor/eglobal.c',
'editor/elight.c',
'editor/eobject.c',
'editor/eswitch.c',
'editor/fixseg.c',
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
]
],
		'transform_target':_apply_target_name,
	}])
	class UserSettings(DXXCommon.UserSettings):
		default_prefix = '/usr/local'
		BIN_DIR = default_prefix + '/bin'
		def __init__(self,ARGUMENTS,target):
			DXXCommon.UserSettings.__init__(self, ARGUMENTS)
			# installation path
			PREFIX = ARGUMENTS.get('prefix', self.default_prefix)
			self.BIN_DIR = PREFIX + '/bin'
			# command-line parms
			self.sharepath = str(ARGUMENTS.get('sharepath', self.__get_DATA_DIR(PREFIX, target)))
		@staticmethod
		def __get_DATA_DIR(prefix,target):
			return prefix + '/share/games/' + target
		@classmethod
		def get_DATA_DIR(cls,target):
			return cls.__get_DATA_DIR(cls.default_prefix, target)
	# Settings to apply to mingw32 builds
	class Win32PlatformSettings(DXXCommon.Win32PlatformSettings):
		def __init__(self,user_settings):
			DXXCommon.Win32PlatformSettings.__init__(self,user_settings)
			user_settings.sharepath = ''
			self.lflags = '-mwindows'
			self.libs = ['glu32', 'wsock32', 'ws2_32', 'winmm', 'mingw32', 'SDLmain', 'SDL']
		def adjust_environment(self,program,env):
			DXXCommon.Win32PlatformSettings.adjust_environment(self, program, env)
			env.RES(os.path.join(program.srcdir, 'arch/win32/%s.rc' % program.target))
			env.Append(CPPPATH = [os.path.join(program.srcdir, 'arch/win32/include')])
	# Settings to apply to Apple builds
	class DarwinPlatformSettings(DXXCommon.DarwinPlatformSettings):
		def __init__(self,user_settings):
			DXXCommon.DarwinPlatformSettings.__init__(self,user_settings)
			user_settings.sharepath = ''
			# Ugly way of linking to frameworks, but kreator has seen uglier
			self.lflags = '-framework ApplicationServices -framework Carbon -framework Cocoa -framework SDL'
			if (user_settings.sdlmixer == 1):
				self.lflags += ' -framework SDL_mixer'
			self.libs = ['../physfs/build/Debug/libphysfs.dylib']
		def adjust_environment(self,program,env):
			DXXCommon.DarwinPlatformSettings.adjust_environment(self, program, env)
			VERSION = str(program.VERSION_MAJOR) + '.' + str(program.VERSION_MINOR)
			if (program.VERSION_MICRO):
				VERSION += '.' + str(program.VERSION_MICRO)
			env['VERSION_NUM'] = VERSION
			env['VERSION_NAME'] = program.PROGRAM_NAME + ' v' + VERSION
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
		self.banner()
		self.user_settings = self.UserSettings(self.ARGUMENTS, self.target)
		if not DXXProgram.static_archive_construction.has_key(self.user_settings.builddir):
			DXXProgram.static_archive_construction[self.user_settings.builddir] = DXXArchive(self.user_settings)
		self.check_platform()
		self.prepare_environment()
		self.check_endian()
		self.process_user_settings()
		self.register_program()

	def prepare_environment(self):
		DXXCommon.prepare_environment(self)
		self.env.Append(CPPDEFINES = [('PROGRAM_NAME', '\\"' + str(self.PROGRAM_NAME) + '\\"'), ('DXX_VERSION_MAJORi', str(self.VERSION_MAJOR)), ('DXX_VERSION_MINORi', str(self.VERSION_MINOR)), ('DXX_VERSION_MICROi', str(self.VERSION_MICRO))])

	def banner(self):
		VERSION_STRING = ' v' + str(self.VERSION_MAJOR) + '.' + str(self.VERSION_MINOR) + '.' + str(self.VERSION_MICRO)
		print '\n===== ' + self.PROGRAM_NAME + VERSION_STRING + ' =====\n'

	def check_platform(self):
		DXXCommon.check_platform(self)
		env = self.env
		# windows or *nix?
		if sys.platform == 'darwin':
			VERSION = str(self.VERSION_MAJOR) + '.' + str(self.VERSION_MINOR)
			if (self.VERSION_MICRO):
				VERSION += '.' + str(self.VERSION_MICRO)
			env['VERSION_NUM'] = VERSION
			env['VERSION_NAME'] = self.PROGRAM_NAME + ' v' + VERSION
		self.platform_settings.libs += ['physfs', 'm']

	def process_user_settings(self):
		DXXCommon.process_user_settings(self)
		env = self.env
		# opengl or software renderer?

		# SDL_mixer support?
		if (self.user_settings.sdlmixer == 1):
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

		env.Append(CPPDEFINES = [('SHAREPATH', '\\"' + str(self.user_settings.sharepath) + '\\"')])

	def _register_program(self,dxxstr,program_specific_objects=[]):
		env = self.env
		exe_target = os.path.join(self.srcdir, self.target)
		static_archive_construction = self.static_archive_construction[self.user_settings.builddir]
		objects = static_archive_construction.objects_common[:]
		objects.extend(program_specific_objects)
		if (self.user_settings.sdlmixer == 1):
			objects.extend(static_archive_construction.objects_arch_sdlmixer)
			objects.extend(self.objects_similar_arch_sdlmixer)
		if (self.user_settings.opengl == 1) or (self.user_settings.opengles == 1):
			self.platform_settings.libs += self.platform_settings.ogllibs
			objects.extend(self.objects_similar_arch_ogl)
		else:
			print "%s: building with Software Renderer" % self.PROGRAM_NAME
			objects.extend(static_archive_construction.objects_arch_sdl)
			objects.extend(self.objects_similar_arch_sdl)
		if (self.user_settings.use_udp == 1):
			objects.extend(self.objects_use_udp)
		if (self.user_settings.editor == 1):
			objects.extend(self.objects_editor)
			objects.extend(static_archive_construction.objects_editor)
			exe_target += '-editor'
		if self.user_settings.program_name:
			exe_target = self.user_settings.program_name
		versid_cppdefines=self.env['CPPDEFINES'][:]
		if self.user_settings.extra_version:
			versid_cppdefines.append(('DESCENT_VERSION_EXTRA', '\\"%s\\"' % self.user_settings.extra_version))
		objects.extend([self.env.StaticObject(target='%s%s%s' % (self.user_settings.builddir, self._apply_target_name(s), self.env["OBJSUFFIX"]), source=s, CPPDEFINES=versid_cppdefines) for s in ['similar/main/vers_id.c']])
		# finally building program...
		env.Program(target='%s%s' % (self.user_settings.builddir, str(exe_target)), source = self.sources + objects, LIBS = self.platform_settings.libs, LINKFLAGS = str(self.platform_settings.lflags))
		if (sys.platform != 'darwin'):
			if self.user_settings.register_install_target:
				install_dir = os.path.join(self.user_settings.DESTDIR, self.user_settings.BIN_DIR)
				env.Install(install_dir, str(exe_target))
				env.Alias('install', install_dir)
		else:
			syspath = sys.path[:]
			cocoa = os.path.join(self.srcdir, 'arch/cocoa')
			sys.path += [cocoa]
			import tool_bundle
			sys.path = syspath
			tool_bundle.TOOL_BUNDLE(env)
			env.MakeBundle(self.PROGRAM_NAME + '.app', exe_target,
					'free.%s-rebirth' % dxxstr, os.path.join(self.srcdir, '%sgl-Info.plist' % dxxstr),
					typecode='APPL', creator='DCNT',
					icon_file=os.path.join(cocoa, '%s-rebirth.icns' % dxxstr),
					subst_dict={'%sgl' % dxxstr : exe_target},	# This is required; manually update version for Xcode compatibility
					resources=[[s, s] for s in [os.path.join(self.srcdir, 'English.lproj/InfoPlist.strings')]])

class D1XProgram(DXXProgram):
	PROGRAM_NAME = 'D1X-Rebirth'
	target = 'd1x-rebirth'
	srcdir = 'd1x-rebirth'
	def __init__(self,prefix):
		self.ARGUMENTS = argumentIndirection(prefix)
		DXXProgram.__init__(self)
	def prepare_environment(self):
		DXXProgram.prepare_environment(self)
		# Flags and stuff for all platforms...
		self.env.Append(CPPPATH = [os.path.join(self.srcdir, f) for f in ['include', 'main', 'arch/include']])
		self.env.Append(CPPDEFINES = [('DXX_BUILD_DESCENT_I', 1)])

	# general source files
	__objects_common = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join(srcdir, f) for f in [
'iff/iff.c',
'main/ai.c',
'main/aipath.c',
'main/automap.c',
'main/bm.c',
'main/bmread.c',
'main/collide.c',
'main/custom.c',
'main/dumpmine.c',
'main/endlevel.c',
'main/fireball.c',
'main/fuelcen.c',
'main/fvi.c',
'main/gamemine.c',
'main/gamesave.c',
'main/gameseq.c',
'main/gauges.c',
'main/hostage.c',
'main/kconfig.c',
'main/laser.c',
'main/menu.c',
'main/mission.c',
'main/multi.c',
'main/newdemo.c',
'main/object.c',
'main/piggy.c',
'main/playsave.c',
'main/polyobj.c',
'main/powerup.c',
'main/render.c',
'main/snddecom.c',
'main/songs.c',
'main/switch.c',
'main/titles.c',
'main/weapon.c',
#'tracker/client/tracker_client.c'
]
],
	}])
	@property
	def objects_common(self):
		return self.__objects_common + DXXProgram.objects_common.fget(self)

	# for editor
	__objects_editor = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join(srcdir, f) for f in [
'editor/ehostage.c',
]
],
	}])
	@property
	def objects_editor(self):
		return self.__objects_editor + DXXProgram.objects_editor.fget(self)

	objects_use_udp = DXXCommon.create_lazy_object_property([os.path.join(srcdir, 'main/net_udp.c')])

	# assembler related
	objects_asm = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'texmap/tmap_ll.asm',
'texmap/tmap_flt.asm',
'texmap/tmapfade.asm',
'texmap/tmap_lin.asm',
'texmap/tmap_per.asm'
]
])
	def register_program(self):
		self._register_program('d1x')

class D2XProgram(DXXProgram):
	PROGRAM_NAME = 'D2X-Rebirth'
	target = 'd2x-rebirth'
	srcdir = 'd2x-rebirth'
	def __init__(self,prefix):
		self.ARGUMENTS = argumentIndirection(prefix)
		DXXProgram.__init__(self)
	def prepare_environment(self):
		DXXProgram.prepare_environment(self)
		# Flags and stuff for all platforms...
		self.env.Append(CPPPATH = [os.path.join(self.srcdir, f) for f in ['include', 'main', 'arch/include']])
		self.env.Append(CPPDEFINES = [('DXX_BUILD_DESCENT_II', 1)])

	# general source files
	__objects_common = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join(srcdir, f) for f in [
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
'main/collide.c',
'main/dumpmine.c',
'main/endlevel.c',
'main/escort.c',
'main/fireball.c',
'main/fuelcen.c',
'main/fvi.c',
'main/gamemine.c',
'main/gamepal.c',
'main/gamesave.c',
'main/gameseq.c',
'main/gauges.c',
'main/kconfig.c',
'main/laser.c',
'main/menu.c',
'main/mission.c',
'main/movie.c',
'main/multi.c',
'main/newdemo.c',
'main/object.c',
'main/piggy.c',
'main/playsave.c',
'main/polyobj.c',
'main/powerup.c',
'main/render.c',
'main/segment.c',
'main/songs.c',
'main/switch.c',
'main/titles.c',
'main/weapon.c',
'misc/physfsrwops.c',
]
],
	}])
	@property
	def objects_common(self):
		return self.__objects_common + DXXProgram.objects_common.fget(self)

	# for editor
	__objects_editor = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join(srcdir, f) for f in [
'main/bmread.c',
]
],
	}])
	@property
	def objects_editor(self):
		return self.__objects_editor + DXXProgram.objects_editor.fget(self)

	objects_use_udp = DXXCommon.create_lazy_object_property([os.path.join(srcdir, 'main/net_udp.c')])

	# assembler related
	objects_asm = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'texmap/tmap_ll.asm',
'texmap/tmap_flt.asm',
'texmap/tmapfade.asm',
'texmap/tmap_lin.asm',
'texmap/tmap_per.asm'
]
])

	def register_program(self):
		self._register_program('d2x')

def register_program(s,program):
	l = [v for (k,v) in ARGLIST if k == s] or [1]
	# Fallback case: build the regular configuration.
	if len(l) == 1:
		try:
			if int(l[0]):
				program([s])
			return
		except ValueError:
			# If not an integer, treat this as a configuration profile.
			pass
	for e in l:
		program(e.split(','))
register_program('d1x', D1XProgram)
register_program('d2x', D2XProgram)

# show some help when running scons -h
Help('DXX-Rebirth, SConstruct file help:' +
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
""" +
	'	 d1x sharepath = ' + D1XProgram.UserSettings.get_DATA_DIR(D1XProgram.target) + '\n' +
	'	 d2x sharepath = ' + D2XProgram.UserSettings.get_DATA_DIR(D2XProgram.target) + '\n' +
	'	 opengles_lib = ' + DXXProgram.UserSettings.default_OGLES_LIB + '\n' +
	'	 rpi_vc_path = ' + DXXProgram.UserSettings.RPI_DEFAULT_VC_PATH + '\n' +
"""
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
