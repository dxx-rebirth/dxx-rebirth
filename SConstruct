#SConstruct
# vim: set fenc=utf-8 sw=4 ts=4 :
# $Format:%H$

# needed imports
from collections import (defaultdict, Counter as collections_counter)
import collections.abc
from dataclasses import dataclass
import base64
import binascii
import errno
import itertools
import shlex
import subprocess
import sys
import typing
import os
import SCons.Util
import SCons.Script
from functools import cached_property

# Disable injecting tools into default namespace
SCons.Defaults.DefaultEnvironment(tools = [])

def message(program, msg: str):
	print(f'{program.program_message_prefix}: {msg}')

def get_Werror_sequence(active_cxxflags: list[str], warning_flags: collections.abc.Sequence[str]) -> list[str]:
	# The leading -W is passed in each element in warning_flags so that if
	# -Werror is already set, the warning_flags can be returned as-is.  This
	# also means that the warning flag is written in the caller as it will be
	# on the command line, so users who grep for `-Wfoo` will find where
	# `-Wfoo` is set.  If the `-W` were composed in this function and the
	# caller passed `foo`, then a search for `-Wfoo` would find nothing.
	if active_cxxflags and '-Werror' in active_cxxflags:
		# Make both paths return a Python list.
		return list(warning_flags)
	# When an option is converted to `-Werror=foo` form, the leading `-W` from
	# the caller must be stripped off to avoid producing `-Werror=-Wfoo`.
	return [f'-Werror={warning[2:]}' for warning in warning_flags]

class StaticSubprocess:
	# This class contains utility functions for calling subprocesses
	# that are expected to return the same output for every call.  The
	# output is cached after the first call, so that callers can request
	# the output again later without causing the program to be run again.
	#
	# Suitable programs are not required to depend solely on the
	# parameters, but are assumed to depend only on state that is
	# unlikely to change in the brief time that SConstruct runs.  For
	# example, a tool might be upgraded by the system administrator,
	# changing its version string.  However, we assume nobody upgrades
	# their tools in the middle of an SConstruct run.  Results are not
	# cached to persistent storage, so an upgrade performed after one
	# SConstruct run, but before the next, will not cause any
	# inconsistencies.
	shlex_split = shlex.split

	@dataclass(eq=False)
	class CachedCall:
		out: bytes
		err: bytes
		returncode: int

	@dataclass(eq=False)
	class CachedVersionCall:
		version_head: str
		def __init__(self, out: bytes, err: bytes, returncode: int):
			oe = out or err
			self.version_head = oe.splitlines()[0].decode() if not returncode and oe else None

	# @staticmethod delayed so that default arguments pick up the
	# undecorated form.
	def pcall(args: list[bytes | str], stderr=None, CachedCall=CachedCall, _call_cache: dict[str, CachedCall]={}) -> CachedCall:
		# Use repr since callers may construct the same argument
		# list independently.
		## >>> a = ['git', '--version']
		## >>> b = ['git', '--version']
		## >>> a is b
		## False
		## >>> id(a) == id(b)
		## False
		## >>> a == b
		## True
		## >>> repr(a) == repr(b)
		## True
		a = repr(args)
		c = _call_cache.get(a)
		if c is not None:
			return c
		p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=stderr)
		(o, e) = p.communicate()
		_call_cache[a] = c = CachedCall(o, e, p.wait())
		return c
	def qcall(args: str, stderr=None, _pcall=pcall, _shlex_split=shlex_split) -> CachedCall:
		return _pcall(_shlex_split(args),stderr)
	@classmethod
	def get_version_head(cls, cmd: bytes | str, _pcall=pcall, _shlex_split=shlex_split) -> str:
		# If cmd is bytes, then it is output from a program and should
		# not be reparsed.
		return _pcall(([cmd] if isinstance(cmd, bytes) else _shlex_split(cmd)) + ['--version'], stderr=subprocess.PIPE, CachedCall=cls.CachedVersionCall).version_head
	pcall = staticmethod(pcall)
	qcall = staticmethod(qcall)
	shlex_split = staticmethod(shlex_split)

class ToolchainInformation(StaticSubprocess):
	@staticmethod
	def get_tool_path(env: SCons.Environment, tool: str, _qcall=StaticSubprocess.qcall):
		# Include $LINKFLAGS since -fuse-ld=gold influences the path
		# printed for the linker.
		tool = env.subst(f'$CXX $CXXFLAGS $LINKFLAGS -print-prog-name={tool}')
		return tool, _qcall(tool).out.strip()
	@staticmethod
	def show_partial_environ(env: SCons.Environment, user_settings, f: collections.abc.Callable[[str], None], msgprefix: str, append_newline='\n'):
		f(f'{msgprefix}: CHOST: {(user_settings.CHOST)!r}{append_newline}')
		for v in (
			'CXX',
			'CPPDEFINES',
			'CPPPATH',
			'CPPFLAGS',
			'CXXFLAGS',
			'LIBS',
			'LINKFLAGS',
		) + (
			(
				'RC',
				'RCFLAGS',
			) if user_settings.host_platform == 'win32' else (
				'FRAMEWORKPATH',
				'FRAMEWORKS',
			) if user_settings.host_platform == 'darwin' else (
			)
		):
			f(f'{msgprefix}: {v}: {(env.get(v, None))!r}{append_newline}')
		penv = env['ENV']
		for v in (
			'CCACHE_PREFIX',
			'DISTCC_HOSTS',
		):
			f(f'{msgprefix}: ${v}: {(penv.get(v, None))!r}{append_newline}')

class Git(StaticSubprocess):
	@dataclass(eq=False)
	class ComputedExtraVersion:
		describe: str
		status: str
		diffstat_HEAD: str
		revparse_HEAD: str
		__slots__ = ('describe', 'status', 'diffstat_HEAD', 'revparse_HEAD')
	UnknownExtraVersion = (
		# If the string is alphanumeric, then `git archive` has rewritten the
		# string to be a commit ID.  Use that commit ID as a guessed default
		# when Git is not available to resolve a current commit ID.
		ComputedExtraVersion('$Format:%(describe:tags,abbrev=12)$', None, None, 'archive_$Format:%H$') if '$Format:%H$'.isalnum() else
		# Otherwise, assume that this is a checked-in copy.
		ComputedExtraVersion(None, None, None, None)
		)
	# None until computed, then ComputedExtraVersion once cached.
	__computed_extra_version: typing.ClassVar[None | ComputedExtraVersion] = None
	__path_git: typing.ClassVar[None | list[str]] = None
	# `__pcall_missing_git`, `__pcall_found_git`, and `pcall` must have
	# compatible signatures.  In any given run, there will be at most
	# one call to `pcall`, which then patches itself out of the call
	# chain.  A run will call either __pcall_missing_git or
	# __pcall_found_git (depending on whether $GIT is blank), but never
	# both in the same run.
	@classmethod
	def __pcall_missing_git(cls, args: list[str], stderr=None, _missing_git=StaticSubprocess.CachedCall(None, None, 1)):
		return _missing_git
	@classmethod
	def __pcall_found_git(cls, args: list[str], stderr=None, _pcall=StaticSubprocess.pcall):
		return _pcall(cls.__path_git + args, stderr=stderr)
	@classmethod
	def pcall(cls, args: list[str], stderr=None):
		git = cls.__path_git
		if git is None:
			cls.__path_git = git = cls.shlex_split(os.environ.get('GIT', 'git'))
		cls.pcall = f = cls.__pcall_found_git if git else cls.__pcall_missing_git
		return f(args, stderr)
	def spcall(cls, args: list[str], stderr=None):
		g = cls.pcall(args, stderr)
		if g.returncode:
			return None
		return g.out
	@classmethod
	def compute_extra_version(cls, _spcall=spcall) -> ComputedExtraVersion:
		c = cls.__computed_extra_version
		if c is None:
			v = cls.__compute_extra_version()
			cls.__computed_extra_version = c = cls.ComputedExtraVersion(
				v,
				_spcall(cls, ['status', '--short', '--branch']).decode(),
				_spcall(cls, ['diff', '--stat', 'HEAD']).decode(),
				_spcall(cls, ['rev-parse', 'HEAD']).rstrip().decode(),
			) if v is not None else cls.UnknownExtraVersion
		return c
	# Run `git describe --tags --abbrev=12`.
	# On failure, return None.
	# On success, return a string containing:
	#	first line of output of describe
	#	'*' if there are unstaged changes else ''
	#	'+' if there are staged changes else ''
	@classmethod
	def __compute_extra_version(cls) -> None | str:
		try:
			g = cls.pcall(['describe', '--tags', '--abbrev=12'], stderr=subprocess.PIPE)
		except OSError as e:
			if e.errno == errno.ENOENT:
				return None
			raise
		if g.returncode:
			return None
		_pcall = cls.pcall
		return (g.out.splitlines()[0] +	\
			(b'*' if _pcall(['diff', '--quiet']).returncode else b'') +	\
			(b'+' if _pcall(['diff', '--quiet', '--cached']).returncode else b'')
		).decode()

