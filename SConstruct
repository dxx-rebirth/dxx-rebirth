#SConstruct

# needed imports
import binascii
import errno
import subprocess
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

def get_Werror_string(l):
	if l and '-Werror' in l:
		return '-W'
	return '-Werror='

class StaticSubprocess:
	class CachedCall:
		def __init__(self,out,err,returncode):
			self.out = out
			self.err = err
			self.returncode = returncode
	__call_cache = {}
	@classmethod
	def pcall(cls,args,stdout,stderr=None):
		a = repr(args)
		try:
			return cls.__call_cache[a]
		except KeyError:
			pass
		p = subprocess.Popen(args, executable=args[0], stdout=stdout, stderr=stderr, close_fds=True)
		(o, e) = p.communicate()
		cls.__call_cache[a] = c = cls.CachedCall(o, e, p.wait())
		return c

class Git(StaticSubprocess):
	__missing_git = StaticSubprocess.CachedCall(None, None, 1)
	__path_git = None
	@classmethod
	def pcall(cls,args,stdout,stderr=None):
		git = cls.__path_git
		if git is None:
			cls.__path_git = git = (os.environ.get('GIT', 'git').split(),)
		git = git[0]
		if not git:
			return cls.__missing_git
		return StaticSubprocess.pcall(git + args, stdout=stdout, stderr=stderr)
	@classmethod
	def spcall(cls,args,stdout,stderr=None):
		g = cls.pcall(args, stdout, stderr)
		if g.returncode:
			return None
		return g.out

