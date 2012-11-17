#SConstruct

# needed imports
import sys
import os
import SCons.Util

def message(program,msg):
	print "%s: %s" % (program.program_message_prefix, msg)

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

class ConfigureTests:
	class Collector:
		def __init__(self):
			self.tests = []
		def __call__(self,f):
			self.tests.append(f.__name__)
			return f
	_implicit_test = Collector()
	_custom_test = Collector()
	implicit_tests = _implicit_test.tests
	custom_tests = _custom_test.tests
	comment_not_supported = '/* not supported */'
	__flags_Werror = {k:['-Werror'] for k in ['CFLAGS', 'CXXFLAGS']}
	__empty_main_program = 'int main(){return 0;}'
	def __init__(self,msgprefix,user_settings):
		self.msgprefix = msgprefix
		self.user_settings = user_settings
		self.__repeated_tests = {}
		self.__automatic_compiler_tests = {
			'.c': self.check_cc_works,
		}
	@classmethod
	def describe(cls,name):
		f = getattr(cls, name)
		if f.__doc__:
			lines = f.__doc__.rstrip().split('\n')
			if lines[-1].startswith("help:"):
				return lines[-1][5:]
		return None
	def _may_repeat(f):
		def wrap(self,*args,**kwargs):
			try:
				return self.__repeated_tests[f.__name__]
			except KeyError as e:
				pass
			r = f(self,*args,**kwargs)
			self.__repeated_tests[f.__name__] = r
			return r
		wrap.__name__ = 'repeat-wrap:' + f.__name__
		wrap.__doc__ = f.__doc__
		return wrap
	def _check_forced(self,context,name):
		return getattr(self.user_settings, 'sconf_%s' % name)
	def _check_macro(self,context,macro_name,macro_value,test,**kwargs):
		r = self.Compile(context, text="""
#define {macro_name} {macro_value}
{test}
""".format(macro_name=macro_name, macro_value=macro_value, test=test), **kwargs)
		if r:
			context.sconf.Define(macro_name, macro_value)
		else:
			context.sconf.Define(macro_name, self.comment_not_supported)
	def __compiler_test_already_done(self,context):
		pass
	def Compile(self,context,text,msg,ext='.c',successflags={},skipped=None,successmsg=None,failuremsg=None):
		self.__automatic_compiler_tests.pop(ext, self.__compiler_test_already_done)(context)
		context.Message('%s: checking %s...' % (self.msgprefix, msg))
		if skipped is not None:
			context.Result('(skipped){skipped}'.format(skipped=skipped))
			return
		frame = None
		try:
			1//0
		except ZeroDivisionError:
			frame = sys.exc_info()[2].tb_frame.f_back
		while frame is not None:
			co_name = frame.f_code.co_name
			if co_name[0:6] == 'check_':
				forced = self._check_forced(context, co_name[6:])
				if forced is not None:
					context.Result('(forced){forced}'.format(forced='yes' if forced else 'no'))
					return forced
				break
			frame = frame.f_back
		env_flags = {k: context.env[k] for k in successflags.keys()}
		context.env.Append(**successflags)
		caller_modified_env_flags = {k: context.env[k] for k in self.__flags_Werror.keys()}
		# Always pass -Werror
		context.env.Append(**self.__flags_Werror)
		# Force verbose output to sconf.log
		cc_env_strings = {}
		for k in ['CCCOMSTR', 'CXXCOMSTR']:
			try:
				cc_env_strings[k] = context.env[k]
				del context.env[k]
			except KeyError:
				pass
		r = context.TryCompile(text + '\n', ext)
		# Restore potential quiet build options
		context.env.Replace(**cc_env_strings)
		context.Result((successmsg if r else failuremsg) or r)
		# On success, revert to base flags + successflags
		# On failure, revert to base flags
		if r:
			context.env.Replace(**caller_modified_env_flags)
		else:
			context.env.Replace(**env_flags)
		return r
	@_may_repeat
	@_implicit_test
	def check_cc_works(self,context):
		"""
help:assume C compiler works
"""
		if not self.Compile(context, text=self.__empty_main_program, msg='whether C compiler works'):
			raise SCons.Errors.StopError("C compiler does not work.")
	@_custom_test
	def check_attribute_format_arg(self,context):
		"""
help:assume compiler supports __attribute__((format_arg))
"""
		macro_name = '__attribute_format_arg(A)'
		macro_value = '__attribute__((format_arg(A)))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
char*a(char*)__attribute_format_arg(1);
""", msg='for function __attribute__((format_arg))')
	@_custom_test
	def check_attribute_format_printf(self,context):
		"""