class ConfigureTests:
	@dataclass(eq=False)
	class _Collector:
		record: collections.abc.Callable[['ConfigureTest'], None]

		@dataclass(eq=False, slots=True)
		class RecordedTest:
			name: str
			desc: str
			# predicate is a sequence of zero-or-more functions that take a
			# UserSettings object as the only argument.  A recorded test is
			# only passed to the SConf logic if at most zero predicates return
			# a false-like value.
			#
			# Callers use this to exclude from execution tests which make no
			# sense in the current environment, such as a Windows-specific test
			# when building for a Linux target.
			predicate: tuple[collections.abc.Callable[['UserSettings'], bool]] = ()

		def __call__(self, f: collections.abc.Callable) -> collections.abc.Callable:
			desc = None
			doc = getattr(f, '__doc__', None)
			if doc is not None:
				doc = doc.rstrip().splitlines()
				if doc and doc[-1].startswith("help:"):
					desc = doc[-1][5:]
			self.record(self.RecordedTest(f.__name__, desc))
			return f

	class Collector(_Collector):
		tests: collections.abc.MutableSequence[collections.abc.Callable[['ConfigureTest'], None]]
		def __init__(self):
			# An unguarded collector maintains a list of tests, and adds
			# to the list as the decorator is called on individual
			# functions.
			self.tests = tests = []
			super().__init__(tests.append)

	class GuardedCollector(_Collector):
		__guard: tuple[collections.abc.Callable[['UserSettings'], bool]]
		def __init__(self, collector, guard: collections.abc.Callable[['UserSettings'], bool]):
			# A guarded collector delegates to the list maintained by
			# the input collector, instead of keeping a list of its own.
			super().__init__(collector.record)
			# `guard` is a single function, but the predicate handling
			# expects a sequence of functions, so store a single-element
			# tuple.
			self.__guard = guard,
			self.__RecordedTest = collector.RecordedTest
		def RecordedTest(self, name: str, desc: str, predicate=()):
			# Record a test with both the guard of this GuardedCollector
			# and any guards from the input collector objects, which may
			# in turn be instances of GuardedCollector with predicates
			# of their own.
			return self.__RecordedTest(name, desc, self.__guard + predicate)

	class CxxRequiredFeature:
		__slots__ = ('main', 'name', 'text')
		name: str
		text: str
		main: str
		def __init__(self, name: str, text: str, main: str = ''):
			self.name = name
			# Avoid generating consecutive underscores if the input
			# string has multiple adjacent unacceptable characters.
			# Pass alphanumeric characters through as-is.  Escape
			# non-alphanumeric by converting them to the hexadecimal
			# representation of their ordinal value.
			name = {'N' : f'test_{"".join(c if c.isalnum() else f"_{(ord(c)):x}" for c in name)}'}
			self.text = text % name
			self.main = f'{{{(main % name)}}}\n' if main else ''
	class Cxx11RequiredFeature(CxxRequiredFeature):
		std: int = 11
	class Cxx14RequiredFeature(CxxRequiredFeature):
		std: int = 14
	class Cxx17RequiredFeature(CxxRequiredFeature):
		std: int = 17
	class Cxx20RequiredFeature(CxxRequiredFeature):
		std: int = 20
	class CxxRequiredFeatures:
		__slots__ = ('features', 'main', 'text')
		features: list['CxxRequiredFeature']
		main: str
		text: str
		def __init__(self, features: list['CxxRequiredFeature']):
			self.features = features
			s = '/* C++{} {} */\n{}'.format
			self.main = '\n'.join((s(f.std, f.name, f.main) for f in features if f.main))
			self.text = '\n'.join((s(f.std, f.name, f.text) for f in features if f.text))
	class PCHAction:
		def __init__(self,context):
			self._context = context
		def __call__(self,text,ext):
			# Ignore caller-supplied text, since _Test always includes a
			# definition of main().  Using the caller-supplied text
			# would provide one main() in the PCH and another in the
			# test which includes the PCH.
			env = self._context.env
			s = env['OBJSUFFIX']
			env['OBJSUFFIX'] = '.h.gch'
			result = self._context.TryCompile('''
/* Define this here.  Use it in the file which includes the PCH.  If the
 * compiler skips including the PCH, and does not fail for that reason
 * alone, then it will fail when the symbol is used in the later test,
 * since the only definition comes from the PCH.
 */
#define dxx_compiler_supports_pch
			''', ext)
			env['OBJSUFFIX'] = s
			return result
	class PreservedEnvironment:
		# One empty list for all the defaults.  The comprehension
		# creates copies, so it is safe for the default value to be
		# shared.
		def __init__(self, env: SCons.Environment, keyviews, _l=[]):
			self.flags = {k: env.get(k, _l).copy() for k in itertools.chain.from_iterable(keyviews)}
		def restore(self, env: SCons.Environment):
			env.Replace(**self.flags)
		def __getitem__(self, name: str):
			return self.flags.__getitem__(name)
	class ForceVerboseLog(PreservedEnvironment):
		def __init__(self, env: SCons.Environment):
			# Force verbose output to sconf.log
			self.flags = {}
			for k in (
				'CXXCOMSTR',
				'LINKCOMSTR',
			):
				try:
					# env is like a dict, but does not have .pop(), so
					# emulate it with a lookup + delete.
					self.flags[k] = env[k]
					del env[k]
				except KeyError:
					pass
	class pkgconfig:
		def _get_pkg_config_exec_path(context, msgprefix: str, pkgconfig: str):
			Display = context.Display
			if not pkgconfig:
				Display(f'{msgprefix}: pkg-config disabled by user settings\n')
				return pkgconfig
			if os.sep in pkgconfig:
				# Split early, so that the user can pass a command with
				# arguments.  Convert to tuple so that the value is not
				# modified later.
				pkgconfig = tuple(StaticSubprocess.shlex_split(pkgconfig))
				Display(f'{msgprefix}: using pkg-config at user specified path {pkgconfig!r}\n')
				return pkgconfig
			join = os.path.join
			# No path specified, search in $PATH
			for p in os.environ.get('PATH', '').split(os.pathsep):
				fp = join(p, pkgconfig)
				try:
					os.close(os.open(fp, os.O_RDONLY))
				except OSError as e:
					# Ignore on permission errors.  If pkg-config is
					# runnable but not readable, the user must
					# specify its path.
					if e.errno == errno.ENOENT or e.errno == errno.EACCES:
						continue
					raise
				Display(f'{msgprefix}: using pkg-config at discovered path {fp}\n')
				return (fp,)
			Display(f'{msgprefix}: no usable pkg-config {pkgconfig!r} found in $PATH\n')
		def __get_pkg_config_path(context,message,user_settings,display_name,
				_get_pkg_config_exec_path=_get_pkg_config_exec_path,
				_cache={}):
			pkgconfig = user_settings.PKG_CONFIG
			if pkgconfig is None:
				CHOST = user_settings.CHOST
				pkgconfig = f'{CHOST}-pkg-config' if CHOST else 'pkg-config'
				if sys.platform == 'win32':
					pkgconfig += '.exe'
			path = _cache.get(pkgconfig, _cache)
			if path is _cache:
				_cache[pkgconfig] = path = _get_pkg_config_exec_path(context, message, pkgconfig)
			return path
		@staticmethod
		def merge(context, message: str, user_settings, pkgconfig_name: str, display_name: str, guess_flags: dict,
				__get_pkg_config_path=__get_pkg_config_path,
				_cache: dict = {}) -> dict:
			Display = context.Display
			Display(f'{message}: checking {display_name} pkg-config {pkgconfig_name}\n')
			pkgconfig = __get_pkg_config_path(context, message, user_settings, display_name)
			if not pkgconfig:
				Display(f'{message}: skipping {display_name} pkg-config; using default flags {guess_flags!r}\n')
				return guess_flags
			cmd = pkgconfig + ('--cflags', '--libs', pkgconfig_name)
			flags = _cache.get(cmd, None)
			if flags is not None:
				Display(f'{message}: reusing {display_name} settings from `{shlex.join(cmd)}`: {flags!r}\n')
				return flags
			mv_cmd = pkgconfig + ('--modversion', pkgconfig_name)
			try:
				Display(f'{message}: reading {pkgconfig_name} version from `{shlex.join(mv_cmd)}`\n')
				v = StaticSubprocess.pcall(mv_cmd, stderr=subprocess.PIPE)
				if v.err:
					for l in v.err.splitlines():
						Display(f'{message}: pkg-config error: {display_name}: {l.decode()}\n')
				if v.returncode:
					Display(f'{message}: pkg-config failed: {display_name}: {v.returncode}; using default flags {guess_flags!r}\n')
					return guess_flags
				if v.out:
					displayed_version = v.out.splitlines()[0]
					try:
						displayed_version = displayed_version.decode()
					except UnicodeDecodeError:
						pass
					Display(f'{message}: {display_name} version: {displayed_version!r}\n')
			except OSError as o:
				Display(f'''{message}: failed with error {repr(o.message) if o.errno is None else f'{o.errno} ("{o.strerror}")'}; using default flags for {pkgconfig_name!r}: {guess_flags!r}\n''')
				flags = guess_flags
			else:
				Display(f'{message}: reading {display_name} settings from `{shlex.join(cmd)}`\n')
				try:
					v = StaticSubprocess.pcall(cmd, stderr=subprocess.PIPE)
					if v.err:
						for l in v.err.splitlines():
							Display(f'{message}: pkg-config error: {display_name}: {l.decode()}\n')
					if v.returncode:
						Display(f'{message}: pkg-config failed: {display_name}: {v.returncode}; using default flags {guess_flags!r}\n')
						return guess_flags
					out = v.out
					flags = {
						k:v for k,v in context.env.ParseFlags(' ' + out.decode()).items()
							if v and (k[0] in 'CL')
					}
					Display(f'{message}: {display_name} settings: {flags!r}\n')
					# Write the full output to the SCons log for later review.
					# Omit it from `Display` since it is rarely necessary to
					# see it during the build.
					context.Log(f'{message}: {display_name} settings full output: {out!r}\n')
				except OSError as o:
					Display(f'''{message}: failed with error {repr(o.message) if o.errno is None else f'{o.errno} ("{o.strerror}")'}; using default flags for {pkgconfig_name!r}: {guess_flags!r}\n''')
					flags = guess_flags
			_cache[cmd] = flags
			return flags

	# Force test to report failure
	sconf_force_failure = 'force-failure'
	# Force test to report success, and modify flags like it
	# succeeded
	sconf_force_success = 'force-success'
	# Force test to report success, do not modify flags
	sconf_assume_success = 'assume-success'
	expect_sconf_success = 'success'
	expect_sconf_failure = 'failure'
	_implicit_test = Collector()
	_custom_test = Collector()
	_guarded_test_windows = GuardedCollector(_custom_test, lambda user_settings: user_settings.host_platform == 'win32')
	_guarded_test_darwin = GuardedCollector(_custom_test, lambda user_settings: user_settings.host_platform == 'darwin')
	implicit_tests = _implicit_test.tests
	custom_tests = _custom_test.tests
	comment_not_supported = '/* not supported */'
	__python_import_struct = None
	__cxx_std_required_features = CxxRequiredFeatures([
		# As of this writing, <gcc-12 is already unsupported, but some
		# platforms, such as Ubuntu 22.04, still try to use gcc-11 by default.
		# Use this test both to verify that Class Template Argument Deduction
		# is generally supported, since it is used in the game, and to exclude
		# an unsupported compiler that does not accept CTAD in one of the
		# contexts the game uses.
		Cxx20RequiredFeature('class template argument deduction', '''
struct Outer_%(N)s
{
	template <typename byte_type>
	struct Inner
	{
		constexpr Inner(byte_type &)
		{
		}
	};
	/* <gcc-12 does not allow declaring a deduction guide in the immediately
	 * enclosing scope:
```
error: deduction guide 'Outer_test_class_20template_20argument
_20deduction::Inner(byte_type&) -> Outer_test_class_20template_20argument_20deduction::Inner<byte_type>' must be declared at namespace scope
```
	 */
	template <typename byte_type>
		Inner(byte_type &) -> Inner<byte_type>;
};
'''),
		Cxx20RequiredFeature('abbreviated function templates', '''
void f_%(N)s(auto &&)
{
}
''',
'''
	f_%(N)s(0);
	f_%(N)s(0u);
	f_%(N)s("");
	f_%(N)s(nullptr);
'''),
		Cxx20RequiredFeature('explicitly defaulted operator==', '''
struct A_%(N)s
{
	int a;
	constexpr bool operator==(const A_%(N)s &) const = default;
};
''',
'''
	constexpr A_%(N)s a1{0};
	constexpr A_%(N)s a2{0};
	static_assert(a1 == a2);
	static_assert(!(a1 != a2));
'''),
		Cxx20RequiredFeature('std::make_unique_for_overwrite', '''
#include <memory>
''', '''
	std::make_unique_for_overwrite<int>();
	std::make_unique_for_overwrite<int[]>(1);
'''),
		Cxx20RequiredFeature('requires clause', '''
template <typename T>
requires(sizeof(T) >= 1)
void f_%(N)s(T)
{
}
''',
'''
	f_%(N)s('a');
'''),
		Cxx20RequiredFeature('std::ranges', '''
#include <algorithm>
#include <ranges>

struct test_borrowed_range {};

template <>
constexpr bool std::ranges::enable_borrowed_range<test_borrowed_range> = true;

template <std::ranges::range R>
static void requires_range(R &) {}

template <std::ranges::input_range R>
static void requires_input_range(R &) {}

template <std::ranges::borrowed_range R>
static void requires_borrowed_range(R &&) {}
''',
'''
	int a[3]{1, 2, 3};
	int b[2]{4, 5};
	const std::ranges::subrange c(b);
	const std::ranges::subrange<int *> c2(b);
	(void)c2;
	const auto m{[](int i) { return i * 2; }};
	(void)(std::ranges::find(a, argc) == a);
	(void)(std::ranges::find(a, argc, m) == a);
	(void)(std::ranges::find(std::ranges::begin(b), std::ranges::end(b), argc) == a);
	(void)(std::ranges::find(std::ranges::begin(b), std::ranges::end(b), argc, m) == a);
	const auto predicate{[](int i) { return i == 3; }};
	(void)(std::ranges::find_if(b, predicate) == b);
	(void)(std::ranges::find_if(std::ranges::begin(b), std::ranges::end(b), predicate) == b);
	(void)(std::ranges::remove_if(b, predicate).begin() == b);
	(void)(std::ranges::equal_range(b, int{4}, {}).begin() == b);
	requires_range(a);
	requires_input_range(a);
	requires_borrowed_range(a);
	return 0;
'''),
		Cxx20RequiredFeature('std::span', '''
#include <span>

void sd_%(N)s(std::span<const char>);
void ss_%(N)s(std::span<const char, 2>);

void sd_%(N)s(std::span<const char>)
{
}

void ss_%(N)s(std::span<const char, 2>)
{
}
''',
'''
	sd_%(N)s("ab");
	ss_%(N)s("a");
'''),
		Cxx17RequiredFeature('constexpr if', '''
template <bool b>
int f_%(N)s()
{
	if constexpr (b)
		return 1;
	else
		return 0;
}
''',
'''
	f_%(N)s<false>();
'''),
		Cxx17RequiredFeature('fold expressions', '''
static inline void g_%(N)s(int) {}
template <int... i>
void f_%(N)s()
{
	(g_%(N)s(i), ...);
	(g_%(N)s(i), ..., (void)0);
	((void)0, ..., g_%(N)s(i));
}
''',
'''
	f_%(N)s<0, 1, 2, 3>();
'''),
		Cxx17RequiredFeature('structured binding declarations', '',
'''
	int a_%(N)s[2] = {0, 1};
	auto &&[b_%(N)s, c_%(N)s] = a_%(N)s;
	(void)b_%(N)s;
	(void)c_%(N)s;
'''),
		Cxx17RequiredFeature('attribute [[fallthrough]]', '''
static inline void f_%(N)s() {}
''',
'''
	switch (argc)
	{
		case 1:
			f_%(N)s();
			[[fallthrough]];
		case 2:
			f_%(N)s();
			break;
	}
'''),
		Cxx17RequiredFeature('attribute [[nodiscard]]', '''
[[nodiscard]]
static int f_%(N)s()
{
	return 0;
}
''',
'''
	auto i_%(N)s = f_%(N)s();
	(void)i_%(N)s;
'''),
		Cxx14RequiredFeature('template variables', '''
template <unsigned U_%(N)s>
int a_%(N)s = U_%(N)s + 1;
''', ''),
		Cxx14RequiredFeature('std::exchange', '''
#include <utility>
''', '''
	argc |= std::exchange(argc, 5);
'''),
		Cxx14RequiredFeature('std::index_sequence', '''
#include <utility>
''', '''
	std::integer_sequence<int, 0> integer_%(N)s = std::make_integer_sequence<int, 1>();
	(void)integer_%(N)s;
	std::index_sequence<0> index_%(N)s = std::make_index_sequence<1>();
	(void)index_%(N)s;
'''),
		Cxx14RequiredFeature('std::make_unique', '''
#include <memory>
''', '''
	std::make_unique<int>(0);
	std::make_unique<int[]>(1);
'''),
		Cxx11RequiredFeature('std::addressof', '''
#include <memory>
''', '''
	struct A_%(N)s
	{
		void operator&() = delete;
	};
	A_%(N)s i_%(N)s;
	(void)std::addressof(i_%(N)s);
'''),
		Cxx11RequiredFeature('constexpr', '''
struct %(N)s {};
static constexpr %(N)s get_%(N)s(){return {};}
''', '''
	get_%(N)s();
'''),
		Cxx11RequiredFeature('nullptr', '''
#include <cstddef>
std::nullptr_t %(N)s1 = nullptr;
int *%(N)s2 = nullptr;
''', '''
	%(N)s2 = %(N)s1;
'''),
		Cxx11RequiredFeature('explicit operator bool', '''
struct %(N)s {
	explicit operator bool();
};
'''),
		Cxx11RequiredFeature('explicitly deleted functions', '''
void b_%(N)s(int c) = delete;
''', '''
	struct A_%(N)s
	{
		void a(int b) = delete;
	};
	A_%(N)s i_%(N)s;
	(void)i_%(N)s;
'''),
		Cxx11RequiredFeature('template aliases', '''
using %(N)s_typedef = int;
template <typename>
struct %(N)s_struct;
template <typename T>
using %(N)s_alias = %(N)s_struct<T>;
''', '''
	%(N)s_struct<int> *a = nullptr;
	%(N)s_alias<int> *b = a;
	%(N)s_typedef *c = nullptr;
	(void)b;
	(void)c;
'''),
		Cxx11RequiredFeature('trailing function return type', '''
auto %(N)s()->int;
'''),
		Cxx11RequiredFeature('class scope static constexpr assignment', '''
struct %(N)s_instance {
};
struct %(N)s_container {
	static constexpr %(N)s_instance a = {};
};
'''),
		Cxx11RequiredFeature('braced base class initialization', '''
struct %(N)s_base {
	int a;
};
struct %(N)s_derived : %(N)s_base {
	%(N)s_derived(int e) : %(N)s_base{e} {}
};
'''),
		Cxx11RequiredFeature('std::array', '''
#include <array>
''', '''
	std::array<int,2>b;
	b[0]=1;
'''
),
		Cxx11RequiredFeature('std::begin(), std::end()', '''
#include <array>
''', f'''
	char ac[1];
	std::array<char, 1> as;
{(''.join([f"""	{{
		auto s = std::{f}({a});
		(void)s;
	}}
""" for f in ('begin', 'end') for a in ('ac', 'as')]
	)
	)}
'''),
		Cxx11RequiredFeature('range-based for', '''
#include "compiler-range_for.h"
''', '''
	int b[2];
	range_for(int&c,b)c=0;
'''
),
		Cxx11RequiredFeature('<type_traits>', '''
#include <type_traits>
''', '''
	typename std::conditional<sizeof(argc) == sizeof(int),int,long>::type a = 0;
	typename std::conditional<sizeof(argc) != sizeof(int),int,long>::type b = 0;
	(void)a;
	(void)b;
'''
),
		Cxx11RequiredFeature('std::unordered_map::emplace', '''
#include <unordered_map>
''', '''
	std::unordered_map<int,int> m;
	m.emplace(0, 0);
'''
),
		Cxx11RequiredFeature('reference qualified methods', '''
struct %(N)s {
	int a()const &{return 1;}
	int a()const &&{return 2;}
};
''', '''
	%(N)s a;
	auto b = a.a() != %(N)s().a();
	(void)b;
'''
),
		Cxx11RequiredFeature('inheriting constructors', '''
/* Test for bug where clang + libc++ + constructor inheritance causes a
 * compilation failure when returning nullptr.
 *
 * Works: gcc
 * Works: clang + gcc libstdc++
 * Works: old clang + old libc++ (cutoff date unknown).
 * Works: new clang + new libc++ + unique_ptr<T>
 * Fails: new clang + new libc++ + unique_ptr<T[]> (v3.6.0 confirmed broken).

memory:2676:32: error: no type named 'type' in 'std::__1::enable_if<false, std::__1::unique_ptr<int [], std::__1::default_delete<int []> >::__nat>'; 'enable_if' cannot be used to disable this declaration
            typename enable_if<__same_or_less_cv_qualified<_Pp, pointer>::value, __nat>::type = __nat()) _NOEXCEPT
                               ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.sconf_temp/conftest_43.cpp:26:11: note: in instantiation of member function 'std::__1::unique_ptr<int [], std::__1::default_delete<int []> >::unique_ptr' requested here
        using B::B;
                 ^
.sconf_temp/conftest_43.cpp:30:2: note: while substituting deduced template arguments into function template 'I' [with _Pp = I]
        return nullptr;
        ^
 */
#include <memory>
class I%(N)s : std::unique_ptr<int[]>
{
public:
	typedef std::unique_ptr<int[]> b%(N)s;
	using b%(N)s::b%(N)s;
};
I%(N)s a%(N)s();
I%(N)s a%(N)s()
{
	return nullptr;
}
''', '''
	I%(N)s i = a%(N)s();
	(void)i;
'''
),
])
	def __init__(self,msgprefix,user_settings,platform_settings):
		self.msgprefix = msgprefix
		self.user_settings = user_settings
		self.platform_settings = platform_settings
		self.successful_flags = defaultdict(list)
		self._sconf_results = []
		self.__tool_versions = []
		self.__defined_macros = ''
		# Some tests check the functionality of the compiler's
		# optimizer.
		#
		# When LTO is used, the optimizer is deferred to link time.
		# Force all tests to be Link tests when LTO is enabled.
		self.Compile = self.Link if user_settings.lto else self._Compile
		self.custom_tests = tuple(t for t in self.custom_tests if all(predicate(user_settings) for predicate in t.predicate))
	def _quote_macro_value(v: str) -> str:
		return v.strip().replace('\n', ' \\\n')
	def _check_sconf_forced(self, calling_function: str) -> tuple:
		return self._check_forced(calling_function), self._check_expected(calling_function)
	@staticmethod
	def _find_calling_sconf_function():
		try:
			1//0
		except ZeroDivisionError:
			frame = sys.exc_info()[2].tb_frame.f_back.f_back
			while frame is not None:
				co_name = frame.f_code.co_name
				if co_name[:6] == 'check_':
					return co_name[6:]
				frame = frame.f_back
		# This assertion is hit if a test is asked to deduce its caller
		# (calling_function=None), but no function in the call stack appears to
		# be a checking function.
		assert False, "SConf caller not specified and no acceptable caller in stack."
	def _check_forced(self, name: str) -> str:
		# This getattr will raise AttributeError if called for a function which
		# is not a registered test.  Tests must be registered as an implicit
		# test (in implicit_tests, usually by applying the @_implicit_test
		# decorator) or a custom test (in custom_tests, usually by applying the
		# @_custom_test decorator).
		#
		# Unregistered tests are never documented and cannot be overridden by
		# the user.
		return getattr(self.user_settings, f'sconf_{name}')
	def _check_expected(self, name: str):
		# The remarks for _check_forced apply here too.
		r = getattr(self.user_settings, f'expect_sconf_{name}')
		if r is not None:
			if r == self.expect_sconf_success:
				return 1
			if r == self.expect_sconf_failure:
				return 0
		return r
	def _check_macro(self, context, macro_name: str, macro_value: str, test: str, _comment_not_supported: str = comment_not_supported, **kwargs):
		self._define_macro(context, macro_name, macro_value if (
			self.Compile(context, text=f'''
#define {macro_name} {macro_value}
{test}
''', **kwargs)
			) else _comment_not_supported)
	def _define_macro(self, context, macro_name: str, macro_value: str) -> None:
		context.sconf.Define(macro_name, macro_value)
		self.__defined_macros += f'#define {macro_name} {macro_value}\n'
	implicit_tests.append(_implicit_test.RecordedTest('check_ccache_distcc_ld_works', "assume ccache, distcc, C++ compiler, and C++ linker work"))
	implicit_tests.append(_implicit_test.RecordedTest('check_ccache_ld_works', "assume ccache, C++ compiler, and C++ linker work"))
	implicit_tests.append(_implicit_test.RecordedTest('check_distcc_ld_works', "assume distcc, C++ compiler, and C++ linker work"))
	implicit_tests.append(_implicit_test.RecordedTest('check_ld_works', "assume C++ compiler and linker work"))
	implicit_tests.append(_implicit_test.RecordedTest('check_ld_blank_libs_works', "assume C++ compiler and linker work with empty $LIBS"))
	implicit_tests.append(_implicit_test.RecordedTest('check_ld_blank_libs_ldflags_works', "assume C++ compiler and linker work with empty $LIBS and empty $LDFLAGS"))
	implicit_tests.append(_implicit_test.RecordedTest('check_cxx_blank_cxxflags_works', "assume C++ compiler works with empty $CXXFLAGS"))
	# This must be the first custom test.  This test verifies the compiler
	# works and disables any use of ccache/distcc for the duration of the
	# configure run.
	#
	# SCons caches configuration results and tests are usually very small, so
	# ccache will provide limited benefit.
	#
	# Some tests are expected to raise a compiler error.  If distcc is used
	# and DISTCC_FALLBACK prevents local retries, then distcc interprets a
	# compiler error as an indication that the volunteer which served that
	# compile is broken and should be blacklisted.  Suppress use of distcc for
	# all tests to avoid spurious blacklist entries.
	#
	# During the main build, compiling remotely can allow more jobs to run in
	# parallel.  Tests are serialized by SCons, so distcc is helpful during
	# testing only if compiling remotely is faster than compiling locally.
	# This may be true for embedded systems that distcc to desktops, but will
	# not be true for desktops or laptops that distcc to similar sized
	# machines.
	@_custom_test
	def check_cxx_works(self,context):
		"""
help:assume C++ compiler works
"""
		# Use !r to print the tuple in an unambiguous form.
		context.Log(f'scons: dxx: version: {(Git.compute_extra_version(),)!r}\n')
		cenv = context.env
		penv = cenv['ENV']
		self.__cxx_com_prefix = cenv['CXXCOM']
		# Require ccache to run the next stage, but allow it to write the
		# result to cache.  This lets the test validate that ccache fails for
		# an unusable CCACHE_DIR and also validate that the next stage handles
		# the input correctly.  Without this, a cached result may hide that
		# the next stage compiler (or wrapper) worked when a prior run
		# performed the test, but is now broken.
		CCACHE_RECACHE = penv.get('CCACHE_RECACHE', None)
		penv['CCACHE_RECACHE'] = '1'
		most_recent_error = self._check_cxx_works(context)
		if most_recent_error is not None:
			raise SCons.Errors.StopError(most_recent_error)
		if CCACHE_RECACHE is None:
			del penv['CCACHE_RECACHE']
		else:
			penv['CCACHE_RECACHE'] = CCACHE_RECACHE
		# If ccache/distcc are in use, disable them during testing.
		# This assignment is also done in _check_cxx_works, but only on an
		# error path.  Repeat it here so that it is effective on the success
		# path.  It cannot be moved above the call to _check_cxx_works because
		# some tests in _check_cxx_works rely on its original value.
		cenv['CXXCOM'] = cenv._dxx_cxxcom_no_prefix
		self._check_cxx_conformance_level(context)
	def _show_tool_version(self, context, tool: str, desc: str, save_tool_version: bool = True):
		# These version results are not used for anything, but are
		# collected here so that users who post only a build log will
		# still supply at least some useful information.
		#
		# This is split into two lines so that the first line is printed
		# before the function call required to format the string for the
		# second line.
		Display = context.Display
		Display(f'{self.msgprefix}: checking version of {desc} {tool!r} ... ')
		try:
			v = StaticSubprocess.get_version_head(tool)
		except OSError as e:
			if e.errno == errno.ENOENT or e.errno == errno.EACCES:
				Display(f'error: {e.strerror}\n')
				raise SCons.Errors.StopError(f'Failed to run {tool!r}.')
			raise
		if save_tool_version:
			self.__tool_versions.append((tool, v))
		Display(f'{v!r}\n')
	def _show_indirect_tool_version(self,context, CXX: str, tool: str, desc: str):
		Display = context.Display
		Display(f'{self.msgprefix}: checking path to {desc} ... ')
		tool, name = ToolchainInformation.get_tool_path(context.env, tool)
		self.__tool_versions.append((tool, name))
		if not name:
			# Strange, but not fatal for this to fail.
			Display(f'! {name!r}\n')
			return
		Display(f'{name!r}\n')
		self._show_tool_version(context,name,desc)
	def _check_cxx_works(self,context,_crc32=binascii.crc32):
		# Test whether the compiler+linker+optional wrapper(s) work.  If
		# anything fails, a StopError is guaranteed on return.  However, to
		# help the user, this function pushes through all the combinations and
		# reports the StopError for the least complicated issue.  If both the
		# compiler and the linker fail, the compiler will be reported, since
		# the linker might work once the compiler is fixed.
		#
		# If a test fails, then the pending StopError allows this function to
		# safely modify the construction environment and process environment
		# without reverting its changes.
		most_recent_error = None
		Link = self.Link
		cenv = context.env
		user_settings = self.user_settings
		use_distcc = user_settings.distcc
		use_ccache = user_settings.ccache
		text = '''
/* When compiling using gcc, defining a virtual function requires linking to a
 * symbol from gcc libstdc++.  If the linker is `gcc`, rather than `g++`, (which
 * can happen if the user sets `CXX=gcc`), then libstdc++ is not linked, and the
 * build fails with an error like:

```
undefined reference to `vtable for __cxxabiv1::__class_type_info'
```

 * Test for that combination here, so that the test will report that the C++
 * linker does not work, rather than waiting for the user to get a link
 * failure in the game binary at the end of the build.
 */
struct test_virtual_function_supported
{
	virtual void a();
};

void test_virtual_function_supported::a() {}
'''
		if user_settings.show_tool_version:
			CXX = cenv['CXX']
			self._show_tool_version(context, CXX, 'C++ compiler')
			if user_settings.show_assembler_version:
				self._show_indirect_tool_version(context, CXX, 'as', 'assembler')
			if user_settings.show_linker_version:
				self._show_indirect_tool_version(context, CXX, 'ld', 'linker')
			if use_distcc:
				self._show_tool_version(context, use_distcc, 'distcc', False)
			if use_ccache:
				self._show_tool_version(context, use_ccache, 'ccache', False)
		# Use C++ single line comment so that it is guaranteed to extend
		# to the end of the line.  repr ensures that embedded newlines
		# will be escaped and that the final character will not be a
		# backslash.
		self.__commented_tool_versions = s = ''.join(f'// {tool!r} => {value!r}\n' for tool, value in self.__tool_versions)
		self.__tool_versions = f'''
/* This test is always false.  Use a non-trivial condition to
 * discourage external text scanners from recognizing that the block
 * is never compiled.  This is intended to encourage SCons to generate a new
 * test if any of the tools have changed their version identifier.
 */
#if 1 < -1
{_crc32(s.encode()):08x}
{s}
#endif
'''
		ToolchainInformation.show_partial_environ(cenv, user_settings, context.Display, self.msgprefix)
		if use_ccache:
			if use_distcc:
				if Link(context, text=text, msg='whether ccache, distcc, C++ compiler, and linker work', calling_function='ccache_distcc_ld_works'):
					return
				most_recent_error = 'ccache and C++ linker work, but distcc does not work.'
				# Disable distcc so that the next call to self.Link tests only
				# ccache+linker.
				del cenv['ENV']['CCACHE_PREFIX']
			if Link(context, text=text, msg='whether ccache, C++ compiler, and linker work', calling_function='ccache_ld_works'):
				return most_recent_error
			most_recent_error = 'C++ linker works, but ccache does not work.'
		elif use_distcc:
			if Link(context, text=text, msg='whether distcc, C++ compiler, and linker work', calling_function='distcc_ld_works'):
				return
			most_recent_error = 'C++ linker works, but distcc does not work.'
		else:
			# This assertion fails if the environment's $CXXCOM was modified
			# to use a prefix, but both user_settings.ccache and
			# user_settings.distcc evaluate to false.
			assert cenv._dxx_cxxcom_no_prefix is cenv['CXXCOM'], "Unexpected prefix in $CXXCOM."
		# If ccache/distcc are in use, then testing with one or both of them
		# failed.  Disable them so that the next test can check whether the
		# local linker works.
		#
		# If they are not in use, this assignment is a no-op.
		cenv['CXXCOM'] = cenv._dxx_cxxcom_no_prefix
		if Link(context, text=text, msg='whether C++ compiler and linker work', calling_function='ld_works'):
			# If ccache or distcc are in use, this block is only reached
			# when one or both of them failed.  `most_recent_error` will
			# be a description of the failure.  If neither are in use,
			# `most_recent_error` will be None.
			return most_recent_error
		# Force only compile, even if LTO is enabled.
		elif self._Compile(context, text=text, msg='whether C++ compiler works', calling_function='cxx_works'):
			specified_LIBS = 'or ' if cenv.get('LIBS') else ''
			if specified_LIBS:
				cenv['LIBS'] = []
				if Link(context, text=text, msg='whether C++ compiler and linker work with blank $LIBS', calling_function='ld_blank_libs_works'):
					# Using $LIBS="" allowed the test to succeed.  $LIBS
					# specifies one or more unusable libraries.  Usually
					# this is because it specifies a library which does
					# not exist or is an incompatible architecture.
					return 'C++ compiler works.  C++ linker works with blank $LIBS.  C++ linker does not work with specified $LIBS.'
			if cenv['LINKFLAGS']:
				cenv['LINKFLAGS'] = []
				if Link(context, text=text, msg='whether C++ compiler and linker work with blank $LIBS and blank $LDFLAGS', calling_function='ld_blank_libs_ldflags_works'):
					# Using LINKFLAGS="" allowed the test to succeed.
					# To avoid bloat, there is no further test to see
					# whether the link will work with user-specified
					# LIBS after LINKFLAGS is cleared.  The user must
					# fix at least one problem anyway.  If the user is
					# unlucky, fixing LINKFLAGS will result in a
					# different error on the next run.  If the user is
					# lucky, fixing LINKFLAGS will allow the build to
					# run.
					return 'C++ compiler works.  C++ linker works with blank $LIBS and blank $LDFLAGS.  C++ linker does not work with blank $LIBS and specified $LDFLAGS.'
			return f'C++ compiler works.  C++ linker does not work with specified {specified_LIBS}blank $LIBS and specified $LINKFLAGS.  C++ linker does not work with blank $LIBS and blank $LINKFLAGS.'
		else:
			if cenv['CXXFLAGS']:
				cenv['CXXFLAGS'] = []
				if self._Compile(context, text=text, msg='whether C++ compiler works with blank $CXXFLAGS', calling_function='cxx_blank_cxxflags_works'):
					return 'C++ compiler works with blank $CXXFLAGS.  C++ compiler does not work with specified $CXXFLAGS.'
			return 'C++ compiler does not work.'
	implicit_tests.append(_implicit_test.RecordedTest('check_cxx20', "assume C++ compiler supports C++20"))
	def _check_cxx_conformance_level(self, context):
		# Testing the compiler option parser only needs Compile, even when LTO
		# is enabled.
		Compile = self._Compile
		for level in (
			# List standards in descending order of preference.
			#
			# C++20 is required, so list it last.
			20,
		):
			opt = f'-std=gnu++{level}'
			if Compile(context, text='', msg=f'whether C++ compiler accepts {opt}', successflags={'CXXFLAGS': (opt,)}, calling_function=f'cxx{level}'):
				return
		raise SCons.Errors.StopError('C++ compiler does not accept any supported C++ -std option.')
	def _Test(self,context,text,msg,action,main='',ext='.cpp',testflags={},successflags={},skipped=None,successmsg=None,failuremsg=None,expect_failure=False,calling_function=None,__flags_Werror = {'CXXFLAGS' : ['-Werror']}):
		if calling_function is None:
			calling_function = self._find_calling_sconf_function()
		context.Message(f'{self.msgprefix}: checking {msg}...')
		if skipped is not None:
			context.Result(f'(skipped){skipped}')
			if self.user_settings.record_sconf_results:
				self._sconf_results.append((calling_function, 'skipped'))
			return
		env_flags = self.PreservedEnvironment(context.env, (successflags.keys(), testflags.keys(), __flags_Werror.keys(), ('CPPDEFINES',)))
		context.env.MergeFlags(successflags)
		forced, expected = self._check_sconf_forced(calling_function)
		caller_modified_env_flags = self.PreservedEnvironment(context.env, (testflags.keys(), __flags_Werror.keys()))
		# Always pass -Werror to configure tests.
		context.env.Append(**__flags_Werror)
		context.env.Append(**testflags)
		# If forced is None, run the test.  Otherwise, skip the test and
		# take an action determined by the value of forced.
		if forced is None:
			r = action(f'''
{self.__tool_versions}
{self.__defined_macros}
{text}

#undef main	/* avoid -Dmain=SDL_main from libSDL (and, on some platforms, from libSDL2) */
'''
# As a special case, a caller may set main=None to prevent generating the
# `main()` function.  This is only useful when the included headers or
# expanded macros from `text` provide a definition of `main`.
f'''{
'' if main is None else
f"""
int main(int argc,char**argv)
{{
	(void)argc;(void)argv;
{main}
;
}}
"""
}
''', ext)
			# Some tests check that the compiler rejects an input.
			# SConf considers the result a failure when the compiler
			# rejects the input.  For tests that consider a rejection to
			# be the good result, this conditional flips the sense of
			# the result so that a compiler rejection is reported as
			# success.
			if expect_failure:
				r = not r
			context.Result((successmsg if r else failuremsg) or r)
			if expected is not None and r != expected:
				raise SCons.Errors.StopError(f'Expected and actual results differ.  Test should {"succeed" if expected else "fail"}, but it did not.')
		else:
			choices = (self.sconf_force_failure, self.sconf_force_success, self.sconf_assume_success)
			if forced not in choices:
				try:
					forced = choices[int(forced)]
				except ValueError:
					raise SCons.Errors.UserError(f'Unknown force value for sconf_{co_name[6:]}: {forced}')
				except IndexError:
					raise SCons.Errors.UserError(f'Out of range force value for sconf_{co_name[6:]}: {forced}')
			if forced == self.sconf_force_failure:
				# Pretend the test returned a failure result
				r = False
			elif forced == self.sconf_force_success or forced == self.sconf_assume_success:
				# Pretend the test succeeded.  Forced success modifies
				# the environment as if the test had run and succeeded.
				# Assumed success modifies the environment as if the
				# test had run and failed.
				#
				# The latter is used when the user arranges for the
				# environment to be correct.  For example, if the
				# compiler understands C++20, but uses a non-standard
				# name for the option, the user would set assume-success
				# and add the appropriate option to CXXFLAGS.
				r = True
			else:
				raise SCons.Errors.UserError(f'Unknown force value for sconf_{co_name[6:]}: {forced}')
			# Flip the sense of the forced value, so that users can
			# treat "force-failure" as force-bad-result regardless of
			# whether the bad result is that the compiler rejected good
			# input or that the compiler accepted bad input.
			if expect_failure:
				r = not r
			context.Result(f'(forced){("(inverted)" if expect_failure else "")}{forced}')
		# On success, revert to base flags + successflags
		# On failure, revert to base flags
		if r and forced != self.sconf_assume_success:
			caller_modified_env_flags.restore(context.env)
			context.env.Replace(CPPDEFINES=env_flags['CPPDEFINES'])
			f = self.successful_flags
			# Move most CPPDEFINES to the generated header, so that
			# command lines are shorter.
			for k, v in successflags.items():
				if k == 'CPPDEFINES':
					continue
				f[k].extend(v)
			d = successflags.get('CPPDEFINES', None)
			if d:
				append_CPPDEFINE = f['CPPDEFINES'].append
				Define = context.sconf.Define
				for v in d:
					# v is 'NAME' for -DNAME
					# v is ('NAME', 'VALUE') for -DNAME=VALUE
					d = (v, None) if isinstance(v, str) else v
					if d[0] in ('_REENTRANT',):
						# Blacklist defines that must not be moved to the
						# configuration header.
						append_CPPDEFINE(v)
						continue
					Define(d[0], d[1])
		else:
			env_flags.restore(context.env)
		self._sconf_results.append((calling_function, r))
		return r
	def _Compile(self,context,_Test=_Test,**kwargs):
		return _Test(self, context,action=context.TryCompile, **kwargs)
	def Link(self,context,_Test=_Test,**kwargs):
		return _Test(self, context,action=context.TryLink, **kwargs)
	# Compile and link a program that uses a system library.  On
	# success, return None.  On failure, return a tuple indicating which
	# stage failed and providing a text error message to show to the
	# user.  Some callers handle failure by retrying with other options.
	# Others abort the SConf run.
	def _soft_check_system_library(self,context,header,main,lib,pretext='',text='',successflags={},testflags={}):
		include = pretext + ''.join([f'#include <{h}>\n' for h in header])
		header = header[-1]
		# Test library.  On success, good.  On failure, test header to
		# give the user more help.
		if self.Link(context, text=include + text, main=main, msg=f'for usable library {lib}', successflags=successflags, testflags=testflags):
			return
		# If linking failed, an error report is inevitable.  Probe
		# progressively simpler configurations to help the user trace
		# the problem.
		successflags = successflags.copy()
		successflags.update(testflags)
		Compile = self.Compile
		if Compile(context, text=include + text, main=main, msg=f'for usable header {header}', testflags=successflags):
			# If this Compile succeeds, then the test program can be
			# compiled, but it cannot be linked.  The required library
			# may be missing or broken.
			return (0, f'Header {header} is usable, but library {lib} is not usable.')
		if Compile(context, text=include, main=main and '', msg=f'whether compiler can parse header {header}', testflags=successflags):
			# If this Compile succeeds, then the test program cannot be
			# compiled, but the header can be used with an empty test
			# program.  Either the test program is broken or it relies
			# on the header working differently than the used header
			# actually works.
			return (1, f'Header {header} is parseable, but cannot compile the test program.')
		CXXFLAGS = successflags.setdefault('CXXFLAGS', [])
		CXXFLAGS.append('-E')
		if Compile(context, text=include, main=main and '', msg=f'whether preprocessor can parse header {header}', testflags=successflags):
			# If this Compile succeeds, then the used header can be
			# preprocessed, but cannot be compiled as C++ even with an
			# empty test program.  The header likely has a C++ syntax
			# error or assumes prerequisite headers will be included by
			# the calling program.
			return (2, f'Header {header} exists, but cannot compile an empty program.')
		CXXFLAGS.extend(('-M', '-MG'))
		# If the header exists and is accepted by the preprocessor, an
		# earlier test would have returned and this Compile would not be
		# reached.  Therefore, this Compile runs only if the header:
		# - Is directly missing
		# - Is indirectly missing (present, but includes a missing
		#   header)
		# - Is present, but rejected by the preprocessor (such as from
		#   an active `#error`)
		if Compile(context, text=include, main=main and '', msg=f'whether preprocessor can locate header {header} (and supporting headers)', expect_failure=True, testflags=successflags):
			# If this Compile succeeds, then the header does not exist,
			# or exists and includes (possibly through layers of
			# indirection) a header which does not exist.  Passing `-MG`
			# makes non-existent headers legal, but still rejects
			# headers with `#error` and similar constructs.
			#
			# `expect_failure=True` inverts it to a failure in the
			# logged output and in the Python value returned from
			# `Compile(...)`.
			# - Compile success means that the header was not found,
			#   which is not an error when `-MG` is passed, but was an
			#   error in earlier tests.
			# - Compile failure means that the header was found, but
			#   unusable, which is an error even when `-MG` is passed.
			#   This can happen if the header contains `#error`
			#   directives that are not preprocessed out.
			# Therefore, use `expect_failure=True` so that "success"
			# (header not found) prints "no" ("cannot locate header")
			# and "failure" (header found, but unusable) prints "yes"
			# ("can locate header").  This keeps the confusing double
			# negatives confined to the code and this comment.  User
			# visible log messages are clear.
			#
			# Compile failure (unusable) is converted to a True return
			# by `expect_failure=True`, so the guarded path should
			# return "unusable" and the fallthrough path should return
			# "missing".
			return (3, f'Header {header} is unusable.')
		return (4, f'Header {header} is missing or includes a missing supporting header.')
	# Compile and link a program that uses a system library.  On
	# success, return None.  On failure, abort the SConf run.
	def _check_system_library(self,*args,**kwargs):
		e = self._soft_check_system_library(*args, **kwargs)
		if e:
			raise SCons.Errors.StopError(e[1])
	# User settings tests are hidden because they do not respect sconf_*
	# overrides, so the user should not be offered an override.
	@_custom_test
	def _check_user_settings_endian(self,context,__endian_names=('little', 'big')):
		cls = self.__class__
		endian = self.user_settings.host_endian
		if endian is None:
			struct = cls.__python_import_struct
			if struct is None:
				import struct
				cls.__python_import_struct = struct
			a = struct.pack('cccc', b'1', b'2', b'3', b'4')
			unpack = struct.unpack
			i = unpack('i', a)
			if i == unpack('<i', a):
				endian = 0
			elif i == unpack('>i', a):
				endian = 1
			else:
				raise SCons.Errors.UserError(f"Unknown host endian: unpack('i', {a!r}) == {i!r}; set host_endian='big' or host_endian='little' as appropriate.")
		else:
			if endian == __endian_names[0]:
				endian = 0
			else:
				endian = 1
		context.Result(f'{self.msgprefix}: checking endian to use...{__endian_names[endian]}')
		self._define_macro(context, 'DXX_WORDS_BIGENDIAN', endian)

	@_custom_test
	def _check_user_settings_words_need_alignment(self,context):
		self._result_check_user_setting(context, self.user_settings.words_need_alignment, 'word alignment fixups', cpp_defines_with_condition_value=('DXX_WORDS_NEED_ALIGNMENT',))

	@_custom_test
	def _check_user_settings_opengl(self,context):
		user_settings = self.user_settings
		Result = context.Result
		_define_macro = self._define_macro
		_define_macro(context, 'DXX_USE_OGL', int(user_settings.opengl))
		_define_macro(context, 'DXX_USE_OGLES', int(user_settings.opengles))
		Result(f'''{self.msgprefix}: building with {
			"OpenGL ES" if user_settings.opengles else
			"OpenGL" if user_settings.opengl else
			"software renderer"
}''')

	def _result_check_user_setting(self, context, condition, label, cpp_defines_if_enabled: typing.Optional[tuple[tuple]]=None, cpp_defines_with_condition_value: typing.Optional[tuple[str]]=None, int=int):
		assert cpp_defines_if_enabled or cpp_defines_with_condition_value
		if cpp_defines_if_enabled and condition:
			for d in cpp_defines_if_enabled:
				assert isinstance(d[0], str), repr(d)
			self.successful_flags['CPPDEFINES'].extend(cpp_defines_if_enabled)
		if cpp_defines_with_condition_value:
			for d in cpp_defines_with_condition_value:
				assert isinstance(d, str)
				self._define_macro(context, d, int(condition))
		context.Result(f'{self.msgprefix}: checking whether to enable {label}...{"yes" if condition else "no"}')

	@_custom_test
	def _check_user_settings_debug(self,context,_CPPDEFINES=(('NDEBUG',), ('RELEASE',))):
		self._result_check_user_setting(context, not self.user_settings.debug, 'release options', cpp_defines_if_enabled=_CPPDEFINES)

	@_custom_test
	def _check_user_settings_memdebug(self,context,_CPPDEFINES=('DXX_USE_DEBUG_MEMORY_ALLOCATOR',)):
		self._result_check_user_setting(context, self.user_settings.memdebug, 'memory allocation tracking', cpp_defines_with_condition_value=_CPPDEFINES)

	@_custom_test
	def _check_user_settings_editor(self,context,_CPPDEFINES=('DXX_USE_EDITOR',)):
		self._result_check_user_setting(context, self.user_settings.editor, 'level editor', cpp_defines_with_condition_value=_CPPDEFINES)

	@_custom_test
	def _check_user_settings_ipv6(self,context,_CPPDEFINES=('DXX_USE_IPv6',)):
		self._result_check_user_setting(context, self.user_settings.ipv6, 'IPv6 support', cpp_defines_with_condition_value=_CPPDEFINES)

	@_custom_test
	def _check_user_settings_stereo_render(self,context,_CPPDEFINES=('DXX_USE_STEREOSCOPIC_RENDER',)):
		self._result_check_user_setting(context, self.user_settings.use_stereo_render, 'stereoscopic rendering', cpp_defines_with_condition_value=_CPPDEFINES)

	@_custom_test
	def _check_user_settings_udp(self,context,_CPPDEFINES=('DXX_USE_MULTIPLAYER', 'DXX_USE_UDP',)):
		self._result_check_user_setting(context, self.user_settings.use_udp, 'multiplayer over UDP', cpp_defines_with_condition_value=_CPPDEFINES)

	@_custom_test
	def _check_user_settings_tracker(self,context,_CPPDEFINES=('DXX_USE_TRACKER',)):
		use_tracker = self.user_settings.use_udp and self.user_settings.use_tracker
		self._result_check_user_setting(context, use_tracker, 'UDP game tracker', cpp_defines_with_condition_value=_CPPDEFINES)

	@_implicit_test
	def check_libpng(self,context,
		_header=(
			'cstdint',
			'png.h',
		),
		_guess_flags={'LIBS' : ['png']},
		_text='''
namespace {
struct d_screenshot
{
	png_struct *png_ptr;
	png_info *info_ptr = nullptr;
	static void png_error_cb(png_struct *, const char *) {}
	static void png_warn_cb(png_struct *, const char *) {}
	static void png_write_cb(png_struct *, uint8_t *, png_size_t) {}
	static void png_flush_cb(png_struct *) {}
};
}
''',
		_main='''
	d_screenshot ss;
	ss.png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, &ss, &d_screenshot::png_error_cb, &d_screenshot::png_warn_cb);
	png_set_write_fn(ss.png_ptr, &ss, &d_screenshot::png_write_cb, &d_screenshot::png_flush_cb);
	ss.info_ptr = png_create_info_struct(ss.png_ptr);
#ifdef PNG_tIME_SUPPORTED
	png_time pt;
	pt.year = 2018;
	pt.month = 1;
	pt.day = 1;
	pt.hour = 1;
	pt.minute = 1;
	pt.second = 1;
	png_set_tIME(ss.png_ptr, ss.info_ptr, &pt);
#endif
#if DXX_USE_OGL
	const auto color_type{PNG_COLOR_TYPE_RGB};
#else
	png_set_PLTE(ss.png_ptr, ss.info_ptr, reinterpret_cast<const png_color *>(&ss), 256 * 3);
	const auto color_type{PNG_COLOR_TYPE_PALETTE};
#endif
	png_set_IHDR(ss.png_ptr, ss.info_ptr, 1, 1, 8, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
#ifdef PNG_TEXT_SUPPORTED
	png_text text_fields[1] = {};
	png_set_text(ss.png_ptr, ss.info_ptr, text_fields, 1);
#endif
	png_write_info(ss.png_ptr, ss.info_ptr);
	png_byte *row_pointers[1024]{};
	png_write_rows(ss.png_ptr, row_pointers, 1024);
	png_write_end(ss.png_ptr, ss.info_ptr);
	png_destroy_write_struct(&ss.png_ptr, &ss.info_ptr);
'''):
		successflags = self.pkgconfig.merge(context, self.msgprefix, self.user_settings, 'libpng', 'libpng', _guess_flags)
		return self._soft_check_system_library(context, header=_header, main=_main, lib='png', text=_text, successflags=successflags)

	@_custom_test
	def _check_user_settings_adldmidi(self,context):
		user_settings = self.user_settings
		adlmidi = user_settings.adlmidi
		if adlmidi == 'none':
			adlmidi_load_type = 'disabled'
		elif adlmidi == 'runtime':
			adlmidi_load_type = 'load dynamically'
		else:
			return
		context.Result(f'{self.msgprefix}: checking how to handle ADL MIDI...{adlmidi_load_type}')
		Define = context.sconf.Define
		Define('DXX_USE_ADLMIDI', int(user_settings._enable_adlmidi()))

	@_custom_test
	def _check_user_settings_screenshot(self,context):
		user_settings = self.user_settings
		screenshot_mode = user_settings.screenshot
		if screenshot_mode == 'png':
			screenshot_format_type = screenshot_mode
		elif screenshot_mode == 'legacy':
			screenshot_format_type = 'tga' if user_settings.opengl else 'pcx'
		elif screenshot_mode == 'none':
			screenshot_format_type = 'screenshot support disabled'
		else:
			return
		context.Result(f'{self.msgprefix}: checking how to format screenshots...{screenshot_format_type}')
		Define = context.sconf.Define
		Define('DXX_USE_SCREENSHOT_FORMAT_PNG', int(screenshot_mode == 'png'))
		Define('DXX_USE_SCREENSHOT_FORMAT_LEGACY', int(screenshot_mode == 'legacy'))
		Define('DXX_USE_SCREENSHOT', int(screenshot_mode != 'none'))
		if screenshot_mode == 'png':
			e = self.check_libpng(context)
			if e:
				raise SCons.Errors.StopError(e[1] + '  Set screenshot=legacy to remove screenshot libpng requirement or set screenshot=none to remove screenshot support.')

	# Require _WIN32_WINNT >= 0x0501 to enable getaddrinfo
	# Require _WIN32_WINNT >= 0x0600 to enable some useful AI_* flags
	@_guarded_test_windows
	def _check_user_CPPDEFINES__WIN32_WINNT(self,context,_msg='%s: checking whether to define _WIN32_WINNT...%s',_CPPDEFINES_WIN32_WINNT=('_WIN32_WINNT', 0x600)):
		env = context.env
		for f in env['CPPDEFINES']:
			# Ignore the case that an element is a string with this
			# value, since that would not define the macro to a
			# useful value.  In CPPDEFINES, only a tuple of
			# (name,number) is useful for macro _WIN32_WINNT.
			if f[0] == '_WIN32_WINNT':
				f = f[1]
				context.Result(_msg % (self.msgprefix, 'no, already set in CPPDEFINES as %s' % (f'{f:#x}' if isinstance(f, int) else f'{f!r}',)))
				return
		for f in env['CPPFLAGS']:
			if f.startswith('-D_WIN32_WINNT='):
				context.Result(_msg % (self.msgprefix, f'no, already set in CPPFLAGS as {f!r}'))
				return
		context.Result(_msg % (self.msgprefix, f'yes, define _WIN32_WINNT={_CPPDEFINES_WIN32_WINNT[1]:#x}'))
		self.successful_flags['CPPDEFINES'].append(_CPPDEFINES_WIN32_WINNT)
		self.__defined_macros += f'#define {_CPPDEFINES_WIN32_WINNT[0]} {_CPPDEFINES_WIN32_WINNT[1]}\n'

	@_guarded_test_windows
	def check_dbghelp_header(self,context,_CPPDEFINES=('DXX_ENABLE_WINDOWS_MINIDUMP',)):
		windows_minidump = self.user_settings.windows_minidump
		self._result_check_user_setting(context, windows_minidump, 'minidump on exception', cpp_defines_if_enabled=_CPPDEFINES)
		if not windows_minidump:
			return
		include_dbghelp_header = '''
// <dbghelp.h> assumes that <windows.h> was included earlier and fails
// to parse if this assumption is false
#include <windows.h>
#include <dbghelp.h>
'''
		if self.Compile(context, text=include_dbghelp_header + '''
#include <stdexcept>

using MiniDumpWriteDump_type = decltype(MiniDumpWriteDump);
MiniDumpWriteDump_type *pMiniDumpWriteDump;

[[noreturn]]
static void terminate_handler()
{
	MINIDUMP_EXCEPTION_INFORMATION mei;
	MINIDUMP_USER_STREAM musa[1];
	MINIDUMP_USER_STREAM_INFORMATION musi;
	EXCEPTION_POINTERS ep;
	mei.ExceptionPointers = &ep;
	musi.UserStreamCount = 1;
	musi.UserStreamArray = musa;
	(*pMiniDumpWriteDump)(GetCurrentProcess(), GetCurrentProcessId(), GetCurrentProcess(), MiniDumpWithFullMemory, &mei, &musi, nullptr);
	ExitProcess(0);
}
''', main='''
	std::set_terminate(&terminate_handler);
''', msg='for usable header dbghelp.h'):
			return
		errtext = "Header dbghelp.h is parseable but cannot compile the test program."	\
			if self.Compile(context, text=include_dbghelp_header, msg='for parseable header dbghelp.h')	\
			else "Header dbghelp.h is missing or unusable."
		raise SCons.Errors.StopError(errtext + "  Upgrade the Windows API header package or disable minidump support.")
	@_custom_test
	def check_libphysfs(self,context,_header=('physfs.h',)):
		main = '''
	PHYSFS_File *f;
	char b[1] = {0};
	if (!PHYSFS_init(""))
		return 1;
	{
		const char *r = PHYSFS_getRealDir("");
		(void)r;
	}
	PHYSFS_isDirectory("");
	{
		const char *sep = PHYSFS_getDirSeparator();
		(void)sep;
	}
	{
		const char *dir = PHYSFS_getBaseDir();
		(void)dir;
	}
	{
		const char *dir = PHYSFS_getUserDir();
		(void)dir;
	}
	f = PHYSFS_openWrite("a");
	PHYSFS_sint64 w{PHYSFS_writeBytes(f, b, 1)};
	(void)w;
	PHYSFS_close(f);
	f = PHYSFS_openRead("a");
	PHYSFS_sint64 rb{PHYSFS_readBytes(f, b, 1)};
	(void)rb;
	PHYSFS_close(f);
	PHYSFS_mount("", nullptr, 0);
	PHYSFS_unmount("");
	PHYSFS_delete("");
'''
		l = ['physfs']
		guess_flags = {'LIBS' : l}
		# If pkg-config is not available, or the `physfs.pc` file is not
		# usable, then `successflags is guess_flags`.  Otherwise,
		# `successflags` is derived from the output of `pkg-config`.
		successflags = self.pkgconfig.merge(context, self.msgprefix, self.user_settings, 'physfs', 'physfs', guess_flags)
		e = self._soft_check_system_library(context, header=_header, main=main, lib='physfs', successflags=successflags)
		if not e:
			# If the test succeeded outright, return.
			return
		if e[0] == 0 and successflags is guess_flags:
			# If the header is usable, but the link test failed, and the link
			# test was done using `guess_flags`, then speculatively add zlib to
			# the link line and retry.  This is a convenience for users who (1)
			# use a physfs that depends on zlib and (2) do not use a pkg-config
			# file, so they have no place to express the dependency on zlib.
			# For users who use pkg-config, the pkg-config output is assumed to
			# be correct, and this speculative retry is skipped.
			context.Display(f'{self.msgprefix}: physfs header usable; adding zlib and retesting library\n')
			# `l is guess_flags['LIBS']` and `successflags is guess_flags`, so
			# `l is successflags['LIBS']`.  Therefore, this `append` modifies
			# the value of `successflags`.
			l.append('z')
			e = self._soft_check_system_library(context, header=_header, main=main, lib='physfs', successflags=successflags)
		if e:
			raise SCons.Errors.StopError(e[1])

	# SDL-only builds do not need GL utility functions
	# OpenGLES builds do not use the GL utility functions
	@GuardedCollector(_custom_test, lambda user_settings: user_settings.opengl and not user_settings.opengles)
	def check_glu(self,context):
		ogllibs = self.platform_settings.ogllibs
		self._check_system_library(context, header=['OpenGL/glu.h' if self.user_settings.host_platform == 'darwin' else 'GL/glu.h'], main='''
	gluPerspective(90.0,1.0,0.1,5000.0);
	gluBuild2DMipmaps (GL_TEXTURE_2D, 0, 1, 1, 1, GL_UNSIGNED_BYTE, nullptr);
''', lib=ogllibs, testflags={'LIBS': ogllibs})

	@_custom_test
	def _check_SDL(self,context):
		if self.user_settings.sdl2:
			check_libSDL = self.check_libSDL2
			check_SDL_image = self.check_SDL2_image
			check_SDL_mixer = self.check_SDL2_mixer
		else:
			check_libSDL = self.check_libSDL
			check_SDL_image = self.check_SDL_image
			check_SDL_mixer = self.check_SDL_mixer
		check_libSDL(context)
		check_SDL_image(context)
		check_SDL_mixer(context)

	@_implicit_test
	def check_libSDL(self,context,_guess_flags={
			'LIBS' : ['SDL'] if sys.platform != 'darwin' else [],
		}):
		self._check_libSDL(context, '', _guess_flags)

	@_implicit_test
	def check_libSDL2(self,context,_guess_flags={
			'LIBS' : ['SDL2'] if sys.platform != 'darwin' else [],
		}):
		if not self.user_settings.opengl:
			raise SCons.Errors.StopError('Rebirth does not support SDL2 without OpenGL.  Set opengl=1 or sdl2=0.')
		self._check_libSDL(context, '2', _guess_flags)

	def _check_libSDL(self,context,sdl2,guess_flags):
		user_settings = self.user_settings
		successflags = self.pkgconfig.merge(context, self.msgprefix, user_settings, f'sdl{sdl2}', f'SDL{sdl2}', guess_flags).copy()
		if user_settings.max_joysticks:
			# If joysticks are enabled, but all possible inputs are
			# disabled, then disable joystick support.
			if not (user_settings.max_axes_per_joystick or user_settings.max_buttons_per_joystick or user_settings.max_hats_per_joystick):
				user_settings.max_joysticks = 0
			elif not user_settings.max_buttons_per_joystick:
				user_settings.max_hats_per_joystick = 0
		else:
			# If joysticks are disabled, then disable all possible
			# inputs.
			user_settings.max_axes_per_joystick = user_settings.max_buttons_per_joystick = user_settings.max_hats_per_joystick = 0
		successflags['CPPDEFINES'] = CPPDEFINES = successflags.get('CPPDEFINES', []).copy()
		# use Redbook if at least one of the following applies
		#    1. we are on SDL1
		#    2. we are building for a platform for which we have a custom CD implementation (currently only win32)
		use_redbook = int(not sdl2 or user_settings.host_platform == 'win32')
		CPPDEFINES.extend((
			('DXX_MAX_JOYSTICKS', user_settings.max_joysticks),
			('DXX_MAX_AXES_PER_JOYSTICK', user_settings.max_axes_per_joystick),
			('DXX_MAX_BUTTONS_PER_JOYSTICK', user_settings.max_buttons_per_joystick),
			('DXX_MAX_HATS_PER_JOYSTICK', user_settings.max_hats_per_joystick),
			('DXX_USE_SDL_REDBOOK_AUDIO', use_redbook),
		))
		context.Display(f'{self.msgprefix}: checking whether to enable joystick support...{"yes" if user_settings.max_joysticks else "no"}\n')
		# SDL2 removed CD-rom support.
		init_cdrom = '0' if sdl2 else 'SDL_INIT_CDROM'
		error_text_opengl_mismatch = f'Rebirth configured with OpenGL enabled, but SDL{sdl2} configured with OpenGL disabled.  Disable Rebirth OpenGL or install an SDL{sdl2} with OpenGL enabled.'
		test_opengl = (f'''
#if !((SDL_MAJOR_VERSION == 1) && (SDL_MINOR_VERSION == 2) && (SDL_PATCHLEVEL >= 50))
#ifndef SDL_VIDEO_OPENGL
#error "{error_text_opengl_mismatch}"
#endif
#endif
''') if user_settings.opengl else ''
		main = '''
	SDL_RWops *ops = reinterpret_cast<SDL_RWops *>(argv);
#if DXX_MAX_JOYSTICKS
#ifdef SDL_JOYSTICK_DISABLED
#error "Rebirth configured with joystick support enabled, but SDL{sdl2} configured with joystick support disabled.  Disable Rebirth joystick support or install an SDL{sdl2} with joystick support enabled."
#endif
#define DXX_SDL_INIT_JOYSTICK	SDL_INIT_JOYSTICK |
#else
#define DXX_SDL_INIT_JOYSTICK
#endif
	SDL_Init(DXX_SDL_INIT_JOYSTICK {init_cdrom} | SDL_INIT_VIDEO | SDL_INIT_AUDIO);
{test_opengl}
#if DXX_MAX_JOYSTICKS
	auto n = SDL_NumJoysticks();
	(void)n;
#endif
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_FreeRW(ops);
	SDL_Quit();
'''
		e = self._soft_check_system_library(context,header=['SDL.h'],main=main.format(init_cdrom=init_cdrom, sdl2=sdl2, test_opengl=test_opengl),
			lib=f'SDL{sdl2} with{"" if test_opengl else "out"} OpenGL', successflags=successflags
		)
		if not e:
			return
		if test_opengl:
			e2 = self._soft_check_system_library(context,header=['SDL.h'],main=main.format(init_cdrom=init_cdrom, sdl2=sdl2, test_opengl=''),
				lib=f'SDL{sdl2} without OpenGL', successflags=successflags
			)
			if not e2 and e[0] == 1:
				e = (None, error_text_opengl_mismatch)
		raise SCons.Errors.StopError(e[1])

	@_implicit_test
	def check_SDL_image(self,context):
		self._check_SDL_image(context, '')

	@_implicit_test
	def check_SDL2_image(self,context):
		self._check_SDL_image(context, '2')

	def _check_SDL_image(self,context,sdl2):
		self._check_SDL_addon_library(context, sdl2, 'SDL%s_image', 'DXX_USE_SDLIMAGE', self.user_settings.sdlimage, '''
	IMG_Init(0);
	SDL_RWops *rw = reinterpret_cast<SDL_RWops *>(argv);
	SDL_Surface *s = IMG_LoadPCX_RW(rw);
	(void)s;
	IMG_Quit();
''')

	# SDL_mixer/SDL2_mixer use the same -I line as SDL/SDL2
	@_implicit_test
	def check_SDL_mixer(self,context):
		self._check_SDL_mixer(context, '')

	@_implicit_test
	def check_SDL2_mixer(self,context):
		self._check_SDL_mixer(context, '2')

	def _check_SDL_mixer(self,context,sdl2):
		self._check_SDL_addon_library(context, sdl2, 'SDL%s_mixer', 'DXX_USE_SDLMIXER', self.user_settings.sdlmixer, '''
	int i = Mix_Init(MIX_INIT_FLAC | MIX_INIT_OGG);
	(void)i;
	Mix_Pause(0);
	Mix_ResumeMusic();
	Mix_Quit();
''')

	def _check_SDL_addon_library(self,context,sdl2,library_format_name,macro_name,use_addon,main):
		library_name = library_format_name % sdl2
		self._define_macro(context, macro_name, int(use_addon))
		context.Display(f'{self.msgprefix}: checking whether to use {library_name}...{"yes" if use_addon else "no"}\n')
		if not use_addon:
			return
		user_settings = self.user_settings
		guess_flags = {
			'LIBS' : [library_name] if sys.platform != 'darwin' else [],
		}
		successflags = self.pkgconfig.merge(context, self.msgprefix, user_settings, library_name, library_name, guess_flags)
		if user_settings.host_platform == 'darwin' and user_settings.macos_add_frameworks:
			successflags = successflags.copy()
			successflags['FRAMEWORKS'] = [library_name]
			relative_headers = f'Library/Frameworks/{library_name}.framework/Headers'
			successflags['CPPPATH'] = [h for h in (os.path.join(os.getenv("HOME"), relative_headers), f'/{relative_headers}') if os.path.isdir(h)]
		# SDL2 headers still use SDL_*.h for their filename, not
		# SDL2_*.h, so expanded library_format_name with an explicitly
		# blank insert, regardless of whether building for SDL1 or SDL2.
		self._check_system_library(context, header=['%s.h' % (library_format_name % '')], main=main, lib=library_name, successflags=successflags)

	@_custom_test
	def check_attribute_error(self,context):
		"""
Test whether the compiler accepts and properly implements gcc's function
attribute [__attribute__((__error__))][1].

A proper implementation will compile correctly if the function is
declared and, after optimizations are applied, the function is not
called.  If this function attribute is not supported, then
DXX_ALWAYS_ERROR_FUNCTION results in link-time errors, rather than
compile-time errors.

This test will report failure if the optimizer does not remove the call
to the marked function.

[1]: https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-g_t_0040code_007berror_007d-function-attribute-3097

help:assume compiler supports __attribute__((error))
"""
		self._check_function_dce_attribute(context, 'error')
		context.sconf.config_h_text += '''
#ifndef DXX_SCONF_NO_INCLUDES
dxx_compiler_attribute_error("must never be called")
void DXX_ALWAYS_ERROR_FUNCTION(const char *);
#endif
'''
	def _check_function_dce_attribute(self,context,attribute):
		__attribute__ = f'__{attribute}__'
		f = f'''
void a()__attribute__(({__attribute__}("a called")));
'''
		macro_name = f'dxx_compiler_attribute_{attribute}(M)'
		Compile = self.Compile
		Define = context.sconf.Define
		if Compile(context, text=f, main='if("0"[0]==\'1\')a();', msg=f'whether compiler optimizes function __attribute__(({__attribute__}))'):
			Define(f'DXX_HAVE_ATTRIBUTE_{attribute.upper()}')
			macro_value = f'__attribute__(({__attribute__}(M)))'
		else:
			Compile(context, text=f, msg=f'whether compiler accepts function __attribute__(({__attribute__}))') and \
			Compile(context, text=f, main='a();', msg=f'whether compiler understands function __attribute__(({__attribute__}))', expect_failure=True)
			macro_value = self.comment_not_supported
		Define(macro_name, macro_value)
		self.__defined_macros += f'#define {macro_name} {macro_value}\n'

	@_custom_test
	def check_builtin_bswap(self,context,
		_main='''
	(void)__builtin_bswap64(static_cast<uint64_t>(argc));
	(void)__builtin_bswap32(static_cast<uint32_t>(argc));
	(void)__builtin_bswap16(static_cast<uint16_t>(argc));
''',
		_successflags_bswap={'CPPDEFINES' : ['DXX_HAVE_BUILTIN_BSWAP']},
	):
		"""
Test whether the compiler accepts the gcc byte swapping intrinsic
functions.  These functions may be optimized into architecture-specific
swap instructions when the idiomatic swap is not.

	u16 = (u16 << 8) | (u16 >> 8);

The 16-bit ([__builtin_bswap16][3]), 32-bit ([__builtin_bswap32][1]),
and 64-bit ([__builtin_bswap64][2]) versions are present in all
supported versions of gcc.

[1]: https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#index-g_t_005f_005fbuiltin_005fbswap32-4135
[2]: https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#index-g_t_005f_005fbuiltin_005fbswap64-4136
[3]: https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#index-g_t_005f_005fbuiltin_005fbswap16-4134
"""
		include = '''
#include <cstdint>
'''
		self.Compile(context, text=include, main=_main, msg='whether compiler implements __builtin_bswap{16,32,64} functions', successflags=_successflags_bswap)

	implicit_tests.append(_implicit_test.RecordedTest('check_optimize_builtin_constant_p', "assume compiler optimizes __builtin_constant_p"))

	@_custom_test
	def check_builtin_constant_p(self,context):
		"""
Test whether the compiler accepts and properly implements gcc's
intrinsic [__builtin_constant_p][1].  A proper implementation will
compile correctly if the intrinsic is recognized and, after
optimizations are applied, the intrinsic returns true for a constant
input and that return value is used to optimize away dead code.
If this intrinsic is not supported, or if applying optimizations
does not make the intrinsic report true, then the test reports
failure.  A failure here disables some compile-time sanity checks.

This test is known to fail when optimizations are disabled.  The failure
is not a bug in the test or in the compiler.

[1]: https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#index-g_t_005f_005fbuiltin_005fconstant_005fp-4082

help:assume compiler supports compile-time __builtin_constant_p
"""
		f = '''
/*
 * Begin `timekeeping.i` from gcc bug #72785, comment #0.  This block
 * provokes gcc-7 into generating a call to ____ilog2_NaN, which then
 * fails to link when ____ilog2_NaN is intentionally undefined.
 *
 * Older versions of gcc declare the expression to be false in
 * this case, and do not generate the call to ____ilog2_NaN.  This looks
 * fragile since b is provably non-zero (`b` = `a` if `a` is not zero,
 * else = `c` = `1`, so `b` is non-zero regardless of the path used in
 * the ternary operator), but the specific non-zero value is not
 * provable.  However, this matches the testcase posted in gcc's
 * Bugzilla and produces the desired results on each of gcc-5 (enable),
 * gcc-6 (enable), and gcc-7 (disable).
 */
int a, b;
int ____ilog2_NaN();
static void by(void) {
	int c = 1;
	b = a ?: c;
	__builtin_constant_p(b) ? b ? ____ilog2_NaN() : 0 : 0;
}
/*
 * End `timekeeping.i`.
 */
int c(int);
static int x(int y){
	return __builtin_constant_p(y) ? 1 : %s;
}
'''
		main = '''
	/*
	** Tell the compiler not to make any assumptions about the value of
	** `a`.  In LTO mode, the compiler would otherwise prove that it
	** knows there are no writes to `a` and therefore prove that `a`
	** must be zero.  That proof then causes it to compile `by()` as:
	**
	**		b = 0 ? 0 : 1;
	**		__builtin_constant_p(1) ? 1 ? ____ilog2_NaN() : 0 : 0;
	**
	** That then generates a reference to the intentionally undefined
	** ____ilog2_NaN, causing the test to report failure.  Prevent this
	** by telling the compiler that it cannot prove the value of `a`,
	** forcing it not to propagate a zero `a` into `by()`.
	**/
	asm("" : "=rm" (a));
	by();
	return x(1) + x(2);
'''
		Define = context.sconf.Define
		if self.Link(context, text=f % 'c(y)', main=main, msg='whether compiler optimizes __builtin_constant_p', calling_function='optimize_builtin_constant_p'):
			Define('DXX_HAVE_BUILTIN_CONSTANT_P')
			Define('DXX_CONSTANT_TRUE(E)', '(__builtin_constant_p((E)) && (E))')
			dxx_builtin_constant_p = '__builtin_constant_p(A)'
		else:
			# This is present because it may be useful to see in the
			# debug log.  It is not expected to modify the build
			# environment.
			self.Compile(context, text=f % '2', main=main, msg='whether compiler accepts __builtin_constant_p')
			dxx_builtin_constant_p = '((void)(A),0)'
		Define('dxx_builtin_constant_p(A)', dxx_builtin_constant_p)

	@_custom_test
	def check_builtin_expect(self,context):
		"""
Test whether the compiler accepts gcc's intrinsic
[__builtin_expect][1].  This intrinsic is a hint to the optimizer,
which it may ignore.  The test does not try to detect whether the
optimizer respects such hints.

When this test succeeds, conditional tests can hint to the optimizer
which path should be considered hot.

[1]: https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#index-g_t_005f_005fbuiltin_005fexpect-4083
"""
		main = '''
return __builtin_expect(argc == 1, 1) ? 1 : 0;
'''
		if self.Compile(context, text='', main=main, msg='whether compiler accepts __builtin_expect'):
			likely = '__builtin_expect(!!(A), 1)'
			unlikely = '__builtin_expect(!!(A), 0)'
		else:
			likely = unlikely = '(!!(A))'
		Define = context.sconf.Define
		Define('likely(A)', likely)
		Define('unlikely(A)', unlikely)

	@_custom_test
	def check_builtin_file(self,context):
		context.sconf.Define('DXX_HAVE_CXX_BUILTIN_FILE_LINE',
			self.Compile(context, text='''
static void f(const char * = __builtin_FILE(), unsigned = __builtin_LINE())
{
	struct A
	{
		unsigned line{__builtin_LINE()};
	};
	static_assert(A{}.line == __LINE__);
}
''', main='f();', msg='whether compiler accepts __builtin_FILE, __builtin_LINE'))

	@_custom_test
	def check_builtin_object_size(self,context):
		"""
Test whether the compiler accepts and optimizes gcc's intrinsic
[__builtin_object_size][1].  If this intrinsic is optimized,
compile-time checks can verify that the caller-specified constant
size of a variable does not exceed the compiler-determined size of
the variable.  If the compiler cannot determine the size of the
variable, no compile-time check is done.

[1]: https://gcc.gnu.org/onlinedocs/gcc/Object-Size-Checking.html#index-g_t_005f_005fbuiltin_005fobject_005fsize-3657

help:assume compiler supports __builtin_object_size
"""
		f = '''
/* a() is never defined.  An optimizing compiler will eliminate the
 * attempt to call it, allowing the Link to succeed.  A non-optimizing
 * compiler will emit the call, and the Link will fail.
 */
int a();
static inline int a(char *c){
	return __builtin_object_size(c,0) == 4 ? 1 : %s;
}
'''
		main = '''
	char c[4];
	return a(c);
'''
		r = self.Link(context, text=f % 'a()', main=main, msg='whether compiler optimizes __builtin_object_size')
		context.sconf.Define('DXX_HAVE_BUILTIN_OBJECT_SIZE', int(r))
		if not r:
			# This test is only to provide additional information in the
			# configure log.  The result does not influence the build.
			self.Compile(context, text=f % '2', main=main, msg='whether compiler accepts __builtin_object_size')

	@_custom_test
	def check_embedded_compound_statement(self,context,
		_compound_statement_native=('', ''),
		_compound_statement_emulated=('[&]', '()')
	):
		"""
Test whether the compiler implements gcc's [statement expression][1]
extension.  If this extension is present, statements can be used where
expressions are expected.  If it is absent, it is emulated by defining,
calling, and then discarding a lambda function.  The compiler produces
better error messages for errors in statement expressions than for
errors in lambdas, so prefer statement expressions when they are
available.

[1]: https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html
"""
		f = '''
	return ({ 1 + 2; });
'''
		t = _compound_statement_native if self.Compile(context, text='', main=f, msg='whether compiler understands embedded compound statements') else _compound_statement_emulated
		Define = context.sconf.Define
		Define('DXX_BEGIN_COMPOUND_STATEMENT', t[0])
		Define('DXX_END_COMPOUND_STATEMENT', t[1])

	@_custom_test
	def check_attribute_always_inline(self,context):
		"""
help:assume compiler supports [[gnu::always_inline]]
"""
		macro_name = 'dxx_compiler_attribute_always_inline()'
		macro_value = '[[gnu::always_inline]]'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test=f'{macro_name} static inline void a(){{}}', main='a();', msg='for C++ function attribute [[gnu::always_inline]]')

	@GuardedCollector(_custom_test, lambda user_settings: user_settings.memdebug)
	def check_attribute_alloc_size(self,context):
		"""
help:assume compiler supports __attribute__((alloc_size))
"""
		macro_name = 'dxx_compiler_attribute_alloc_size(A,...)'
		macro_value = '__attribute__((alloc_size(A, ## __VA_ARGS__)))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
