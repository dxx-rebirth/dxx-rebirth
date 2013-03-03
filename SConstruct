#SConstruct

# needed imports
import sys
import os
import SCons.Util

class argumentIndirection:
	def __init__(self,prefix):
		self.prefix = prefix
		self.ARGUMENTS = ARGUMENTS
	def get(self,name,value):
		return self.ARGUMENTS.get('%s_%s' % (self.prefix, name), self.ARGUMENTS.get(name,value))

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

class DXXCommon:
	__endian = checkEndian()
	class UserSettings:
		def __init__(self,ARGUMENTS):

			# Paths for the Videocore libs/includes on the Raspberry Pi
			self.RPI_DEFAULT_VC_PATH='/opt/vc'

			self.debug = int(ARGUMENTS.get('debug', 0))
			self.profiler = int(ARGUMENTS.get('profiler', 0))
			self.opengl = int(ARGUMENTS.get('opengl', 1))
			self.asm = int(ARGUMENTS.get('asm', 0))
			self.editor = int(ARGUMENTS.get('editor', 0))
			self.extra_version = ARGUMENTS.get('extra_version', None)
			self.sdlmixer = int(ARGUMENTS.get('sdlmixer', 1))
			self.ipv6 = int(ARGUMENTS.get('ipv6', 0))
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
			default_builddir = ''
			builddir_prefix = ARGUMENTS.get('builddir_prefix', None)
			builddir_suffix = ARGUMENTS.get('builddir_suffix', None)
			if builddir_prefix is not None or builddir_suffix is not None:
				if builddir_prefix is not None:
					default_builddir = builddir_prefix
				if os.environ.has_key('CC'):
					default_builddir += '%s-' % os.path.basename(os.environ['CC'])
				for a in [
					('debug', 'dbg'),
					('profiler', 'prf'),
					('editor', 'ed'),
					('opengl', 'ogl'),
					('opengles', 'es'),
					('raspberrypi', 'rpi'),
				]:
					if getattr(self, a[0]):
						default_builddir += a[1]
				if builddir_suffix is not None:
					default_builddir += builddir_prefix
			self.builddir = str(ARGUMENTS.get('builddir', default_builddir))
			if self.builddir != '' and self.builddir[-1:] != '/':
				self.builddir += '/'
	# Base class for platform-specific settings processing
	class _PlatformSettings:
		ogllibs = ''
		osasmdef = None
		platform_sources = []
	# Settings to apply to mingw32 builds
	class Win32PlatformSettings(_PlatformSettings):
		osdef = '_WIN32'
		osasmdef = 'win32'
		def adjust_environment(self,program,env):
			env.Append(CPPDEFINES = ['_WIN32', 'HAVE_STRUCT_TIMEVAL'])
	class DarwinPlatformSettings(_PlatformSettings):
		osdef = '__APPLE__'
		def __init__(self,user_settings):
			user_settings.asm = 0
		def adjust_environment(self,program,env):
			env.Append(CPPDEFINES = ['HAVE_STRUCT_TIMESPEC', 'HAVE_STRUCT_TIMEVAL', '__unix__'])
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

	def __lazy_objects(self,name,source,transform_target):
		try:
			return self.__lazy_object_cache[name]
		except KeyError as e:
			if transform_target is None:
				transform_target = lambda _, n: os.path.splitext(n)[0]
			value = [self.env.StaticObject(target='%s%s%s' % (self.user_settings.builddir, transform_target(self, s), self.env["OBJSUFFIX"]), source=s) for s in source]
			self.__lazy_object_cache[name] = value
			return value

	@staticmethod
	def create_lazy_object_property(sources,transform_target=None):
		name = repr(sources)
		l = lambda s: s.__lazy_objects(name, sources, transform_target)
		return property(l)

	def __init__(self):
		self.__lazy_object_cache = {}

	def prepare_environment(self):
		# Acquire environment object...
		self.env = Environment(ENV = os.environ, tools = ['mingw'])
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
			if os.environ.has_key(cc):
				self.env[cc] = os.environ[cc]
		for flags in ['CFLAGS', 'CXXFLAGS']:
			if os.environ.has_key(flags):
				self.env[flags] += SCons.Util.CLVar(os.environ[flags])
		self.sources = self.objects_common[:]

	def check_endian(self):
		# set endianess
		if (self.__endian == "big"):
			print "%s: BigEndian machine detected" % self.PROGRAM_NAME
			self.asm = 0
			self.env.Append(CPPDEFINES = ['WORDS_BIGENDIAN'])
		elif (self.__endian == "little"):
			print "%s: LittleEndian machine detected" % self.PROGRAM_NAME

	def check_platform(self):
		env = self.env
		# windows or *nix?
		if sys.platform == 'win32':
			print "%s: compiling on Windows" % self.PROGRAM_NAME
			platform = self.Win32PlatformSettings
		elif sys.platform == 'darwin':
			print "%s: compiling on Mac OS X" % self.PROGRAM_NAME
			platform = self.DarwinPlatformSettings
			sys.path += ['./arch/cocoa']
		else:
			print "%s: compiling on *NIX" % self.PROGRAM_NAME
			platform = self.LinuxPlatformSettings
		self.platform_settings = platform(self.user_settings)
		self.platform_settings.adjust_environment(self, env)
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
			env.Append(ASCOM = ' -f ' + str(platform_settings.osasmdef) + ' -d' + str(platform_settings.osdef) + ' -Itexmap/ ')
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