class ConfigureTests:
	class Collector:
		def __init__(self):
			self.tests = []
		def __call__(self,f):
			self.tests.append(f.__name__)
			return f
	class PreservedEnvironment:
		def __init__(self,env,keys):
			self.flags = {k: env.get(k, [])[:] for k in keys}
			# Do not distribute tests when run under ccache
			# Assume distcc users also use ccache.  Harmless when wrong,
			# saves a bit of latency when right.
			self.CCACHE_PREFIX = env['ENV'].pop('CCACHE_PREFIX', None)
		def restore(self,env):
			env.Replace(**self.flags)
			if self.CCACHE_PREFIX:
				env['ENV']['CCACHE_PREFIX'] = self.CCACHE_PREFIX
		def __getitem__(self,key):
			return self.flags.__getitem__(key)
	class ForceVerboseLog:
		def __init__(self,env):
			# Force verbose output to sconf.log
			self.cc_env_strings = {}
			for k in ['CXXCOMSTR']:
				try:
					self.cc_env_strings[k] = env[k]
					del env[k]
				except KeyError:
					pass
		def restore(self,env):
			# Restore potential quiet build options
			env.Replace(**self.cc_env_strings)
	class pkgconfig:
		__pkg_config_path_cache = {}
		__pkg_config_data_cache = {}
		@staticmethod
		def _get_pkg_config_name(user_settings):
			p = user_settings.PKG_CONFIG
			if p is not None:
				return p
			p = user_settings.CHOST
			if p:
				return p + '-pkg-config'
			return 'pkg-config'
		@staticmethod
		def _get_pkg_config_exec_path(context,message,pkgconfig):
			if not pkgconfig:
				message("pkg-config is disabled by user settings")
				return pkgconfig
			if os.sep in pkgconfig:
				message("using pkg-config at user specified path %s" % pkgconfig)
				return pkgconfig
			# No path specified, search in $PATH
			for p in os.environ.get('PATH', '').split(os.pathsep):
				fp = os.path.join(p, pkgconfig)
				try:
					os.close(os.open(fp, os.O_RDONLY))
				except OSError as e:
					# Ignore on permission errors.  If pkg-config is
					# runnable but not readable, the user must
					# specify its path.
					if e.errno == errno.ENOENT or e.errno == errno.EACCES:
						continue
					raise
				message("using pkg-config at discovered path %s" % fp)
				return fp
			message("no usable pkg-config %r found in $PATH" % pkgconfig)
		@classmethod
		def _get_pkg_config_path(cls,context,message,user_settings,display_name):
			pkgconfig = cls._get_pkg_config_name(user_settings)
			cache = cls.__pkg_config_path_cache
			try:
				return cache[pkgconfig]
			except KeyError:
				pass
			cache[pkgconfig] = path = cls._get_pkg_config_exec_path(context, message, pkgconfig)
			return path
		@classmethod
		def _find_pkg_config(cls,context,message,user_settings,pkgconfig_name,display_name):
			message("checking %s pkg-config %s" % (display_name, pkgconfig_name))
			pkgconfig = cls._get_pkg_config_path(context, message, user_settings, display_name)
			if not pkgconfig:
				message("skipping %s pkg-config" % display_name)
				return {}
			cmd = '%s --cflags --libs %s' % (pkgconfig, pkgconfig_name)
			cache = cls.__pkg_config_data_cache
			try:
				flags = cache[cmd]
				message("reusing %s settings from `%s`" % (display_name, cmd))
				return flags
			except KeyError as e:
				message("reading %s settings from `%s`" % (display_name, cmd))
				try:
					flags = context.env.ParseFlags('!' + cmd)
				except OSError as o:
					message("%s pkg-config failed; user must add required flags via environment for `%s`" % (display_name, cmd))
					flags = {}
				cache[cmd] = flags
				return flags
		@classmethod
		def merge(cls,context,message,user_settings,pkgconfig_name,display_name):
			flags = cls._find_pkg_config(context, message, user_settings, pkgconfig_name, display_name)
			return {k:v for k,v in flags.items() if v and (k[0] == 'C' or k[0] == 'L')}
	# Force test to report failure
	sconf_force_failure = 'force-failure'
	# Force test to report success, and modify flags like it
	# succeeded
	sconf_force_success = 'force-success'
	# Force test to report success, do not modify flags
	sconf_assume_success = 'assume-success'
	_implicit_test = Collector()
	_custom_test = Collector()
	implicit_tests = _implicit_test.tests
	custom_tests = _custom_test.tests
	comment_not_supported = '/* not supported */'
	__flags_Werror = {k:['-Werror'] for k in ['CXXFLAGS']}
	_cxx_conformance_cxx11 = 11
	_cxx_conformance_cxx14 = 14
	def __init__(self,msgprefix,user_settings,platform_settings):
		self.msgprefix = msgprefix
		self.user_settings = user_settings
		self.platform_settings = platform_settings
		self.successful_flags = {}
		self.__cxx_conformance = None
		self.__automatic_compiler_tests = {
			'.cpp': self.check_cxx_works,
		}
	def message(self,msg):
		print "%s: %s" % (self.msgprefix, msg)
	@classmethod
	def describe(cls,name):
		f = getattr(cls, name)
		if f.__doc__:
			lines = f.__doc__.rstrip().split('\n')
			if lines[-1].startswith("help:"):
				return lines[-1][5:]
		return None
	@staticmethod
	def _quote_macro_value(v):
		return v.strip().replace('\n', ' \\\n')
	def _check_forced(self,context,name):
		return getattr(self.user_settings, 'sconf_%s' % name)
	def _check_macro(self,context,macro_name,macro_value,test,**kwargs):
		macro_value = self._quote_macro_value(macro_value)
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
	def _check_compiler_works(self,context,ext):
		self.__automatic_compiler_tests.pop(ext, self.__compiler_test_already_done)(context)
	def _extend_successflags(self,k,v):
		self.successful_flags.setdefault(k, []).extend(v)
	def Compile(self,context,**kwargs):
		if self.user_settings.lto:
			self.Compile = self.Link
			return self.Link(context, **kwargs)
		return self._Test(context,action=context.TryCompile, **kwargs)
	def Link(self,context,**kwargs):
		return self._Test(context,action=context.TryLink, **kwargs)
	def _Test(self,context,text,msg,action,main='',ext='.cpp',testflags={},successflags={},skipped=None,successmsg=None,failuremsg=None,expect_failure=False):
		self._check_compiler_works(context,ext)
		context.Message('%s: checking %s...' % (self.msgprefix, msg))
		if skipped is not None:
			context.Result('(skipped){skipped}'.format(skipped=skipped))
			return
		env_flags = self.PreservedEnvironment(context.env, successflags.keys() + testflags.keys() + self.__flags_Werror.keys() + ['CPPDEFINES'])
		context.env.MergeFlags(successflags)
		frame = None
		forced = None
		try:
			1//0
		except ZeroDivisionError:
			frame = sys.exc_info()[2].tb_frame.f_back
		while frame is not None:
			co_name = frame.f_code.co_name
			if co_name[0:6] == 'check_':
				forced = self._check_forced(context, co_name[6:])
				break
			frame = frame.f_back
		caller_modified_env_flags = self.PreservedEnvironment(context.env, self.__flags_Werror.keys() + testflags.keys())
		# Always pass -Werror
		context.env.Append(**self.__flags_Werror)
		context.env.Append(**testflags)
		if forced is None:
			cc_env_strings = self.ForceVerboseLog(context.env)
			undef_SDL_main = '\n#undef main	/* avoid -Dmain=SDL_main from libSDL */\n'
			r = action(text + undef_SDL_main + 'int main(int argc,char**argv){(void)argc;(void)argv;' + main + ';}\n', ext)
			if expect_failure:
				r = not r
			cc_env_strings.restore(context.env)
			context.Result((successmsg if r else failuremsg) or r)
		else:
			choices = (self.sconf_force_failure, self.sconf_force_success, self.sconf_assume_success)
			if forced not in choices:
				try:
					forced = choices[int(forced)]
				except ValueError:
					raise SCons.Errors.UserError("Unknown force value for sconf_%s: %s" % (co_name[6:], forced))
				except IndexError:
					raise SCons.Errors.UserError("Out of range force value for sconf_%s: %s" % (co_name[6:], forced))
			if forced == self.sconf_force_failure:
				r = False
			elif forced == self.sconf_force_success or forced == self.sconf_assume_success:
				r = True
			else:
				raise SCons.Errors.UserError("Unknown force value for sconf_%s: %s" % (co_name[6:], forced))
			if expect_failure:
				r = not r
			context.Result('(forced){inverted}{forced}'.format(forced=forced, inverted='(inverted)' if expect_failure else ''))
		# On success, revert to base flags + successflags
		# On failure, revert to base flags
		if r and forced != self.sconf_assume_success:
			caller_modified_env_flags.restore(context.env)
			context.env.Replace(CPPDEFINES=env_flags['CPPDEFINES'])
			CPPDEFINES = []
			for v in successflags.pop('CPPDEFINES', []):
				d = v
				if isinstance(d, str):
					d = (d,None)
				if d[0] in ('_REENTRANT',):
					# Blacklist defines that must not be moved to the
					# configuration header.
					CPPDEFINES.append(v)
					continue
				context.sconf.Define(d[0], d[1])
			successflags['CPPDEFINES'] = CPPDEFINES
			for (k,v) in successflags.items():
				self._extend_successflags(k, v)
		else:
			env_flags.restore(context.env)
		return r
	def _soft_check_system_library(self,context,header,main,lib,successflags={}):
		include = '\n'.join(['#include <%s>' % h for h in header])
		# Test library.  On success, good.  On failure, test header to
		# give the user more help.
		if self.Link(context, text=include, main=main, msg='for usable library ' + lib, successflags=successflags):
			return
		if self.Compile(context, text=include, main=main, msg='for usable header ' + header[-1], testflags=successflags):
			return (0, "Header %s is usable, but library %s is not usable." % (header[-1], lib))
		if self.Compile(context, text=include, main=main, msg='for parseable header ' + header[-1], testflags=successflags):
			return (1, "Header %s is parseable, but cannot compile the test program." % (header[-1]))
		return (2, "Header %s is missing or unusable." % (header[-1]))
	def _check_system_library(self,*args,**kwargs):
		e = self._soft_check_system_library(*args, **kwargs)
		if e:
			raise SCons.Errors.StopError(e[1])
	@_custom_test
	def check_libphysfs(self,context):
		main = '''
	PHYSFS_File *f;
	char b[1] = {0};
	PHYSFS_init("");
	f = PHYSFS_openWrite("a");
	PHYSFS_sint64 w = PHYSFS_write(f, b, 1, 1);
	(void)w;
	PHYSFS_close(f);
	f = PHYSFS_openRead("a");
	PHYSFS_sint64 r = PHYSFS_read(f, b, 1, 1);
	(void)r;
	PHYSFS_close(f);
'''
		l = ['physfs']
		successflags = {'LIBS' : l}
		e = self._soft_check_system_library(context, header=['zlib.h', 'physfs.h'], main=main, lib='physfs', successflags=successflags)
		if not e:
			return
		if e[0] == 0:
			self.message("physfs header usable; adding zlib and retesting library")
			l.append('z')
			e = self._soft_check_system_library(context, header=['zlib.h', 'physfs.h'], main=main, lib='physfs', successflags=successflags)
		if e:
			raise SCons.Errors.StopError(e[1])
	@_custom_test
	def check_libSDL(self,context):
		successflags = self.pkgconfig.merge(context, self.message, self.user_settings, 'sdl', 'SDL')
		self._check_system_library(context,header=['SDL.h'],main='''
	SDL_RWops *ops = reinterpret_cast<SDL_RWops *>(argv);
	SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_CDROM | SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	SDL_QuitSubSystem(SDL_INIT_CDROM);
	SDL_FreeRW(ops);
	SDL_Quit();
''',
			lib='SDL', successflags=successflags
		)
	@_custom_test
	def check_SDL_mixer(self,context):
		msg = 'whether to use SDL_mixer'
		context.Display('%s: checking %s...' % (self.msgprefix, msg))
		# SDL_mixer support?
		context.Result(self.user_settings.sdlmixer)
		if not self.user_settings.sdlmixer:
			return
		self._extend_successflags('CPPDEFINES', ['USE_SDLMIXER'])
		successflags = self.pkgconfig.merge(context, self.message, self.user_settings, 'SDL_mixer', 'SDL_mixer')
		if self.user_settings.host_platform == 'darwin':
			successflags['FRAMEWORKS'] = ['SDL_mixer']
			successflags['CPPPATH'] = [os.path.join(os.getenv("HOME"), 'Library/Frameworks/SDL_mixer.framework/Headers'), '/Library/Frameworks/SDL_mixer.framework/Headers']
		self._check_system_library(context,header=['SDL_mixer.h'],main='''
	int i = Mix_Init(MIX_INIT_FLAC | MIX_INIT_OGG);
	(void)i;
	Mix_Pause(0);
	Mix_ResumeMusic();
	Mix_Quit();
''',
			lib='SDL_mixer', successflags=successflags)
	@_implicit_test
	def check_cxx_works(self,context):
		"""
help:assume C++ compiler works
"""
		if self.Link(context, text='', msg='whether C++ compiler and linker work'):
			return
		if self.Compile(context, text='', msg='whether C++ compiler works'):
			raise SCons.Errors.StopError("C++ compiler works, but C++ linker does not work.")
		raise SCons.Errors.StopError("C++ compiler does not work.")
	@_custom_test
	def check_compiler_template_parentheses_warning(self,context):
		# Test for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=51064
		text = '''
template <unsigned S1, unsigned S2 = ((S1 + 4 - 1) & ~(4 - 1))>
struct T {};
'''
		main = 'T<3> t;(void)t;'
		if self.Cxx11Compile(context, text=text, main=main, msg='whether C++ compiler accepts parenthesized template computations', testflags={'CXXFLAGS' : ['-Wparentheses']}) or \
			self.Cxx11Compile(context, text=text, main=main, msg='whether C++ compiler understands -Wno-parentheses', successflags={'CXXFLAGS' : ['-Wno-parentheses']}):
			return
		raise SCons.Errors.StopError("C++ compiler errors on template computed expressions, even with -Wno-parentheses.")
	@_custom_test
	def check_compiler_missing_field_initializers(self,context):
		f = {'CXXFLAGS' : ['-Wmissing-field-initializers']}
		text = 'struct A{int a;};'
		main = 'A a{};(void)a;'
		if not self.Cxx11Compile(context, text=text, main=main, msg='whether C++ compiler warns for {} initialization', testflags=f, expect_failure=True) or \
			self.Cxx11Compile(context, text=text, main=main, msg='whether C++ compiler understands -Wno-missing-field-initializers', successflags={'CXXFLAGS' : ['-Wno-missing-field-initializers']}) or \
			not self.Cxx11Compile(context, text=text, main=main, msg='whether C++ compiler always errors for {} initialization', expect_failure=True):
			return
		raise SCons.Errors.StopError("C++ compiler errors on {} initialization, even with -Wno-missing-field-initializers.")
	@_custom_test
	def check_compiler_visibility_hidden(self,context):
		'''
help:assume compiler accepts -fvisibility=hidden
'''
		self.Compile(context, text='', main='', msg='whether compiler accepts -fvisibility=hidden', successflags={'CXXFLAGS' : ['-fvisibility=hidden']})
	@_custom_test
	def check_attribute_error(self,context):
		"""
help:assume compiler supports __attribute__((error))
"""
		f = '''
void a()__attribute__((__error__("a called")));
'''
		if self.Compile(context, text=f, main='if("0"[0]==\'1\')a();', msg='whether compiler optimizes function __attribute__((__error__))'):
			context.sconf.Define('DXX_HAVE_ATTRIBUTE_ERROR')
			context.sconf.Define('__attribute_error(M)', '__attribute__((__error__(M)))')
		else:
			self.Compile(context, text=f, msg='whether compiler accepts function __attribute__((__error__))') and \
			self.Compile(context, text=f, main='a();', msg='whether compiler understands function __attribute__((__error__))', expect_failure=True)
			context.sconf.Define('__attribute_error(M)', self.comment_not_supported)
	@_custom_test
	def check_builtin_bswap(self,context):
		b = '(void)__builtin_bswap{bits}(static_cast<uint{bits}_t>(argc));'
		include = '''
#include <cstdint>
'''
		main = '''
	{b64}
	{b32}
#ifdef DXX_HAVE_BUILTIN_BSWAP16
	{b16}
#endif
'''.format(
			b64 = b.format(bits=64),
			b32 = b.format(bits=32),
			b16 = b.format(bits=16),
		)
		if self.Cxx11Compile(context, text=include, main=main, msg='whether compiler implements __builtin_bswap{16,32,64} functions', successflags={'CPPDEFINES' : ['DXX_HAVE_BUILTIN_BSWAP', 'DXX_HAVE_BUILTIN_BSWAP16']}):
			return
		if self.Cxx11Compile(context, text=include, main=main, msg='whether compiler implements __builtin_bswap{32,64} functions', successflags={'CPPDEFINES' : ['DXX_HAVE_BUILTIN_BSWAP']}):
			return
	@_custom_test
	def check_builtin_constant_p(self,context):
		"""
help:assume compiler supports compile-time __builtin_constant_p
"""
		f = '''
int c(int);
static int a(int b){
	return __builtin_constant_p(b) ? 1 : %s;
}
'''
		main = 'return a(1) + a(2)'
		if self.Link(context, text=f % 'c(b)', main=main, msg='whether compiler optimizes __builtin_constant_p'):
			context.sconf.Define('DXX_HAVE_BUILTIN_CONSTANT_P')
			context.sconf.Define('dxx_builtin_constant_p(A)', '__builtin_constant_p(A)')
		else:
			self.Compile(context, text=f % '2', main=main, msg='whether compiler accepts __builtin_constant_p')
			context.sconf.Define('dxx_builtin_constant_p(A)', '((void)(A),0)')
	@_custom_test
	def check_builtin_expect(self,context):
		main = '''
return __builtin_expect(argc == 1, 1) ? 1 : 0;
'''
		if self.Compile(context, text='', main=main, msg='whether compiler accepts __builtin_expect'):
			context.sconf.Define('likely(A)', '__builtin_expect(!!(A), 1)')
			context.sconf.Define('unlikely(A)', '__builtin_expect(!!(A), 0)')
		else:
			macro_value = '(!!(A))'
			context.sconf.Define('likely(A)', macro_value)
			context.sconf.Define('unlikely(A)',  macro_value)
	@_custom_test
	def check_builtin_object_size(self,context):
		"""
help:assume compiler supports __builtin_object_size
"""
		f = '''
int a();
static inline int a(char *c){
	return __builtin_object_size(c,0) == 4 ? 1 : %s;
}
'''
		main = '''
	char c[4];
	return a(c);
'''
		if self.Link(context, text=f % 'a()', main=main, msg='whether compiler optimizes __builtin_object_size'):
			context.sconf.Define('DXX_HAVE_BUILTIN_OBJECT_SIZE')
		else:
			self.Compile(context, text=f % '2', main=main, msg='whether compiler accepts __builtin_object_size')
	@_custom_test
	def check_embedded_compound_statement(self,context):
		f = '''
	return ({ 1 + 2; });
'''
		if self.Compile(context, text='', main=f, msg='whether compiler understands embedded compound statements'):
			context.sconf.Define('DXX_BEGIN_COMPOUND_STATEMENT', '')
			context.sconf.Define('DXX_END_COMPOUND_STATEMENT', '')
		else:
			context.sconf.Define('DXX_BEGIN_COMPOUND_STATEMENT', '[&]')
			context.sconf.Define('DXX_END_COMPOUND_STATEMENT', '()')
		context.sconf.Define('DXX_ALWAYS_ERROR_FUNCTION(F,S)', r'( DXX_BEGIN_COMPOUND_STATEMENT {	\
	void F() __attribute_error(S);	\
	F();	\
} DXX_END_COMPOUND_STATEMENT )')
	@_custom_test
	def check_attribute_always_inline(self,context):
		"""
help:assume compiler supports __attribute__((always_inline))
"""
		macro_name = '__attribute_always_inline()'
		macro_value = '__attribute__((__always_inline__))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test=macro_name + 'static inline void a(){}', main='a();', msg='for function __attribute__((always_inline))')
	@_custom_test
	def check_attribute_alloc_size(self,context):
		"""
help:assume compiler supports __attribute__((alloc_size))
"""
		macro_name = '__attribute_alloc_size(A,...)'
		macro_value = '__attribute__((alloc_size(A, ## __VA_ARGS__)))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