char*a(int)dxx_compiler_attribute_alloc_size(1);
char*b(int,int)dxx_compiler_attribute_alloc_size(1,2);
""", msg='for function __attribute__((alloc_size))')
	@_custom_test
	def check_attribute_cold(self,context):
		"""
Test whether the compiler accepts the C++ function attribute
[[gnu::cold]].  Use this to annotate functions which are
rarely called, such as error reporting functions.
"""
		macro_name = 'dxx_compiler_attribute_cold'
		macro_value = '[[gnu::cold]]'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
dxx_compiler_attribute_cold char*a(int);
""", msg='for C++ function attribute [[gnu::cold]]')
	@_custom_test
	def check_attribute_format_arg(self,context):
		"""
help:assume compiler supports __attribute__((format_arg))
"""
		macro_name = 'dxx_compiler_attribute_format_arg(A)'
		macro_value = '__attribute__((format_arg(A)))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
char*a(char*)dxx_compiler_attribute_format_arg(1);
""", msg='for function __attribute__((format_arg))')
	@_custom_test
	def check_attribute_format_printf(self,context):
		"""
help:assume compiler supports __attribute__((format(printf)))
"""
		macro_name = 'dxx_compiler_attribute_format_printf(A,B)'
		macro_value = '__attribute__((format(printf,A,B)))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
int a(char*,...)dxx_compiler_attribute_format_printf(1,2);
int b(char*)dxx_compiler_attribute_format_printf(1,0);
""", msg='for function __attribute__((format(printf)))')
	@GuardedCollector(_custom_test, lambda user_settings: user_settings.memdebug)
	def check_attribute_malloc(self,context):
		"""
help:assume compiler supports __attribute__((malloc))
"""
		macro_name = 'dxx_compiler_attribute_malloc()'
		macro_value = '__attribute__((malloc))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