class DXXArchive(DXXCommon):
	srcdir = 'common'
	target = 'dxx-common'
	# Use a prefix of "common" since that is the source directory
	# governed by these arguments.
	ARGUMENTS = argumentIndirection('common')
	objects_common = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
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
	def __init__(self,builddir):
		self.PROGRAM_NAME = 'DXX-Archive'
		DXXCommon.__init__(self)
		self.user_settings = self.UserSettings(ARGUMENTS)
		self.user_settings.builddir = builddir
		self.prepare_environment()
		self.check_endian()
		self.check_platform()
		self.process_user_settings()

class DXXProgram(DXXCommon):
	# version number
	VERSION_MAJOR = 0
	VERSION_MINOR = 57
	VERSION_MICRO = 3
	static_archive_construction = {}
	def _apply_target_name(self,name):
		return os.path.join(os.path.dirname(name), '.%s.%s' % (self.target, os.path.splitext(os.path.basename(name))[0]))
	objects_similar_arch_ogl = DXXCommon.create_lazy_object_property([os.path.join('similar', f) for f in [
'arch/ogl/gr.c',
'arch/ogl/ogl.c',
]
], _apply_target_name)
	objects_similar_arch_sdl = DXXCommon.create_lazy_object_property([os.path.join('similar', f) for f in [
'arch/sdl/gr.c',
]
])
	objects_similar_arch_sdlmixer = DXXCommon.create_lazy_object_property([os.path.join('similar', f) for f in [
'arch/sdl/jukebox.c'
]
], _apply_target_name)
	objects_similar_common = DXXCommon.create_lazy_object_property([os.path.join('similar', f) for f in [
'arch/sdl/event.c',
'arch/sdl/init.c',
'arch/sdl/key.c',
'arch/sdl/mouse.c',
'arch/sdl/timer.c',
'main/console.c',
'main/credits.c',
'main/digiobj.c',
'main/effects.c',
'main/gamefont.c',
'main/hud.c',
'main/kmatrix.c',
'main/mglobal.c',
'main/morph.c',
'main/newmenu.c',
'main/paging.c',
'main/player.c',
'main/scores.c',
'main/slew.c',
'main/terrain.c',
'main/texmerge.c',
'main/vclip.c',
'mem/mem.c',
'misc/hash.c',
'misc/physfsx.c',
]
], _apply_target_name)
	objects_similar_editor = DXXCommon.create_lazy_object_property([os.path.join('similar', f) for f in [
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
'editor/texture.c',
]
], _apply_target_name)
	class UserSettings(DXXCommon.UserSettings):
		def __init__(self,ARGUMENTS,target):
			DXXCommon.UserSettings.__init__(self, ARGUMENTS)
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
			env.RES('arch/win32/%s.rc' % program.target)
			env.Append(CPPPATH = [os.path.join(self.srcdir, 'arch/win32/include')])
			self.platform_sources = ['common/arch/win32/messagebox.c']
	# Settings to apply to Apple builds
	# This appears to be unused.  The reference to sdl_only fails to
	# execute.
	class DarwinPlatformSettings(DXXCommon.DarwinPlatformSettings):
		def __init__(self,user_settings):
			DXXCommon.DarwinPlatformSettings.__init__(self)
			user_settings.sharepath = ''
			if (user_settings.sdlmixer == 1):
				print "including SDL_mixer"
				platform_settings.lflags += ' -framework SDL_mixer'
			# Ugly way of linking to frameworks, but kreator has seen uglier
			self.lflags = '-framework ApplicationServices -framework Carbon -framework Cocoa -framework SDL'
			if (sdl_only == 0):
				self.lflags += ' -framework OpenGL'
			self.libs = ['../physfs/build/Debug/libphysfs.dylib']
		def adjust_environment(self,program,env):
			DXXCommon.DarwinPlatformSettings.adjust_environment(self, program, env)
			self.platform_sources = [os.path.join(program.srcdir, f) for f in ['arch/cocoa/SDLMain.m', 'arch/carbon/messagebox.c']]
	# Settings to apply to Linux builds
	class LinuxPlatformSettings(DXXCommon.LinuxPlatformSettings):
		def __init__(self,user_settings):
			DXXCommon.LinuxPlatformSettings.__init__(self,user_settings)
			user_settings.sharepath += '/'
			self.lflags = os.environ["LDFLAGS"] if os.environ.has_key('LDFLAGS') else ''
		def adjust_environment(self,program,env):
			DXXCommon.LinuxPlatformSettings.adjust_environment(self, program, env)
			self.libs = env['LIBS']
			env.Append(CPPPATH = [os.path.join(program.srcdir, 'arch/linux/include')])

	def __init__(self):
		DXXCommon.__init__(self)
		self.user_settings = self.UserSettings(self.ARGUMENTS, self.target)
		if not DXXProgram.static_archive_construction.has_key(self.user_settings.builddir):
			DXXProgram.static_archive_construction[self.user_settings.builddir] = DXXArchive(self.user_settings.builddir)
		self.prepare_environment()
		self.banner()
		self.check_endian()
		self.check_platform()
		self.process_user_settings()
		self.register_program()

	def prepare_environment(self):
		DXXCommon.prepare_environment(self)
		self.VERSION_STRING = ' v' + str(self.VERSION_MAJOR) + '.' + str(self.VERSION_MINOR) + '.' + str(self.VERSION_MICRO)
		self.env.Append(CPPDEFINES = [('PROGRAM_NAME', '\\"' + str(self.PROGRAM_NAME) + '\\"'), ('DXX_VERSION_MAJORi', str(self.VERSION_MAJOR)), ('DXX_VERSION_MINORi', str(self.VERSION_MINOR)), ('DXX_VERSION_MICROi', str(self.VERSION_MICRO))])

	def banner(self):
		print '\n===== ' + self.PROGRAM_NAME + self.VERSION_STRING + ' =====\n'

	def check_platform(self):
		DXXCommon.check_platform(self)
		env = self.env
		# windows or *nix?
		if sys.platform == 'darwin':
			VERSION = str(VERSION_MAJOR) + '.' + str(VERSION_MINOR)
			if (VERSION_MICRO):
				VERSION += '.' + str(VERSION_MICRO)
			env['VERSION_NUM'] = VERSION
			env['VERSION_NAME'] = self.PROGRAM_NAME + ' v' + VERSION
			import tool_bundle
		self.platform_settings.libs += ['physfs', 'm']

	def process_user_settings(self):
		DXXCommon.process_user_settings(self)
		env = self.env
		# opengl or software renderer?

		# SDL_mixer support?
		if (self.user_settings.sdlmixer == 1):
			if (sys.platform != 'darwin'):
				self.platform_settings.libs += ['SDL_mixer']

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
			objects.extend(self.objects_arch_sdlmixer)
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
		objects.extend(self.objects_similar_common)
		if (self.user_settings.editor == 1):
			objects.extend(self.objects_editor)
			objects.extend(self.objects_similar_editor)
			objects.extend(static_archive_construction.objects_editor)
		# finally building program...
		env.Program(target='%s%s' % (self.user_settings.builddir, str(exe_target)), source = self.sources + objects, LIBS = self.platform_settings.libs, LINKFLAGS = str(self.platform_settings.lflags))
		if (sys.platform != 'darwin'):
			env.Install(self.user_settings.BIN_DIR, str(exe_target))
			env.Alias('install', self.user_settings.BIN_DIR)
		else:
			tool_bundle.TOOL_BUNDLE(env)
			env.MakeBundle(self.PROGRAM_NAME + '.app', exe_target,
					'free.%s-rebirth' % dxxstr, '%sgl-Info.plist' % dxxstr,
					typecode='APPL', creator='DCNT',
					icon_file='arch/cocoa/%s-rebirth.icns' % dxxstr,
					subst_dict={'%sgl' % dxxstr : exe_target},	# This is required; manually update version for Xcode compatibility
					resources=[['English.lproj/InfoPlist.strings', 'English.lproj/InfoPlist.strings']])