char*a(int)__attribute_alloc_size(1);
char*b(int,int)__attribute_alloc_size(1,2);
""", msg='for function __attribute__((alloc_size))')
	@_custom_test
	def check_attribute_cold(self,context):
		"""
help:assume compiler supports __attribute__((cold))
"""
		macro_name = '__attribute_cold'
		macro_value = '__attribute__((cold))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
__attribute_cold char*a(int);
""", msg='for function __attribute__((cold))')
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
	@_custom_test
	def check_attribute_malloc(self,context):
		"""
help:assume compiler supports __attribute__((malloc))
"""
		macro_name = '__attribute_malloc()'
		macro_value = '__attribute__((malloc))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
int *a()__attribute_malloc();
""", msg='for function __attribute__((malloc))')
	@_custom_test
	def check_attribute_nonnull(self,context):
		"""
help:assume compiler supports __attribute__((nonnull))
"""
		macro_name = '__attribute_nonnull(...)'
		macro_value = '__attribute__((nonnull __VA_ARGS__))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
int a(int*)__attribute_nonnull();
int b(int*)__attribute_nonnull((1));
""", msg='for function __attribute__((nonnull))')
	@_custom_test
	def check_attribute_used(self,context):
		"""
help:assume compiler supports __attribute__((used))
"""
		macro_name = '__attribute_used'
		macro_value = '__attribute__((used))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
static void a()__attribute_used;
static void a(){}
""", msg='for function __attribute__((used))')
	@_custom_test
	def check_attribute_unused(self,context):
		"""
help:assume compiler supports __attribute__((unused))
"""
		macro_name = '__attribute_unused'
		macro_value = '__attribute__((unused))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
__attribute_unused
static void a(){}
""", msg='for function __attribute__((unused))')
	@_custom_test
	def check_attribute_warn_unused_result(self,context):
		"""
help:assume compiler supports __attribute__((warn_unused_result))
"""
		macro_name = '__attribute_warn_unused_result'
		macro_value = '__attribute__((warn_unused_result))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
int a()__attribute_warn_unused_result;
int a(){return 0;}
""", msg='for function __attribute__((warn_unused_result))')
	@_implicit_test
	def check_cxx11(self,context):
		"""
help:assume C++ compiler supports C++11
"""
		return self._check_cxx_std_flag(context, ('-std=gnu++0x', '-std=c++0x'), self._cxx_conformance_cxx11)
	@_implicit_test
	def check_cxx14(self,context):
		"""
help:assume C++ compiler supports C++14
"""
		return self._check_cxx_std_flag(context, ('-std=gnu++14', '-std=c++14'), self._cxx_conformance_cxx14)
	def _check_cxx_std_flag(self,context,flags,level):
		for f in flags:
			r = self.Compile(context, text='', msg='whether C++ compiler accepts {f}'.format(f=f), successflags={'CXXFLAGS': [f]})
			if r:
				return level
		return 0
	def Cxx11Compile(self,context,*args,**kwargs):
		kwargs.setdefault('skipped', self.__skip_missing_cxx_std(context, self._cxx_conformance_cxx11, 'no C++11 support'))
		return self.Compile(context,*args,**kwargs)
	def Cxx14Compile(self,context,*args,**kwargs):
		kwargs.setdefault('skipped', self.__skip_missing_cxx_std(context, self._cxx_conformance_cxx14, 'no C++14 support'))
		return self.Compile(context,*args,**kwargs)
	def __skip_missing_cxx_std(self,context,level,text):
		if self.__cxx_conformance is None:
			self.__cxx_conformance = self.check_cxx14(context) or self.check_cxx11(context)
		if self.__cxx_conformance < level:
			return text
	@_implicit_test
	def check_boost_array(self,context,**kwargs):
		"""
help:assume Boost.Array works
"""
		return self.Compile(context, msg='for Boost.Array', successflags={'CPPDEFINES' : ['DXX_HAVE_BOOST_ARRAY']}, **kwargs)
	@_implicit_test
	def check_cxx_array(self,context,**kwargs):
		"""
help:assume <array> works
"""
		return self.Cxx11Compile(context, msg='for <array>', successflags={'CPPDEFINES' : ['DXX_HAVE_CXX_ARRAY']}, **kwargs)
	@_implicit_test
	def check_cxx_tr1_array(self,context,**kwargs):
		"""
help:assume <tr1/array> works
"""
		return self.Compile(context, msg='for <tr1/array>', successflags={'CPPDEFINES' : ['DXX_HAVE_CXX_TR1_ARRAY']}, **kwargs)
	@_custom_test
	def _check_cxx_array(self,context):
		include = '''
#include "compiler-array.h"
'''
		main = '''
	array<int,2>b;
	b[0]=1;
'''
		how = self.check_cxx_array(context, text=include, main=main) or self.check_boost_array(context, text=include, main=main) or self.check_cxx_tr1_array(context, text=include, main=main)
		if not how:
			raise SCons.Errors.StopError("C++ compiler does not support <array> or Boost.Array or <tr1/array>.")
	@_custom_test
	def check_cxx11_function_auto(self,context):
		"""
help:assume compiler supports C++11 function declarator syntax
"""
		f = '''
auto f()->int;
'''
		if not self.Cxx11Compile(context, text=f, msg='for C++11 function declarator syntax'):
			raise SCons.Errors.StopError("C++ compiler does not support C++11 function declarator syntax.")
	def _check_static_assert_method(self,context,msg,f,testflags={},_Compile=Compile,**kwargs):
		return _Compile(self, context, text=f % 'true', msg=msg % 'true', testflags=testflags, **kwargs) and \
			_Compile(self, context, text=f % 'false', msg=msg % 'false', expect_failure=True, successflags=testflags, **kwargs)
	@_implicit_test
	def check_boost_static_assert(self,context,f):
		"""
help:assume Boost.StaticAssert works
"""
		return self._check_static_assert_method(context, 'for Boost.StaticAssert when %s', f, testflags={'CPPDEFINES' : ['DXX_HAVE_BOOST_STATIC_ASSERT']})
	@_implicit_test
	def check_c_typedef_static_assert(self,context,f):
		"""
help:assume C typedef-based static assertion works
"""
		return self._check_static_assert_method(context, 'for C typedef static assertion when %s', f, testflags={'CPPDEFINES' : ['DXX_HAVE_C_TYPEDEF_STATIC_ASSERT']})
	@_implicit_test
	def check_cxx11_static_assert(self,context,f):
		"""
help:assume compiler supports C++ intrinsic static_assert
"""
		return self._check_static_assert_method(context, 'for C++11 intrinsic static_assert when %s', f, testflags={'CPPDEFINES' : ['DXX_HAVE_CXX11_STATIC_ASSERT']}, _Compile=ConfigureTests.Cxx11Compile)
	@_custom_test
	def _check_static_assert(self,context):
		f = '''
#include "compiler-static_assert.h"
static_assert(%s, "");
'''
		how = self.check_cxx11_static_assert(context,f) or self.check_boost_static_assert(context,f) or self.check_c_typedef_static_assert(context,f)
		if not how:
			raise SCons.Errors.StopError("C++ compiler does not support static_assert or Boost.StaticAssert or typedef-based static assertion.")
	@_implicit_test
	def check_boost_type_traits(self,context,f):
		"""