int *a()dxx_compiler_attribute_malloc();
""", msg='for function __attribute__((malloc))')
	@_custom_test
	def check_attribute_nonnull(self,context):
		"""
help:assume compiler supports __attribute__((nonnull))
"""
		macro_name = 'dxx_compiler_attribute_nonnull(...)'
		macro_value = '__attribute__((nonnull __VA_ARGS__))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
int a(int*)dxx_compiler_attribute_nonnull();
int b(int*)dxx_compiler_attribute_nonnull((1));
""", msg='for function __attribute__((nonnull))')

	@_custom_test
	def check_attribute_used(self,context):
		"""
help:assume compiler supports __attribute__((used))
"""
		macro_name = 'dxx_compiler_attribute_used'
		macro_value = '__attribute__((used))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
static void a()dxx_compiler_attribute_used;
static void a(){}
""", msg='for function __attribute__((used))')
	@_custom_test
	def check_attribute_unused(self,context):
		"""
help:assume compiler supports __attribute__((unused))
"""
		macro_name = 'dxx_compiler_attribute_unused'
		macro_value = '__attribute__((unused))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
dxx_compiler_attribute_unused
static void a(){}
""", msg='for function __attribute__((unused))', successflags={'CXXFLAGS' : get_Werror_sequence(context.env['CXXFLAGS'], ('-Wunused',))})

	@_custom_test
	def check_cxx11_static_assert(self,context,_f='''
static_assert(%(expr)s, "global");
struct A
{
	static const bool value = %(expr)s;
	static_assert(%(expr)s, "class literal");
	static_assert(A::value, "class static");
	A()
	{
		static_assert(%(expr)s, "constructor literal");
		static_assert(value, "constructor static");
	}
};
template <typename>
struct B
{
	static const bool value = %(expr)s;
	static_assert(%(expr)s, "template class literal");
	static_assert(value, "template class static");
	B(A a)
	{
		static_assert(%(expr)s, "constructor literal");
		static_assert(value, "constructor self static");
		static_assert(A::value, "constructor static");
		static_assert(a.value, "constructor member");
	}
	template <typename R>
		B(B<R> &&)
		{
			static_assert(%(expr)s, "template constructor literal");
			static_assert(value, "template constructor self static");
			static_assert(B<R>::value, "template constructor static");
		}
};
template <typename T>
static void f(B<T> b)
{
	static_assert(%(expr)s, "template function literal");
	static_assert(B<T>::value, "template function static");
	static_assert(b.value, "template function member");
}
void f(A a);
void f(A a)
{
	static_assert(%(expr)s, "function literal");
	static_assert(A::value, "function static");
	static_assert(a.value, "function member");
	f(B<long>(B<int>(a)));
}
'''
			,_msg='for C++11 intrinsic static_assert when %s',_tdict={'expr' : 'true&&true'},_fdict={'expr' : 'false||false'}):
		"""
help:assume compiler supports C++ intrinsic static_assert
"""
		_Compile = self.Compile
		if not (_Compile(context, text=_f % _tdict, main='f(A());', msg=_msg % 'true') and \
				_Compile(context, text=_f % _fdict, main='f(A());', msg=_msg % 'false', expect_failure=True)):
			raise SCons.Errors.StopError('C++ compiler does not support tested versions of C++11 static_assert.')

	@_custom_test
	def check_namespace_disambiguate(self,context,_successflags={'CPPDEFINES' : ['DXX_HAVE_CXX_DISAMBIGUATE_USING_NAMESPACE']}):
		'''
Test how the compiler handles the ambiguity of a namespace at top level scope
and a class with the same name in an inner scope.

- gcc (all tested versions) treat `A` as a namespace, and find the variable
  declared in that namespace.
- clang (all tested versions) treat `A` as ambiguous and fail the compilation.

The gcc behavior is useful to allow selectively poisoning the dcx/dsx
namespaces when used in the wrong scope, without breaking legitimate accesses
to them.  Therefore, when the compiler can handle this ambiguity,
`common/include/dsx-ns.h` forward declares appropriate ambiguous names.
See the comments in that header for more details.

help:assume compiler can disambiguate classes and namespaces
'''
		self.Compile(context, text='''
namespace A
{
	int a;
}
using namespace A;
namespace B
{
	class A;
}
using namespace B;
''', main='return A::a;', msg='whether compiler handles classes from "using namespace"', successflags=_successflags)

	@_custom_test
	def check_cxx_std_required_features(self,context,_features=__cxx_std_required_features):
		# First test all the features at once.  If all work, then done.
		# If any fail, then the configure run will stop.
		_Compile = self.Compile
		if _Compile(context, text=_features.text, main=_features.main, msg='for required C++11, C++14, C++17, C++20 standard features'):
			return
		# Some failed.  Run each test separately and report to the user
		# which ones failed.
		failures = tuple(f.name for f in _features.features if not _Compile(context, text=f.text, main=f.main, msg=f'for C++{f.std} {f.name}'))
		raise SCons.Errors.StopError((f'C++ compiler does not support {", ".join(failures)}.'
		) if failures else 'C++ compiler supports each feature individually, but not all of them together.  Please report this as a bug in the Rebirth configure script.')

	def _show_pch_count_message(self,context,which,user_setting):
		count = user_setting if user_setting else 0
		context.Display(f'{self.msgprefix}: checking when to pre-compile {which} headers...{f"""if used at least {count} time{"s" if count > 1 else ""}""" if count > 0 else "never"}\n')
		return count > 0

	implicit_tests.append(_implicit_test.RecordedTest('check_pch_compile', "assume C++ compiler can create pre-compiled headers"))
	implicit_tests.append(_implicit_test.RecordedTest('check_pch_use', "assume C++ compiler can use pre-compiled headers"))

	@_custom_test
	def _check_pch(self,context,
		_Test=_Test,
		_testflags_compile_pch={'CXXFLAGS' : ['-x', 'c++-header']},
		_testflags_use_pch={'CXXFLAGS' : ['-Winvalid-pch', '-include', None]}
	):
		self.pch_flags = None
		# Always evaluate both
		co = self._show_pch_count_message(context, 'own', self.user_settings.pch)
		cs = self._show_pch_count_message(context, 'system', self.user_settings.syspch)
		context.did_show_result = True
		if not co and not cs:
			return
		context.Display(f'{self.msgprefix}: checking when to compute pre-compiled header input *pch.cpp...{"if missing" if self.user_settings.pch_cpp_assume_unchanged else "always"}\n')
		result = _Test(self, context, action=self.PCHAction(context), text='', msg='whether compiler can create pre-compiled headers', testflags=_testflags_compile_pch, calling_function='pch_compile')
		if not result:
			raise SCons.Errors.StopError("C++ compiler cannot create pre-compiled headers.")
		_testflags_use_pch = _testflags_use_pch.copy()
		_testflags_use_pch['CXXFLAGS'][-1] = str(context.lastTarget)[:-4]
		result = self.Compile(context, text='''
/* This symbol is defined in the PCH.  If the PCH is included, this
 * symbol will preprocess away to nothing.  If the PCH is not included,
 * then the compiler is not using PCHs as expected.
 */
dxx_compiler_supports_pch
''', msg='whether compiler uses pre-compiled headers', testflags=_testflags_use_pch, calling_function='pch_use')
		if not result:
			raise SCons.Errors.StopError("C++ compiler cannot use pre-compiled headers.")
		self.pch_flags = _testflags_compile_pch

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
		Compile = self.Compile
		if Compile(context, text=text.format(type=','.join(('int',)*count), value=','.join(('0',)*count)), main='a()', msg='whether compiler handles 20-element tuples'):
			return
		count = 2
		raise SCons.Errors.StopError(
			"Compiler cannot handle tuples of 20 elements.  Raise the template instantiation depth." \
			if Compile(context, text=text.format(type=','.join(('int',)*count), value=','.join(('0',)*count)), main='a()', msg='whether compiler handles 2-element tuples') \
			else "Compiler cannot handle tuples of 2 elements."
		)

	@_implicit_test
	def check_poison_valgrind(self,context):
		'''
help:add Valgrind annotations; wipe certain freed memory when running under Valgrind
'''
		context.Message(f'{self.msgprefix}: checking whether to use Valgrind poisoning...')
		r = 'valgrind' in self.user_settings.poison
		context.Result(r)
		self._define_macro(context, 'DXX_HAVE_POISON_VALGRIND', int(r))
		if not r:
			return
		text = '''
#define DXX_HAVE_POISON	1
#include "compiler-poison.h"
'''
		main = '''
	DXX_MAKE_MEM_UNDEFINED(std::span<int, 1>(&argc, 1));
'''
		if self.Compile(context, text=text, main=main, msg='whether Valgrind memcheck header works'):
			return True
		raise SCons.Errors.StopError("Valgrind poison requested, but <valgrind/memcheck.h> does not work.")

	@_implicit_test
	def check_poison_overwrite(self,context):
		'''
help:always wipe certain freed memory
'''
		context.Message(f'{self.msgprefix}: checking whether to use overwrite poisoning...')
		r = 'overwrite' in self.user_settings.poison
		context.Result(r)
		self._define_macro(context, 'DXX_HAVE_POISON_OVERWRITE', int(r))
		return r

	@_custom_test
	def _check_poison_method(self,context,
		_methods=(check_poison_overwrite, check_poison_valgrind),
		poison=0
	):
		# Always run both checks.  The user may want a program that
		# always uses overwrite poisoning and, when running under
		# Valgrind, marks the memory as undefined.
		for f in _methods:
			if f(self, context):
				poison = 1
		self._define_macro(context, 'DXX_HAVE_POISON', poison)
	implicit_tests.append(_implicit_test.RecordedTest('check_size_type_size', "assume size_t is formatted as `size_t`"))
	implicit_tests.append(_implicit_test.RecordedTest('check_size_type_long', "assume size_t is formatted as `unsigned long`"))
	implicit_tests.append(_implicit_test.RecordedTest('check_size_type_int', "assume size_t is formatted as `unsigned int`"))
	implicit_tests.append(_implicit_test.RecordedTest('check_size_type_I64', "assume size_t is formatted as `unsigned I64`"))

	@_custom_test
	def _check_size_type_format_modifier(self,context,_text_format='''
#include <cstddef>
#define DXX_PRI_size_type {}
dxx_compiler_attribute_format_printf(1, 2)
void f(const char *, ...);
void f(const char *, ...)
{{
}}
'''.format,_main='''
	std::size_t s = 0;
	f("%" DXX_PRI_size_type, s);
'''):
		'''
The test must declare a custom function to call with this format string.
gcc has hardcoded knowledge about how printf works, but that knowledge
on Mingw64 differs from the processing of
__attribute__((format(printf,...))).  Mingw64 requires I64u for
__attribute__((format)) functions, but llu for printf.  Mingw32 is
consistent in its use of u.  Linux gcc is consistent in its use of
lu.

-- cut --
#include <cstdio>
#include <cstddef>
__attribute__((format(printf,1,2))) void f(const char *,...);
void a() {
	std::size_t b = 0;
	printf("%I64u", b);
	f("%I64u", b);
	printf("%llu", b);
	f("%llu", b);
	printf("%lu", b);
	f("%lu", b);
	printf("%u", b);
	f("%u", b);
}
-- cut --

$ x86_64-w64-mingw32-g++-5.4.0 -x c++ -S -Wformat -o /dev/null -
<stdin>: In function 'void a()':
<stdin>:6:19: warning: format '%u' expects argument of type 'unsigned int', but argument 2 has type 'std::size_t {aka long long unsigned int}' [-Wformat=]
<stdin>:9:13: warning: unknown conversion type character 'l' in format [-Wformat=]
<stdin>:9:13: warning: too many arguments for format [-Wformat-extra-args]
<stdin>:10:17: warning: format '%lu' expects argument of type 'long unsigned int', but argument 2 has type 'std::size_t {aka long long unsigned int}' [-Wformat=]
<stdin>:11:12: warning: format '%lu' expects argument of type 'long unsigned int', but argument 2 has type 'std::size_t {aka long long unsigned int}' [-Wformat=]
<stdin>:12:16: warning: format '%u' expects argument of type 'unsigned int', but argument 2 has type 'std::size_t {aka long long unsigned int}' [-Wformat=]
<stdin>:13:11: warning: format '%u' expects argument of type 'unsigned int', but argument 2 has type 'std::size_t {aka long long unsigned int}' [-Wformat=]

$ i686-w64-mingw32-g++-5.4.0 -x c++ -S -Wformat -o /dev/null -
<stdin>: In function 'void a()':
<stdin>:7:14: warning: format '%I64u' expects argument of type 'long long unsigned int', but argument 2 has type 'std::size_t {aka unsigned int}' [-Wformat=]
<stdin>:8:18: warning: format '%llu' expects argument of type 'long long unsigned int', but argument 2 has type 'std::size_t {aka unsigned int}' [-Wformat=]
<stdin>:9:13: warning: unknown conversion type character 'l' in format [-Wformat=]
<stdin>:9:13: warning: too many arguments for format [-Wformat-extra-args]
<stdin>:10:17: warning: format '%lu' expects argument of type 'long unsigned int', but argument 2 has type 'std::size_t {aka unsigned int}' [-Wformat=]
<stdin>:11:12: warning: format '%lu' expects argument of type 'long unsigned int', but argument 2 has type 'std::size_t {aka unsigned int}' [-Wformat=]

$ mingw32-g++-5.4.0 -x c++ -S -Wformat -o /dev/null -
<stdin>: In function 'void a()':
<stdin>:6:19: warning: format '%I64u' expects argument of type 'long long unsigned int', but argument 2 has type 'std::size_t {aka unsigned int}' [-Wformat=]
<stdin>:7:14: warning: format '%I64u' expects argument of type 'long long unsigned int', but argument 2 has type 'std::size_t {aka unsigned int}' [-Wformat=]
<stdin>:8:18: warning: unknown conversion type character 'l' in format [-Wformat=]
<stdin>:8:18: warning: too many arguments for format [-Wformat-extra-args]
<stdin>:9:13: warning: unknown conversion type character 'l' in format [-Wformat=]
<stdin>:9:13: warning: too many arguments for format [-Wformat-extra-args]
<stdin>:10:17: warning: format '%lu' expects argument of type 'long unsigned int', but argument 2 has type 'std::size_t {aka unsigned int}' [-Wformat=]
<stdin>:11:12: warning: format '%lu' expects argument of type 'long unsigned int', but argument 2 has type 'std::size_t {aka unsigned int}' [-Wformat=]

$ x86_64-pc-linux-gnu-g++-5.4.0 -x c++ -S -Wformat -o /dev/null -
<stdin>: In function 'void a()':
<stdin>:6:19: warning: format '%u' expects argument of type 'unsigned int', but argument 2 has type 'std::size_t {aka long unsigned int}' [-Wformat=]
<stdin>:7:14: warning: format '%u' expects argument of type 'unsigned int', but argument 2 has type 'std::size_t {aka long unsigned int}' [-Wformat=]
<stdin>:8:18: warning: format '%llu' expects argument of type 'long long unsigned int', but argument 2 has type 'std::size_t {aka long unsigned int}' [-Wformat=]
<stdin>:9:13: warning: format '%llu' expects argument of type 'long long unsigned int', but argument 2 has type 'std::size_t {aka long unsigned int}' [-Wformat=]
<stdin>:12:16: warning: format '%u' expects argument of type 'unsigned int', but argument 2 has type 'std::size_t {aka long unsigned int}' [-Wformat=]
<stdin>:13:11: warning: format '%u' expects argument of type 'unsigned int', but argument 2 has type 'std::size_t {aka long unsigned int}' [-Wformat=]

'''
		# Test types in order of decreasing probability.
		for DXX_PRI_size_type, calling_function in (
			# Linux
			('z', 'size_type_size'),
			('l', 'size_type_long'),
			# Win64
			('I64', 'size_type_I64'),
			# Win32
			('', 'size_type_int'),
		):
			f = f'"{DXX_PRI_size_type}u"'
			if self.Compile(context, text=_text_format(f), main=_main, msg=f'whether to format std::size_t with "%{DXX_PRI_size_type}u"', calling_function=calling_function):
				context.sconf.Define('DXX_PRI_size_type', f)
				return
		raise SCons.Errors.StopError("C++ compiler rejects all candidate format strings for std::size_t.")
	implicit_tests.append(_implicit_test.RecordedTest('check_compiler_accepts_useless_cast', "assume compiler accepts -Wuseless-cast"))

	@_custom_test
	def check_compiler_useless_cast(self,context):
		Compile = self.Compile
		flags = {'CXXFLAGS' : get_Werror_sequence(context.env['CXXFLAGS'], ('-Wuseless-cast',))}
		if Compile(context, text='''
/*
 * SDL on Raspbian provokes a warning from -Wuseless-cast
 *
 * Reported-by: derhass <https://github.com/dxx-rebirth/dxx-rebirth/issues/257>
 */
#include <SDL_endian.h>

''', main='''
	return SDL_Swap32(argc);
''', msg='whether compiler argument -Wuseless-cast works with SDL', successflags=flags):
			return
		# <=clang-16 does not understand -Wuseless-cast
		# This test does not influence the compile environment, but is
		# run to distinguish in the output whether the failure is
		# because the compiler does not accept -Wuseless-cast or because
		# SDL's headers provoke a warning from -Wuseless-cast.
		Compile(context, text='', main='', msg='whether compiler accepts -Wuseless-cast', testflags=flags, calling_function='compiler_accepts_useless_cast')

	@_custom_test
	def check_compiler_ptrdiff_cast_int(self,context):
		context.sconf.Define('DXX_ptrdiff_cast_int', 'static_cast<int>'
			if self.Compile(context, text='''
#include <cstdio>
''', main='''
	char a[8];
	snprintf(a, sizeof(a), "%.*s", static_cast<int>(&argv[0][1] - &argv[0][0]), "0");
''', msg='whether to cast operator-(T*,T*) to int')
			else '',
		'''
For mingw32-gcc-5.4.0 and x86_64-pc-linux-gnu-gcc-5.4.0, gcc defines
`long operator-(T *, T *)`.

For mingw32, gcc reports a useless cast converting `long` to `int`.
For x86_64-pc-linux-gnu, gcc does not report a useless cast converting
`long` to `int`.

Various parts of the code take the difference of two pointers, then cast
it to `int` to be passed as a parameter to `snprintf` field width
conversion `.*`.  The field width conversion is defined to take `int`,
so the cast is necessary to avoid a warning from `-Wformat` on
x86_64-pc-linux-gnu, but is not necessary on mingw32.  However, the cast
causes a -Wuseless-cast warning on mingw32.  Resolve these conflicting
requirements by defining a macro that expands to `static_cast<int>` on
platforms where the cast is required and expands to nothing on platforms
where the cast is useless.
'''
		)

	@_custom_test
	def check_compiler_size_t_cast_unsigned_int(self,context):
		context.sconf.Define('DXX_size_t_cast_unsigned_int', 'static_cast<unsigned>'
			if self.Compile(context, text='''
#include <cstddef>
''', main='''
	std::size_t i{1000};
	unsigned u{static_cast<unsigned>(i)};
	(void)u;
''', msg='whether to cast std::size_t to unsigned int')
			else '')

	@_custom_test
	def check_strcasecmp_present(self,context,_successflags={'CPPDEFINES' : ['DXX_HAVE_STRCASECMP']}):
		main = '''
	return !strcasecmp(argv[0], argv[0] + 1) && !strncasecmp(argv[0] + 1, argv[0], 1);
'''
		self.Compile(context, text='#include <cstring>', main=main, msg='for strcasecmp', successflags=_successflags)

	@_custom_test
	def check_getaddrinfo_present(self,context,_successflags={'CPPDEFINES' : ['DXX_HAVE_GETADDRINFO']}):
		self.Compile(context, text='''
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif
''', main='''
	addrinfo *res = nullptr;
	addrinfo hints{};
#ifdef AI_NUMERICSERV
	/* Test that it is defined to a term that can be used with bit-or */
	hints.ai_flags |= AI_NUMERICSERV;
#endif
#if DXX_USE_IPv6
	hints.ai_flags |= AI_V4MAPPED | AI_ALL;
#endif
	int i = getaddrinfo("", "", &hints, &res);
	(void)i;
	freeaddrinfo(res);
	return 0;
''', msg='for getaddrinfo', successflags=_successflags)

	@_guarded_test_windows
	def check_inet_ntop_present(self,context,_successflags={'CPPDEFINES' : ['DXX_HAVE_INET_NTOP']}):
		# Linux and OS X have working inet_ntop on all supported
		# platforms.  Only Windows sometimes lacks support for this
		# function.
		if self.Compile(context, text='''
#include <winsock2.h>
#include <ws2tcpip.h>
''', main='''
	struct sockaddr_in sai;
	char dbuf[64];
	return inet_ntop(AF_INET, &sai.sin_addr, dbuf, sizeof(dbuf)) ? 0 : 1;
''', msg='for inet_ntop', successflags=_successflags):
			return
		if self.user_settings.ipv6:
			raise SCons.Errors.StopError("IPv6 enabled and inet_ntop not available: disable IPv6 or upgrade headers to support inet_ntop.")

	@_custom_test
	def check_timespec_present(self,context,_successflags={'CPPDEFINES' : ['DXX_HAVE_STRUCT_TIMESPEC']}):
		self.Compile(context, text='''
#include <time.h>
''', main='''
	struct timespec ts;
	(void)ts;
	return 0;
''', msg='for struct timespec', successflags=_successflags)

	@_custom_test
	def check_warn_implicit_fallthrough(self,context,_successflags={'CXXFLAGS' : ['-Wimplicit-fallthrough=5']}):
		main = '''
	int a = 1;
	switch (argc) {
		case 1:
			++ a;
			[[fallthrough]];
		case 2:
			++ a;
			break;
	}
	(void)a;
'''
		sconf = context.sconf
		self.Compile(context, text='', main=main, msg='whether compiler accepts -Wimplicit-fallthrough=5', successflags=_successflags)

	@_custom_test
	def check_compiler_overzealous_unused_lambda_capture(self,context):
		'''
<clang-5: untested
>clang-8: untested
>=clang-5 && <=clang-8: when -Wunused is passed, clang will warn when a lambda captures by reference a local variable that is itself a reference, regardless of whether the lambda body uses the captured value.  The warning is:

```
similar/main/object.cpp:1060:13: error: lambda capture 'vmobjptr' is not required to be captured for this use [-Werror,-Wunused-lambda-capture]
	auto l = [&vmobjptr, &r, &num_to_free](bool (*predicate)(const vcobjptr_t)) -> bool {
```

<gcc-8 require such variables to be captured and will fail the build if the capture is removed.
>=gcc-8 permit the variable not to be captured, and will not warn regardless of whether it is captured.

Test whether the compiler warns for this case and, if it does, tell it
not to warn.  Once support is removed for gcc versions which require the
capture, this test can be dropped and the warning reinstated.

If clang learns to treat -Wunused-lambda-capture (by way of -Wunused) as
a request to warn only about captures that are _actually_ unused, rather
than also those that are used but not required, this test will stop
disabling the warning on clang.  Since this quirk was discussed when the
warning was added[1], and the warning was added in its current form
anyway, it seems unlikely this will be fixed soon.

[1]: https://marc.info/?l=cfe-commits&m=148494796318826&w=2
|: https://marc.info/?l=cfe-commits&m=148509467111194&w=2
|: https://marc.info/?l=cfe-commits&m=148520882106501&w=2
		'''
		if not self.Compile(context, text='''
int a;
''', main='''
	auto &b = a;
	[&b]() {
		++b;
	}();
''', msg='whether compiler allows lambda capture of local references'):
			self.successful_flags['CXXFLAGS'].append('-Wno-unused-lambda-capture')

	@_custom_test
	def check_compiler_overzealous_warn_extension_string_literal_operator_template(self,context):
		'''
String literal operator templates are a GNU extension.  GCC accepts them
without complaint in mode `-std=gnu++14` (or later).  Clang warns that
the template is a GNU extension, even when given `-std=gnu++14`, which
requests C++14 with GNU extensions.  Since warnings are normally treated
as errors, this breaks the build.  If the warning is suppressed, Clang
can compile the file correctly.

Test for whether the compiler will accept a string literal operator
template with the default options and, if not, add the clang option to
suppress the warning.
		'''
		if not self.Compile(context, text='''
template <typename T, T... v>
struct literal_as_type {};

template <typename T, T... v>
constexpr literal_as_type<T, v...> operator""_literal_as_type();
''', main='''
	auto b = decltype("a"_literal_as_type){};
	(void)b;
''', msg='whether compiler accepts string literal operator templates'):
			self.successful_flags['CXXFLAGS'].append('-Wno-gnu-string-literal-operator-template')

	@_custom_test
	def check_compiler_overzealous_braced_scalar_init(self,context,_text='''
/* clang-16 issues a spurious -Wbraced-scalar-init warning if the result
 * is returned anonymously.

```
error: braces around scalar initializer [-Werror,-Wbraced-scalar-init]
```

 * Removing the braces allows an implicit narrowing of the result, which would
 * be a trap for a future maintenance programmer.
 *
 * gcc accepts the anonymous brace-initialized result.
 */
unsigned f(unsigned i);
unsigned f(unsigned i)
{
	return {i + 1};
}

unsigned g(unsigned i);
unsigned g(unsigned i)
{
	++i;
	return {i};
}
''',_main='''
	f({5});
	g({6});
''',successflags={'CXXFLAGS' : ['-Wno-braced-scalar-init']}):
		if self.Compile(context, text=_text, main=_main, msg='whether compiler accepts braced variables as function arguments and return values'):
			return
		if self.Compile(context, text=_text, main=_main, msg='whether compiler accepts -Wno-braced-scalar-init', successflags=successflags):
			return
		raise SCons.Errors.StopError("C++ compiler rejects braced scalar initialization, even with `-Wno-braced-scalar-init`.")

	__preferred_compiler_options = (
		# Support for option '-fstrict-flex-arrays':
		# <=gcc-12: no
		# >=gcc-13: yes
		# <=clang-16: no
		# >=clang-17: untested
		'-fstrict-flex-arrays',
		'-fvisibility=hidden',
		'-Wduplicated-branches',
		'-Wduplicated-cond',
		'-Wstrict-flex-arrays',
		'-Wsuggest-attribute=noreturn',
		'-Wsuggest-final-types',
		'-Wsuggest-override',
		'-Wlogical-op',
		'-Wold-style-cast',
		'-Wredundant-decls',
	)
	__preferred_win32_linker_options = (
		'-Wl,--large-address-aware',
		'-Wl,--dynamicbase',
		'-Wl,--nxcompat',
	)
	# No build directives can modify the process environment, so
	# modifying this at class scope is safe.  Either every build will
	# have $SOURCE_DATE_EPOCH set or every build will have it clear.
	if os.getenv('SOURCE_DATE_EPOCH') is None:
		__preferred_win32_linker_options += ('-Wl,--insert-timestamp',)
	def __mangle_compiler_option_name(opt):
		return f'check_compiler_option{opt.replace("-", "_").replace("=", "_")}'
	def __mangle_linker_option_name(opt):
		return f'check_linker_option{opt.replace("-", "_").replace(",", "_")}'
	@_custom_test
	def check_preferred_compiler_options(self,context,
		ccopts=list(__preferred_compiler_options),
		ldopts=(),
		_text='''
/* clang on OS X incorrectly diagnoses mistakes in Apple system headers
 * even when -Wsystem-headers is not specified.  Include one known buggy
 * header here, so that the compilation will fail and the compiler will be
 * marked as not supporting -Wold-style-cast.
 */
#if defined(__APPLE__) && defined(__MACH__)
#include <ApplicationServices/ApplicationServices.h>
#endif

#include <SDL.h>
''',
		_mangle_compiler_option_name=__mangle_compiler_option_name,
		_mangle_linker_option_name=__mangle_linker_option_name
	):
		if self.user_settings.host_platform == 'win32':
			ldopts = self.__preferred_win32_linker_options
		Compile = self.Compile
		Link = self.Link
		f, desc = (Link, 'linker') if ldopts else (Compile, 'compiler')
		if f(context, text=_text, main='''
''', msg=f'whether {desc} accepts preferred options', successflags={'CXXFLAGS' : ccopts, 'LINKFLAGS' : ldopts}, calling_function=f'preferred_{desc}_options'):
			# Everything is supported.  Skip individual tests.
			return
		# Compiler+linker together failed.  Check if compiler alone will work.
		if f is Compile or not Compile(context, text=_text, main='', msg='whether compiler accepts preferred options', successflags={'CXXFLAGS' : ccopts}):
			# Compiler alone failed.
			# Run down the individual compiler options to find any that work.
			for opt in ccopts:
				Compile(context, text=_text, main='', msg=f'whether compiler accepts option {opt}', successflags={'CXXFLAGS' : (opt,)}, calling_function=_mangle_compiler_option_name(opt)[6:])
		# Run down the individual linker options to find any that work.
		for opt in ldopts:
			Link(context, text=_text, main='', msg=f'whether linker accepts option {opt}', successflags={'LINKFLAGS' : (opt,)}, calling_function=_mangle_linker_option_name(opt)[6:])
	@classmethod
	def register_preferred_compiler_options(cls,
		ccopts = ('-Wextra',) + __preferred_compiler_options,
		# Always register target-specific tests on the class.  Individual
		# targets will decide whether to run the tests.
		ldopts = __preferred_win32_linker_options,
		RecordedTest = Collector.RecordedTest,
		record = implicit_tests.append,
		_mangle_compiler_option_name=__mangle_compiler_option_name,
		_mangle_linker_option_name=__mangle_linker_option_name
	):
		del cls.register_preferred_compiler_options
		record(RecordedTest('check_preferred_linker_options', 'assume linker accepts preferred options'))
		mangle = _mangle_compiler_option_name
		for opt in ccopts:
			record(RecordedTest(mangle(opt), f'assume compiler accepts {opt}'))
		mangle = _mangle_linker_option_name
		for opt in ldopts:
			record(RecordedTest(mangle(opt), f'assume linker accepts {opt}'))
		assert cls.custom_tests[0].name == cls.check_cxx_works.__name__, cls.custom_tests[0].name
		assert cls.custom_tests[-1].name == cls._cleanup_configure_test_state.__name__, cls.custom_tests[-1].name

	@_implicit_test
	def check_boost_test(self,context):
		self._check_system_library(context, header=('boost/test/unit_test.hpp',), pretext='''
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Rebirth
''', main=None, lib='Boost.Test', text='''
BOOST_AUTO_TEST_CASE(f)
{
	BOOST_TEST(true);
}
''', testflags={'LIBS' : ['boost_unit_test_framework']})

	@_custom_test
	def _check_boost_test_required(self,context):
		register_runtime_test_link_targets = self.user_settings.register_runtime_test_link_targets
		context.Display(f'{self.msgprefix}: checking whether to build runtime tests...')
		context.Result(register_runtime_test_link_targets)
		if register_runtime_test_link_targets:
			self.check_boost_test(context)

	# dylibbundler is used to embed libraries into macOS app bundles,
	# however it's only meaningful for macOS builds, and, for those,
	# only required when frameworks are not used for the build.  Builds
	# should not fail on other operating system targets if it's absent.
	@GuardedCollector(_guarded_test_darwin, lambda user_settings: user_settings.macos_bundle_libs and not user_settings.macos_add_frameworks)
	def _check_dylibbundler(self, context, _common_error_text='; dylibbundler is required for compilation for a macOS target when not using frameworks and bundling libraries.  Set macos_bundle_libs=False or macos_add_frameworks=True, or install dylibbundler.'):
		if sys.platform != 'darwin':
			raise SCons.Errors.StopError(f'dylibbundler can only be run on macOS build hosts')
		context.Display(f'{self.msgprefix}: checking whether dylibbundler is installed and accepts -h...')
		try:
			p = StaticSubprocess.pcall(('dylibbundler', '-h'), stderr=subprocess.PIPE)
		except FileNotFoundError as e:
			context.Result(f'no; {e}')
			raise SCons.Errors.StopError(f'dylibbundler not found{_common_error_text}')
		expected = b'dylibbundler is a utility'
		first_output_line = p.out.splitlines()
		if first_output_line:
			first_output_line = first_output_line[0]
		# This test allows the expected text to appear anywhere in the
		# output.  Only the first line of output will be shown to SCons'
		# stdout.  The full output will be written to the SConf log.
		if p.returncode:
			reason = f'successful exit, but return code was {p.returncode}'
		elif expected not in p.out:
			reason = f'output to contain {expected.decode()!r}, but first line of output was: {first_output_line!r}'
		else:
			context.Result(f'yes; {first_output_line}')
			return
		context.Result(f'no; expected {reason}')
		context.Log(f'''scons: dylibbundler return code: {p.returncode!r}
scons: dylibbundler stdout: {p.out!r}
scons: dylibbundler stderr: {p.err!r}
''')
		raise SCons.Errors.StopError('`dylibbundler -h` failed to return expected output; dylibbundler is required for compilation for a macOS target when not using frameworks.  Set macos_bundle_libs=False or macos_add_frameworks=False (and handle the libraries manually), or install dylibbundler.')

	# This must be the last custom test.  It does not test the environment,
	# but is responsible for reversing test-environment-specific changes made
	# by check_cxx_works.
	@_custom_test
	def _cleanup_configure_test_state(self,context):
		sconf = context.sconf
		sconf.config_h_text = self.__commented_tool_versions + sconf.config_h_text
		context.env['CXXCOM'] = self.__cxx_com_prefix
		context.did_show_result = True
		ToolchainInformation.show_partial_environ(context.env, self.user_settings, context.Display, self.msgprefix)