class D1XProgram(DXXProgram):
	PROGRAM_NAME = 'D1X-Rebirth'
	target = 'd1x-rebirth'
	srcdir = 'd1x-rebirth'
	ARGUMENTS = argumentIndirection('d1x')
	def prepare_environment(self):
		DXXProgram.prepare_environment(self)
		# Flags and stuff for all platforms...
		self.env.Append(CPPPATH = [os.path.join(self.srcdir, f) for f in ['include', 'main', 'arch/include']])
		self.env.Append(CPPDEFINES = [('DXX_BUILD_DESCENT_I', 1)])

	# general source files
	objects_common = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'2d/font.c',
'2d/palette.c',
'2d/pcx.c',
'3d/interp.c',
'arch/sdl/digi.c',
'arch/sdl/digi_audio.c',
'iff/iff.c',
'main/ai.c',
'main/aipath.c',
'main/automap.c',
'main/bm.c',
'main/bmread.c',
'main/cntrlcen.c',
'main/collide.c',
'main/config.c',
'main/controls.c',
'main/custom.c',
'main/dumpmine.c',
'main/endlevel.c',
'main/fireball.c',
'main/fuelcen.c',
'main/fvi.c',
'main/game.c',
'main/gamecntl.c',
'main/gamemine.c',
'main/gamerend.c',
'main/gamesave.c',
'main/gameseg.c',
'main/gameseq.c',
'main/gauges.c',
'main/hostage.c',
'main/inferno.c',
'main/kconfig.c',
'main/laser.c',
'main/lighting.c',
'main/menu.c',
'main/mission.c',
'main/multi.c',
'main/multibot.c',
'main/newdemo.c',
'main/object.c',
'main/physics.c',
'main/piggy.c',
'main/playsave.c',
'main/polyobj.c',
'main/powerup.c',
'main/render.c',
'main/robot.c',
'main/snddecom.c',
'main/songs.c',
'main/state.c',
'main/switch.c',
'main/text.c',
'main/titles.c',
'main/wall.c',
'main/weapon.c',
'misc/args.c',
#'tracker/client/tracker_client.c'
]
])

	# for editor
	objects_editor = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'editor/ehostage.c',