help:assume compiler supports __attribute__((format(printf)))
"""
		macro_name = '__attribute_format_printf(A,B)'
		macro_value = '__attribute__((format(printf,A,B)))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
int a(char*,...)__attribute_format_printf(1,2);
int b(char*)__attribute_format_printf(1,0);
""", msg='for function __attribute__((format(printf)))')

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
	__shared_program_instance = [0]
	__endian = checkEndian()
	@property
	def program_message_prefix(self):
		return '%s.%d' % (self.PROGRAM_NAME, self.program_instance)
	# Settings which affect how the files are compiled
	class UserBuildSettings:
		# Paths for the Videocore libs/includes on the Raspberry Pi
		RPI_DEFAULT_VC_PATH='/opt/vc'
		default_OGLES_LIB = 'GLES_CM'
		_default_prefix = '/usr/local'
		def default_builddir(self):
			builddir_prefix = self.builddir_prefix
			builddir_suffix = self.builddir_suffix
			default_builddir = builddir_prefix or ''
			if builddir_prefix is not None or builddir_suffix is not None:
				fields = [
					self.host_platform,
					os.path.basename(self.CC) if self.CC else None,
				]
				compiler_flags = '\n'.join((getattr(self, attr) or '') for attr in ['CPPFLAGS', 'CFLAGS'])
				if compiler_flags:
					# Mix in CRC of CFLAGS to get reasonable uniqueness
					# when flags are changed.  A full hash is
					# unnecessary here.
					import binascii
					crc = binascii.crc32(compiler_flags)
					if crc < 0:
						crc = crc + 0x100000000
					fields.append('{:08x}'.format(crc))
				fields.append(''.join(a[1] if getattr(self, a[0]) else (a[2] if len(a) > 2 else '')
				for a in (
					('debug', 'dbg'),
					('profiler', 'prf'),
					('editor', 'ed'),
					('opengl', 'ogl', 'sdl'),
					('opengles', 'es'),
					('raspberrypi', 'rpi'),
				)))
				default_builddir += '-'.join([f for f in fields if f])
				if builddir_suffix is not None:
					default_builddir += builddir_prefix
			return default_builddir
		def default_memdebug(self):
			return self.debug
		# automatic setup for raspberrypi
		def default_opengles(self):
			if self.raspberrypi:
				return True
			return False
		def selected_OGLES_LIB(self):
			if self.raspberrypi:
				return 'GLESv2'
			return self.default_OGLES_LIB
		def __default_DATA_DIR(self):
			return self.prefix + '/share/games/' + self._program.target
		@staticmethod
		def _generic_variable(key,help,default):
			return (key, help, default)
		@staticmethod
		def _enum_variable(key,help,default,allowed_values):
			return EnumVariable(key, help, default, allowed_values)
		def _options(self):
			return (
			{
				'variable': BoolVariable,
				'arguments': [
					('sconf_%s' % name[6:], None, ConfigureTests.describe(name) or ('assume result of %s' % name)) for name in ConfigureTests.implicit_tests + ConfigureTests.custom_tests
				],
			},
			{
				'variable': BoolVariable,
				'arguments': (
					('raspberrypi', False, 'build for Raspberry Pi (automatically sets opengles and opengles_lib)'),
				),
			},
			{
				'variable': self._generic_variable,
				'arguments': (
					('rpi_vc_path', self.RPI_DEFAULT_VC_PATH, 'directory for RPi VideoCore libraries'),
					('opengles_lib', self.selected_OGLES_LIB, 'name of the OpenGL ES library to link against'),
					('prefix', self._default_prefix, 'installation prefix directory (Linux only)'),
					('sharepath', self.__default_DATA_DIR, 'directory for shared game data (Linux only)'),
				),
			},
			{
				'variable': BoolVariable,
				'arguments': (
					('debug', False, 'build DEBUG binary which includes asserts, debugging output, cheats and more output'),
					('memdebug', self.default_memdebug, 'build with malloc tracking'),
					('profiler', False, 'profiler build'),
					('opengl', True, 'build with OpenGL support'),
					('opengles', self.default_opengles, 'build with OpenGL ES support'),
					('asm', False, 'build with ASSEMBLER code (only with opengl=0, requires NASM and x86)'),
					('editor', False, 'include editor into build (!EXPERIMENTAL!)'),
					('sdlmixer', True, 'build with SDL_Mixer support for sound and music (includes external music support)'),
					('ipv6', False, 'enable IPv6 compability'),
					('use_udp', True, 'enable UDP support'),
					('use_tracker', True, 'enable Tracker support (requires UDP)'),
					('verbosebuild', False, 'print out all compiler/linker messages during building'),
				),
			},
			{
				'variable': self._generic_variable,
				'arguments': (
					('CC', os.environ.get('CC'), 'C compiler command'),
					('CXX', os.environ.get('CXX'), 'C++ compiler command'),
					('CFLAGS', os.environ.get('CFLAGS'), 'C compiler flags'),
					('CPPFLAGS', os.environ.get('CPPFLAGS'), 'C preprocessor flags'),
					('CXXFLAGS', os.environ.get('CXXFLAGS'), 'C++ compiler flags'),
					('LDFLAGS', os.environ.get('LDFLAGS'), 'Linker flags'),
					('LIBS', os.environ.get('LIBS'), 'Libraries to link'),
					('RC', os.environ.get('RC'), 'Windows resource compiler command'),
					('extra_version', None, 'text to append to version, such as VCS identity'),
				),
			},
			{
				'variable': self._enum_variable,
				'arguments': (
					('host_platform', None, 'cross-compile to specified platform', {'allowed_values' : ['win32', 'darwin', 'linux']}),
				),
			},
			{
				'variable': self._generic_variable,
				'arguments': (
					('builddir_prefix', None, 'prefix to generated build directory'),
					('builddir_suffix', None, 'suffix to generated build directory'),
					# This must be last so that default_builddir will
					# have access to other properties.
					('builddir', self.default_builddir, 'build in specified directory'),
				),
			},
		)
		def __init__(self,program=None):
			self._program = program
		def init(self,prefix,variables):
			def names(name):
				# Mask out the leading underscore form.
				return [('%s_%s' % (p, name)) for p in prefix if p]
			visible_arguments = []
			def FormatVariableHelpText(env, opt, help, default, actual, aliases):
				if not opt in visible_arguments:
					return ''
				l = []
				if default is not None:
					if isinstance(default, str) and not default.isalnum():
						default = '"%s"' % default
					l.append("default: {default}".format(default=default))
				actual = getattr(self, opt, None)
				if actual is not None:
					if isinstance(actual, str) and not actual.isalnum():
						actual = '"%s"' % actual
					l.append("current: {current}".format(current=actual))
				return "  {opt:13}  {help} ".format(opt=opt, help=help) + ("[" + "; ".join(l) + "]" if len(l) else '') + '\n'
			variables.FormatVariableHelpText = FormatVariableHelpText
			for grp in self._options():
				variable = grp['variable']
				for opt in grp['arguments']:
					(name,value,help) = opt[0:3]
					kwargs = opt[3] if len(opt) > 3 else {}
					if callable(value):
						value = value()
					for n in names(name):
						if n not in variables.keys():
							variables.Add(variable(key=n, help=help, default=None, **kwargs))
					visible_arguments.append(name)
					variables.Add(variable(key=name, help=help, default=value, **kwargs))
					d = SCons.Environment.SubstitutionEnvironment()
					variables.Update(d)
					for n in names(name) + [name]:
						try:
							value = d[n]
							break
						except KeyError as e:
							pass
					setattr(self, name, value)
			if self.builddir != '' and self.builddir[-1:] != '/':
				self.builddir += '/'
		def clone(self):
			clone = DXXCommon.UserBuildSettings(None)
			for grp in clone._options():
				for o in grp['arguments']:
					name = o[0]
					value = getattr(self, name)
					setattr(clone, name, value)
			return clone
	class UserInstallSettings:
		def _options(self):
			return (
			{
				'variable': self._generic_variable,
				'arguments': (
					('DESTDIR', None, 'installation stage directory'),
					('program_name', None, 'name of built program'),
				),
			},
			{
				'variable': BoolVariable,
				'arguments': (
					('register_install_target', True, 'report install target to SCons core'),
				),
			},
		)
	class UserSettings(UserBuildSettings,UserInstallSettings):
		def _options(self):
			return DXXCommon.UserBuildSettings._options(self) + DXXCommon.UserInstallSettings._options(self)
	# Base class for platform-specific settings processing
	class _PlatformSettings:
		tools = None
		ogllibs = ''
		osasmdef = None
		platform_sources = []
		platform_objects = []
		def __init__(self,program,user_settings):
			self.__program = program
			self.user_settings = user_settings
		@property
		def env(self):
			return self.__program.env
	# Settings to apply to mingw32 builds
	class Win32PlatformSettings(_PlatformSettings):
		tools = ['mingw']
		osdef = '_WIN32'
		osasmdef = 'win32'
		def adjust_environment(self,program,env):
			env.Append(CPPDEFINES = ['_WIN32', 'HAVE_STRUCT_TIMEVAL'])
	class DarwinPlatformSettings(_PlatformSettings):
		osdef = '__APPLE__'
		def __init__(self,program,user_settings):
			DXXCommon._PlatformSettings.__init__(self,program,user_settings)
			user_settings.asm = 0
		def adjust_environment(self,program,env):
			env.Append(CPPDEFINES = ['HAVE_STRUCT_TIMESPEC', 'HAVE_STRUCT_TIMEVAL', '__unix__'])
			env.Append(CPPPATH = [os.path.join(program.srcdir, '../physfs'), os.path.join(os.getenv("HOME"), 'Library/Frameworks/SDL.framework/Headers'), '/Library/Frameworks/SDL.framework/Headers'])
			env.Append(FRAMEWORKS = ['ApplicationServices', 'Carbon', 'Cocoa', 'SDL'])
			if (self.user_settings.opengl == 1) or (self.user_settings.opengles == 1):
				env.Append(FRAMEWORKS = ['OpenGL'])
			env.Append(FRAMEWORKPATH = [os.path.join(os.getenv("HOME"), 'Library/Frameworks'), '/System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks'])
			env['LIBPATH'] = '../physfs/build/Debug'
	# Settings to apply to Linux builds
	class LinuxPlatformSettings(_PlatformSettings):
		osdef = '__LINUX__'
		osasmdef = 'elf'
		__opengl_libs = ['GL', 'GLU']
		__pkg_config_sdl = {}
		def __init__(self,program,user_settings):
			DXXCommon._PlatformSettings.__init__(self,program,user_settings)
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
					message(program, "reading SDL settings from `%s`" % cmd)
				self.__pkg_config_sdl[cmd] = env.backtick(cmd)
				flags = self.__pkg_config_sdl[cmd]
			env.MergeFlags(flags)

	def __init__(self):
		LazyObjectConstructor.__init__(self)
		self.sources = []
		self.__shared_program_instance[0] += 1
		self.program_instance = self.__shared_program_instance[0]

	def prepare_environment(self):
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
		self.env.Append(CCFLAGS = ['-Wall', '-Wundef', '-Werror=redundant-decls', '-Werror=pointer-arith', '-Werror=undef', '-funsigned-char', '-Werror=implicit-int', '-Werror=implicit-function-declaration', '-Werror=format-security', '-pthread'])
		self.env.Append(CFLAGS = ['-std=gnu99', '-Wwrite-strings'])
		self.env.Append(CPPPATH = ['common/include', 'common/main', '.', self.user_settings.builddir])
		self.env.Append(CPPFLAGS = ['-Wno-sign-compare'])
		if (self.user_settings.editor == 1):
			self.env.Append(CPPPATH = ['common/include/editor'])
		# Get traditional compiler environment variables
		for cc in ['CC', 'CXX', 'RC']:
			value = getattr(self.user_settings, cc)
			if value is not None:
				self.env[cc] = value
		for flags in ['CFLAGS', 'CPPFLAGS', 'CXXFLAGS', 'LIBS']:
			value = getattr(self.user_settings, flags)
			if value is not None:
				self.env.Append(**{flags : SCons.Util.CLVar(value)})
		if self.user_settings.LDFLAGS:
			self.env.Append(LINKFLAGS = SCons.Util.CLVar(self.user_settings.LDFLAGS))
		self.sources += self.objects_common[:]

	def check_endian(self):
		# set endianess
		if (self.__endian == "big"):
			message(self, "BigEndian machine detected")
			self.asm = 0
			self.env.Append(CPPDEFINES = ['WORDS_BIGENDIAN'])
		elif (self.__endian == "little"):
			message(self, "LittleEndian machine detected")

	def check_platform(self):
		# windows or *nix?
		platform_name = self.user_settings.host_platform or sys.platform
		if self._argument_prefix_list:
			prefix = ' with prefix list %s' % list(self._argument_prefix_list)
		else:
			prefix = ''
		message(self, "compiling on %s for %s into %s%s" % (sys.platform, platform_name, self.user_settings.builddir or '.', prefix))
		if platform_name == 'win32':
			platform = self.Win32PlatformSettings
		elif platform_name == 'darwin':
			platform = self.DarwinPlatformSettings
		else:
			platform = self.LinuxPlatformSettings
		self.platform_settings = platform(self, self.user_settings)
		# Acquire environment object...
		self.env = Environment(ENV = os.environ, tools = platform.tools)
		self.platform_settings.adjust_environment(self, self.env)

	def process_user_settings(self):
		env = self.env
		# opengl or software renderer?
		if (self.user_settings.opengl == 1) or (self.user_settings.opengles == 1):
			if (self.user_settings.opengles == 1):
				message(self, "building with OpenGL ES")
				env.Append(CPPDEFINES = ['OGLES'])
			else:
				message(self, "building with OpenGL")
			env.Append(CPPDEFINES = ['OGL'])

		# assembler code?
		if (self.user_settings.asm == 1) and (self.user_settings.opengl == 0):
			message(self, "including: ASSEMBLER")
			env.Replace(AS = 'nasm')
			env.Append(ASCOM = ' -f ' + str(self.platform_settings.osasmdef) + ' -d' + str(self.platform_settings.osdef) + ' -Itexmap/ ')
			self.sources += asm_sources
		else:
			env.Append(CPPDEFINES = ['NO_ASM'])

		# SDL_mixer support?
		if (self.user_settings.sdlmixer == 1):
			message(self, "including SDL_mixer")
			env.Append(CPPDEFINES = ['USE_SDLMIXER'])

		# debug?
		if (self.user_settings.debug == 1):
			message(self, "including: DEBUG")
			env.Prepend(CFLAGS = ['-g'])
			env.Prepend(CXXFLAGS = ['-g'])
		else:
			env.Append(CPPDEFINES = ['NDEBUG', 'RELEASE'])
			env.Prepend(CFLAGS = ['-O2'])
			env.Prepend(CXXFLAGS = ['-O2'])
		if self.user_settings.memdebug:
			message(self, "including: MEMDEBUG")
			env.Append(CPPDEFINES = ['DEBUG_MEMORY_ALLOCATIONS'])

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
			env.Append(LIBPATH = self.user_settings.rpi_vc_path + '/lib')
			env.Append(LIBS = ['bcm_host'])