ConfigureTests.register_preferred_compiler_options()

@dataclass(eq=False, slots=True)
class LazyObjectConstructor:
	class LazyObjectState:
		def __init__(self, sources: collections.abc.Sequence[str], transform_env=None, StaticObject_hook=None, transform_target=None):
			# `sources` must be non-empty, since it would have no use if
			# it was empty.
			#
			# Every element in `sources` must be a string.  Verify this
			# with an assertion that directly checks that the sequence
			# is not empty and indirectly, by way of attribute lookup,
			# checks that the elements are string-like.
			assert([s.encode for s in sources]), "sources must be a non-empty list of strings"
			self.sources = sources
			self.transform_env = transform_env
			self.StaticObject_hook = StaticObject_hook
			# If transform_target is not used, let references to
			# `self.transform_target` fall through to
			# `cls.transform_target`.
			if transform_target:
				self.transform_target = transform_target

		@staticmethod
		def transform_target(_, name, _splitext=os.path.splitext):
			return _splitext(name)[0]

	def __lazy_objects(self, source: collections.abc.Sequence[LazyObjectState],
			cache={}
		) -> tuple['SCons.Environment.StaticObject']:
		env = self.env
		# Use id because name needs to be hashable and have a 1-to-1
		# mapping to source.
		name = (id(env), id(source))
		value = cache.get(name)
		if value is None:
			prepare_target_name = '{}{}'.format
			StaticObject = env.StaticObject
			OBJSUFFIX = env['OBJSUFFIX']
			builddir = self.builddir
			value = []
			append = value.append
			for s in source:
				transform_target = s.transform_target
				transform_env = s.transform_env
				StaticObject_hook = s.StaticObject_hook
				StaticObject_kwargs = {} if transform_env is None else transform_env(self, env)
				for srcname in s.sources:
					target = builddir.File(prepare_target_name(transform_target(self, srcname), OBJSUFFIX))
					s = StaticObject(target=target, source=srcname,
							**StaticObject_kwargs
							)
					append(s)
					if StaticObject_hook is not None:
						StaticObject_hook(self, env, srcname, target, s)
			# Convert to a tuple so that attempting to modify a cached
			# result raises an error.
			cache[name] = value = tuple(value)
		return value

	def create_lazy_object_states_getter(states: collections.abc.Sequence[LazyObjectState], __lazy_objects=__lazy_objects):
		def get_objects(self):
			return __lazy_objects(self, states)
		return get_objects

	@staticmethod
	def create_lazy_object_getter(sources: collections.abc.Sequence[str], __LazyObjectState=LazyObjectState, __create_lazy_object_states_getter=create_lazy_object_states_getter):
		return __create_lazy_object_states_getter((__LazyObjectState(sources=sources),))

	create_lazy_object_states_getter = staticmethod(create_lazy_object_states_getter)

class FilterHelpText:
	_sconf_align = None
	def __init__(self):
		self.visible_arguments = []
	def FormatVariableHelpText(self, env, opt, help, default, actual, aliases):
		if not opt in self.visible_arguments:
			return ''
		if not self._sconf_align:
			self._sconf_align = len(max((s for s in self.visible_arguments if s[:6] == 'sconf_'), key=len))
		l = []
		if default is not None:
			if isinstance(default, str) and not default.isalnum():
				default = f'"{default}"'
			l.append(f'default: {default}')
		actual = getattr(self, opt, None)
		if actual is not None:
			if isinstance(actual, str) and not actual.isalnum():
				actual = f'"{actual}"'
			l.append(f'current: {actual}')
		return f' {opt:{self._sconf_align if opt[:6] == "sconf_" else 15}}  {help}{f""" [{"; ".join(l)}]""" if l else ""}\n'

class PCHManager:
	@dataclass(eq=False)
	class ScannedFile:
		candidates: defaultdict(set)

	syspch_cpp_filename: str | None = None
	ownpch_cpp_filename: str | None = None
	syspch_object_node: typing.Union['StaticObject', None] = None
	ownpch_object_node: typing.Union['StaticObject', None] = None
	required_pch_object_node: typing.Union['StaticObject', None] = None

	# Compile on first use, so that non-PCH builds skip the compile
	_re_preproc_match: collections.abc.Callable[[bytes], 're.Match'] = None
	_re_include_match: collections.abc.Callable[[bytes], 're.Match'] = None
	_re_singleline_comments_sub: collections.abc.Callable[[bytes, bytes], bytes]  = None
	# Source files are tracked at class scope because all builds share
	# the same source tree.
	_cls_scanned_files: dict[str, ScannedFile] = None

	_instance_scanned_files: dict['fs.File', ScannedFile]
	pch_CXXFLAGS: list[str]

	# Import required modules when the first PCHManager is created.
	@classmethod
	def __initialize_cls_static_variables(cls):
		from re import compile as c
		# Match C preprocessor directives that start with i or e,
		# capture any arguments, and allow no arguments.  This accepts:
		# - #if
		# - #ifdef
		# - #ifndef
		# - #else
		# - #endif
		# - #include
		# - #error
		cls._re_preproc_match = c(br'#\s*([ie]\w+)(?:\s+(.*))?').match
		# Capture the argument in a #include statement, including the
		# angle brackets or quotes.
		#
		# Rebirth currently has no computed includes, so ignore computed
		# includes.
		cls._re_include_match = c(br'(<[^>]+>|"[^"]+")').match
		# Strip a single-line C++ style comment ("// Text") and any
		# preceding whitespace.  The C preprocessor will discard these
		# when compiling the header.  Discarding them from the
		# environment Value will prevent rebuilding pch.cpp when the
		# only change is in such a comment.
		cls._re_singleline_comments_sub = c(br'\s*//.*').sub
		# dict with key=filename with path, value=ScannedFile
		cls._cls_scanned_files = {}
		from tempfile import mkstemp
		cls._tempfile_mkstemp = staticmethod(mkstemp)

	def __init__(self,program,configure_pch_flags,common_pch_manager):
		if self._re_preproc_match is None:
			self.__initialize_cls_static_variables()
			assert self._re_preproc_match is not None
		self.user_settings = user_settings = program.user_settings
		assert user_settings.syspch or user_settings.pch or not configure_pch_flags
		self.env = env = program.env
		# dict with key=fs.File, value=ScannedFile
		self._instance_scanned_files = {}
		self._common_pch_manager = common_pch_manager
		syspch_cpp_filename = ownpch_cpp_filename = None
		syspch_object_node = None
		CXXFLAGS = env['CXXFLAGS'] + (configure_pch_flags['CXXFLAGS'] if configure_pch_flags else [])
		File = env.File
		builddir = program.builddir
		pch_subdir_node = builddir.Dir(program.srcdir)
		if user_settings.syspch:
			self.syspch_cpp_node = pch_subdir_node.File('syspch.cpp')
			self.syspch_cpp_filename = syspch_cpp_filename = str(self.syspch_cpp_node)
			self.required_pch_object_node = self.syspch_object_node = syspch_object_node = env.StaticObject(target=f'{syspch_cpp_filename}.gch', source=self.syspch_cpp_node, CXXCOM=env._dxx_cxxcom_no_ccache_prefix, CXXFLAGS=CXXFLAGS)
		if user_settings.pch:
			self.ownpch_cpp_node = pch_subdir_node.File('ownpch.cpp')
			self.ownpch_cpp_filename = ownpch_cpp_filename = str(self.ownpch_cpp_node)
			if syspch_object_node:
				CXXFLAGS += ['-include', syspch_cpp_node, '-Winvalid-pch']
			self.required_pch_object_node = self.ownpch_object_node = ownpch_object_node = env.StaticObject(target=f'{ownpch_cpp_filename}.gch', source=self.ownpch_cpp_node, CXXCOM=env._dxx_cxxcom_no_ccache_prefix, CXXFLAGS=CXXFLAGS)
			env.Depends(ownpch_object_node, builddir.File('dxxsconf.h'))
			if syspch_object_node:
				env.Depends(ownpch_object_node, syspch_object_node)
		if not configure_pch_flags:
			return
		self.pch_CXXFLAGS = ['-include', self.ownpch_cpp_node or syspch_cpp_node, '-Winvalid-pch']
		# If assume unchanged and the file exists, set __files_included
		# to a dummy value.  This bypasses scanning source files and
		# guarantees that the text of pch.cpp is not changed.  SCons
		# will still recompile pch.cpp into a new .gch file if pch.cpp
		# includes files that SCons recognizes as changed.
		if user_settings.pch_cpp_assume_unchanged and \
			(not syspch_cpp_filename or os.path.exists(syspch_cpp_filename)) and \
			(not ownpch_cpp_filename or os.path.exists(ownpch_cpp_filename)):
			self.__files_included = True
		else:
			# collections.defaultdict with key from ScannedFile.candidates,
			# value is a collections.Counter with key=tuple of preprocessor
			# guards, value=count of times this header was included under
			# that set of guards.
			self.__files_included = defaultdict(collections_counter)
		self.__env_Program = env.Program
		self.__env_StaticObject = env.StaticObject
		env.Program = self.Program
		env.StaticObject = self.StaticObject

	def record_file(self,env,source_file):
		# Every scanned file goes into self._cls_scanned_files to
		# prevent duplicate scanning from multiple targets.
		f = self._cls_scanned_files.get(source_file, None)
		if f is None:
			self._cls_scanned_files[source_file] = f = self.scan_file(env, source_file)
		self._instance_scanned_files[source_file] = f
		return f

	# Scan a file for preprocessor directives related to conditional
	# compilation and file inclusion.
	#
	# The #include directives found are eventually written to pch.cpp
	# with their original preprocessor guards in place.  Since the
	# preprocessor guards are kept and the C preprocessor will evaluate
	# them when compiling the header, this scanner does not attempt to
	# track which branches are true and which are false.
	#
	# This scanner makes no attempt to normalize guard conditions.  It
	# considers each of these examples to be distinct guards, even
	# though a full preprocessor will produce the same result for each:
	#
	#	#if 1
	#	#if 2
	#	#if 3 < 5
	#
	# or:
	#
	#	#ifdef A
	#	#if defined(A)
	#	#if (defined(A))
	#	#if !!defined(A)
	#
	# or:
	#
	# 	#ifndef A
	# 	#else
	# 	#endif
	#
	# 	#ifdef A
	# 	#endif
	#
	# Include directives are followed only if the calling file will not
	# be part of the output pch.cpp.  When the calling file will be in
	# pch.cpp, then the C preprocessor will include the called file, so
	# there is no need to scan it for other headers to include.
	#
	# This scanner completely ignores pragmas, #define, #undef, and
	# computed includes.
	@classmethod
	def scan_file(cls,env,source_filenode):
		match_preproc_directive = cls._re_preproc_match
		match_include_directive = cls._re_include_match
		preceding_line = None
		lines_since_preprocessor = None
		# defaultdict with key=name of header to include, value=set of
		# preprocessor guards under which an include was seen.  Set is
		# used because duplicate inclusions from a single source file
		# should not adjust the usage counter.
		candidates = defaultdict(set)
		# List of currently active preprocessor guards
		guard = []
		header_search_path = None
		os_path_join = os.path.join
		os_path_exists = os.path.exists
		for line in map(bytes.strip, source_filenode.get_contents().splitlines()):
			if preceding_line is not None:
				# Basic support for line continuation.
				line = b'%s %s' % (preceding_line[:-1], line)
				preceding_line = None
			elif not line.startswith(b'#'):
				# Allow unlimited non-preprocessor lines before the
				# first preprocessor line.  Once one preprocessor line
				# is found, track how many lines since the most recent
				# preprocessor line was seen.  If too many
				# non-preprocessor lines appear in a row, assume the
				# scanner is now down in source text and move to the
				# next file.
				if lines_since_preprocessor is not None:
					lines_since_preprocessor += 1
					if lines_since_preprocessor > 50:
						break
				continue
			lines_since_preprocessor = 0
			# Joined lines are rare.  Ignore complicated quoting.
			if line.endswith(b'\\'):
				preceding_line = line
				continue
			m = match_preproc_directive(line)
			if not m:
				# Not a preprocessor directive or not a directive that
				# this scanner handles.
				continue
			directive = m.group(1)
			if directive == b'include':
				m = match_include_directive(m.group(2))
				if not m:
					# This directive is probably a computed include.
					continue
				name = m.group(1)
				bare_name = name[1:-1]
				if name.startswith(b'"'):
					# Canonicalize paths to non-system headers
					if name == b'"dxxsconf.h"':
						# Ignore generated header here.  PCH generation
						# will insert it in the right order.
						continue
					if not name.endswith(b'.h"'):
						# Exclude non-header files from PCH.
						#
						# Starting in gcc-7, static functions defined in
						# valptridx.tcc generate -Wunused-function
						# warnings if the including file does not
						# instantiate any templates that use the static
						# function.
						continue
					if header_search_path is None:
						header_search_path = [
							d.encode() for d in ([os.path.dirname(str(source_filenode))] + env['CPPPATH'])
							# Filter out SDL paths
							if not d.startswith('/')
						]
					name = None
					for d in header_search_path:
						effective_name = os_path_join(d, bare_name)
						if os_path_exists(effective_name):
							name = effective_name
							break
					else:
						# name is None if:
						# - A system header was included using quotes.
						#
						# - A game-specific header was included in a
						# shared file.  A game-specific preprocessor
						# guard will prevent the preprocessor from
						# including the file, but the PCH scan logic
						# looks inside branches that the C preprocessor
						# will evaluate as false.
						continue
					name = env.File(name.decode())
				candidates[name].add(tuple(guard))
			elif directive == b'endif':
				while guard:
					g = guard.pop()
					if g == b'#else':
						# guard should always be True here, but test to
						# avoid ugly errors if scanning an ill-formed
						# source file.
						if guard:
							guard.pop()
						break
					if not g.startswith(b'#elif '):
						break
			elif directive == b'else':
				# #else is handled separately because it has no
				# arguments
				guard.append(b'#%s' % (directive))
			elif directive in (
				b'elif',
				b'if',
				b'ifdef',
				b'ifndef',
			):
				guard.append(b'#%s %s' % (directive, m.group(2)))
			elif directive not in (b'error',):
				raise SCons.Errors.StopError(f'Scanning {source_filenode!s} found unhandled C preprocessor directive {directive!r}')
		return cls.ScannedFile(candidates)

	def _compute_pch_text(self):
		self._compute_indirect_includes()
		own_header_inclusion_threshold = self.user_settings.pch
		sys_header_inclusion_threshold = self.user_settings.syspch
		configured_threshold = 0x7fffffff if self.user_settings.pch_cpp_exact_counts else None
		# defaultdict with key=name of tuple of active preprocessor
		# guards, value=tuple of (included file, count of times file was
		# seen with this set of guards, count of times file would be
		# included with this set of guards defined).
		syscpp_includes = defaultdict(list)
		owncpp_includes = defaultdict(list) if own_header_inclusion_threshold else None
		for included_file, usage_dict in self.__files_included.items():
			if isinstance(included_file, (bytes, str)):
				# System header
				cpp_includes = syscpp_includes
				name = (included_file.encode() if isinstance(included_file, str) else included_file)
				threshold = own_header_inclusion_threshold if sys_header_inclusion_threshold is None else sys_header_inclusion_threshold
			else:
				# Own header
				cpp_includes = owncpp_includes
				name = b'"%s"' % (str(included_file).encode())
				threshold = own_header_inclusion_threshold
			if not threshold:
				continue
			count_threshold = configured_threshold or threshold
			g = usage_dict.get((), 0)
			# As a special case, if this header is included
			# without any preprocessor guards, ignore all the
			# conditional includes.
			guards = \
				[((), g)] if (g >= threshold) else \
				sorted(usage_dict.items(), reverse=True)
			while guards:
				preprocessor_guard_directives, local_count_seen = guards.pop()
				total_count_seen = local_count_seen
				if total_count_seen < count_threshold:
					# If not eligible on its own, add in the count from
					# preprocessor guards that are always true when this
					# set of guards is true.  Since the scanner does not
					# normalize preprocessor directives, this is a
					# conservative count of always-true guards.
					g = preprocessor_guard_directives
					while g and total_count_seen < count_threshold:
						g = g[:-1]
						total_count_seen += usage_dict.get(g, 0)
					if total_count_seen < threshold:
						# If still not eligible, skip.
						continue
				cpp_includes[preprocessor_guard_directives].append((name, local_count_seen, total_count_seen))

		if syscpp_includes:
			self.__generated_syspch_lines = self._compute_pch_generated_lines(syscpp_includes)
		if owncpp_includes:
			self.__generated_ownpch_lines = self._compute_pch_generated_lines(owncpp_includes)

	def _compute_pch_generated_lines(self,cpp_includes):
		generated_pch_lines = []
		# Append guarded #include directives for files which passed the
		# usage threshold above.  This loop could try to combine related
		# preprocessor guards, but:
		# - The C preprocessor handles the noncombined guards correctly.
		# - As a native program optimized for text processing, the C
		# preprocessor almost certainly handles guard checking faster
		# than this script could handle guard merging.
		# - This loop runs whenever pch.cpp might be regenerated, even
		# if the result eventually shows that pch.cpp has not changed.
		# The C preprocessor will only run over the file when it is
		# actually changed and is processed to build a new .gch file.
		for preprocessor_guard_directives, included_file_tuples in sorted(cpp_includes.items()):
			generated_pch_lines.append(b'')
			generated_pch_lines.extend(preprocessor_guard_directives)
			# local_count_seen is the direct usage count for this
			# combination of preprocessor_guard_directives.
			#
			# total_count_seen is the sum of local_count_seen and all
			# guards that are a superset of this
			# preprocessor_guard_directives.  The total stops counting
			# when it reaches threshold, so it may undercount actual
			# usage.
			for (name, local_count_seen, total_count_seen) in sorted(included_file_tuples):
				assert(isinstance(name, bytes)), repr(name)
				generated_pch_lines.append(b'#include %s\t// %u %u' % (name, local_count_seen, total_count_seen))
			# d[2] == l if d is '#else' or d is '#elif'
			# Only generate #endif when d is a '#if*' directive, since
			# '#else/#elif' do not introduce a new scope.
			generated_pch_lines.extend(b'#endif' for d in preprocessor_guard_directives if d[2:3] != b'l')
		return generated_pch_lines

	def _compute_indirect_includes(self):
		own_header_inclusion_threshold = None if self.user_settings.pch_cpp_exact_counts else self.user_settings.pch
		# Count how many times each header is used for each preprocessor
		# guard combination.  After the outer loop finishes,
		# files_included is a dictionary that maps the name of the
		# include file to a collections.counter instance.  The mapped
		# counter maps a preprocessor guard to a count of how many times
		# it was used.
		#
		# Given:
		#
		# a.cpp
		# #include "a.h"
		# #ifdef A
		# #include "b.h"
		# #endif
		#
		# b.cpp
		# #include "b.h"
		#
		# files_included = {
		#	'a.h' : { () : 1 }
		#	'b.h' : {
		#		('#ifdef A',) : 1,	# From a.cpp
		#		() : 1,	# From b.cpp
		#	}
		# }
		files_included = self.__files_included
		for scanned_file in self._instance_scanned_files.values():
			for included_file, guards in scanned_file.candidates.items():
				i = files_included[included_file]
				for g in guards:
					i[g] += 1
		# If own_header_inclusion_threshold == 1, then every header
		# found will be listed in pch.cpp, so any indirect headers will
		# be included by the C preprocessor.
		if own_header_inclusion_threshold == 1:
			return
		# For each include file which is below the threshold, scan it
		# for includes which may end up above the threshold.
		File = self.env.File
		includes_to_check = sorted(files_included.iterkeys(), key=str)
		while includes_to_check:
			included_file = includes_to_check.pop()
			if isinstance(included_file, (bytes, str)):
				# System headers are str.  Internal headers are
				# fs.File.
				continue
			guards = files_included[included_file]
			unconditional_use_count = guards.get((), 0)
			if unconditional_use_count >= own_header_inclusion_threshold and own_header_inclusion_threshold:
				# Header will be unconditionally included in the PCH.
				continue
			f = self.record_file(self.env, File(included_file))
			for nested_included_file, nested_guards in sorted(f.candidates.items(), key=str):
				if not isinstance(included_file, (bytes, str)) and not nested_included_file in files_included:
					# If the header is a system header, it will be
					# str.  Skip system headers.
					#
					# Otherwise, if it has not been seen before,
					# append it to includes_to_check for recursive
					# scanning.
					includes_to_check.append(nested_included_file)
				i = files_included[nested_included_file]
				# If the nested header is included
				# unconditionally, skip the generator.
				for g in (nested_guards if unconditional_use_count else (a + b for a in guards for b in nested_guards)):
					i[g] += 1

	@classmethod
	def write_pch_inclusion_file(cls,target,source,env):
		target = str(target[0])
		dn, bn = os.path.split(target)
		fd, path = cls._tempfile_mkstemp(suffix='', prefix=f'{bn}.', dir=dn, text=True)
		# source[0].get_contents() returns the comment-stripped form
		os.write(fd, source[0].__generated_pch_text)
		os.close(fd)
		os.rename(path, target)

	def StaticObject(self,target,source,CXXFLAGS=None,*args,**kwargs):
		env = self.env
		source = env.File(source)
		o = self.__env_StaticObject(target=target, source=source, CXXFLAGS=self.pch_CXXFLAGS + (env['CXXFLAGS'] if CXXFLAGS is None else CXXFLAGS), *args, **kwargs)
		# Force an order dependency on the .gch file.  It is never
		# referenced by the command line or the source files, so SCons
		# may not recognize it as an input.
		env.Requires(o, self.required_pch_object_node)
		if self.__files_included is not True:
			self.record_file(env, source)
		return o

	def Program(self,*args,**kwargs):
		self._register_pch_commands()
		if self._common_pch_manager:
			self._common_pch_manager._register_pch_commands()
		return self.__env_Program(*args, **kwargs)

	def _register_pch_commands(self):
		if self.__files_included:
			# Common code calls this function once for each game which
			# uses it.  Only one call is necessary for proper operation.
			# Ignore all later calls.
			return
		self._compute_pch_text()
		syspch_lines = self.__generated_syspch_lines
		pch_begin_banner_template = b'''
// BEGIN PCH GENERATED FILE
// %r
// Threshold=%u

// SConf generated header
#include "dxxsconf.h"
'''
		pch_end_banner = (b'''
// END PCH GENERATED FILE
''',)
		if self.syspch_object_node:
			self._register_write_pch_inclusion(self.syspch_cpp_node,
				(
					(pch_begin_banner_template % (self.syspch_cpp_filename, self.user_settings.syspch),),
					syspch_lines,
					pch_end_banner,
				)
			)
			# ownpch.cpp will include syspch.cpp.gch from the command
			# line, so clear syspch_lines to avoid repeating system
			# includes in ownpch.cpp
			syspch_lines = ()
		if self.ownpch_object_node:
			self._register_write_pch_inclusion(self.ownpch_cpp_node,
				(
					(pch_begin_banner_template % (self.ownpch_cpp_filename, self.user_settings.pch),),
					(b'// System headers' if syspch_lines else b'',),
					syspch_lines,
					(b'''
// Own headers
''',),
					self.__generated_ownpch_lines,
					pch_end_banner,
				)
			)

	def _register_write_pch_inclusion(self,node,lineseq):
		# The contents of pch.cpp are taken from the iterables in
		# lineseq.  Set the contents as an input Value instead of
		# listing file nodes, so that pch.cpp is not rebuilt if a change
		# to a source file does not change what headers are listed in
		# pch.cpp.
		#
		# Strip C++ single line comments so that changes in comments are
		# ignored when deciding whether pch.cpp needs to be rebuilt.
		env = self.env
		text = b'\n'.join(itertools.chain.from_iterable(lineseq))
		v = env.Value(self._re_singleline_comments_sub(b'', text))
		v.__generated_pch_text = text
		# Use env.Command instead of env.Textfile or env.Substfile since
		# the latter use their input both for freshness and as a data
		# source.  This Command writes the commented form of the Value
		# to the file, but uses a comment-stripped form for SCons
		# freshness checking.
		env.Command(node, v, self.write_pch_inclusion_file)