'editor/texpage.c',
]
])

	objects_use_udp = DXXCommon.create_lazy_object_property([os.path.join(srcdir, 'main/net_udp.c')])

	# SDL_mixer sound implementation
	objects_arch_sdlmixer = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'arch/sdl/digi_mixer.c',
]
])

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
		versid_cppdefines=self.env['CPPDEFINES'][:]
		if self.user_settings.extra_version:
			versid_cppdefines.append(('DESCENT_VERSION_EXTRA', '\\"%s\\"' % self.user_settings.extra_version))
		versid_sources = [self.env.StaticObject(target='%s%s%s' % (self.user_settings.builddir, self._apply_target_name(s), self.env["OBJSUFFIX"]), source=os.path.join(self.srcdir, s), CPPDEFINES=versid_cppdefines) for s in ['main/vers_id.c']]
		self._register_program('d1x', versid_sources)

class D2XProgram(DXXProgram):
	PROGRAM_NAME = 'D2X-Rebirth'
	target = 'd2x-rebirth'
	srcdir = 'd2x-rebirth'
	ARGUMENTS = argumentIndirection('d2x')
	def prepare_environment(self):
		DXXProgram.prepare_environment(self)
		# Flags and stuff for all platforms...
		self.env.Append(CPPPATH = [os.path.join(self.srcdir, f) for f in ['include', 'main', 'arch/include']])
		self.env.Append(CPPDEFINES = [('DXX_BUILD_DESCENT_II', 1)])

	# general source files
	objects_common = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'2d/font.c',