help:assume Boost.TypeTraits works
"""
		return self.Compile(context, text=f, msg='for Boost.TypeTraits', ext='.cpp', successflags={'CPPDEFINES' : ['DXX_HAVE_BOOST_TYPE_TRAITS']})
	@_implicit_test
	def check_cxx11_type_traits(self,context,f):
		"""
help:assume <type_traits> works
"""
		return self.Cxx11Compile(context, text=f, msg='for <type_traits>', ext='.cpp', successflags={'CPPDEFINES' : ['DXX_HAVE_CXX11_TYPE_TRAITS']})
	@_custom_test
	def _check_type_traits(self,context):
		f = '''
#include "compiler-type_traits.h"
typedef tt::conditional<true,int,long>::type a;
typedef tt::conditional<false,int,long>::type b;
'''
		if self.check_cxx11_type_traits(context, f) or self.check_boost_type_traits(context, f):
			context.sconf.Define('DXX_HAVE_TYPE_TRAITS')
	@_implicit_test
	def check_boost_foreach(self,context,**kwargs):
		"""
help:assume Boost.Foreach works
"""
		return self.Compile(context, msg='for Boost.Foreach', successflags={'CPPDEFINES' : ['DXX_HAVE_BOOST_FOREACH']}, **kwargs)
	@_implicit_test
	def check_cxx11_range_for(self,context,**kwargs):
		return self.Cxx11Compile(context, msg='for C++11 range-based for', successflags={'CPPDEFINES' : ['DXX_HAVE_CXX11_RANGE_FOR']}, **kwargs)
	@_custom_test
	def _check_range_based_for(self,context):
		include = '''
#include "compiler-range_for.h"
'''
		main = '''
	int b[2];
	range_for(int&c,b)c=0;
'''
		if not self.check_cxx11_range_for(context, text=include, main=main) and not self.check_boost_foreach(context, text=include, main=main):
			raise SCons.Errors.StopError("C++ compiler does not support range-based for or Boost.Foreach.")
	@_custom_test
	def check_cxx11_constexpr(self,context):
		f = '''
struct A {};
constexpr A a(){return {};}
'''
		if not self.Cxx11Compile(context, text=f, msg='for C++11 constexpr'):
			raise SCons.Errors.StopError("C++ compiler does not support constexpr.")
	@_implicit_test
	def check_pch(self,context):
		for how in [{'CXXFLAGS' : ['-x', 'c++-header']}]:
			result = self.Compile(context, text='', msg='whether compiler supports pre-compiled headers', testflags=how)
			if result:
				self.pch_flags = how
				return result
	@_custom_test
	def _check_pch(self,context):
		self.pch_flags = None
		msg = 'when to pre-compile headers'
		context.Display('%s: checking %s...' % (self.msgprefix, msg))
		if self.user_settings.pch:
			count = int(self.user_settings.pch)
		else:
			count = 0
		if count <= 0:
			context.Result('never')
			return
		context.Display('if used at least %u time%s\n' % (count, 's' if count > 1 else ''))
		if not self.check_pch(context):
			raise SCons.Errors.StopError("C++ compiler does not support pre-compiled headers.")
	@_custom_test
	def check_cxx11_explicit_bool(self,context):
		"""
help:assume compiler supports explicit operator bool
"""
		f = '''
struct A{explicit operator bool();};
'''
		r = self.Cxx11Compile(context, text=f, msg='for explicit operator bool')
		macro_name = 'dxx_explicit_operator_bool'
		if r:
			context.sconf.Define(macro_name, 'explicit')
			context.sconf.Define('DXX_HAVE_EXPLICIT_OPERATOR_BOOL')
		else:
			context.sconf.Define(macro_name, self.comment_not_supported)
	@_custom_test
	def check_cxx11_explicit_delete(self,context):
		"""
help:assume compiler supports explicitly deleted functions
"""
		f = '''
int a()=delete;
'''
		r = self.Cxx11Compile(context, text=f, msg='for explicitly deleted functions')
		macro_name = 'DXX_CXX11_EXPLICIT_DELETE'
		if r:
			context.sconf.Define(macro_name, '=delete')
			context.sconf.Define('DXX_HAVE_CXX11_EXPLICIT_DELETE')
		else:
			context.sconf.Define(macro_name, self.comment_not_supported)
	@_implicit_test
	def check_cxx11_free_begin(self,context,**kwargs):
		return self.Cxx11Compile(context, msg='for C++11 functions begin(), end()', successflags={'CPPDEFINES' : ['DXX_HAVE_CXX11_BEGIN']}, **kwargs)
	@_implicit_test
	def check_boost_free_begin(self,context,**kwargs):
		return self.Compile(context, msg='for Boost.Range functions begin(), end()', successflags={'CPPDEFINES' : ['DXX_HAVE_BOOST_BEGIN']}, **kwargs)
	@_custom_test
	def _check_free_begin_function(self,context):
		f = '''
#include "compiler-begin.h"
struct A {
	typedef int *iterator;
	typedef const int *const_iterator;
	iterator begin(){return 0;}
	iterator end(){return 0;}
	const_iterator begin() const{return 0;}
	const_iterator end() const{return 0;}
};
#define F(C){\
	C int a[1]{0};\
	C A b{};\
	if(begin(a)||end(a)||begin(b)||end(b))return 1;\
}
'''
		main = 'F()F(const)'
		if not self.check_cxx11_free_begin(context, text=f, main=main) and not self.check_boost_free_begin(context, text=f, main=main):
			raise SCons.Errors.StopError("C++ compiler does not support free functions begin() and end().")
	@_implicit_test
	def check_cxx11_addressof(self,context,**kwargs):
		return self.Cxx11Compile(context, msg='for C++11 function addressof()', successflags={'CPPDEFINES' : ['DXX_HAVE_CXX11_ADDRESSOF']}, **kwargs)
	@_implicit_test
	def check_boost_addressof(self,context,**kwargs):
		return self.Compile(context, msg='for Boost.Utility function addressof()', successflags={'CPPDEFINES' : ['DXX_HAVE_BOOST_ADDRESSOF']}, **kwargs)
	@_custom_test
	def _check_free_addressof_function(self,context):
		f = '''
#include "compiler-addressof.h"
struct A {
	void operator&();
};
'''
		main = '''
	A b;
	return addressof(b) != 0;
'''
		if not self.check_cxx11_addressof(context, text=f, main=main) and not self.check_boost_addressof(context, text=f, main=main):
			raise SCons.Errors.StopError("C++ compiler does not support free function addressof().")
	@_custom_test
	def check_cxx14_exchange(self,context):
		f = '''
#include "compiler-exchange.h"
'''
		self.Cxx14Compile(context, text=f, main='return exchange(argc, 5)', msg='for C++14 exchange', successflags={'CPPDEFINES' : ['DXX_HAVE_CXX14_EXCHANGE']})
	@_custom_test
	def check_cxx14_integer_sequence(self,context):
		f = '''
#include <utility>
using std::integer_sequence;
using std::index_sequence;
'''
		self.Cxx14Compile(context, text=f, msg='for C++14 integer_sequence', successflags={'CPPDEFINES' : ['DXX_HAVE_CXX14_INTEGER_SEQUENCE']})
	@_custom_test
	def check_cxx14_make_unique(self,context):
		f = '''
#include "compiler-make_unique.h"
'''
		main = '''
	make_unique<int>(0);
	make_unique<int[]>(1);
'''
		self.Cxx14Compile(context, text=f, main=main, msg='for C++14 make_unique', successflags={'CPPDEFINES' : ['DXX_HAVE_CXX14_MAKE_UNIQUE']})
	@_implicit_test
	def check_cxx11_inherit_constructor(self,context,text,fmtargs,**kwargs):
		"""
help:assume compiler supports inheriting constructors
"""
		macro_value = self._quote_macro_value('''
	typedef B,##__VA_ARGS__ _dxx_constructor_base_type;
	using _dxx_constructor_base_type::_dxx_constructor_base_type;''')
		if self.Cxx11Compile(context, text=text.format(macro_value=macro_value, **fmtargs), msg='for C++11 inherited constructors', **kwargs):
			return macro_value
		return None
	@_implicit_test
	def check_cxx11_variadic_forward_constructor(self,context,text,fmtargs,**kwargs):
		"""