class DXXCommon(LazyObjectConstructor):
	# version number
	VERSION_MAJOR: typing.Final[int] = 0
	VERSION_MINOR: typing.Final[int] = 61
	VERSION_MICRO: typing.Final[int] = 0
	DXX_VERSION_SEQ: typing.Final[str] = ','.join([str(VERSION_MAJOR), str(VERSION_MINOR), str(VERSION_MICRO)])
	pch_manager = None
	# dict compilation_database_dict_fn_to_entries:
	#	key: str: name of JSON file to which the data will be written
	#	value: tuple: (SCons.Environment, list_of_entries_to_write)
	# The first environment to use the named JSON file will be stored as
	# the environment in the tuple, and later used to provide
	# env.Textfile for all the entries written to the file.  When other
	# environments write to the same JSON file, they will append their
	# entries to the original list, and reuse the initial environment.
	# Since the build system does not customize env.Textfile, any
	# instance of it should be equally good.  This design permits
	# writing all the records for a build into a single file, even
	# though the build will use multiple SCons.Environment instances to
	# compile its source files.
	compilation_database_dict_fn_to_entries: dict[str, tuple[SCons.Environment, list]] = {}

	@dataclass(eq=False)
	class RuntimeTest(LazyObjectConstructor):
		target: str
		source: collections.abc.Sequence[str]
		nodefaultlibs: bool = True
		def __init__(self, target: str, source: collections.abc.Sequence[str]):
			self.target = target
			self.source = LazyObjectConstructor.create_lazy_object_getter(source)

	@cached_property
	def program_message_prefix(self):
		return f'{self.PROGRAM_NAME}.{self.program_instance}'

	@cached_property
	def builddir(self):
		return self.env.Dir(self.user_settings.builddir)

	# Settings which affect how the files are compiled
	class UserBuildSettings:
		class IntVariable:
			def __new__(cls,key,help,default):
				return (key, help, default, cls._validator, int)
			@staticmethod
			def _validator(key, value, env):
				try:
					int(value)
					return True
				except ValueError:
					raise SCons.Errors.UserError(f'Invalid value for integer-only option {key}: {value}.')
		class UIntVariable(IntVariable):
			@staticmethod
			def _validator(key, value, env):
				try:
					if int(value) < 0:
						raise ValueError
					return True
				except ValueError:
					raise SCons.Errors.UserError(f'Invalid value for unsigned-integer-only option {key}: {value}.')
		# Paths for the Videocore libs/includes on the Raspberry Pi
		RPI_DEFAULT_VC_PATH: str = '/opt/vc'
		default_OGLES_LIB: str = 'GLES_CM'
		default_EGL_LIB: str = 'EGL'
		_default_prefix: str = '/usr/local'
		__stdout_is_not_a_tty: bool | None = None
		__has_git_dir: bool | None = None
		def default_poison(self):
			return 'overwrite' if self.debug else 'none'
		def default_builddir(self):
			builddir_prefix = self.builddir_prefix
			builddir_suffix = self.builddir_suffix
			if builddir_prefix is not None or builddir_suffix is not None:
				fields = [
					self.host_platform,
					os.path.basename(self.CXX) if self.CXX else None,
				]
				compiler_flags = [(getattr(self, attr) or '').strip() for attr in ('CPPFLAGS', 'CXXFLAGS')]
				compiler_flags.append(str(not not self.sharepath))
				compiler_flags = '\n'.join(compiler_flags)
				if compiler_flags:
					# Mix in CRC of compiler flags to get reasonable uniqueness
					# when flags are changed.  This allows multiple builds with
					# varying flags to be built without overwriting each
					# other's outputs.  A full hash is unnecessary here, since
					# an accidental collision is unlikely, and the only
					# negative consequence of such a collision is that the
					# build will overwrite objects that would need to be built
					# again later when the flags are switched back.
					fields.append(f'{(binascii.crc32(compiler_flags.encode())):08x}')
				if self.pch:
					fields.append(f'p{self.pch}')
				elif self.syspch:
					fields.append(f'sp{self.syspch}')
				fields.append(''.join(a[1] if getattr(self, a[0]) else (a[2] if len(a) > 2 else '')
				for a in (
					('debug', 'dbg'),
					('lto', 'lto'),
					('editor', 'ed'),
					('opengl', 'ogl', 'sdl'),
					('opengles', 'es'),
					('raspberrypi', 'rpi'),
				)))
				default_builddir = (builddir_prefix or '') + '-'.join([f for f in fields if f])
				if builddir_suffix is not None:
					default_builddir += builddir_suffix
			else:
				default_builddir = 'build/'
			return default_builddir
		def default_memdebug(self):
			return self.debug
		# automatic setup for raspberrypi
		def default_opengles(self):
			if self.raspberrypi in ('yes',):
				return True
			return False
		def default_sdl2(self):
			if self.raspberrypi in ('mesa',) or self.host_platform == 'darwin':
				return True
			return False
		@classmethod
		def default_verbosebuild(cls):
			# Enable verbosebuild when the output is not directed to a
			# terminal.  When output is not a terminal, it is likely
			# either a pager or a log file, both of which can readily
			# handle the very long lines generated by verbose mode.
			r = cls.__stdout_is_not_a_tty
			if r is None:
				isatty = getattr(sys.stdout, 'isatty', None)
				# If isatty is None, then assume output is a TTY.
				cls.__stdout_is_not_a_tty = r = False if isatty is None else not isatty()
			return r
		def default_words_need_alignment(self):
			if self.raspberrypi in ('yes', 'mesa'):
				return True
			return False
		def default_use_stereo_render(self):
			return self.opengl and not self.opengles
		def selected_OGLES_LIB(self):
			if self.raspberrypi == 'yes':
				return 'brcmGLESv2'
			return self.default_OGLES_LIB
		def selected_EGL_LIB(self):
			if self.raspberrypi == 'yes':
				return 'brcmEGL'
			return self.default_EGL_LIB
		def need_dynamic_library_load(self):
			return self.adlmidi == 'runtime'
		def _enable_adlmidi(self):
			return self.adlmidi != 'none'

		def __default_DATA_DIR(self):
			platform_settings_type = self._program.get_platform_settings_type(self.host_platform)
			sharepath = platform_settings_type.sharepath
			if sharepath is None:
				return None
			return sharepath(prefix=self.prefix, program_target=self._program.target)

		def _generic_variable(key,help,default):
			return (key, help, default)
		def __get_configure_tests(tests,_filter=lambda s: s.name[0] != '_'):
			# Construct combined list on first use, then cache it
			# forever.
			try:
				return tests.__configure_tests
			except AttributeError:
				# Freeze the results into a tuple, to prevent accidental
				# modification later.
				#
				# In Python 3, filter returns an iterable that is
				# exhausted after one full traversal.  Since this object
				# is intended to be retained and reused, the first
				# traversal must copy the results into a container that
				# can be walked multiple times.  A tuple serves this
				# purpose, in addition to freezing the contents.
				tests.__configure_tests = c = tuple(filter(_filter, tests.implicit_tests + tests.custom_tests))
				return c
		@classmethod
		def __get_has_git_dir(cls):
			r = cls.__has_git_dir
			if r is None:
				# SConstruct is always at the top of the repository.
				# The user might have run `scons` from elsewhere and
				# used `-f` to indicate this file, but a false negative
				# is acceptable here.
				cls.__has_git_dir = r = os.path.exists(os.environ.get('GIT_DIR', '.git'))
			return r
		def _options(self,
				__get_configure_tests=__get_configure_tests,
				generic_variable=_generic_variable,
				BoolVariable=BoolVariable,
				EnumVariable=EnumVariable,
				conftests=ConfigureTests,
				getenv=os.environ.get
			):
			tests = __get_configure_tests(conftests)
			expect_sconf_tuple = ('0', '1', conftests.expect_sconf_success, conftests.expect_sconf_failure)
			sconf_tuple = ('0', '1', '2', conftests.sconf_force_failure, conftests.sconf_force_success, conftests.sconf_assume_success)
			sys_platform = sys.platform
			for platform in ('linux', 'freebsd', 'openbsd'):
				if sys_platform.startswith(platform):
					sys_platform = platform
					break
			return (
			{
				'variable': EnumVariable,
				'arguments': [
					(f'expect_sconf_{t.name[6:]}',
						None,
						None,
						{'allowed_values' : expect_sconf_tuple}
					) for t in tests
				],
			},
			{
				'variable': EnumVariable,
				'arguments': [
					(f'sconf_{t.name[6:]}',
						None,
						t.desc or f'assume result of {t.name}',
						{'allowed_values' : sconf_tuple}
					) for t in tests
				],
			},
			{
				'variable': EnumVariable,
				'arguments': (
					('host_platform',
						sys_platform,
						'cross-compile to specified platform',
						{
							'map': {'msys':'win32'},
							'allowed_values' : ('darwin', 'linux', 'freebsd', 'openbsd', 'win32', 'haiku1'),
							}
						),
					('raspberrypi', None, 'build for Raspberry Pi (automatically selects opengles)', {'ignorecase': 2, 'map': {'1':'yes', 'true':'yes', '0':'no', 'false':'no'}, 'allowed_values': ('yes', 'no', 'mesa')}),
				),
			},
			{
				'variable': BoolVariable,
				'arguments': (
					('record_sconf_results', False, 'write sconf results to dxxsconf.h'),
					('git_describe_version', self.__get_has_git_dir(), 'include git --describe in extra_version'),
					('git_status', True, 'include git status'),
					('versid_depend_all', False, 'rebuild vers_id.cpp if any object file changes'),
				),
			},
			{
				'variable': generic_variable,
				'arguments': (
					('rpi_vc_path', self.RPI_DEFAULT_VC_PATH, 'directory for RPi VideoCore libraries'),
					('opengles_lib', self.selected_OGLES_LIB, 'name of the OpenGL ES library to link against'),
					('egl_lib', self.selected_EGL_LIB, 'name of the OpenGL ES Graphics Library to link against'),
					('prefix', self._default_prefix, 'installation prefix directory (Linux only)'),
					('sharepath', self.__default_DATA_DIR, 'directory for shared game data'),
				),
			},
			{
				'variable': self.UIntVariable,
				'arguments': (
					('lto', 0, 'enable gcc link time optimization'),
					('pch', None, 'pre-compile own headers used at least this many times'),
					('syspch', None, 'pre-compile system headers used at least this many times'),
					('max_joysticks', 8, 'maximum number of usable joysticks'),
					('max_axes_per_joystick', 128, 'maximum number of axes per joystick'),
					('max_buttons_per_joystick', 128, 'maximum number of buttons per joystick'),
					('max_hats_per_joystick', 4, 'maximum number of hats per joystick'),
				),
			},
			{
				'variable': BoolVariable,
				'arguments': (
					('pch_cpp_assume_unchanged', False, 'assume text of *pch.cpp is unchanged'),
					('pch_cpp_exact_counts', False, None),
					('check_header_includes', False, 'compile test each header (developer option)'),
					('debug', False, 'build DEBUG binary which includes asserts, debugging output, cheats and more output'),
					('memdebug', self.default_memdebug, 'build with malloc tracking'),
					('opengl', True, 'build with OpenGL support'),
					('opengles', self.default_opengles, 'build with OpenGL ES support'),
					('editor', False, 'include editor into build (!EXPERIMENTAL!)'),
					('sdl2', self.default_sdl2, 'use libSDL2+SDL2_mixer (!EXPERIMENTAL!)'),
					# Build with SDL_Image support for PCX file support
					# Currently undocumented because the user experience
					# without PCX support is ugly, so this should always
					# be left enabled.
					('sdlimage', True, None),
					('sdlmixer', True, 'build with SDL_Mixer support for sound and music (includes external music support)'),
					('ipv6', False, 'enable UDP/IPv6 for multiplayer'),
					('use_udp', True, 'enable UDP support'),
					('use_tracker', True, 'enable Tracker support (requires UDP)'),
					('verbosebuild', self.default_verbosebuild, 'print out all compiler/linker messages during building'),
					# This is only examined for Mac OS X targets.
					#
					# Some users, particularly those who install
					# dependencies via Homebrew, may have the required C
					# headers and libraries available, but not packaged
					# as a Framework.  Such users should set
					# macos_add_frameworks=False, and (if the required
					# headers and libraries are not in a default search
					# location), use the standard C search path
					# directives to tell the compiler where to find the
					# required files.
					#
					# For users who have the framework installed in the
					# standard place, if macos_add_frameworks=True,
					# SCons should find the headers and libraries
					# automatically.
					('macos_add_frameworks', True, 'add required frameworks to CPPPATH, FRAMEWORKS, and FRAMEWORKPATH search variables (MacOS only); Homebrew users may want macos_add_frameworks=False'),
					# This is only examined for Mac OS X targets.
					#
					# dylibbundler can have issues on some systems, so
					# it is an optional step.  If used (and successful),
					# dylibbundler includes required libraries in the
					# generated app bundle and updates the executable
					# to reference them within the bundle.
					('macos_bundle_libs', False, 'bundle required libs into app bundle using dylibbundler'),
					# This is only examined for Windows targets, so
					# there is no need to make the default value depend
					# on the host_platform.
					('windows_minidump', True, 'generate a minidump on unhandled C++ exception (Windows only)'),
					('words_need_alignment', self.default_words_need_alignment, 'align words at load (needed for many non-x86 systems)'),
					('register_compile_target', True, 'report compile targets to SCons core'),
					('register_cpp_output_targets', None, None),
					('register_runtime_test_link_targets', False, None),
					('enable_build_failure_summary', True, 'print failed nodes and their commands'),
					('wrap_PHYSFS_read', False, None),
					('wrap_PHYSFS_write', False, None),
					# This is intentionally undocumented.  If a bug
					# report includes a log with this set to False, the
					# reporter will be asked to provide a log with the
					# value set to True.  Try to prevent the extra round
					# trip by hiding the option.
					('show_tool_version', True, None),
					# Only applicable if show_tool_version=True
					('show_assembler_version', True, None),
					('show_linker_version', True, None),
				),
			},
			{
				'variable': BoolVariable,
				'arguments': (
					('use_stereo_render', self.default_use_stereo_render, 'enable stereoscopic rendering'),
				),
			},
			{
				'variable': generic_variable,
				'arguments': (
					('CHOST', getenv('CHOST'), 'CHOST of output'),
					('CXX', getenv('CXX'), 'C++ compiler command'),
					('PKG_CONFIG', getenv('PKG_CONFIG'), 'PKG_CONFIG to run (Linux only)'),
					('RC', getenv('RC'), 'Windows resource compiler command'),
					('extra_version', None, 'text to append to version, such as VCS identity'),
					('ccache', None, 'path to ccache'),
					('compilation_database', None, None),
					('distcc', None, 'path to distcc'),
					('distcc_hosts', getenv('DISTCC_HOSTS'), 'hosts to distribute compilation'),
					# This is only examined for Mac OS X targets.
					#
					# This option signs a bundle (and the libraries
					# if embedded with macos_bundle_libs) with the
					# designated code signing identity.  To use it,
					# pass the common name of the certificate in the
					# Keychain you wish to use to sign the bundle and
					# libraries (as applicable).
					('macos_code_signing_identity', None, 'sign app bundle and embedded libs (if applicable) with this identity'),
					# This is only examined for Mac OS X targets.
					#
					# This option specifies a Keychain item to use
					# for credentials when submitting a signed app
					# bundle for notarization.  If this is not specified,
					# then specifying an Apple ID and team ID is required.
					# For details about storing credentials in a
					# compatible Keychain entry, see the store-credentials
					# example at:
					# https://developer.apple.com/documentation/security/notarizing_macos_software_before_distribution/customizing_the_notarization_workflow
					('macos_notarization_keychain_item', None, 'keychain item for notarization credentials'),
					# This is only examined for Mac OS X targets.
					#
					# An Apple ID to be used for notarization submissions.
					# If a valid Keychain item is not used, then this is
					# required.
					('macos_notarization_apple_id', None, 'Apple ID for notarization submissions'),
					# This is only examined for Mac OS X targets.
					#
					# The Apple team ID to use for notarization submissions.
					# If a valid Keychain item is not used, then this is
					# required.
					('macos_notarization_team_id', None, 'Apple team ID for notarization submissions'),
					# This is only examined for Mac OS X targets.
					#
					# The Apple app-specific password to use for
					# notarization submissions.  If a valid Keychain item
					# is not used, then this will be used if provided.
					# Otherwise, the submission process will provide an
					# interactive prompt for the password.
					('macos_notarization_password', None, 'Apple app-specific password for notarization submissions'),
				),
			},
			{
				'variable': generic_variable,
				'stack': ' ',
				'arguments': (
					('CPPFLAGS', getenv('CPPFLAGS'), 'C preprocessor flags'),
					('CXXFLAGS', getenv('CXXFLAGS'), 'C++ compiler flags'),
					('LINKFLAGS', getenv('LDFLAGS'), 'Linker flags'),
					('LIBS', getenv('LIBS'), 'Libraries to link'),
					# These are intentionally undocumented.  They are
					# meant for developers who know the implications of
					# using them.
					('CPPFLAGS_unchecked', None, None),
					('CXXFLAGS_unchecked', None, None),
					('LINKFLAGS_unchecked', None, None),
					# Flags that are injected into the compilation
					# database, but which are not used in the actual
					# compilation.  Use this to work around clang's
					# inability to find its own headers when invoked
					# from custom tools.
					('CXXFLAGS_compilation_database', None, None),
				),
			},
			{
				'variable': EnumVariable,
				'arguments': (
					('host_endian', None, 'endianness of host platform', {'allowed_values' : ('little', 'big')}),
					('adlmidi', 'none', 'include ADL MIDI support (none: disabled; runtime: dynamically load at runtime)', {'allowed_values' : ('none', 'runtime')}),
					('screenshot', 'png', 'screenshot file format', {'allowed_values' : ('none', 'legacy', 'png')}),
				),
			},
			{
				'variable': ListVariable,
				'arguments': (
					('poison', self.default_poison, 'method for poisoning free memory', {'names' : ('valgrind', 'overwrite')}),
				),
			},
			{
				'variable': generic_variable,
				'arguments': (
					('builddir_prefix', None, 'prefix to generated build directory'),
					('builddir_suffix', None, 'suffix to generated build directory'),
					# This must be last so that default_builddir will
					# have access to other properties.
					('builddir', self.default_builddir, 'build in specified directory'),
				),
			},
		)
		_generic_variable = staticmethod(_generic_variable)

		@staticmethod
		def _names(name,prefix):
			return [f'{p}{"_" if p else ""}{name}' for p in prefix]

		def __init__(self,program=None):
			self._program = program

		def register_variables(self,prefix,variables,filtered_help):
			self.known_variables = []
			append_known_variable = self.known_variables.append
			add_variable = variables.Add
			for grp in self._options():
				variable = grp['variable']
				stack = grp.get('stack', None)
				for opt in grp['arguments']:
					(name,value,help) = opt[0:3]
					kwargs = opt[3] if len(opt) > 3 else {}
					if name not in variables.keys():
						if help is not None:
							filtered_help.visible_arguments.append(name)
						add_variable(variable(key=name, help=help, default=None if callable(value) else value, **kwargs))
					names = self._names(name, prefix)
					for n in names:
						if n not in variables.keys():
							add_variable(variable(key=n, help=help, default=None, **kwargs))
					if not name in names:
						names.append(name)
					append_known_variable((names, name, value, stack))
					if stack:
						for n in names:
							add_variable(self._generic_variable(key=f'{n}_stop', help=None, default=None))
		def read_variables(self,program,variables,d):
			verbose_settings_init = os.getenv('DXX_SCONS_DEBUG_USER_SETTINGS')
			for (namelist,cname,dvalue,stack) in self.known_variables:
				value = None
				found_value = False
				for n in namelist:
					try:
						v = d[n]
						found_value = True
						if stack:
							if callable(v):
								if verbose_settings_init:
									message(program, f'append to stackable {cname!r} from {n!r} by call to {v!r}({dvalue!r}, {value!r}, {stack!r})')
								value = v(dvalue=dvalue, value=value, stack=stack)
							else:
								if value:
									value = (value, v)
									if verbose_settings_init:
										message(program, f'append to stackable {cname!r} from {n!r} by join of {stack!r}.join({value!r})')
									value = stack.join(value)
								else:
									if verbose_settings_init:
										message(program, f'assign to stackable {cname!r} from {n!r} value {v!r}')
									value = v
							stop = f'{n}_stop'
							if d.get(stop, None):
								if verbose_settings_init:
									message(program, f'terminating search early due to presence of {stop}')
								break
							continue
						if verbose_settings_init:
							message(program, f'assign to non-stackable {cname!r} from {n!r} value {v!r}')
						value = v
						break
					except KeyError as e:
						pass
				if not found_value:
					if callable(dvalue):
						value = dvalue()
						if verbose_settings_init:
							message(program, f'assign to {cname!r} by default value {value!r} from call {dvalue!r}')
					else:
						value = dvalue
						if verbose_settings_init:
							message(program, f'assign to {cname!r} by default value {value!r} from direct')
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
		tools = ('g++', 'gnulink')
		ogllibs = []
		platform_objects = ()
		sharepath = None

		def __init__(self,program,user_settings):
			self.__program = program
			self.user_settings = user_settings

		@property
		def builddir(self):
			return self.__program.builddir

		@property
		def env(self):
			return self.__program.env

	# Settings to apply to mingw32 builds
	class Win32PlatformSettings(_PlatformSettings):
		ogllibs = ['opengl32', 'glu32']
		tools = ('mingw',)
		def adjust_environment(self,program,env):
			env.Append(
				CPPDEFINES = ['_WIN32', 'WIN32_LEAN_AND_MEAN'],
				LINKFLAGS = ['-mwindows'],
			)
	class DarwinPlatformSettings(_PlatformSettings):
		# Darwin targets include Objective-C (not Objective-C++) code to
		# access Apple-specific functionality.  Add 'gcc' to the target
		# list to support this.
		#
		# Darwin targets need a special linker, because OS X uses
		# frameworks instead of standard libraries.  Using `gnulink`
		# omits framework-related arguments, causing the linker to skip
		# including required libraries.  SCons's `applelink` target
		# understands these quirks and ensures that framework-related
		# arguments are included.
		tools = ('gcc', 'g++', 'applelink')
		def adjust_environment(self,program,env):
			if self.user_settings.sdl2 == False:
				raise SCons.Errors.StopError('macOS builds do not support SDL 1.2.')
			macos_add_frameworks = self.user_settings.macos_add_frameworks
			if macos_add_frameworks:
				# The user may or may not have a private installation of
				# frameworks.  If there are no private frameworks, then
				# the path to the private frameworks should not be
				# added, because some versions of the tools issue a
				# diagnostic for searching a non-existant path.
				library_frameworks = os.path.join(os.getenv("HOME"), 'Library/Frameworks')
				if os.path.isdir(library_frameworks):
					# The private framework directory exists.  Check
					# whether the user has a private copy of the SDL
					# framework.
					env.Append(FRAMEWORKPATH = [library_frameworks])
					SDL_private_framework = os.path.join(library_frameworks, 'SDL.framework/Headers')
					if os.path.isdir(SDL_private_framework):
						# Yes, so add its headers to the C preprocessor
						# path.
						env.Append(CPPPATH = [SDL_private_framework])
					# else No, so omit the non-existant directory from
					# the C preprocessor path.
				# Check whether a system-wide SDL framework is
				# available.
				SDL_system_framework = '/Library/Frameworks/SDL.framework/Headers'
				if os.path.isdir(SDL_system_framework):
					env.Append(CPPPATH = [SDL_system_framework])
			env.Append(
				CPPDEFINES = ['__unix__'],
				FRAMEWORKS = ['ApplicationServices', 'Cocoa'],
				LINKFLAGS = ['-Wl,-rpath,@loader_path/../Frameworks'],	# Allow libraries & frameworks to go in app bundle
			)
			if macos_add_frameworks:
				env.Append(FRAMEWORKS = ['SDL'])
			if self.user_settings.opengl or self.user_settings.opengles:
				env.Append(
					CPPDEFINES = [('GL_SILENCE_DEPRECATION',)],
					FRAMEWORKS = ['OpenGL'],
				)
	# Settings to apply to Linux builds
	class LinuxPlatformSettings(_PlatformSettings):
		sharepath = '{prefix}/share/games/{program_target}'.format
		@property
		def ogllibs(self):
			user_settings = self.user_settings
			return [user_settings.opengles_lib, user_settings.egl_lib] if user_settings.opengles else ['GL', 'GLU']
		@staticmethod
		def get_platform_objects(_empty=()):
			return _empty
		def adjust_environment(self,program,env):
			env.Append(
				CXXFLAGS = ['-pthread'],
			)

	class HaikuPlatformSettings(LinuxPlatformSettings):
		def adjust_environment(self,program,env):
			# Note: the lack of super() is deliberate.  Haiku does not want the
			# environment changes that LinuxPlatformSettings.adjust_environment
			# would provide, so there is no call to the base class method.
			env.Append(
				LIBS = ['network'],
			)

	def __init__(self,user_settings,__program_instance=itertools.count(1)):
		self.program_instance = next(__program_instance)
		self.user_settings = user_settings

	@staticmethod
	def __get_found_includes(node, env, scanner, path):
		# Always call the original and always return at
		# least what it returned.  If this node is in the
		# whitelist, then also apply the hook logic.
		# Otherwise, this is a pass-through.
		r = node.__get_found_includes(env, scanner, path)
		return (r + [env.File(env['DXX_EFFECTIVE_SOURCE'])]) \
			if node in node._dxx_node_header_target_set else r

	def create_header_targets(self,__shared_header_file_list=[],__shared_cpp_dict={}):
		env = self.env
		builddir = self.builddir
		check_header_includes = __shared_cpp_dict.get(builddir)
		if check_header_includes is None:
			# Generate the list once, on first use.  Any other targets
			# will reuse it.
			#
			# Touch the file into existence.  It contains only a comment, but
			# must exist and have an extension of '.cpp'.
			check_header_includes = env.Textfile(target=builddir.File('check_header_includes.cpp'), source=env.Value('''
/* This file is always empty.  It is only present to act as the source
 * file for SCons targets that test individual headers.
 */
'''))
			__shared_cpp_dict[builddir] = check_header_includes
			# This is ugly, but all other known methods are worse.  The
			# object file needs to depend on the header that created it
			# and recursively depend on headers that contributed to the
			# header that created it.  Adding a dependency using
			# env.Depend creates the first-level dependency, but does
			# not recurse into the header.  Overriding
			# get_found_includes will cause SCons to recurse in the
			# desired way.
			#
			# Calling get_found_includes requires parameters that are
			# not easily obtained here.  There does not appear to be a
			# way to patch the node's dependency list to insert the
			# header other than to override get_found_includes.
			#
			# Newer SCons applied a performance optimization based on
			# using Python __slots__.  Using __slots__ makes it a
			# runtime error to overwrite get_found_includes on the
			# instance, so overwrite the class method instead.  However,
			# some instances of the class may not want this hook, so
			# keep a whitelist of nodes for which the hook is active.
			c = check_header_includes[0].__class__
			# If the attribute is missing, this is the first time that
			# an env.Textfile has been created for this purpose.  Create
			# the attribute so that subsequent passes do not add extra
			# copies of the same hook.
			#
			# Since a flag attribute is needed anyway, make it useful by
			# storing the hook whitelist in it.
			if not hasattr(c, '_dxx_node_header_target_set'):
				c._dxx_node_header_target_set = set()
				c.__get_found_includes = c.get_found_includes
				c.get_found_includes = self.__get_found_includes
			# Update the whitelist here, outside the hasattr block.
			# This is necessary so that the hook is created once, but
			# every env.Textfile created by this block is whitelisted.
			c._dxx_node_header_target_set.add(check_header_includes[0])
		if not __shared_header_file_list:
			headers = Git.pcall(['ls-files', '-z', '--', '*.h']).out
			if not headers:
				g = Git.pcall(['--version'], stderr=subprocess.STDOUT)
				raise SCons.Errors.StopError(
					"`git ls-files` failed.  `git --version` failed.  Check that Git is installed." \
						if g.returncode else \
					"`git ls-files` failed, but `git --version` works.  Check that scons is run from a Git repository."
				)
			# Filter out OS X related directories.  Files in those
			# directories assume they are only ever built on OS X, so
			# they unconditionally include headers specific to OS X.
			excluded_directories = (
				'common/arch/cocoa/',
			)
			# Use `.extend()` instead of assignment because
			# `__shared_header_file_list` is shared among all invocations of
			# this function.  An assignment would redirect the name
			# `__shared_header_file_list` to a non-shared copy of the list,
			# forcing future calls to generate their own list.
			__shared_header_file_list.extend([h for h in headers.decode().split('\0') if h and not h.startswith(excluded_directories)])
			if not __shared_header_file_list:
				raise SCons.Errors.StopError("`git ls-files` found headers, but none can be checked.")
		Depends = env.Depends
		StaticObject = env.StaticObject
		CPPFLAGS_template = env['CPPFLAGS']
		CPPFLAGS_no_sconf = CPPFLAGS_template + ['-g0', '-include', '$DXX_EFFECTIVE_SOURCE']
		CPPFLAGS_with_sconf = ['-include', 'dxxsconf.h'] + CPPFLAGS_no_sconf
		CXXCOMSTR = env.__header_check_output_COMSTR
		CXXFLAGS = env['CXXFLAGS']
		target = os.path.join(
				str(builddir.Dir(self.srcdir).Dir('check_header_includes')),
				f'${{DXX_EFFECTIVE_SOURCE}}{env["OBJSUFFIX"]}'
				)
		for name in __shared_header_file_list:
			if not name:
				continue
			if self.srcdir == 'common' and not name.startswith('common/'):
				# Skip game-specific headers when testing common
				continue
			if self.srcdir[0] == 'd' and name[0] == 'd' and not name.startswith(self.srcdir):
				# Skip d1 in d2 and d2 in d1
				continue
			# Compiler feature headers cannot include dxxsconf.h because
			# it confuses the dependency resolver when SConf runs.
			# Calling code must include dxxsconf.h before including the
			# compiler feature header, so add the inclusion here.
			#
			# For best test coverage, only headers that must avoid
			# including dxxsconf.h receive an implicit include.  Any
			# header which needs dxxsconf.h and can include it without
			# side effects must do so.
			StaticObject(target=target,
				CPPFLAGS=CPPFLAGS_with_sconf if name[:24] == 'common/include/compiler-' else CPPFLAGS_no_sconf,
				CXXCOMSTR=CXXCOMSTR,
				CXXFLAGS=CXXFLAGS,
				DXX_EFFECTIVE_SOURCE=name,
				source=check_header_includes)

	def _cpp_output_StaticObject(self,target=None,source=None,DXX_EFFECTIVE_SOURCE='$SOURCE',*args,**kwargs):
		CPPFLAGS = kwargs.get('CPPFLAGS', None)
		CXXFLAGS = kwargs.get('CXXFLAGS', None)
		env = self.env
		OBJSUFFIX = env['OBJSUFFIX']
		StaticObject = env.__cpp_output_StaticObject
		StaticObject(
			target=f'{target.removesuffix(OBJSUFFIX)}.i',
			source=source, OBJSUFFIX='.i',
			# Bypass ccache
			CXXCOM=env._dxx_cxxcom_no_prefix,
			CPPFLAGS=(env['CPPFLAGS'] if CPPFLAGS is None else CPPFLAGS),
			CXXFLAGS=(env['CXXFLAGS'] if CXXFLAGS is None else CXXFLAGS) + ['-E'],
			CXXCOMSTR=env.__generate_cpp_output_COMSTR,
			DXX_EFFECTIVE_SOURCE=DXX_EFFECTIVE_SOURCE,
		)
		return StaticObject(target=target, source=source, DXX_EFFECTIVE_SOURCE=DXX_EFFECTIVE_SOURCE, *args, **kwargs)

	def _compilation_database_StaticObject(self,target=None,source=None,*args,**kwargs):
		env = self.env
		StaticObject = env.__compilation_database_StaticObject
		objects = StaticObject(target=target, source=source, *args, **kwargs)
		# Exclude ccache/distcc from the persisted command line.  Store
		# `directory` as the directory of the `SConstruct` file.
		# Calls to `str` are necessary here to coerce SCons.Node objects
		# into strings that `json.dumps` can handle.
		# Prefer $DXX_EFFECTIVE_SOURCE if available, so that entries written
		# for check_header_includes=1 will name the specific header being
		# checked, not the dummy file `check_header_includes.cpp`.
		relative_file_path = str(kwargs.get('DXX_EFFECTIVE_SOURCE', source))
		# clang documentation is silent on whether this must be
		# absolute, but `clang-check` refuses to find files when this is
		# relative.
		directory = env.Dir('.').get_abspath()
		CXXFLAGS_compilation_database = self.user_settings.CXXFLAGS_compilation_database
		_dxx_cxxcom_no_prefix = env._dxx_cxxcom_no_prefix
		if CXXFLAGS_compilation_database:
			_dxx_cxxcom_no_prefix = _dxx_cxxcom_no_prefix.replace('$CXXFLAGS', '$CXXFLAGS $CXXFLAGS_compilation_database')
			kwargs['CXXFLAGS_compilation_database'] = CXXFLAGS_compilation_database
		self._compilation_database_entries.extend([
			{
				'command' : env.Override(kwargs).subst(_dxx_cxxcom_no_prefix, target=[o], source=source),
				'directory' : directory,
				'file' : relative_file_path,
				'output' : str(o),
				}
			for o in objects])
		return objects

	def _compilation_database_Program(self,target=None,source=None,*args,**kwargs):
		env = self.env
		Program = env.__compilation_database_Program
		objects = Program(target=target, source=source, *args, **kwargs)
		source = [s[0] for s in source] if source else []
		directory = env.Dir('.').get_abspath()
		self._compilation_database_entries.extend([
			{
				'command' : env.Override(kwargs).subst(env['LINKCOM'], target=[o], source=source),
				'directory' : directory,
				# The `'file'` key is mandatory.
				# `clang/lib/Tooling/JSONCompilationDatabase.cpp` will reject a
				# database that has any entries which lack a `'file'` key.  The
				# `'file'` key does not make sense for an output executable, so
				# invent a plausible looking string.  This should be safe,
				# since clang cannot usefully target an output executable, so
				# it should never need to interpret this `'file'` value.  This
				# dictionary entry is only present as a convenience to other
				# tools which may want to use the compilation database to know
				# the link flags.
				'file' : str(list(map(str, source))),
				'output' : str(o),
				}
			for o in objects])
		return objects

	def create_special_target_nodes(self,archive):
		env = self.env
		StaticObject = env.StaticObject
		env._rebirth_nopch_StaticObject = StaticObject
		user_settings = self.user_settings
		if user_settings.register_cpp_output_targets:
			env.__cpp_output_StaticObject = StaticObject
			env.StaticObject = StaticObject = self._cpp_output_StaticObject
		compilation_database = user_settings.compilation_database
		if compilation_database:
			env.__compilation_database_StaticObject = StaticObject
			env.StaticObject = self._compilation_database_StaticObject
			env.__compilation_database_Program = env.Program
			env.Program = self._compilation_database_Program
			# Only set self._compilation_database_entries if collection
			# of the compilation database is enabled.
			try:
				compilation_database_entries = self.compilation_database_dict_fn_to_entries[compilation_database][1]
			except KeyError:
				compilation_database_entries = []
				self.compilation_database_dict_fn_to_entries[compilation_database] = (env, compilation_database_entries)
			self._compilation_database_entries = compilation_database_entries

		if user_settings.check_header_includes:
			# Create header targets before creating the PCHManager, so that
			# create_header_targets() does not call the PCHManager
			# StaticObject hook.
			self.create_header_targets()
		if user_settings.register_runtime_test_link_targets:
			self._register_runtime_test_link_targets()
		configure_pch_flags = archive.configure_pch_flags
		if configure_pch_flags or env.GetOption('clean'):
			self.pch_manager = PCHManager(self, configure_pch_flags, archive.pch_manager)

	@staticmethod
	def _quote_cppdefine(s,f=repr,b2a_hex=binascii.b2a_hex):
		r = ''
		prior = False
		for c in f(s):
			# No xdigit support in str
			if c in '+,-./:=_' or (c.isalnum() and not (prior and (c.isdigit() or c in 'abcdefABCDEF'))):
				r += c
			elif c == '\n':
				r += r'\n'
			else:
				r += rf'\\x{b2a_hex(c.encode()).decode()}'
				prior = True
				continue
			prior = False
		return f'\\"{r}\\"'

	@staticmethod
	def _quote_cppdefine_as_char_initializer(k,v):
		# - Format the input as `name=value`
		# - map(ord, formatted_string) to produce a map object that yields an
		#   iterable of integers representing the character codes of the
		#   formatted string
		# - map(str, inner_map) to convert the iterable of integers to an
		#   iterable of strings, where each string is the decimal digits of an
		#   input integer
		# - ','.join(outer_map) to convert the iterable of strings to a
		#   comma-separated string
		return f'DESCENT_{k}', ','.join(map(str, map(ord, f'{k}={v!r}')))

	@staticmethod
	def _encode_cppdefine_for_identifier(s,b32encode=base64.b32encode):
		# Identifiers cannot contain `=`; `z` is not generated by
		# b32encode, so use it in place of the pad character.
		if not s:
			return s
		return (b32encode(s.encode())	\
			.rstrip()	\
			.decode()	\
			.replace('=', 'z')	\
			)

	def prepare_environment(self):
		# Prettier build messages......
		# Move target to end of C++ source command
		target_string = ' -o $TARGET'
		env = self.env
		user_settings = self.user_settings
		# Expand $CXX immediately.
		# $CCFLAGS is never used.  Remove it.
		cxxcom = env['CXXCOM'] \
			.replace('$CXX ', f'{env["CXX"]} ') \
			.replace('$CCFLAGS ', '')
		if target_string + ' ' in cxxcom:
			cxxcom = f'{cxxcom.replace(target_string, "")}{target_string}'
		env._dxx_cxxcom_no_prefix = cxxcom
		distcc_path = user_settings.distcc
		distcc_cxxcom = f'{distcc_path} {cxxcom}' if distcc_path else cxxcom
		env._dxx_cxxcom_no_ccache_prefix = distcc_cxxcom
		ccache_path = user_settings.ccache
		# Add ccache/distcc only for compile, not link
		if ccache_path:
			cxxcom = f'{ccache_path} {cxxcom}'
			if distcc_path is not None:
				penv = self.env['ENV']
				if distcc_path:
					penv['CCACHE_PREFIX'] = distcc_path
				elif distcc_path is not None:
					penv.pop('CCACHE_PREFIX', None)
		elif distcc_path:
			cxxcom = distcc_cxxcom
		# Expand $LINK immediately.
		linkcom = env['LINKCOM'].replace('$LINK ', f'{env["LINK"]} ')
		# Move target to end of link command
		if target_string + ' ' in linkcom:
			linkcom = f'{linkcom.replace(target_string, "")}{target_string}'
		# Add $CXXFLAGS to link command
		cxxflags = '$CXXFLAGS '
		if ' ' + cxxflags not in linkcom:
			linkflags = '$LINKFLAGS'
			linkcom = linkcom.replace(linkflags, cxxflags + linkflags)
		env.Replace(
			CXXCOM = cxxcom,
			LINKCOM = linkcom,
		)
		# Custom DISTCC_HOSTS per target
		distcc_hosts = user_settings.distcc_hosts
		if distcc_hosts is not None:
			env['ENV']['DISTCC_HOSTS'] = distcc_hosts
		if user_settings.verbosebuild:
			env.__header_check_output_COMSTR	= None
			env.__generate_cpp_output_COMSTR	= None
		else:
			target = self.target[:3]
			nonblank_builddir = user_settings.builddir or '.'
			env.__header_check_output_COMSTR	= f'CHK {target} {nonblank_builddir} $DXX_EFFECTIVE_SOURCE'
			env.__generate_cpp_output_COMSTR	= f'CPP {target} {nonblank_builddir} $DXX_EFFECTIVE_SOURCE'
			env.Replace(
				CXXCOMSTR						= f'CXX {target} {nonblank_builddir} $SOURCE',
			# `builddir` is implicit since $TARGET is the full path to
			# the output
				LINKCOMSTR						= f'LD  {target} $TARGET',
				RCCOMSTR						= f'RC  {target} {nonblank_builddir} $SOURCE',
			)
			del nonblank_builddir

		cxxflags = [
			'-ftabstop=4',
			'-Wall',
			'-Wformat=2',
			'-Wextra',
			'-Wmissing-braces',
			'-Wmissing-include-dirs',
			'-Wuninitialized',
			'-Wundef',
			'-Wpointer-arith',
			'-Wcast-qual',
			'-Wmissing-declarations',
			'-Wvla',
			]
		if self.user_settings.lto:
			# clang does not support `=N` syntax, so use the bare form `-flto`
			# for `user_settings.lto=1`.  This allows `lto=1` to do the right
			# thing for clang users.  gcc effectively treats `-flto` as
			# `-flto=1`.
			#
			# Users who set `user_settings.lto=2` (or higher) need a compiler that
			# understands the `=N` syntax.  This script makes no attempt to
			# detect whether the compiler understands that syntax.
			#
			# The list of flags is prepended to the compiler options, so a
			# user-specified CXXFLAGS can override `-flto` later, if needed.
			cxxflags.append(f'-flto={self.user_settings.lto}' if self.user_settings.lto > 1 else '-flto')
		env.Prepend(CXXFLAGS = cxxflags)
		env.Append(
			CXXFLAGS = ['-funsigned-char'],
			CPPPATH = ['common/include', 'common/main', '.'],
			CPPFLAGS = SCons.Util.CLVar('-Wno-sign-compare'),
			CPPDEFINES = [
			# PhysFS 2.1 and later deprecate functions PHYSFS_read,
			# PHYSFS_write, which Rebirth uses extensively.  PhysFS 2.0
			# does not implement the new non-deprecated functions.
			# Disable the deprecation error until PhysFS 2.0 support is
			# removed.
				('PHYSFS_DEPRECATED', ''),
				# Every file that sees `CGameArg` must know whether a sharepath
				# exists.  Most files do not need to know the value of
				# sharepath.  Pass only its existence, so that changing the
				# sharepath does not needlessly rebuild those files.
				('DXX_USE_SHAREPATH', int(not not user_settings.sharepath)),
			],
		)
		add_flags = defaultdict(list)
		if user_settings.builddir:
			add_flags['CPPPATH'].append(user_settings.builddir)
		if user_settings.editor:
			add_flags['CPPPATH'].append('common/include/editor')
		CLVar = SCons.Util.CLVar
		for flags in ('CPPFLAGS', 'CXXFLAGS', 'LIBS', 'LINKFLAGS'):
			value = getattr(self.user_settings, flags)
			if value is not None:
				add_flags[flags] = CLVar(value)
		env.Append(**add_flags)

	@cached_property
	def platform_settings(self):
		# windows or *nix?
		platform_name = self.user_settings.host_platform
		try:
			machine = os.uname()[4]
		except AttributeError:
			machine = None
		message(self, f'compiling on {sys.platform!r}/{machine!r} for {platform_name!r} into {self.user_settings.builddir or "."}{f" with prefix list {self._argument_prefix_list}" if self._argument_prefix_list else ""}')
		return self.get_platform_settings_type(platform_name)(self, self.user_settings)

	@classmethod
	def get_platform_settings_type(cls,platform_name):
		# By happy accident, LinuxPlatformSettings produces the desired
		# result on OpenBSD, so there is no need for specific handling
		# of `platform_name == 'openbsd'`.
		return (
			cls.Win32PlatformSettings if platform_name == 'win32' else (
				cls.DarwinPlatformSettings if platform_name == 'darwin' else
				cls.HaikuPlatformSettings if platform_name == 'haiku1' else
				cls.LinuxPlatformSettings
			)
		)

	@cached_property
	def env(self):
		platform_settings = self.platform_settings
		# Acquire environment object...
		user_settings = self.user_settings
		# Get traditional compiler environment variables
		kw = {}
		chost_aware_tools = ('CXX', 'RC')
		for cc in chost_aware_tools:
			value = getattr(user_settings, cc)
			if value is not None:
				kw[cc] = value
		tools = platform_settings.tools + ('textfile',)
		if user_settings.register_runtime_test_link_targets:
			tools += 'ar',
		env = Environment(ENV = os.environ, tools = tools, **kw)
		CHOST = user_settings.CHOST
		if CHOST:
			denv = None
			for cc in chost_aware_tools:
				if cc in kw:
					# If the user set a value, that value is always
					# used.
					continue
				if denv is None:
					# Lazy load denv, so that its creation can be skipped if
					# the user set all the required values.
					denv = Environment(tools = tools)
				# If the option in the base environment, ignoring both
				# user_settings and the process environment, matches the
				# option in the customized environment, then assume that
				# this is an SCons default, not a user-chosen value.
				#
				# Override SCons default values with a CHOST qualified form.
				# Keep user-chosen values as the user set them.
				value = denv.get(cc)
				if value and env.get(cc) == value:
					env[cc] = f'{CHOST}-{value}'
		platform_settings.adjust_environment(self, env)
		return env

	def process_user_settings(self):
		env = self.env
		user_settings = self.user_settings

		# Insert default CXXFLAGS.  User-specified CXXFLAGS, if any, are
		# appended to this list and will override these defaults.  The
		# defaults are present to ensure that a user who does not set
		# any options gets a good default experience.
		env.Prepend(CXXFLAGS = ['-g', '-O2'])
		# Raspberry Pi?
		if user_settings.raspberrypi == 'yes':
			rpi_vc_path = user_settings.rpi_vc_path
			message(self, f'Raspberry Pi: using VideoCore libs in {rpi_vc_path!r}')
			env.Append(
				CPPDEFINES = ['RPI'],
			# use CPPFLAGS -isystem instead of CPPPATH because these those header files
			# are not very clean and would trigger some warnings we usually consider as
			# errors. Using them as system headers will make gcc ignoring any warnings.
				CPPFLAGS = [
				f'-isystem{rpi_vc_path}/include',
				f'-isystem{rpi_vc_path}/include/interface/vcos/pthreads',
				f'-isystem{rpi_vc_path}/include/interface/vmcs_host/linux',
			],
				LIBPATH = f'{rpi_vc_path}/lib',
				LIBS = ['bcm_host'],
			)

	def _register_runtime_test_link_targets(self):
		runtime_test_boost_tests = self.runtime_test_boost_tests
		if not runtime_test_boost_tests:
			return
		env = self.env
		user_settings = self.user_settings
		builddir = env.Dir(user_settings.builddir).Dir(self.srcdir)
		library = env.Library(builddir.File(f'{env["LIBPREFIX"]}{self.srcdir}{env["LIBSUFFIX"]}'), self.get_library_objects())
		for test in runtime_test_boost_tests:
			LIBS = [] if test.nodefaultlibs else env['LIBS'].copy()
			LIBS.extend((
				'boost_unit_test_framework',
				library,
				))
			env.Program(target=builddir.File(test.target), source=test.source(self), LIBS=LIBS)

	runtime_test_boost_tests: collections.abc.Sequence[RuntimeTest] = None