'2d/palette.c',
'2d/pcx.c',
'3d/interp.c',
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
'main/controls.c',
'main/dumpmine.c',
'main/endlevel.c',
'main/escort.c',
'main/fireball.c',
'main/fuelcen.c',
'main/fvi.c',
'main/game.c',
'main/gamecntl.c',
'main/gamemine.c',
'main/gamepal.c',
'main/gamerend.c',
'main/gamesave.c',
'main/gameseg.c',
'main/gameseq.c',
'main/gauges.c',
'main/hostage.c',
'main/inferno.c',
'main/kconfig.c',
'main/laser.c',
'main/lighting.c',
'main/menu.c',
'main/mission.c',
'main/movie.c',
'main/multi.c',
'main/multibot.c',
'main/newdemo.c',
'main/object.c',
'main/physics.c',
'main/piggy.c',
'main/playsave.c',
'main/polyobj.c',
'main/powerup.c',
'main/render.c',
'main/robot.c',
'main/segment.c',
'main/songs.c',
'main/state.c',
'main/switch.c',
'main/text.c',
'main/titles.c',
'main/wall.c',
'main/weapon.c',
'misc/args.c',
'misc/physfsrwops.c',
]
])

	# for editor
	objects_editor = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'editor/texpage.c',
'main/bmread.c',
]
])

	objects_use_udp = DXXCommon.create_lazy_object_property([os.path.join(srcdir, 'main/net_udp.c')])

	# SDL_mixer sound implementation
	objects_arch_sdlmixer = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'arch/sdl/digi_mixer.c',
]
])

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
		versid_cppdefines=self.env['CPPDEFINES'][:]
		if self.user_settings.extra_version:
			versid_cppdefines.append(('DESCENT_VERSION_EXTRA', '\\"%s\\"' % self.user_settings.extra_version))
		versid_sources = [self.env.StaticObject(target='%s%s%s' % (self.user_settings.builddir, self._apply_target_name(s), self.env["OBJSUFFIX"]), source=os.path.join(self.srcdir, s), CPPDEFINES=versid_cppdefines) for s in ['main/vers_id.c']]
		self._register_program('d2x', versid_sources)

program_d1x = None
program_d2x = None
if int(ARGUMENTS.get('d1x', 1)):
	program_d1x = D1XProgram()
if int(ARGUMENTS.get('d2x', 1)):
	program_d2x = D2XProgram()

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
	(('	 d1x sharepath = ' + program_d1x.user_settings.DATA_DIR + '\n') if program_d1x else '') +
	(('	 d2x sharepath = ' + program_d2x.user_settings.DATA_DIR + '\n') if program_d2x else '') +
	(('	 d2x opengles_lib = ' + program_d2x.user_settings.default_OGLES_LIB + '\n') if program_d2x else '') +
	(('	 d2x rpi_vc_path = ' + program_d2x.user_settings.RPI_DEFAULT_VC_PATH + '\n') if program_d2x else '') +
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