class DXXArchive(DXXCommon):
	srcdir = 'common'
	target = 'dxx-common'
	__objects_common = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'2d/2dsline.cpp',
'2d/bitblt.cpp',
'2d/bitmap.cpp',
'2d/box.cpp',
'2d/canvas.cpp',
'2d/circle.cpp',
'2d/disc.cpp',
'2d/gpixel.cpp',
'2d/line.cpp',
'2d/pixel.cpp',
'2d/rect.cpp',
'2d/rle.cpp',
'2d/scalec.cpp',
'3d/clipper.cpp',
'3d/draw.cpp',
'3d/globvars.cpp',
'3d/instance.cpp',
'3d/matrix.cpp',
'3d/points.cpp',
'3d/rod.cpp',
'3d/setup.cpp',
'arch/sdl/joy.cpp',
'arch/sdl/rbaudio.cpp',
'arch/sdl/window.cpp',
'maths/fixc.cpp',
'maths/rand.cpp',
'maths/tables.cpp',
'maths/vecmat.cpp',
'misc/error.cpp',
'misc/hmp.cpp',
'misc/ignorecase.cpp',
'misc/strio.cpp',
'misc/strutil.cpp',
'texmap/ntmap.cpp',
'texmap/scanline.cpp'
]
])
	objects_editor = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'editor/func.cpp',