class DXXArchive(DXXCommon):
	PROGRAM_NAME: typing.Final[str] = 'DXX-Archive'
	_argument_prefix_list = None
	srcdir: typing.Final[str] = 'common'
	target: typing.Final[str] = 'dxx-common'

	get_library_objects = DXXCommon.create_lazy_object_getter((
'common/maths/fixc.cpp',
'common/maths/tables.cpp',
'common/maths/vecmat.cpp',
))

	RuntimeTest = DXXCommon.RuntimeTest
	runtime_test_boost_tests = (
		RuntimeTest('test-enumerate', (
			'common/unittest/enumerate.cpp',
			)),
		RuntimeTest('test-serial', (
			'common/unittest/serial.cpp',
			)),
		RuntimeTest('test-partial-range', (
			'common/unittest/partial_range.cpp',
			)),
		RuntimeTest('test-valptridx-range', (
			'common/unittest/valptridx-range.cpp',
			)),
		RuntimeTest('test-vecmat', (
			'common/unittest/vecmat.cpp',
			)),
		RuntimeTest('test-xrange', (
			'common/unittest/xrange.cpp',
			)),
		RuntimeTest('test-zip', (
			'common/unittest/zip.cpp',
			)),
			)
	del RuntimeTest

	def get_objects_common(self,
		__get_objects_common=DXXCommon.create_lazy_object_getter((
'common/2d/2dsline.cpp',
'common/2d/bitblt.cpp',
'common/2d/bitmap.cpp',
'common/2d/box.cpp',
'common/2d/canvas.cpp',
'common/2d/circle.cpp',
'common/2d/disc.cpp',
'common/2d/gpixel.cpp',
'common/2d/line.cpp',
'common/2d/palette.cpp',
'common/2d/pixel.cpp',
'common/2d/rect.cpp',
'common/2d/rle.cpp',
'common/2d/scalec.cpp',
'common/3d/draw.cpp',
'common/3d/globvars.cpp',
'common/3d/instance.cpp',
'common/3d/matrix.cpp',
'common/3d/points.cpp',
'common/3d/rod.cpp',
'common/3d/setup.cpp',
'common/arch/sdl/event.cpp',
'common/arch/sdl/joy.cpp',
'common/arch/sdl/key.cpp',
'common/arch/sdl/mouse.cpp',
'common/arch/sdl/timer.cpp',
'common/arch/sdl/window.cpp',
'common/main/cglobal.cpp',
'common/main/cli.cpp',
'common/main/cmd.cpp',
'common/main/cvar.cpp',
'common/main/piggy.cpp',
'common/maths/rand.cpp',
'common/mem/mem.cpp',
'common/misc/error.cpp',
'common/misc/hash.cpp',
'common/misc/hmp.cpp',
'common/misc/ignorecase.cpp',
'common/misc/physfsrwops.cpp',
'common/misc/physfsx.cpp',
'common/misc/strutil.cpp',
'common/misc/vgrphys.cpp',
'common/misc/vgwphys.cpp',
)), \
		__get_objects_use_adlmidi=DXXCommon.create_lazy_object_getter((
'common/music/adlmidi_dynamic.cpp',
)),
		__get_objects_use_sdl1=DXXCommon.create_lazy_object_getter((
'common/arch/sdl/rbaudio.cpp',
))
		):
		value = list(__get_objects_common(self))
		extend = value.extend
		extend(self.get_library_objects())
		user_settings = self.user_settings
		if user_settings._enable_adlmidi():
			extend(__get_objects_use_adlmidi(self))
		if not user_settings.sdl2:
			extend(__get_objects_use_sdl1(self))
		extend(self.platform_settings.get_platform_objects())
		return value

	get_objects_editor = DXXCommon.create_lazy_object_getter((
'common/editor/autosave.cpp',
'common/editor/func.cpp',
'common/ui/button.cpp',
'common/ui/checkbox.cpp',
'common/ui/dialog.cpp',
'common/ui/file.cpp',
'common/ui/gadget.cpp',
'common/ui/icon.cpp',
'common/ui/inputbox.cpp',
'common/ui/keypad.cpp',
'common/ui/keypress.cpp',
'common/ui/listbox.cpp',
'common/ui/menu.cpp',
'common/ui/menubar.cpp',
'common/ui/message.cpp',
'common/ui/radio.cpp',
'common/ui/scroll.cpp',
'common/ui/ui.cpp',
'common/ui/uidraw.cpp',
'common/ui/userbox.cpp',
))
	# for non-ogl
	get_objects_arch_sdl = DXXCommon.create_lazy_object_getter((
'common/3d/clipper.cpp',
'common/texmap/ntmap.cpp',
'common/texmap/scanline.cpp',
'common/texmap/tmapflat.cpp',
))
	# for ogl
	get_objects_arch_ogl = DXXCommon.create_lazy_object_getter((
'common/arch/ogl/ogl_extensions.cpp',
'common/arch/ogl/ogl_sync.cpp',
))
	get_objects_arch_sdlmixer = DXXCommon.create_lazy_object_getter((
'common/arch/sdl/digi_mixer_music.cpp',
'common/arch/sdl/jukebox.cpp',
))
	class Win32PlatformSettings(DXXCommon.Win32PlatformSettings):
		__get_platform_objects = LazyObjectConstructor.create_lazy_object_getter((
'common/arch/win32/except.cpp',
'common/arch/win32/messagebox.cpp',
))
		__get_sdl2_objects = LazyObjectConstructor.create_lazy_object_getter((
'common/arch/win32/rbaudio.cpp',
))
		def get_platform_objects(self):
			result = self.__get_platform_objects()
			if self.user_settings.sdl2:
				result += self.__get_sdl2_objects()
			return result

	class DarwinPlatformSettings(DXXCommon.DarwinPlatformSettings):
		get_platform_objects = LazyObjectConstructor.create_lazy_object_getter((
			'common/arch/cocoa/messagebox.mm',
		))

	class LinuxPlatformSettings(DXXCommon.LinuxPlatformSettings):
		__get_sdl2_objects = LazyObjectConstructor.create_lazy_object_getter((
'common/arch/sdl/messagebox.cpp',
))
		def get_platform_objects(self):
			result = ()
			if self.user_settings.sdl2:
				result += self.__get_sdl2_objects()
			return result

	def __init__(self,user_settings):
		user_settings = user_settings.clone()
		super().__init__(user_settings)
		if not user_settings.register_compile_target:
			return
		self.prepare_environment()
		self.process_user_settings()
		self.configure_environment()
		self.create_special_target_nodes(self)

	def configure_environment(self):
		user_settings = self.user_settings
		builddir = user_settings.builddir or '.'
		env = self.env
		try:
			builddir = env.Dir(builddir)
		except TypeError as e:
			raise SCons.Errors.StopError(e.args[0])
		tests = ConfigureTests(self.program_message_prefix, user_settings, self.platform_settings)
		log_file = builddir.File('sconf.log')
		conf = env.Configure(custom_tests = {
				k.name:getattr(tests, k.name) for k in tests.custom_tests
			},
			conf_dir=builddir.Dir('.sconf_temp'),
			log_file=log_file,
			config_h=builddir.File('dxxsconf.h'),
			clean=False,
			help=False
		)
		self.configure_added_environment_flags = tests.successful_flags
		self.configure_pch_flags = None
		if not conf.env:
			return
		cc_env_strings = tests.ForceVerboseLog(conf.env)
		try:
			for k in tests.custom_tests:
				getattr(conf, k.name)()
		except SCons.Errors.StopError as e:
			raise SCons.Errors.StopError(f'{e.args[0]}  See {log_file} for details.', *e.args[1:])
		cc_env_strings.restore(conf.env)
		if user_settings.pch:
			conf.Define('DXX_VERSION_SEQ', self.DXX_VERSION_SEQ)
		if user_settings.record_sconf_results:
			conf.config_h_text += f'''
/*
SConf test results
---
{''.join(f"""check_{n}={v}
""" for n,v in tests._sconf_results)}
 */
'''
		conf.Finish()
		self.configure_pch_flags = tests.pch_flags
		add_flags = self.configure_added_environment_flags
		CLVar = SCons.Util.CLVar
		for flags in ('CPPFLAGS', 'CXXFLAGS', 'LINKFLAGS'):
			value = getattr(user_settings, f'{flags}_unchecked')
			if value:
				add_flags[flags] += CLVar(value)
		# Wrapping PHYSFS_readBytes / PHYSFS_writeBytes is only useful when
		# Valgrind poisoning is enabled.  If Valgrind poisoning is not
		# enabled, the wrappers are not built (unless the appropriate
		# preprocessor define is added to $CPPFLAGS by the user).  The
		# link will fail if wrapping is enabled, Valgrind poisoning is
		# disabled, and the preprocessor define is not set.
		#
		# Only $LINKFLAGS are set here.  The preprocessor define is only
		# relevant to 2 source files, both of which delegate it to
		# header common/misc/vg-wrap-physfs.h, so omit it from the
		# general options to avoid unnecessary rebuilds of other source
		# files.
		if user_settings.wrap_PHYSFS_read:
			add_flags['LINKFLAGS'].extend((
					'-Wl,--wrap,PHYSFS_readBytes',
					'-Wl,--wrap,PHYSFS_readSBE16',
					'-Wl,--wrap,PHYSFS_readSBE32',
					'-Wl,--wrap,PHYSFS_readSLE16',
					'-Wl,--wrap,PHYSFS_readSLE32',
					'-Wl,--wrap,PHYSFS_readULE16',
					'-Wl,--wrap,PHYSFS_readULE32',
					))
		if user_settings.wrap_PHYSFS_write:
			add_flags['LINKFLAGS'].extend((
					'-Wl,--wrap,PHYSFS_writeBytes',
					'-Wl,--wrap,PHYSFS_writeSBE16',
					'-Wl,--wrap,PHYSFS_writeSBE32',
					'-Wl,--wrap,PHYSFS_writeSLE16',
					'-Wl,--wrap,PHYSFS_writeSLE32',
					'-Wl,--wrap,PHYSFS_writeULE16',
					'-Wl,--wrap,PHYSFS_writeULE32',
					))
		env.MergeFlags(add_flags)