help:assume compiler supports variadic template-based constructor forwarding
"""
		macro_value = self._quote_macro_value('''
    template <typename... Args>
        D(Args&&... args) :
            B,##__VA_ARGS__(std::forward<Args>(args)...) {}
''')
		if self.Cxx11Compile(context, text='#include <algorithm>\n' + text.format(macro_value=macro_value, **fmtargs), msg='for C++11 variadic templates on constructors', **kwargs):
			return macro_value
		return None
	@_custom_test
	def _check_forward_constructor(self,context):
		text = '''
#define {macro_name}{macro_parameters} {macro_value}
struct A {{
	A(int){{}}
}};
struct B:A {{
{macro_name}(B,A);
}};
'''
		macro_name = 'DXX_INHERIT_CONSTRUCTORS'
		macro_parameters = '(D,B,...)'
		# C++03 support is possible with enumerated out template
		# variations.  If someone finds a worthwhile compiler without
		# variadic templates, enumerated templates can be added.
		for f in (self.check_cxx11_inherit_constructor, self.check_cxx11_variadic_forward_constructor):
			macro_value = f(context, text=text, main='B(0)', fmtargs={'macro_name':macro_name, 'macro_parameters':macro_parameters})
			if macro_value:
				break
		if not macro_value:
			raise SCons.Errors.StopError("C++ compiler does not support constructor forwarding.")
		context.sconf.Define(macro_name + macro_parameters, macro_value)
	@_custom_test
	def check_cxx11_template_alias(self,context):
		text = '''
template <typename>
struct A;
template <typename T>
using B = A<T>;
'''
		main = '''
	A<int> *a = 0;
	B<int> *b = a;
	(void)b;
'''
		if self.Cxx11Compile(context, text=text, main=main, msg='for C++11 template aliases'):
			context.sconf.Define('DXX_HAVE_CXX11_TEMPLATE_ALIAS')
	@_custom_test
	def check_cxx11_ref_qualifier(self,context):
		text = '''
struct A {
	int a()&{return 1;}
	int a()&&{return 2;}
};
'''
		main = '''
	A a;
	return a.a() != A().a();
'''
		if self.Cxx11Compile(context, text=text, main=main, msg='for C++11 reference qualified methods'):
			context.sconf.Define('DXX_HAVE_CXX11_REF_QUALIFIER')
	@_custom_test
	def check_deep_tuple(self,context):
		text = '''
#include <tuple>
static inline std::tuple<{type}> make() {{
	return std::make_tuple({value});
}}
static void a(){{
	std::tuple<{type}> t = make();
	(void)t;
}}
'''
		count = 20
		if self.Compile(context, text=text.format(type=','.join(('int',)*count), value=','.join(('0',)*count)), main='a()', msg='whether compiler handles 20-element tuples'):
			return
		count = 2
		if self.Compile(context, text=text.format(type=','.join(('int',)*count), value=','.join(('0',)*count)), main='a()', msg='whether compiler handles 2-element tuples'):
			raise SCons.Errors.StopError("Compiler cannot handle tuples of 20 elements.  Raise the template instantiation depth.")
		raise SCons.Errors.StopError("Compiler cannot handle tuples of 2 elements.")
	@_implicit_test
	def check_poison_valgrind(self,context):
		'''
help:add Valgrind annotations; wipe certain freed memory when running under Valgrind
'''
		context.Message('%s: checking %s...' % (self.msgprefix, 'whether to use Valgrind poisoning'))
		r = 'valgrind' in self.user_settings.poison
		context.Result(r)
		if not r:
			return
		text = '''
#include "poison.h"
'''
		main = '''
	DXX_MAKE_MEM_UNDEFINED(&argc, sizeof(argc));
'''
		if self.Compile(context, text=text, main=main, msg='whether Valgrind memcheck header works', successflags={'CPPDEFINES' : ['DXX_HAVE_POISON_VALGRIND']}):
			return True
		raise SCons.Errors.StopError("Valgrind poison requested, but <valgrind/memcheck.h> does not work.")
	@_implicit_test
	def check_poison_overwrite(self,context):
		'''
help:always wipe certain freed memory
'''
		context.Message('%s: checking %s...' % (self.msgprefix, 'whether to use overwrite poisoning'))
		r = 'overwrite' in self.user_settings.poison
		context.Result(r)
		if r:
			context.sconf.Define('DXX_HAVE_POISON_OVERWRITE')
		return r
	@_custom_test
	def _check_poison_method(self,context):
		poison = None
		for f in (
			self.check_poison_valgrind,
			self.check_poison_overwrite,
		):
			if f(context):
				poison = True
		if poison:
			context.sconf.Define('DXX_HAVE_POISON')
	@_custom_test
	def check_strcasecmp_present(self,context):
		main = '''
	return !strcasecmp(argv[0], argv[0] + 1) && !strncasecmp(argv[0] + 1, argv[0], 1);
'''
		self.Compile(context, text='#include <cstring>', main=main, msg='for strcasecmp', successflags={'CPPDEFINES' : ['DXX_HAVE_STRCASECMP']})

class LazyObjectConstructor:
	def __get_lazy_object(self,srcname,transform_target):
		env = self.env
		o = env.StaticObject(target='%s%s%s' % (self.user_settings.builddir, transform_target(self, srcname), env["OBJSUFFIX"]), source=srcname)
		if env._dxx_pch_node:
			env.Depends(o, self.env._dxx_pch_node)
		return o
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
				value.extend([self.__get_lazy_object(srcname, transform_target) for srcname in s['source']])
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

class FilterHelpText:
	def __init__(self):
		self.visible_arguments = []
		self._sconf_align = None
	def FormatVariableHelpText(self, env, opt, help, default, actual, aliases):
		if not opt in self.visible_arguments:
			return ''
		if not self._sconf_align:
			self._sconf_align = len(max((s for s in self.visible_arguments if s[:6] == 'sconf_'), key=len))
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
		return (" {opt:%u}  {help}" % (self._sconf_align if opt[:6] == 'sconf_' else 15)).format(opt=opt, help=help) + (" [" + "; ".join(l) + "]" if l else '') + '\n'

class DXXCommon(LazyObjectConstructor):
	__shared_program_instance = [0]
	__shared_header_file_list = []
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
					os.path.basename(self.CXX) if self.CXX else None,
				]
				compiler_flags = '\n'.join((getattr(self, attr) or '').strip() for attr in ['CPPFLAGS', 'CXXFLAGS'])
				if compiler_flags:
					# Mix in CRC of CXXFLAGS to get reasonable uniqueness
					# when flags are changed.  A full hash is
					# unnecessary here.
					crc = binascii.crc32(compiler_flags)
					if crc < 0:
						crc = crc + 0x100000000
					fields.append('{:08x}'.format(crc))
				if self.pch:
					fields.append('p%s' % self.pch)
				fields.append(''.join(a[1] if getattr(self, a[0]) else (a[2] if len(a) > 2 else '')
				for a in (
					('debug', 'dbg'),
					('lto', 'lto'),
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
				'variable': self._enum_variable,
				'arguments': [
					('sconf_%s' % name[6:], None, ConfigureTests.describe(name) or ('assume result of %s' % name), {'allowed_values' : ['0', '1', '2', ConfigureTests.sconf_force_failure, ConfigureTests.sconf_force_success, ConfigureTests.sconf_assume_success]}) for name in ConfigureTests.implicit_tests + ConfigureTests.custom_tests if name[0] != '_'
				],
			},
			{
				'variable': BoolVariable,
				'arguments': (
					('raspberrypi', False, 'build for Raspberry Pi (automatically sets opengles and opengles_lib)'),
					('git_describe_version', os.path.exists(os.environ.get('GIT_DIR', '.git')), 'include git --describe in extra_version'),
					('git_status', True, 'include git status'),
					('versid_depend_all', False, 'rebuild vers_id.cpp if any object file changes'),
				),
			},
			{
				'variable': self._generic_variable,
				'arguments': (
					('rpi_vc_path', self.RPI_DEFAULT_VC_PATH, 'directory for RPi VideoCore libraries'),
					('opengles_lib', self.selected_OGLES_LIB, 'name of the OpenGL ES library to link against'),
					('prefix', self._default_prefix, 'installation prefix directory (Linux only)'),
					('sharepath', self.__default_DATA_DIR, 'directory for shared game data (Linux only)'),
					('pch', None, 'pre-compile headers used this many times'),
				),
			},
			{
				'variable': BoolVariable,
				'arguments': (
					('check_header_includes', False, 'compile test each header (developer option)'),
					('debug', False, 'build DEBUG binary which includes asserts, debugging output, cheats and more output'),
					('memdebug', self.default_memdebug, 'build with malloc tracking'),
					('lto', False, 'enable gcc link time optimization'),
					('profiler', False, 'profiler build'),
					('opengl', True, 'build with OpenGL support'),
					('opengles', self.default_opengles, 'build with OpenGL ES support'),
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
					('CHOST', os.environ.get('CHOST'), 'CHOST of output'),
					('CXX', os.environ.get('CXX'), 'C++ compiler command'),
					('PKG_CONFIG', os.environ.get('PKG_CONFIG'), 'PKG_CONFIG to run (Linux only)'),
					('RC', os.environ.get('RC'), 'Windows resource compiler command'),
					('extra_version', None, 'text to append to version, such as VCS identity'),
					('ccache', None, 'path to ccache'),
					('distcc', None, 'path to distcc'),
					('distcc_hosts', os.environ.get('DISTCC_HOSTS'), 'hosts to distribute compilation'),
				),
			},
			{
				'variable': self._generic_variable,
				'stack': ' ',
				'arguments': (
					('CPPFLAGS', os.environ.get('CPPFLAGS'), 'C preprocessor flags'),
					('CXXFLAGS', os.environ.get('CXXFLAGS'), 'C++ compiler flags'),
					('LDFLAGS', os.environ.get('LDFLAGS'), 'Linker flags'),
					('LIBS', os.environ.get('LIBS'), 'Libraries to link'),
				),
			},
			{
				'variable': self._enum_variable,
				'arguments': (
					('host_platform', 'linux' if sys.platform == 'linux2' else sys.platform, 'cross-compile to specified platform', {'allowed_values' : ['win32', 'darwin', 'linux']}),
				),
			},
			{
				'variable': ListVariable,
				'arguments': (
					('poison', 'none', 'method for poisoning free memory', {'names' : ('valgrind', 'overwrite')}),
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
		@staticmethod
		def _names(name,prefix):
			return ['%s%s%s' % (p, '_' if p else '', name) for p in prefix]
		def __init__(self,program=None):
			self._program = program
		def register_variables(self,prefix,variables):
			self.known_variables = []
			for grp in self._options():
				variable = grp['variable']
				stack = grp.get('stack', None)
				for opt in grp['arguments']:
					(name,value,help) = opt[0:3]
					kwargs = opt[3] if len(opt) > 3 else {}
					if name not in variables.keys():
						filtered_help.visible_arguments.append(name)
						variables.Add(variable(key=name, help=help, default=None if callable(value) else value, **kwargs))
					names = self._names(name, prefix)
					for n in names:
						if n not in variables.keys():
							variables.Add(variable(key=n, help=help, default=None, **kwargs))
					if not name in names:
						names.append(name)
					self.known_variables.append((names, name, value, stack))
					if stack:
						for n in names:
							variables.Add(self._generic_variable(key='%s_stop' % n, help=None, default=None))
		def read_variables(self,variables,d):
			for (namelist,cname,dvalue,stack) in self.known_variables:
				value = None
				found_value = False
				for n in namelist:
					try:
						v = d[n]
						found_value = True
						if stack:
							if callable(v):
								value = v(dvalue=dvalue, value=value, stack=stack)
							else:
								if value:
									value = stack.join([value, v])
								else:
									value = v
							if d.get(n + '_stop', None):
								break
							continue
						value = v
						break
					except KeyError as e:
						pass
				if not found_value:
					value = dvalue
				if callable(value):
					value = value()
				setattr(self, cname, value)
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
		platform_objects = []
		def __init__(self,program,user_settings):
			self.__program = program
			self.user_settings = user_settings
		@property
		def env(self):
			return self.__program.env
	# Settings to apply to mingw32 builds
	class Win32PlatformSettings(_PlatformSettings):
		ogllibs = ('opengl32',)
		tools = ['mingw']
		def adjust_environment(self,program,env):
			env.Append(CPPDEFINES = ['_WIN32', 'HAVE_STRUCT_TIMEVAL', 'WIN32_LEAN_AND_MEAN'])
	class DarwinPlatformSettings(_PlatformSettings):
		def __init__(self,program,user_settings):
			DXXCommon._PlatformSettings.__init__(self,program,user_settings)
		def adjust_environment(self,program,env):
			env.Append(CPPDEFINES = ['HAVE_STRUCT_TIMESPEC', 'HAVE_STRUCT_TIMEVAL', '__unix__'])
			env.Append(CPPPATH = [os.path.join(os.getenv("HOME"), 'Library/Frameworks/SDL.framework/Headers'), '/Library/Frameworks/SDL.framework/Headers'])
			env.Append(FRAMEWORKS = ['ApplicationServices', 'Cocoa', 'SDL'])
			if (self.user_settings.opengl == 1) or (self.user_settings.opengles == 1):
				env.Append(FRAMEWORKS = ['OpenGL'])
			env.Append(FRAMEWORKPATH = [os.path.join(os.getenv("HOME"), 'Library/Frameworks'), '/System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks'])
	# Settings to apply to Linux builds
	class LinuxPlatformSettings(_PlatformSettings):
		__opengl_libs = ['GL', 'GLU']
		__pkg_config_sdl = {}
		def __init__(self,program,user_settings):
			DXXCommon._PlatformSettings.__init__(self,program,user_settings)
			if (user_settings.opengles == 1):
				self.ogllibs = [ user_settings.opengles_lib, 'EGL']
			else:
				self.ogllibs = self.__opengl_libs
		def adjust_environment(self,program,env):
			env.Append(CPPDEFINES = ['HAVE_STRUCT_TIMESPEC', 'HAVE_STRUCT_TIMEVAL'])
			env.Append(CCFLAGS = ['-pthread'])

	def __init__(self):
		LazyObjectConstructor.__init__(self)
		self.sources = []
		self.__shared_program_instance[0] += 1
		self.program_instance = self.__shared_program_instance[0]

	@staticmethod
	def _collect_pch_candidates(target,source,env):
		for t in target:
			scanner = t.get_source_scanner(source[0])
			deps = scanner(source[0], env, scanner.path(env))
			for d in deps:
				ds = str(d)
				env.__dxx_pch_candidates[ds] = env.__dxx_pch_candidates.get(ds, 0) + 1
		return (target, source)

	@staticmethod
	def write_pch_inclusion_file(target, source, env):
		with open(str(target[0]), 'wt') as f:
			f.write('/* BEGIN PCH GENERATED FILE\n * Threshold=%u\n */\n' % env.__dxx_pch_inclusion_count)
			for (name,count) in env.__dxx_pch_candidates.items():
				if count >= env.__dxx_pch_inclusion_count:
					f.write('#include "%s"\t/* %u */\n' % (name, count))
					env.Depends(target, name)
			f.write('/* END PCH GENERATED FILE */\n')

	def create_header_targets(self):
		fs = SCons.Node.FS.get_default_fs()
		builddir = self.user_settings.builddir
		check_header_includes = os.path.join(builddir, 'check_header_includes.cpp')
		if not self.__shared_header_file_list:
			open(check_header_includes, 'wt')
			headers = Git.pcall(['ls-files', '-z', '--', '*.h'], stdout=subprocess.PIPE).out
			excluded_directories = (
				'common/arch/cocoa/',
				'common/arch/carbon/',
			)
			self.__shared_header_file_list.extend([h for h in headers.split('\0') if h and not h.startswith(excluded_directories)])
		dirname = os.path.join(builddir, self.srcdir)
		kwargs = {
			'CXXFLAGS' : self.env['CXXFLAGS'][:],
			'source' : check_header_includes
		}
		Depends = self.env.Depends
		StaticObject = self.env.StaticObject
		OBJSUFFIX = self.env['OBJSUFFIX']
		for name in self.__shared_header_file_list:
			if not name:
				continue
			if self.srcdir == 'common' and not name.startswith('common/'):
				# Skip game-specific headers when testing common
				continue
			if self.srcdir[0] == 'd' and name[0] == 'd' and not name.startswith(self.srcdir):
				# Skip d1 in d2 and d2 in d1
				continue
			CPPFLAGS = self.env['CPPFLAGS'][:]
			if name[:24] == 'common/include/compiler-':
				CPPFLAGS.extend(['-include', 'dxxsconf.h'])
			CPPFLAGS.extend(['-include', name])
			if self.user_settings.verbosebuild:
				kwargs['CXXCOMSTR'] = "Checking %s %s %s" % (self.target, builddir, name)
			Depends(StaticObject(target=os.path.join('%s/chi/%s%s' % (dirname, name, OBJSUFFIX)), CPPFLAGS=CPPFLAGS, **kwargs), fs.File(name))

	def create_pch_node(self,dirname,configure_pch_flags):
		if self.user_settings.check_header_includes:
			self.create_header_targets()
		if not configure_pch_flags:
			self.env._dxx_pch_node = None
			return
		dirname = os.path.join(self.user_settings.builddir, dirname)
		target = os.path.join(dirname, 'pch.h.gch')
		source = os.path.join(dirname, 'pch.cpp')
		self.env._dxx_pch_node = self.env.StaticObject(target=target, source=source, CXXFLAGS=self.env['CXXFLAGS'] + configure_pch_flags['CXXFLAGS'])
		self.env.Append(CXXFLAGS = ['-include', str(self.env._dxx_pch_node[0])[:-4], '-Winvalid-pch'])
		self.env.__dxx_pch_candidates = {}
		self.env.__dxx_pch_inclusion_count = int(self.user_settings.pch)
		self.env['BUILDERS']['StaticObject'].add_emitter('.cpp', self._collect_pch_candidates)
		self.env.Command(source, None, self.write_pch_inclusion_file)

	def _quote_cppdefine(self,s,f=repr):
		r = ''
		prior = False
		for c in f(s):
			# No xdigit support in str
			if c in ' ()*+,-./:=[]_' or (c.isalnum() and not (prior and (c.isdigit() or c in 'abcdefABCDEF'))):
				r += c
			elif c == '\n':
				r += r'\n'
			else:
				r += '\\\\x' + binascii.b2a_hex(c)
				prior = True
				continue
			prior = False
		return '\\"' + r + '\\"'

	def prepare_environment(self):
		# Prettier build messages......
		# Move target to end of C++ source command
		target_string = ' -o $TARGET'
		cxxcom = self.env['CXXCOM']
		if target_string + ' ' in cxxcom:
			cxxcom = cxxcom.replace(target_string, '') + target_string
		# Add ccache/distcc only for compile, not link
		if self.user_settings.ccache:
			cxxcom = self.user_settings.ccache + ' ' + cxxcom
			if self.user_settings.distcc:
				self.env['ENV']['CCACHE_PREFIX'] = self.user_settings.distcc
		elif self.user_settings.distcc:
			cxxcom = self.user_settings.distcc + ' ' + cxxcom
		self.env['CXXCOM'] = cxxcom
		# Move target to end of link command
		linkcom = self.env['LINKCOM']
		if target_string + ' ' in linkcom:
			linkcom = linkcom.replace(target_string, '') + target_string
		# Add $CXXFLAGS to link command
		cxxflags = '$CXXFLAGS '
		if ' ' + cxxflags not in linkcom:
			linkflags = '$LINKFLAGS'
			linkcom = linkcom.replace(linkflags, cxxflags + linkflags)
		self.env['LINKCOM'] = linkcom
		# Custom DISTCC_HOSTS per target
		distcc_hosts = self.user_settings.distcc_hosts
		if distcc_hosts is not None:
			self.env['ENV']['DISTCC_HOSTS'] = distcc_hosts
		if (self.user_settings.verbosebuild == 0):
			builddir = self.user_settings.builddir if self.user_settings.builddir != '' else '.'
			self.env["CXXCOMSTR"]    = "Compiling %s %s $SOURCE" % (self.target, builddir)
			self.env["LINKCOMSTR"]   = "Linking %s $TARGET" % self.target

		# Use -Wundef to catch when a shared source file includes a
		# shared header that misuses conditional compilation.  Use
		# -Werror=undef to make this fatal.  Both are needed, since
		# gcc 4.5 silently ignores -Werror=undef.  On gcc 4.5, misuse
		# produces a warning.  On gcc 4.7, misuse produces an error.
		Werror = get_Werror_string(self.user_settings.CXXFLAGS)
		self.env.Prepend(CXXFLAGS = ['-Wall', Werror + 'extra', Werror + 'missing-declarations', Werror + 'pointer-arith', Werror + 'undef', Werror + 'missing-braces', Werror + 'unused', Werror + 'format-security', Werror + 'redundant-decls'])
		self.env.Append(CXXFLAGS = ['-funsigned-char'])
		self.env.Append(CPPPATH = ['common/include', 'common/main', '.', self.user_settings.builddir])
		self.env.Append(CPPFLAGS = SCons.Util.CLVar('-Wno-sign-compare'))
		if (self.user_settings.editor == 1):
			self.env.Append(CPPPATH = ['common/include/editor'])
		# Get traditional compiler environment variables
		for cc in ('CXX', 'RC',):
			value = getattr(self.user_settings, cc)
			if value is not None:
				self.env[cc] = value
		for flags in ['CPPFLAGS', 'CXXFLAGS', 'LIBS']:
			value = getattr(self.user_settings, flags)
			if value is not None:
				self.env.Append(**{flags : SCons.Util.CLVar(value)})
		if self.user_settings.LDFLAGS:
			self.env.Append(LINKFLAGS = SCons.Util.CLVar(self.user_settings.LDFLAGS))
		if self.user_settings.lto:
			f = ['-flto', '-fno-fat-lto-objects']
			self.env.Append(CXXFLAGS = f)

	def check_endian(self):
		# set endianess
		if (self.__endian == "big"):
			message(self, "BigEndian machine detected")
			self.env.Append(CPPDEFINES = ['WORDS_BIGENDIAN'])
		elif (self.__endian == "little"):
			message(self, "LittleEndian machine detected")

	def check_platform(self):
		# windows or *nix?
		platform_name = self.user_settings.host_platform
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

		# debug?
		if not self.user_settings.debug:
			env.Append(CPPDEFINES = ['NDEBUG', 'RELEASE'])
		env.Prepend(CXXFLAGS = ['-g', '-O2'])
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
			message(self, "Raspberry Pi: using VideoCore libs in \"%s\"" % self.user_settings.rpi_vc_path)
			env.Append(CPPDEFINES = ['RPI', 'WORDS_NEED_ALIGNMENT'])
			# use CPPFLAGS -isystem instead of CPPPATH because these those header files
			# are not very clean and would trigger some warnings we usually consider as
			# errors. Using them as system headers will make gcc ignoring any warnings.
			env.Append(CPPFLAGS = [
				'-isystem='+self.user_settings.rpi_vc_path+'/include',
				'-isystem='+self.user_settings.rpi_vc_path+'/include/interface/vcos/pthreads',
				'-isystem='+self.user_settings.rpi_vc_path+'/include/interface/vmcs_host/linux'])
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
'3d/clipper.cpp',
'texmap/tmapflat.cpp'
]
])
	# for ogl
	objects_arch_ogl = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'arch/ogl/ogl_extensions.cpp',
'arch/ogl/ogl_sync.cpp'
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
	class DarwinPlatformSettings(LazyObjectConstructor, DXXCommon.DarwinPlatformSettings):
		platform_objects = LazyObjectConstructor.create_lazy_object_property([
			'common/arch/cocoa/messagebox.mm',
			'common/arch/cocoa/SDLMain.m'
		])

		def __init__(self, program, user_settings):
			LazyObjectConstructor.__init__(self)
			DXXCommon.DarwinPlatformSettings.__init__(self, program, user_settings)
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
		self.create_pch_node(self.srcdir, self.configure_pch_flags)

	def configure_environment(self):
		fs = SCons.Node.FS.get_default_fs()
		builddir = fs.Dir(self.user_settings.builddir or '.')
		tests = ConfigureTests(self.program_message_prefix, self.user_settings, self.platform_settings)
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
		self.configure_added_environment_flags = tests.successful_flags
		self.configure_pch_flags = None
		if not conf.env:
			return
		try:
			for k in tests.custom_tests:
				getattr(conf, k)()
		except SCons.Errors.StopError as e:
			raise SCons.Errors.StopError(e.args[0] + '  See {log_file} for details.'.format(log_file=log_file), *e.args[1:])
		self.env = conf.Finish()
		self.configure_pch_flags = tests.pch_flags

class DXXProgram(DXXCommon):
	# version number
	VERSION_MAJOR = 0
	VERSION_MINOR = 58
	VERSION_MICRO = 1
	static_archive_construction = {}
	# None when unset.  Tuple of one once cached.
	_computed_extra_version = None
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
'main/ai.cpp',
'main/aipath.cpp',
'main/automap.cpp',
'main/bm.cpp',
'main/cntrlcen.cpp',
'main/collide.cpp',
'main/config.cpp',
'main/console.cpp',
'main/controls.cpp',
'main/credits.cpp',
'main/digiobj.cpp',
'main/effects.cpp',
'main/endlevel.cpp',
'main/fireball.cpp',
'main/fuelcen.cpp',
'main/fvi.cpp',
'main/game.cpp',
'main/gamecntl.cpp',
'main/gamefont.cpp',
'main/gamemine.cpp',
'main/gamerend.cpp',
'main/gamesave.cpp',
'main/gameseg.cpp',
'main/gameseq.cpp',
'main/gauges.cpp',
'main/hostage.cpp',
'main/hud.cpp',
'main/iff.cpp',
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
'main/piggy.cpp',
'main/player.cpp',
'main/playsave.cpp',
'main/polyobj.cpp',
'main/powerup.cpp',
'main/render.cpp',
'main/robot.cpp',
'main/scores.cpp',
'main/segment.cpp',
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
'misc/hash.cpp',
'misc/physfsx.cpp',
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
'editor/group.cpp',
'editor/info.cpp',
'editor/kbuild.cpp',
'editor/kcurve.cpp',
'editor/kfuncs.cpp',
'editor/kgame.cpp',
'editor/khelp.cpp',
'editor/kmine.cpp',
'editor/ksegmove.cpp',
'editor/ksegsel.cpp',
'editor/ksegsize.cpp',
'editor/ktmap.cpp',
'editor/kview.cpp',
'editor/med.cpp',
'editor/meddraw.cpp',
'editor/medmisc.cpp',
'editor/medrobot.cpp',
'editor/medsel.cpp',
'editor/medwall.cpp',
'editor/mine.cpp',
'editor/objpage.cpp',
'editor/segment.cpp',
'editor/seguvs.cpp',
'editor/texpage.cpp',
'editor/texture.cpp',
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
			self.platform_objects = self.platform_objects[:]
		def adjust_environment(self,program,env):
			DXXCommon.DarwinPlatformSettings.adjust_environment(self, program, env)
			VERSION = str(program.VERSION_MAJOR) + '.' + str(program.VERSION_MINOR)
			if (program.VERSION_MICRO):
				VERSION += '.' + str(program.VERSION_MICRO)
			env['VERSION_NUM'] = VERSION
			env['VERSION_NAME'] = program.PROGRAM_NAME + ' v' + VERSION
	# Settings to apply to Linux builds
	class LinuxPlatformSettings(DXXCommon.LinuxPlatformSettings):
		def __init__(self,program,user_settings):
			DXXCommon.LinuxPlatformSettings.__init__(self,program,user_settings)
			if user_settings.sharepath and user_settings.sharepath[-1] != '/':
				user_settings.sharepath += '/'

	@property
	def objects_common(self):
		objects_common = self.__objects_common
		if (self.user_settings.use_udp == 1):
			objects_common = objects_common + self.objects_use_udp
		return objects_common + self.platform_settings.platform_objects
	def __init__(self,prefix,variables):
		self.variables = variables
		self._argument_prefix_list = prefix
		DXXCommon.__init__(self)
		self.banner()
		self.user_settings = self.UserSettings(program=self)
		self.user_settings.register_variables(prefix=prefix, variables=self.variables)

	def init(self,substenv):
		self.user_settings.read_variables(self.variables, substenv)
		if not DXXProgram.static_archive_construction.has_key(self.user_settings.builddir):
			DXXProgram.static_archive_construction[self.user_settings.builddir] = DXXArchive(self.user_settings)
		self.check_platform()
		self.prepare_environment()
		self.check_endian()
		self.process_user_settings()
		self.register_program()

	def prepare_environment(self):
		DXXCommon.prepare_environment(self)
		archive = DXXProgram.static_archive_construction[self.user_settings.builddir]
		self.env.MergeFlags(archive.configure_added_environment_flags)
		self.create_pch_node(self.srcdir, archive.configure_pch_flags)
		self.env.Append(CPPDEFINES = [('DXX_VERSION_SEQ', ','.join([str(self.VERSION_MAJOR), str(self.VERSION_MINOR), str(self.VERSION_MICRO)]))])
		# For PRIi64
		self.env.Append(CPPDEFINES = [('__STDC_FORMAT_MACROS',)])
		self.env.Append(CPPPATH = [os.path.join(self.srcdir, f) for f in ['include', 'main']])

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
		env.Append(LIBS = ['m'])

	def process_user_settings(self):
		DXXCommon.process_user_settings(self)
		env = self.env
		# opengl or software renderer?

		# profiler?
		if (self.user_settings.profiler == 1):
			env.Append(LINKFLAGS = '-pg')

		#editor build?
		if (self.user_settings.editor == 1):
			env.Append(CPPPATH = [os.path.join(self.srcdir, 'include/editor')])

		env.Append(CPPDEFINES = [('SHAREPATH', '\\"' + str(self.user_settings.sharepath) + '\\"')])

	def register_program(self):
		self._register_program(self.shortname)

	@classmethod
	def compute_extra_version(cls):
		c = cls._computed_extra_version
		if c is None:
			s = ds = None
			v = cls._compute_extra_version()
			if v:
				s = Git.spcall(['status', '--short', '--branch'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
				ds = Git.spcall(['diff', '--stat', 'HEAD'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
			cls._computed_extra_version = c = (v or '', s, ds)
		return c

	@classmethod
	def _compute_extra_version(cls):
		try:
			g = Git.pcall(['describe', '--tags', '--abbrev=8'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		except OSError as e:
			if e.errno == errno.ENOENT:
				return None
			raise
		if g.returncode:
			return None
		c = Git.pcall(['diff', '--quiet', '--cached'], stdout=subprocess.PIPE, stderr=subprocess.PIPE).returncode
		d = Git.pcall(['diff', '--quiet'], stdout=subprocess.PIPE, stderr=subprocess.PIPE).returncode
		return g.out.split('\n')[0] + ('+' if c else '') + ('*' if d else '')

	def _register_program(self,dxxstr,program_specific_objects=[]):
		env = self.env
		exe_target = os.path.join(self.srcdir, self.target)
		static_archive_construction = self.static_archive_construction[self.user_settings.builddir]
		objects = static_archive_construction.objects_common[:]
		objects.extend(self.objects_common)
		objects.extend(program_specific_objects)
		if (self.user_settings.sdlmixer == 1):
			objects.extend(static_archive_construction.objects_arch_sdlmixer)
			objects.extend(self.objects_similar_arch_sdlmixer)
		if (self.user_settings.opengl == 1) or (self.user_settings.opengles == 1):
			env.Append(LIBS = self.platform_settings.ogllibs)
			objects.extend(static_archive_construction.objects_arch_ogl)
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
		versid_build_environ = ['CXX', 'CPPFLAGS', 'CXXFLAGS', 'LINKFLAGS']
		versid_cppdefines = env['CPPDEFINES'][:]
		versid_cppdefines.extend([('DESCENT_%s' % k, self._quote_cppdefine(env.get(k, ''))) for k in versid_build_environ])
		v = StaticSubprocess.pcall(env['CXX'].split(' ') + ['--version'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		if not v.returncode and (v.out or v.err):
			v = (v.out or v.err).split('\n')[0]
			versid_cppdefines.append(('DESCENT_%s' % 'CXX_version', self._quote_cppdefine(v)))
			versid_build_environ.append('CXX_version')
		extra_version = self.user_settings.extra_version
		if extra_version is None:
			extra_version = 'v%u.%u' % (self.VERSION_MAJOR, self.VERSION_MINOR)
			if self.VERSION_MICRO:
				extra_version += '.%u' % self.VERSION_MICRO
		git_describe_version = (self.compute_extra_version() if self.user_settings.git_describe_version else ('', '', ''))
		if git_describe_version[0] and not (extra_version and (extra_version == git_describe_version[0] or (extra_version[0] == 'v' and extra_version[1:] == git_describe_version[0]))):
			# Suppress duplicate output
			if extra_version:
				extra_version += ' '
			extra_version += git_describe_version[0]
		if extra_version:
			versid_cppdefines.append(('DESCENT_VERSION_EXTRA', self._quote_cppdefine(extra_version, f=str)))
		versid_cppdefines.append(('DESCENT_git_status', self._quote_cppdefine(git_describe_version[1])))
		versid_build_environ.append('git_status')
		versid_cppdefines.append(('DESCENT_git_diffstat', self._quote_cppdefine(git_describe_version[2])))
		versid_build_environ.append('git_diffstat')
		versid_cppdefines.append(('DXX_RBE"(A)"', "'" + ''.join(['A(%s)' % k for k in versid_build_environ]) + "'"))
		versid_environ = self.env['ENV'].copy()
		# Direct mode conflicts with __TIME__
		versid_environ['CCACHE_NODIRECT'] = 1
		versid_objlist = [self.env.StaticObject(target='%s%s%s' % (self.user_settings.builddir, self._apply_target_name(s), self.env["OBJSUFFIX"]), source=s, CPPDEFINES=versid_cppdefines, ENV=versid_environ) for s in ['similar/main/vers_id.cpp']]
		if self.user_settings.versid_depend_all:
			env.Depends(versid_objlist[0], objects)
		if env._dxx_pch_node:
			env.Depends(versid_objlist[0], env._dxx_pch_node)
		objects.extend(versid_objlist)
		# finally building program...
		exe_node = env.Program(target=os.path.join(self.user_settings.builddir, str(exe_target)), source = self.sources + objects)
		if self.user_settings.host_platform != 'darwin':
			if self.user_settings.register_install_target:
				install_dir = (self.user_settings.DESTDIR or '') + self.user_settings.BIN_DIR
				env.Install(install_dir, exe_node)
				env.Alias('install', install_dir)
		else:
			syspath = sys.path[:]
			cocoa = 'common/arch/cocoa'
			sys.path += [cocoa]
			import tool_bundle
			sys.path = syspath
			tool_bundle.TOOL_BUNDLE(env)
			env.MakeBundle(os.path.join(self.user_settings.builddir, self.PROGRAM_NAME + '.app'), exe_node,
					'free.%s-rebirth' % dxxstr, os.path.join(cocoa, 'Info.plist'),
					typecode='APPL', creator='DCNT',
					icon_file=os.path.join(cocoa, '%s-rebirth.icns' % dxxstr),
					resources=[[os.path.join(self.srcdir, s), s] for s in ['English.lproj/InfoPlist.strings']])

	def GenerateHelpText(self):
		return self.variables.GenerateHelpText(self.env)

class D1XProgram(DXXProgram):
	PROGRAM_NAME = 'D1X-Rebirth'
	target = 'd1x-rebirth'
	srcdir = 'd1x-rebirth'
	shortname = 'd1x'
	def prepare_environment(self):
		DXXProgram.prepare_environment(self)
		# Flags and stuff for all platforms...
		self.env.Append(CPPDEFINES = ['DXX_BUILD_DESCENT_I'])

	# general source files
	__objects_common = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join(srcdir, f) for f in [
'main/bmread.cpp',
'main/custom.cpp',
'main/snddecom.cpp',
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
'main/hostage.cpp',
'editor/ehostage.cpp',
]
],
	}])
	@property
	def objects_editor(self):
		return self.__objects_editor + DXXProgram.objects_editor.fget(self)

class D2XProgram(DXXProgram):
	PROGRAM_NAME = 'D2X-Rebirth'
	target = 'd2x-rebirth'
	srcdir = 'd2x-rebirth'
	shortname = 'd2x'
	def prepare_environment(self):
		DXXProgram.prepare_environment(self)
		# Flags and stuff for all platforms...
		self.env.Append(CPPDEFINES = ['DXX_BUILD_DESCENT_II'])

	# general source files
	__objects_common = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join(srcdir, f) for f in [
'libmve/decoder8.cpp',
'libmve/decoder16.cpp',
'libmve/mve_audio.cpp',
'libmve/mvelib.cpp',
'libmve/mveplay.cpp',
'main/escort.cpp',
'main/gamepal.cpp',
'main/movie.cpp',
'misc/physfsrwops.cpp',
]
],
	}])
	@property
	def objects_common(self):
		return self.__objects_common + DXXProgram.objects_common.fget(self)

	# for editor
	__objects_editor = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join(srcdir, f) for f in [
'main/bmread.cpp',
]
],
	}])
	@property
	def objects_editor(self):
		return self.__objects_editor + DXXProgram.objects_editor.fget(self)

variables = Variables([v for (k,v) in ARGLIST if k == 'site'] or ['site-local.py'], ARGUMENTS)
filtered_help = FilterHelpText()
variables.FormatVariableHelpText = filtered_help.FormatVariableHelpText
def _filter_duplicate_prefix_elements(e,s):
	r = e not in s
	s.add(e)
	return r
def register_program(program,other_program):
	s = program.shortname
	import itertools
	l = [v for (k,v) in ARGLIST if k == s or k == 'dxx'] or [other_program.shortname not in ARGUMENTS]
	# Fallback case: build the regular configuration.
	if len(l) == 1:
		try:
			if int(l[0]):
				return [program((s,''), variables)]
			return []
		except ValueError:
			# If not an integer, treat this as a configuration profile.
			pass
	r = []
	seen = set()
	for e in l:
		for prefix in itertools.product(*[v.split('+') for v in e.split(',')]):
			duplicates = set()
			prefix = tuple(p for p in prefix if _filter_duplicate_prefix_elements(p, duplicates))
			if prefix in seen:
				continue
			seen.add(prefix)
			prefix = ['%s%s%s' % (s, '_' if p else '', p) for p in prefix] + list(prefix)
			r.append(program(prefix, variables))
	return r
d1x = register_program(D1XProgram, D2XProgram)
d2x = register_program(D2XProgram, D1XProgram)

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
	dxx=VALUE        Equivalent to d1x=VALUE d2x=VALUE
"""
substenv = SCons.Environment.SubstitutionEnvironment()
variables.Update(substenv)
for d in d1x + d2x:
	d.init(substenv)
	h += d.PROGRAM_NAME + ('.%d:\n' % d.program_instance) + d.GenerateHelpText()
Help(h)
unknown = variables.UnknownVariables()
# Delete known unregistered variables
unknown.pop('d1x', None)
unknown.pop('d2x', None)
unknown.pop('dxx', None)
unknown.pop('site', None)
ignore_unknown_variables = unknown.pop('ignore_unknown_variables', '0')
if unknown:
	try:
		ignore_unknown_variables = int(ignore_unknown_variables)
	except ValueError:
		ignore_unknown_variables = False
	if not ignore_unknown_variables:
		raise SCons.Errors.StopError('Unknown values specified on command line.' +
''.join(['\n\t%s' % k for k in unknown.keys()]) +
'\nRemove unknown values or set ignore_unknown_variables=1 to continue.')

#EOF