'ui/button.cpp',
'ui/checkbox.cpp',
'ui/dialog.cpp',
'ui/file.cpp',
'ui/gadget.cpp',
'ui/icon.cpp',
'ui/inputbox.cpp',
'ui/keypad.cpp',
'ui/keypress.cpp',
'ui/keytrap.cpp',
'ui/listbox.cpp',
'ui/menu.cpp',
'ui/menubar.cpp',
'ui/message.cpp',
'ui/radio.cpp',
'ui/scroll.cpp',
'ui/ui.cpp',
'ui/uidraw.cpp',
'ui/userbox.cpp'
]
])
	# for non-ogl
	objects_arch_sdl = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'texmap/tmapflat.cpp'
]
])
	objects_arch_sdlmixer = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'arch/sdl/digi_mixer_music.cpp',
]
])
	class Win32PlatformSettings(LazyObjectConstructor, DXXCommon.Win32PlatformSettings):
		platform_objects = LazyObjectConstructor.create_lazy_object_property([
'common/arch/win32/messagebox.cpp'
])
		def __init__(self,program,user_settings):
			LazyObjectConstructor.__init__(self)
			DXXCommon.Win32PlatformSettings.__init__(self, program, user_settings)
			self.user_settings = user_settings
	@property
	def objects_common(self):
		objects_common = self.__objects_common
		return objects_common + self.platform_settings.platform_objects
	def __init__(self,user_settings):
		self.PROGRAM_NAME = 'DXX-Archive'
		self._argument_prefix_list = None
		DXXCommon.__init__(self)
		self.user_settings = user_settings.clone()
		self.check_platform()
		self.prepare_environment()
		self.check_endian()
		self.process_user_settings()
		self.configure_environment()

	def configure_environment(self):
		fs = SCons.Node.FS.get_default_fs()
		builddir = fs.Dir(self.user_settings.builddir or '.')
		tests = ConfigureTests(self.program_message_prefix, self.user_settings)
		log_file=fs.File('sconf.log', builddir)
		conf = self.env.Configure(custom_tests = {
				k:getattr(tests, k) for k in tests.custom_tests
			},
			conf_dir=fs.Dir('.sconf_temp', builddir),
			log_file=log_file,
			config_h=fs.File('dxxsconf.h', builddir),
			clean=False,
			help=False
		)
		if not conf.env:
			return
		try:
			for k in tests.custom_tests:
				getattr(conf, k)()
		except SCons.Errors.StopError as e:
			raise SCons.Errors.StopError(e.args[0] + '  See {log_file} for details.'.format(log_file=log_file), *e.args[1:])
		self.env = conf.Finish()

class DXXProgram(DXXCommon):
	# version number
	VERSION_MAJOR = 0
	VERSION_MINOR = 58
	VERSION_MICRO = 1
	static_archive_construction = {}
	def _apply_target_name(self,name):
		return os.path.join(os.path.dirname(name), '.%s.%s' % (self.target, os.path.splitext(os.path.basename(name))[0]))
	objects_similar_arch_ogl = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join('similar', f) for f in [
'arch/ogl/gr.cpp',
'arch/ogl/ogl.cpp',
]
],
		'transform_target':_apply_target_name,
	}])
	objects_similar_arch_sdl = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join('similar', f) for f in [
'arch/sdl/gr.cpp',
]
],
		'transform_target':_apply_target_name,
	}])
	objects_similar_arch_sdlmixer = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join('similar', f) for f in [
'arch/sdl/digi_mixer.cpp',
'arch/sdl/jukebox.cpp'
]
],
		'transform_target':_apply_target_name,
	}])
	__objects_common = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join('similar', f) for f in [
'2d/font.cpp',
'2d/palette.cpp',
'2d/pcx.cpp',
'3d/interp.cpp',
'arch/sdl/digi.cpp',
'arch/sdl/digi_audio.cpp',
'arch/sdl/event.cpp',
'arch/sdl/init.cpp',
'arch/sdl/key.cpp',
'arch/sdl/mouse.cpp',
'arch/sdl/timer.cpp',
'main/aipath.cpp',
'main/automap.cpp',
'main/cntrlcen.cpp',
'main/config.cpp',
'main/console.cpp',
'main/controls.cpp',
'main/credits.cpp',
'main/digiobj.cpp',
'main/effects.cpp',
'main/fvi.cpp',
'main/game.cpp',
'main/gamecntl.cpp',
'main/gamefont.cpp',
'main/gamerend.cpp',
'main/gamesave.cpp',
'main/gameseg.cpp',
'main/gauges.cpp',
'main/hostage.cpp',
'main/hud.cpp',
'main/inferno.cpp',
'main/kconfig.cpp',
'main/kmatrix.cpp',
'main/laser.cpp',
'main/lighting.cpp',
'main/menu.cpp',
'main/mglobal.cpp',
'main/mission.cpp',
'main/morph.cpp',
'main/multi.cpp',
'main/multibot.cpp',
'main/newdemo.cpp',
'main/newmenu.cpp',
'main/object.cpp',
'main/paging.cpp',
'main/physics.cpp',
'main/player.cpp',
'main/playsave.cpp',
'main/polyobj.cpp',
'main/powerup.cpp',
'main/render.cpp',
'main/robot.cpp',
'main/scores.cpp',
'main/slew.cpp',
'main/songs.cpp',
'main/state.cpp',
'main/switch.cpp',
'main/terrain.cpp',
'main/texmerge.cpp',
'main/text.cpp',
'main/titles.cpp',
'main/vclip.cpp',
'main/wall.cpp',
'main/weapon.cpp',
'mem/mem.cpp',
'misc/args.cpp',
'misc/hash.c',
'misc/physfsx.c',
]
],
		'transform_target':_apply_target_name,
	}])
	objects_editor = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join('similar', f) for f in [
'editor/autosave.cpp',
'editor/centers.cpp',
'editor/curves.cpp',
'main/dumpmine.cpp',
'editor/eglobal.cpp',
'editor/elight.cpp',
'editor/eobject.cpp',
'editor/eswitch.cpp',
'editor/fixseg.cpp',
'editor/group.cpp',
'editor/info.cpp',
'editor/kbuild.cpp',
'editor/kcurve.cpp',
'editor/kfuncs.cpp',
'editor/kgame.cpp',
'editor/khelp.cpp',
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

	objects_use_udp = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join('similar', f) for f in [
'main/net_udp.cpp',
]
],
		'transform_target':_apply_target_name,
	}])
	class UserSettings(DXXCommon.UserSettings):
		@property
		def BIN_DIR(self):
			# installation path
			return self.prefix + '/bin'
	# Settings to apply to mingw32 builds
	class Win32PlatformSettings(DXXCommon.Win32PlatformSettings):
		def __init__(self,program,user_settings):
			DXXCommon.Win32PlatformSettings.__init__(self,program,user_settings)
			user_settings.sharepath = ''
			self.platform_objects = self.platform_objects[:]
		def adjust_environment(self,program,env):
			DXXCommon.Win32PlatformSettings.adjust_environment(self, program, env)
			rcbasename = os.path.join(program.srcdir, 'arch/win32/%s' % program.target)
			self.platform_objects.append(env.RES(target='%s%s%s' % (program.user_settings.builddir, rcbasename, env["OBJSUFFIX"]), source='%s.rc' % rcbasename))
			env.Append(CPPPATH = [os.path.join(program.srcdir, 'arch/win32/include')])
			env.Append(LINKFLAGS = '-mwindows')
			env.Append(LIBS = ['glu32', 'wsock32', 'ws2_32', 'winmm', 'mingw32', 'SDLmain', 'SDL'])
	# Settings to apply to Apple builds
	class DarwinPlatformSettings(DXXCommon.DarwinPlatformSettings):
		def __init__(self,program,user_settings):
			DXXCommon.DarwinPlatformSettings.__init__(self,program,user_settings)
			user_settings.sharepath = ''
		def adjust_environment(self,program,env):
			DXXCommon.DarwinPlatformSettings.adjust_environment(self, program, env)
			VERSION = str(program.VERSION_MAJOR) + '.' + str(program.VERSION_MINOR)
			if (program.VERSION_MICRO):
				VERSION += '.' + str(program.VERSION_MICRO)
			env['VERSION_NUM'] = VERSION
			env['VERSION_NAME'] = program.PROGRAM_NAME + ' v' + VERSION
			self.platform_sources = [os.path.join(program.srcdir, f) for f in ['arch/cocoa/SDLMain.m', 'arch/carbon/messagebox.c']]
			env.Append(FRAMEWORKS = ['ApplicationServices', 'Carbon', 'Cocoa', 'SDL'])
			if (self.user_settings.sdlmixer == 1):
				env.Append(FRAMEWORKS = ['SDL_mixer'])
			env.Append(LIBS = ['../physfs/build/Debug/libphysfs.dylib'])
	# Settings to apply to Linux builds
	class LinuxPlatformSettings(DXXCommon.LinuxPlatformSettings):
		def __init__(self,program,user_settings):
			DXXCommon.LinuxPlatformSettings.__init__(self,program,user_settings)
			user_settings.sharepath += '/'

	@property
	def objects_common(self):
		objects_common = self.__objects_common
		if (self.user_settings.use_udp == 1):
			objects_common = objects_common + self.objects_use_udp
		return objects_common + self.platform_settings.platform_objects
	def __init__(self,prefix):
		self.variables = Variables('site-local.py', ARGUMENTS)
		self._argument_prefix_list = prefix
		DXXCommon.__init__(self)
		self.banner()
		self.user_settings = self.UserSettings(program=self)
		self.user_settings.init(prefix=prefix, variables=self.variables)
		if not DXXProgram.static_archive_construction.has_key(self.user_settings.builddir):
			DXXProgram.static_archive_construction[self.user_settings.builddir] = DXXArchive(self.user_settings)
		self.check_platform()
		self.prepare_environment()
		self.check_endian()
		self.process_user_settings()
		self.register_program()

	def prepare_environment(self):
		DXXCommon.prepare_environment(self)
		self.env.Append(CPPDEFINES = [('DXX_VERSION_MAJORi', str(self.VERSION_MAJOR)), ('DXX_VERSION_MINORi', str(self.VERSION_MINOR)), ('DXX_VERSION_MICROi', str(self.VERSION_MICRO))])
		self.env.Append(CPPPATH = [os.path.join(self.srcdir, f) for f in ['include', 'main', 'arch/include']])

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
		env.Append(LIBS = ['physfs', 'm'])

	def process_user_settings(self):
		DXXCommon.process_user_settings(self)
		env = self.env
		# opengl or software renderer?

		# SDL_mixer support?
		if (self.user_settings.sdlmixer == 1):
			if (sys.platform != 'darwin'):
				env.Append(LIBS = ['SDL_mixer'])

		# profiler?
		if (self.user_settings.profiler == 1):
			env.Append(LINKFLAGS = '-pg')

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
			env.Append(LIBS = self.platform_settings.ogllibs)
			objects.extend(self.objects_similar_arch_ogl)
		else:
			message(self, "building with Software Renderer")
			objects.extend(static_archive_construction.objects_arch_sdl)
			objects.extend(self.objects_similar_arch_sdl)
		if (self.user_settings.editor == 1):
			objects.extend(self.objects_editor)
			objects.extend(static_archive_construction.objects_editor)
			exe_target += '-editor'
		if self.user_settings.program_name:
			exe_target = self.user_settings.program_name
		versid_cppdefines=self.env['CPPDEFINES'][:]
		if self.user_settings.extra_version:
			versid_cppdefines.append(('DESCENT_VERSION_EXTRA', '\\"%s\\"' % self.user_settings.extra_version))
		objects.extend([self.env.StaticObject(target='%s%s%s' % (self.user_settings.builddir, self._apply_target_name(s), self.env["OBJSUFFIX"]), source=s, CPPDEFINES=versid_cppdefines) for s in ['similar/main/vers_id.cpp']])
		# finally building program...
		env.Program(target='%s%s' % (self.user_settings.builddir, str(exe_target)), source = self.sources + objects)
		if (sys.platform != 'darwin'):
			if self.user_settings.register_install_target:
				install_dir = os.path.join(self.user_settings.DESTDIR or '', self.user_settings.BIN_DIR)
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

	def GenerateHelpText(self):
		return self.variables.GenerateHelpText(self.env)

class D1XProgram(DXXProgram):
	PROGRAM_NAME = 'D1X-Rebirth'
	target = 'd1x-rebirth'
	srcdir = 'd1x-rebirth'
	def prepare_environment(self):
		DXXProgram.prepare_environment(self)
		# Flags and stuff for all platforms...
		self.env.Append(CPPDEFINES = [('DXX_BUILD_DESCENT_I', 1)])

	# general source files
	__objects_common = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join(srcdir, f) for f in [
'iff/iff.c',
'main/ai.c',
'main/bm.c',
'main/bmread.c',
'main/collide.c',
'main/custom.c',
'main/endlevel.c',
'main/fireball.c',
'main/fuelcen.c',
'main/gamemine.c',
'main/gameseq.c',
'main/piggy.c',
'main/snddecom.c',
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
'main/hostage.c',
'editor/ehostage.c',
]
],
	}])
	@property
	def objects_editor(self):
		return self.__objects_editor + DXXProgram.objects_editor.fget(self)

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
	def prepare_environment(self):
		DXXProgram.prepare_environment(self)
		# Flags and stuff for all platforms...
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
'main/bm.c',
'main/collide.c',
'main/endlevel.c',
'main/escort.c',
'main/fireball.c',
'main/fuelcen.c',
'main/gamemine.c',
'main/gamepal.c',
'main/gameseq.c',
'main/movie.c',
'main/piggy.c',
'main/segment.c',
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
	import itertools
	l = [v for (k,v) in ARGLIST if k == s] or [1]
	# Fallback case: build the regular configuration.
	if len(l) == 1:
		try:
			if int(l[0]):
				return [program((s,))]
			return []
		except ValueError:
			# If not an integer, treat this as a configuration profile.
			pass
	r = []
	for e in l:
		for prefix in itertools.product(*[v.split('+') for v in e.split(',')]):
			r.append(program(prefix))
	return r
d1x = register_program('d1x', D1XProgram)
d2x = register_program('d2x', D2XProgram)

# show some help when running scons -h
h = 'DXX-Rebirth, SConstruct file help:' + """

	Type 'scons' to build the binary.
	Type 'scons install' to build (if it hasn't been done) and install.
	Type 'scons -c' to clean up.
	
	Extra options (add them to command line, like 'scons extraoption=value'):
	d1x=[0/1]        Disable/enable D1X-Rebirth
	d1x=prefix-list  Enable D1X-Rebirth with prefix-list modifiers
	d2x=[0/1]        Disable/enable D2X-Rebirth
	d2x=prefix-list  Enable D2X-Rebirth with prefix-list modifiers
"""
for d in d1x + d2x:
	h += d.PROGRAM_NAME + ('.%d:\n' % d.program_instance) + d.GenerateHelpText()
Help(h)

#EOF