class DXXProgram(DXXCommon):
	LazyObjectState = DXXCommon.LazyObjectState
	def _generate_kconfig_ui_table(program, env, source: typing.Final[str], target, kconfig_static_object: 'StaticObject') -> None:
			builddir = program.builddir.Dir(program.target)
			# Bypass ccache, if any, since this is a preprocess only
			# call.
			CXXFLAGS = env['CXXFLAGS']
			CXXFLAGS = CXXFLAGS.copy() if CXXFLAGS else []
			CXXFLAGS.append('-E')
			CPPDEFINES = env['CPPDEFINES']
			CPPDEFINES = CPPDEFINES.copy() if CPPDEFINES else []
			CPPDEFINES.extend((
					# Define these tokens to themselves so that
					# `#ifndef` does not try to redefine them.
					('DXX_KCONFIG_UI_ENUM', 'DXX_KCONFIG_UI_ENUM'),
					('DXX_KCONFIG_UI_LABEL', 'DXX_KCONFIG_UI_LABEL'),
					# Define this token to itself so that instances of
					# the kc_item structure are defined in the output
					# file.
					('kc_item', 'kc_item'),
					))
			cpp_kconfig_udlr = env._rebirth_nopch_StaticObject(target=str(target)[:-1] + 'ui-table.i', source=source[:-3] + 'ui-table.cpp', CPPDEFINES=CPPDEFINES, CXXCOM=env._dxx_cxxcom_no_ccache_prefix, CXXFLAGS=CXXFLAGS)
			generated_udlr_header = builddir.File('kconfig.udlr.h')
			generate_kconfig_udlr = env.File('similar/main/generate-kconfig-udlr.py')
			env.Command(generated_udlr_header, [cpp_kconfig_udlr, generate_kconfig_udlr], [[sys.executable, generate_kconfig_udlr, '$SOURCE', '$TARGET']])
			env.Depends(kconfig_static_object, generated_udlr_header)

	static_archive_construction = {}

	def _apply_target_name(self,name):
		return os.path.join(os.path.dirname(name), f'.{self.target}.{os.path.splitext(os.path.basename(name))[0]}')

	def _apply_env_version_seq(self,env,_empty={}):
		if self.user_settings.pch:
			return _empty
		CPPDEFINES = env['CPPDEFINES']
		CPPDEFINES = CPPDEFINES.copy() if CPPDEFINES else []
		CPPDEFINES.append(('DXX_VERSION_SEQ', self.DXX_VERSION_SEQ))
		return {
				'CPPDEFINES' : CPPDEFINES
				}

	get_objects_similar_arch_ogl = DXXCommon.create_lazy_object_states_getter((LazyObjectState(sources=(
'similar/arch/ogl/gr.cpp',
'similar/arch/ogl/ogl.cpp',
),
		transform_target=_apply_target_name,
	),
	))
	get_objects_similar_arch_sdl = DXXCommon.create_lazy_object_states_getter((LazyObjectState(sources=(
'similar/arch/sdl/gr.cpp',
),
		transform_target=_apply_target_name,
	),
	))
	get_objects_similar_arch_sdlmixer = DXXCommon.create_lazy_object_states_getter((LazyObjectState(sources=(
'similar/arch/sdl/digi_mixer.cpp',
),
		transform_target=_apply_target_name,
	),
	))
	__get_objects_common = DXXCommon.create_lazy_object_states_getter((LazyObjectState(sources=(
'similar/2d/font.cpp',
'similar/2d/palette.cpp',
'similar/2d/pcx.cpp',
'similar/3d/interp.cpp',
'similar/arch/sdl/digi.cpp',
'similar/arch/sdl/digi_audio.cpp',
'similar/arch/sdl/init.cpp',
'similar/main/ai.cpp',
'similar/main/aipath.cpp',
'similar/main/automap.cpp',
'similar/main/bm.cpp',
'similar/main/cntrlcen.cpp',
'similar/main/collide.cpp',
'similar/main/config.cpp',
'similar/main/console.cpp',
'similar/main/controls.cpp',
'similar/main/credits.cpp',
'similar/main/digiobj.cpp',
'similar/main/effects.cpp',
'similar/main/endlevel.cpp',
'similar/main/fireball.cpp',
'similar/main/fuelcen.cpp',
'similar/main/fvi.cpp',
'similar/main/game.cpp',
'similar/main/gamecntl.cpp',
'similar/main/gamefont.cpp',
'similar/main/gamemine.cpp',
'similar/main/gamerend.cpp',
'similar/main/gamesave.cpp',
'similar/main/gameseg.cpp',
'similar/main/gameseq.cpp',
'similar/main/gauges.cpp',
'similar/main/hostage.cpp',
'similar/main/hud.cpp',
'similar/main/iff.cpp',
'similar/main/kmatrix.cpp',
'similar/main/laser.cpp',
'similar/main/lighting.cpp',
'similar/main/menu.cpp',
'similar/main/mglobal.cpp',
'similar/main/mission.cpp',
'similar/main/morph.cpp',
'similar/main/newdemo.cpp',
'similar/main/newmenu.cpp',
'similar/main/object.cpp',
'similar/main/paging.cpp',
'similar/main/physics.cpp',
'similar/main/piggy.cpp',
'similar/main/player.cpp',
'similar/main/polyobj.cpp',
'similar/main/powerup.cpp',
'similar/main/render.cpp',
'similar/main/robot.cpp',
'similar/main/scores.cpp',
'similar/main/segment.cpp',
'similar/main/slew.cpp',
'similar/main/songs.cpp',
'similar/main/state.cpp',
'similar/main/switch.cpp',
'similar/main/terrain.cpp',
'similar/main/texmerge.cpp',
'similar/main/text.cpp',
'similar/main/titles.cpp',
'similar/main/vclip.cpp',
'similar/main/wall.cpp',
'similar/main/weapon.cpp',
'similar/misc/args.cpp',
),
		transform_target=_apply_target_name,
	), LazyObjectState(sources=(
'similar/main/inferno.cpp',
),
		transform_env = lambda self, env: {
			'CPPDEFINES' : tuple(itertools.chain(env['CPPDEFINES'], env.__dxx_CPPDEFINE_SHAREPATH, env.__dxx_CPPDEFINE_git_version))
			},
		transform_target=_apply_target_name,
	), LazyObjectState(sources=(
'similar/main/kconfig.cpp',
),
		StaticObject_hook=_generate_kconfig_ui_table,
		transform_target=_apply_target_name,
	), LazyObjectState(sources=(
'similar/misc/physfsx.cpp',
),
		transform_env = lambda self, env: {
			'CPPDEFINES' : tuple(itertools.chain(env['CPPDEFINES'], env.__dxx_CPPDEFINE_SHAREPATH))
			},
		transform_target=_apply_target_name,
	), LazyObjectState(sources=(
'similar/main/playsave.cpp',
),
		transform_env=_apply_env_version_seq,
		transform_target=_apply_target_name,
	),
	))
	get_objects_editor = DXXCommon.create_lazy_object_states_getter((LazyObjectState(sources=(
'similar/editor/centers.cpp',
'similar/editor/curves.cpp',
'similar/main/dumpmine.cpp',
'similar/editor/eglobal.cpp',
'similar/editor/elight.cpp',
'similar/editor/eobject.cpp',
'similar/editor/eswitch.cpp',
'similar/editor/group.cpp',
'similar/editor/info.cpp',
'similar/editor/kbuild.cpp',
'similar/editor/kcurve.cpp',
'similar/editor/kfuncs.cpp',
'similar/editor/kgame.cpp',
'similar/editor/khelp.cpp',
'similar/editor/kmine.cpp',
'similar/editor/ksegmove.cpp',
'similar/editor/ksegsel.cpp',
'similar/editor/ksegsize.cpp',
'similar/editor/ktmap.cpp',
'similar/editor/kview.cpp',
'similar/editor/med.cpp',
'similar/editor/meddraw.cpp',
'similar/editor/medmisc.cpp',
'similar/editor/medrobot.cpp',
'similar/editor/medsel.cpp',
'similar/editor/medwall.cpp',
'similar/editor/mine.cpp',
'similar/editor/objpage.cpp',
'similar/editor/segment.cpp',
'similar/editor/seguvs.cpp',
'similar/editor/texpage.cpp',
'similar/editor/texture.cpp',
),
		transform_target=_apply_target_name,
	),
	))

	class UserSettings(DXXCommon.UserSettings):
		@property
		def BIN_DIR(self):
			# installation path
			return f'{self.prefix}/bin'
	# Settings to apply to mingw32 builds
	class Win32PlatformSettings(DXXCommon.Win32PlatformSettings):
		def adjust_environment(self,program,env):
			super().adjust_environment(program, env)
			rcdir = 'similar/arch/win32'
			j = os.path.join
			resfile = env.RES(target=j(program.user_settings.builddir, rcdir, f'{program.target}.res{env["OBJSUFFIX"]}'), source=j(rcdir, 'dxx-rebirth.rc'))
			Depends = env.Depends
			File = env.File
			Depends(resfile, File(j(rcdir, 'dxx-rebirth.manifest')))
			Depends(resfile, File(j(rcdir, f'{program.target}.ico')))
			self.platform_objects = [resfile]
			env.Prepend(
				CXXFLAGS = ['-fno-omit-frame-pointer'],
				RCFLAGS = [f'-D{d[0]}={d[1]}' for d in program.env_CPPDEFINES] + ['-D' f'DXX_VERSION_SEQ={program.DXX_VERSION_SEQ}'],
			)
			env.Append(
				LIBS = ['wsock32', 'ws2_32', 'winmm', 'mingw32'],
			)
	# Settings to apply to Apple builds
	class DarwinPlatformSettings(DXXCommon.DarwinPlatformSettings):
		def adjust_environment(self,program,env):
			super().adjust_environment(program, env)
			VERSION = f'{program.VERSION_MAJOR}.{program.VERSION_MINOR}'
			if (program.VERSION_MICRO):
				VERSION += f'.{program.VERSION_MICRO}'
			env.Replace(
				VERSION_NUM = VERSION,
				VERSION_NAME = f'{program.PROGRAM_NAME} v{VERSION}',
			)
	# Settings to apply to Linux builds
	class LinuxPlatformSettings(DXXCommon.LinuxPlatformSettings):
		def __init__(self,program,user_settings):
			super().__init__(program, user_settings)
			if user_settings.sharepath and user_settings.sharepath[-1] != '/':
				user_settings.sharepath += '/'
		def adjust_environment(self,program,env):
			super().adjust_environment(program, env)
			user_settings = self.user_settings
			if user_settings.need_dynamic_library_load():
				env.Append(LIBS = ['dl'])

	def get_objects_common(self,
		__get_objects_common=__get_objects_common,
		__get_objects_multiplayer=DXXCommon.create_lazy_object_states_getter((LazyObjectState(sources=(
'similar/main/multi.cpp',
'similar/main/multibot.cpp',
),
		transform_target=_apply_target_name,
	),
	)),
		__get_objects_use_udp=DXXCommon.create_lazy_object_states_getter((LazyObjectState(sources=(
'similar/main/net_udp.cpp',
),
		transform_env= _apply_env_version_seq,
		transform_target=_apply_target_name,
	),
	))
		):
		value = list(__get_objects_common(self))
		extend = value.extend
		if self.user_settings.use_udp:
			extend(__get_objects_multiplayer(self))
			extend(__get_objects_use_udp(self))
		extend(self.platform_settings.platform_objects)
		return value

	def __init__(self,prefix,variables,filtered_help):
		self.variables = variables
		self._argument_prefix_list = prefix
		user_settings = self.UserSettings(program=self)
		super().__init__(user_settings)
		compute_extra_version = Git.compute_extra_version()
		git_describe_version = compute_extra_version.describe
		extra_version = f'v{self.VERSION_MAJOR}.{self.VERSION_MINOR}.{self.VERSION_MICRO}'
		if git_describe_version and not (extra_version == git_describe_version or extra_version[1:] == git_describe_version):
			extra_version += ' ' + git_describe_version
		print(f'===== {self.PROGRAM_NAME} {extra_version} {compute_extra_version.revparse_HEAD} =====')
		user_settings.register_variables(prefix, variables, filtered_help)

	# Run `init()`, but decorate any exception that occurs with a description
	# of the profile for this instance.  This allows the user to see which
	# profile triggered the exception.
	def init_with_decoration(self, substenv):
		try:
			self.init(substenv)
		except Exception as e:
			# Patch the exception's arguments instead of using Python exception
			# chaining.  Using `raise Exception('') from e` causes SCons not to
			# show the traceback for `e`.
			a = e.args or ()
			e.args = (f'Failed to initialize profile {self._argument_prefix_list!r}{f": {a[0]}" if a else ""}',) + a[1:]
			raise

	def init(self,substenv):
		user_settings = self.user_settings
		user_settings.read_variables(self, self.variables, substenv)
		archive = DXXProgram.static_archive_construction.get(user_settings.builddir, None)
		if archive is None:
			DXXProgram.static_archive_construction[user_settings.builddir] = archive = DXXArchive(user_settings)
		if user_settings.register_compile_target:
			self.prepare_environment(archive)
			self.process_user_settings()
		self.register_program()
		if user_settings.enable_build_failure_summary:
			self._register_build_failure_hook(archive)
		return self.variables.GenerateHelpText(self.env)

	def prepare_environment(self,archive):
		super().prepare_environment()
		env = self.env
		env.MergeFlags(archive.configure_added_environment_flags)
		self.create_special_target_nodes(archive)
		sharepath = self.user_settings.sharepath
		env.__dxx_CPPDEFINE_SHAREPATH = (
				('DXX_SHAREPATH', self._quote_cppdefine(sharepath, f=str)),
				) if sharepath else ()
		SCons.Script.Main.progress_display(f'{self.program_message_prefix}: default sharepath is {(sharepath or None)!r}')
		CPPDEFINES = list(self.env_CPPDEFINES)
		# For PRIi64
		CPPDEFINES.append(('__STDC_FORMAT_MACROS',))
		env.Append(
			CPPDEFINES = CPPDEFINES,
			CPPPATH = [os.path.join(self.srcdir, 'main')],
			LIBS = ['m'],
		)

	def register_program(self):
		user_settings = self.user_settings
		exe_target = user_settings.program_name
		if not exe_target:
			exe_target = os.path.join(self.srcdir, self.target)
			if user_settings.editor:
				exe_target += '-editor'
		env = self.env
		PROGSUFFIX = env['PROGSUFFIX']
		if PROGSUFFIX and not exe_target.endswith(PROGSUFFIX):
			exe_target += PROGSUFFIX
		exe_target = self.builddir.File(exe_target)
		if user_settings.register_compile_target:
			exe_target = self._register_program(exe_target)
			ToolchainInformation.show_partial_environ(env, user_settings, SCons.Script.Main.progress_display, self.program_message_prefix, append_newline='')
		if user_settings.register_install_target:
			bundledir = self._register_install(self.shortname, exe_target)
			if user_settings.macos_code_signing_identity is not None:
				self._macos_sign_and_notarize_bundle(self.shortname, bundledir)

	def _register_program(self,exe_target):
		env = self.env
		user_settings = self.user_settings
		static_archive_construction = self.static_archive_construction[user_settings.builddir]
		objects = static_archive_construction.get_objects_common()
		git_describe_version = Git.compute_extra_version() if user_settings.git_describe_version else Git.UnknownExtraVersion
		env.__dxx_CPPDEFINE_git_version = (
				('DXX_git_commit', git_describe_version.revparse_HEAD),
				('DXX_git_describe', self._encode_cppdefine_for_identifier(git_describe_version.describe))
				)
		objects.extend(self.get_objects_common())
		if user_settings.sdlmixer:
			objects.extend(static_archive_construction.get_objects_arch_sdlmixer())
			objects.extend(self.get_objects_similar_arch_sdlmixer())
		if user_settings.opengl or user_settings.opengles:
			env.Append(LIBS = self.platform_settings.ogllibs)
			static_objects_arch = static_archive_construction.get_objects_arch_ogl
			objects_similar_arch = self.get_objects_similar_arch_ogl
		else:
			static_objects_arch = static_archive_construction.get_objects_arch_sdl
			objects_similar_arch = self.get_objects_similar_arch_sdl
		objects.extend(static_objects_arch())
		objects.extend(objects_similar_arch())
		if user_settings.editor:
			objects.extend(self.get_objects_editor())
			objects.extend(static_archive_construction.get_objects_editor())
		versid_build_environ = ['CXX', 'CPPFLAGS', 'CXXFLAGS', 'LINKFLAGS']
		versid_cppdefines = env['CPPDEFINES'].copy()
		extra_version = user_settings.extra_version
		# Possible results:
		# extra_version is unset, git is available: use only git
		# extra_version is unset, git is not available: use version from SConstruct
		# extra_version is set, git is available: use "extra_version git_version"
		# extra_version is set, git is not available: use "extra_version"
		git_describe_version_describe_output = git_describe_version.describe
		if extra_version is None:
			extra_version =	git_describe_version_describe_output or	f'v{self.VERSION_MAJOR}.{self.VERSION_MINOR}{f".{self.VERSION_MICRO}" if self.VERSION_MICRO else ""}'
		else:
			if git_describe_version_describe_output:
				extra_version = f'{extra_version} {git_describe_version_describe_output}'
			# else use only extra_version as it was set in user_settings
		get_version_head = StaticSubprocess.get_version_head
		ld_path = ToolchainInformation.get_tool_path(env, 'ld')[1]
		_quote_cppdefine_as_char_initializer = self._quote_cppdefine_as_char_initializer
		versid_cppdefines.extend([_quote_cppdefine_as_char_initializer(k, env.get(k, '')) for k in versid_build_environ])
		if extra_version:
			# VERSION_EXTRA is special.  Format it as a string so that
			# it can be pasted into g_descent_version (in vers_id.cpp)
			# and look normal when printed as part of the startup
			# banner.  Since it is pasted into g_descent_version, it is
			# NOT included in versid_build_environ as an independent
			# field.
			versid_cppdefines.append(('DESCENT_VERSION_EXTRA', ','.join(map(str, map(ord, extra_version)))))
		versid_cppdefines.extend((
			_quote_cppdefine_as_char_initializer('CXX_version', get_version_head(env['CXX'])),
			_quote_cppdefine_as_char_initializer('LINK', ld_path.decode()),
			_quote_cppdefine_as_char_initializer('git_status', git_describe_version.status),
			_quote_cppdefine_as_char_initializer('git_diffstat', git_describe_version.diffstat_HEAD),
		))
		if ld_path:
			versid_cppdefines.append(
				_quote_cppdefine_as_char_initializer('LINK_version', get_version_head(ld_path))
			)
			versid_build_environ.append('LINK_version')
		versid_build_environ.extend((
			'CXX_version',
			'LINK',
			'git_status',
			'git_diffstat',
		))
		versid_cppdefines.append(('DXX_RBE"(A)"', f'"{"".join([f"A({k})" for k in versid_build_environ])}"'))
		versid_environ = self.env['ENV'].copy()
		# Direct mode conflicts with __TIME__
		versid_environ['CCACHE_NODIRECT'] = 1
		versid_cpp = 'similar/main/vers_id.cpp'
		versid_obj = env.StaticObject(target=f'{user_settings.builddir}{self._apply_target_name(versid_cpp)}{self.env["OBJSUFFIX"]}', source=versid_cpp, CPPDEFINES=versid_cppdefines, ENV=versid_environ)
		Depends = env.Depends
		# If $SOURCE_DATE_EPOCH is set, add its value as a shadow
		# dependency to ensure that vers_id.cpp is rebuilt if
		# $SOURCE_DATE_EPOCH on this run differs from $SOURCE_DATE_EPOCH
		# on last run.  If it is unset, use None for this test, so that
		# unset differs from all valid values.  Using a fixed value for
		# unset means that vers_id.cpp will not be rebuilt for this
		# dependency if $SOURCE_DATE_EPOCH was previously unset and is
		# currently unset, regardless of when it was most recently
		# built, but vers_id.cpp will be rebuilt if the user changes
		# $SOURCE_DATE_EPOCH and reruns scons.
		#
		# The process environment is not modified, so the default None
		# is never seen by tools that would reject it.
		Depends(versid_obj, env.Value(os.getenv('SOURCE_DATE_EPOCH')))
		if user_settings.versid_depend_all:
			# Optional fake dependency to force vers_id to rebuild so
			# that it picks up the latest timestamp.
			Depends(versid_obj, objects)
		objects.append(versid_obj)
		# finally building program...
		return env.Program(target=exe_target, source = objects)

	def _show_build_failure_summary():
		build_failures = SCons.Script.GetBuildFailures()
		if not build_failures:
			# If all targets built successfully, return.
			return
		reportable_failures = []
		total = 0
		for f in build_failures:
			if not f:
				continue
			e = f.executor
			if not e:
				continue
			total += 1
			e = e.get_build_env()
			# Test whether the build environment for this failure is one for
			# which failures should be reported.  If the attribute is present,
			# a report is desired.  If the attribute is absent, then this
			# failure should not be reported, but this method was called
			# because some other build environment was marked to report
			# failures.
			try:
				e.__show_build_failure_summary
			except AttributeError:
				continue
			reportable_failures.append(f)
		if not total:
			# If no failed targets had a usable executor, then nothing useful
			# can be reported.  Return silently.
			return
		print(f'Failed target count: total={total}; targets with enable_build_failure_summary=1: {len(reportable_failures)}')
		if reportable_failures:
			# If all the failures were in profiles that do not report failures,
			# then `reportable_failures` is empty.
			#
			# Sacrifice some precision on the command so that the printed
			# output is valid shell input.
			print(f'''Failed node list:
	{"""
	""".join(str(f.node) for f in reportable_failures)}
Failed command list:
 {"""
 """.join(" ".join(f.command) if isinstance(f.command, list) else f.command for f in reportable_failures)}''')

	def _register_build_failure_hook(self,archive,show_build_failure_summary=[_show_build_failure_summary]):
		# Always mark the environment as report-enabled.
		# Only register the atexit hook once.
		env = self.env
		env.__show_build_failure_summary = None
		archive.env.__show_build_failure_summary = None
		if not show_build_failure_summary:
			return
		from atexit import register
		register(show_build_failure_summary.pop())

	def _register_install(self,dxxstr,exe_node):
		env = self.env
		if self.user_settings.host_platform != 'darwin':
				install_dir = self.user_settings.BIN_DIR
				env.Install(install_dir, exe_node)
				env.Alias('install', install_dir)
		else:
			syspath = sys.path.copy()
			cocoa = 'common/arch/cocoa'
			sys.path += [cocoa]
			import tool_bundle
			sys.path = syspath
			tool_bundle.TOOL_BUNDLE(env)
			# bundledir is an SCons.Environment.Dir corresponding to the
			# first argument passed to MakeBundle, after any adjustments
			# made inside MakeBundle.
			#
			# appfile is an SCons.Environment.File corresponding to the
			# game executable copied into the `.app` directory.
			bundledir, appfile = env.MakeBundle(os.path.join(self.user_settings.builddir, f'{self.PROGRAM_NAME}.app'), exe_node,
					f'free.{dxxstr}-rebirth', os.path.join(cocoa, 'Info.plist'),
					typecode='APPL', creator='DCNT',
					icon_file=os.path.join(cocoa, f'{dxxstr}-rebirth.icns'),
					resources=[[os.path.join(self.srcdir, s), s] for s in ['English.lproj/InfoPlist.strings']])
			if self.user_settings.macos_bundle_libs and not self.user_settings.macos_add_frameworks:
				message(self, f'Bundling libraries for {bundledir!s}')
				# If the user has set $PATH, use it in preference to the
				# built-in SCons path.
				dylibenv = env['ENV']
				user_environment_path = os.environ.get('PATH')
				if user_environment_path:
					dylibenv = dylibenv.copy()
					dylibenv['PATH'] = user_environment_path
				env.Command(target=bundledir.Dir('Contents/libs'),
						source=appfile,
						action="dylibbundler -od -b -x $SOURCE -d $TARGET",
						ENV = dylibenv)
			return bundledir

	def _macos_sign_and_notarize_bundle(self,dxxstr,bundledir):
		if self.user_settings.host_platform != 'darwin' or bundledir is None:
			return
		env = self.env
		compressed_bundle = env.File(os.path.join(self.user_settings.builddir, f'{self.PROGRAM_NAME}.zip'))
		if self.user_settings.macos_notarization_keychain_item is not None:
			env.Command(target=compressed_bundle,
					source=bundledir,
					action=[['common/arch/macos/notarize_dxx_bundles.zsh', '--signing-identity', self.user_settings.macos_code_signing_identity, '--notarization-keychain-profile', self.user_settings.macos_notarization_keychain_item, '--binary-name', f'{dxxstr}-rebirth', '--app-bundle-path', '$SOURCE', '--zip-path', '$TARGET']])
		elif self.user_settings.macos_notarization_apple_id is not None and self.user_settings.macos_notarization_team_id is not None:
			if self.user_settings.macos_notarization_password is None:
				env.Command(target=compressed_bundle,
						source=bundledir,
						action=[['common/arch/macos/notarize_dxx_bundles.zsh', '--signing-identity', self.user_settings.macos_code_signing_identity, '--apple-id', self.user_settings.macos_notarization_apple_id, '--team-id', self.user_settings.macos_notarization_team_id, '--binary-name', f'{dxxstr}-rebirth', '--app-bundle-path', '$SOURCE', '--zip-path', '$TARGET']])
			else:
				notarization_password_env = env['ENV'].copy()
				notarization_password_env['DXX_NOTARIZATION_PASSWORD'] = self.user_settings.macos_notarization_password
				env.Command(target=compressed_bundle,
						source=bundledir,
						action=[['common/arch/macos/notarize_dxx_bundles.zsh', '--signing-identity', self.user_settings.macos_code_signing_identity, '--apple-id', self.user_settings.macos_notarization_apple_id, '--team-id', self.user_settings.macos_notarization_team_id, '--binary-name', f'{dxxstr}-rebirth', '--app-bundle-path', '$SOURCE', '--zip-path', '$TARGET']],
						ENV = notarization_password_env)
		else:
			raise SCons.Errors.StopError('Either macos_notarization_keychain_item or both macos_notarization_apple_id and macos_notarization_team_id must be specified for notarization')

class D1XProgram(DXXProgram):
	LazyObjectState = DXXProgram.LazyObjectState
	PROGRAM_NAME: typing.Final[str] = 'D1X-Rebirth'
	target: typing.Final[str]
	srcdir: typing.Final[str] = 'd1x-rebirth'
	target = srcdir
	shortname: typing.Final[str] = 'd1x'
	env_CPPDEFINES: typing.Final[tuple] = ('DXX_BUILD_DESCENT', 1),

	# general source files
	def get_objects_common(self,
		__get_dxx_objects_common=DXXProgram.get_objects_common, \
		__get_dsx_objects_common=DXXCommon.create_lazy_object_states_getter((LazyObjectState(sources=(
'd1x-rebirth/main/custom.cpp',
'd1x-rebirth/main/snddecom.cpp',
),
	),
	LazyObjectState(sources=(
			# In Descent 1, bmread.cpp is used for both the regular
			# build and the editor build.
			#
			# In Descent 2, bmread.cpp is only used for the editor
			# build.
			#
			# Handle that inconsistency by defining it in the
			# per-program lookup (D1XProgram, D2XProgram), not in the
			# shared program lookup (DXXProgram).
'similar/main/bmread.cpp',
),
		transform_target=DXXProgram._apply_target_name,
	),
	))
		) -> collections.abc.MutableSequence['StaticObject']:
		value = __get_dxx_objects_common(self)
		value.extend(__get_dsx_objects_common(self))
		return value

	# for editor
	def get_objects_editor(self,
		__get_dxx_objects_editor=DXXProgram.get_objects_editor,
		__get_dsx_objects_editor=DXXCommon.create_lazy_object_getter((
'd1x-rebirth/editor/ehostage.cpp',
),
	)) -> collections.abc.MutableSequence['StaticObject']:
		value = list(__get_dxx_objects_editor(self))
		value.extend(__get_dsx_objects_editor(self))
		return value

class D2XProgram(DXXProgram):
	LazyObjectState = DXXProgram.LazyObjectState
	PROGRAM_NAME: typing.Final[str] = 'D2X-Rebirth'
	target: typing.Final[str]
	srcdir: typing.Final[str] = 'd2x-rebirth'
	target = srcdir
	shortname: typing.Final[str] = 'd2x'
	env_CPPDEFINES: typing.Final[tuple] = ('DXX_BUILD_DESCENT', 2),

	# general source files
	def get_objects_common(self,
		__get_dxx_objects_common=DXXProgram.get_objects_common, \
		__get_dsx_objects_common=DXXCommon.create_lazy_object_getter((
'd2x-rebirth/libmve/decoder8.cpp',
'd2x-rebirth/libmve/decoder16.cpp',
'd2x-rebirth/libmve/mve_audio.cpp',
'd2x-rebirth/libmve/mvelib.cpp',
'd2x-rebirth/libmve/mveplay.cpp',
'd2x-rebirth/main/escort.cpp',
'd2x-rebirth/main/gamepal.cpp',
'd2x-rebirth/main/movie.cpp',
	))) -> collections.abc.MutableSequence['StaticObject']:
		value = __get_dxx_objects_common(self)
		value.extend(__get_dsx_objects_common(self))
		return value

	# for editor
	def get_objects_editor(self,
		__get_dxx_objects_editor=DXXProgram.get_objects_editor, \
		__get_dsx_objects_editor=DXXCommon.create_lazy_object_states_getter((LazyObjectState(sources=(
			# See comment by D1XProgram reference to bmread.cpp for why
			# this is here instead of in the usual handling for similar
			# files.
'similar/main/bmread.cpp',
),
		transform_target=DXXProgram._apply_target_name,
		),
		))) -> collections.abc.MutableSequence['StaticObject']:
		value = list(__get_dxx_objects_editor(self))
		value.extend(__get_dsx_objects_editor(self))
		return value

def register_program(program, other_program, variables, filtered_help, append, _itertools_product=itertools.product) -> None:
	s = program.shortname
	l = [v for (k,v) in ARGLIST if k == s or k == 'dxx'] or (other_program.shortname not in ARGUMENTS,)
	# Legacy case: build one configuration.
	if len(l) == 1:
		try:
			if not int(l[0]):
				# If the user specifies an integer that evaluates to
				# False, this configuration is disabled.  This allows
				# the user to build only one game, instead of both.
				return
			# Coerce to an empty string, then reuse the case for stacked
			# profiles.  This is slightly less efficient, but reduces
			# maintenance by not having multiple sites that call
			# program().
			l = '',
		except ValueError:
			# If not an integer, treat this as a configuration profile.
			pass
	seen = set()
	add_seen = seen.add
	for e in l:
		for prefix in _itertools_product(*[v.split('+') for v in e.split(',')]):
			duplicates = set()
			# This would be simpler if set().add(V) returned the result
			# of `V not in self`, as seen before the add.  Instead, it
			# always returns None.
			#
			# Test for presence, then add if not present.  add() always
			# returns None, which coerces to False in boolean context.
			# This is slightly inefficient since we know the right hand
			# side will always be True after negation, so we ought to be
			# able to skip testing its truth value.  However, the
			# alternative is to use a wrapper function that checks
			# membership, adds the value, and returns the result of the
			# test.  Calling such a function is even less efficient.
			#
			# In most cases, this loop is not run often enough for the
			# efficiency to matter.
			prefix = tuple([
				p for p in prefix	\
					if (p not in duplicates and not duplicates.add(p))
			])
			if prefix in seen:
				continue
			add_seen(prefix)
			append(program(tuple((f'{s}{p and "_"}{p}' for p in prefix)) + prefix, variables, filtered_help))

def main(register_program,_d1xp=D1XProgram,_d2xp=D2XProgram):
	if os.path.isdir('build'):
		SConsignFile('build/.sconsign')
	variables = Variables([v for k, v in ARGLIST if k == 'site'] or ['site-local.py'], ARGUMENTS)
	filtered_help = FilterHelpText()
	dxx = []
	register_program(_d1xp, _d2xp, variables, filtered_help, dxx.append)
	register_program(_d2xp, _d1xp, variables, filtered_help, dxx.append)
	substenv = SCons.Environment.SubstitutionEnvironment()
	variables.FormatVariableHelpText = filtered_help.FormatVariableHelpText
	variables.Update(substenv)
# show some help when running scons -h
	Help("""DXX-Rebirth, SConstruct file help:

	Type 'scons' to build the binary.
	Type 'scons install' to build (if it hasn't been done) and install.
	Type 'scons -c' to clean up.

	Extra options (add them to command line, like 'scons extraoption=value'):
	d1x=[0/1]        Disable/enable D1X-Rebirth
	d1x=prefix-list  Enable D1X-Rebirth with prefix-list modifiers
	d2x=[0/1]        Disable/enable D2X-Rebirth
	d2x=prefix-list  Enable D2X-Rebirth with prefix-list modifiers
	dxx=VALUE        Equivalent to d1x=VALUE d2x=VALUE
""" +	\
		''.join([f'{d.program_message_prefix}:\n{d.init_with_decoration(substenv)}' for d in dxx])
	)
	if not dxx:
		return
	compilation_database_dict_fn_to_entries = DXXCommon.compilation_database_dict_fn_to_entries
	if compilation_database_dict_fn_to_entries:
		import json
		for fn, entry_tuple in compilation_database_dict_fn_to_entries.items():
			env, entries = entry_tuple
			e = env.Entry(fn)
			# clang tools search for this specific filename.  As a
			# convenience to the caller, if the target is a directory,
			# assume this filename in the named directory.  If the
			# caller wishes to generate the file under some other name,
			# that will be respected, though clang tools may refuse to
			# see it.
			if e.isdir() or fn.endswith('/'):
				e = e.File('compile_commands.json')
			# Sort by key so that dictionary ordering changes do not
			# cause unnecessary regeneration of the file.
			env.Textfile(target=e, source=env.Value(json.dumps(entries, indent=4, sort_keys=True)))

	unknown = variables.UnknownVariables()
	# Delete known unregistered variables
	unknown.pop('d1x', None)
	unknown.pop('d2x', None)
	unknown.pop('dxx', None)
	unknown.pop('site', None)
	ignore_unknown_variables = unknown.pop('ignore_unknown_variables', '0')
	if unknown:
		# Protect user from misspelled options by reporting an error.
		# Provide a way for the user to override the check, which might
		# be necessary if the user is setting an option that is only
		# understood by older (or newer) versions of SConstruct.
		try:
			ignore_unknown_variables = int(ignore_unknown_variables)
		except ValueError:
			ignore_unknown_variables = False
		j = f'''Unknown values specified on command line.{
	''.join((f"""
	{k}""" for k in unknown.keys()))
}'''
		if not ignore_unknown_variables:
			raise SCons.Errors.StopError(j +
'\nRemove unknown values or set ignore_unknown_variables=1 to continue.')
		print(f'''warning: {j}
Build will continue, but these values have no effect.
''')

main(register_program)

# Local Variables:
# tab-width: 4
# End:
