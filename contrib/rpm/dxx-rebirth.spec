# This spec file accepts the following RPM macros to influence what is built and how the build is invoked:
#	_datarootdir: standard use; controls where to find game data files
#		Note that this is only a default.  The games can be told at runtime to
#		search an additional directory.
#	_prefix: standard use
#	use_d1x:	Define to 0 to skip building Descent 1.  Define to 1 to build Descent 1.  If undefined, assume 1.
#	use_d2x:	Define to 0 to skip building Descent 2.  Define to 1 to build Descent 2.  If undefined, assume 1.
#	use_opengl:	Define to 0 to skip building the OpenGL-enabled renderer.  Define to 1 to build the OpenGL-enabled renderer.  Most people will want this renderer.  If undefined, assume 1.
#	use_sdl:	Define to 0 to skip building the SDL-only renderer.  Define to 1 to build the SDL-only renderer.  Most people will not want this renderer.  If undefined, assume 0.
#	ccache:	Define to an empty string to disable ccache.  Define to the path to ccache to use ccache.  If undefined, $(type -p ccache) is used to search for ccache.  If no ccache is found, then the build runs without ccache.
#	__nprocs:	number of concurrent build jobs to run.  If undefined, job runs with SCons default (one job at a time).
#	_host:	CHOST.  If defined, SConstruct will automatically use CHOST-qualified names for tools.  If undefined, unqualified names are used.
#
#	You cannot disable both d1x and d2x, because that would build nothing.
#	You cannot disable both sdl and opengl, because that would build nothing.

Summary: Source port of Descent 1 and Descent 2
Name: dxx-rebirth
Version: 0.60_beta2_pre20190815
Release: 1
License: DXX-Rebirth
Group: Games/Arcade
Url: https://www.dxx-rebirth.com
Source: %{name}_v%{version}-src.tar.gz

# SDL libraries are required in both the SDL-only build and the OpenGL-enabled build.
BuildRequires: gcc-c++ libSDL-devel libSDL_mixer-devel libphysfs-devel scons

# If _datarootdir is unset, pretend it has the value of _prefix/share.
# Set _dxx_base_sharepath to _datarootdir/games
# Set _d1x_sharepath / _d2x_sharepath from that, and use those for the individual
# games.
%global _dxx_base_sharepath %{?_datarootdir:%_datarootdir}%{?!_datarootdir:%_prefix/share}/games
%global _use_d1x	%{?use_d1x}%{?!use_d1x:1}
%global _use_d2x	%{?use_d2x}%{?!use_d2x:1}
%global _use_opengl	%{?use_opengl}%{?!use_opengl:1}
%global _use_sdl	%{?use_sdl}%{?!use_sdl:0}

%if %{_use_opengl}
BuildRequires: libGL-devel
%endif

%if %{_use_d1x}
%global _d1x_sharepath %_dxx_base_sharepath/descent

%package -n d1x-rebirth
Summary: Supporting files for D1X-Rebirth
%description -n d1x-rebirth
Supporting files for D1X-Rebirth

%if %{_use_opengl}
%package -n d1x-rebirth-gl
Requires: d1x-rebirth
Summary: Descent 1 for Linux, OpenGL version
%description -n d1x-rebirth-gl
OpenGL-enabled source port of Descent 1.
%endif

%if %{_use_sdl}
%package -n d1x-rebirth-sdl
Requires: d1x-rebirth
Summary: Descent 1 for Linux, SDL version
%description -n d1x-rebirth-sdl
SDL-only source port of Descent 1.
%endif
%endif

%description
Empty top-level package to define the subpackages for D1X-Rebirth and D2X-Rebirth, SDL and GL variants.
- d1x-rebirth-gl
- d1x-rebirth-sdl
- d2x-rebirth-gl
- d2x-rebirth-sdl

%if %{_use_d2x}
%global _d2x_sharepath %_dxx_base_sharepath/descent2
%package -n d2x-rebirth
Summary: Supporting files for D2X-Rebirth
%description -n d2x-rebirth
Supporting files for D2X-Rebirth

%if %{_use_opengl}
%package -n d2x-rebirth-gl
Requires: d2x-rebirth
Summary: Descent 2 for Linux, OpenGL version
%description -n d2x-rebirth-gl
OpenGL-enabled source port of Descent 2.
%endif

%if %{_use_sdl}
%package -n d2x-rebirth-sdl
Requires: d2x-rebirth
Summary: Descent 2 for Linux, SDL version
%description -n d2x-rebirth-sdl
SDL-only source port of Descent 2.
%endif
%endif

%prep
%setup -n %{name}_v%{version}-src

%build

%if %{_use_d1x}
%if %{_use_d2x}
%define _requested_games dxx
%else
%define _requested_games d1x
%endif
%else
%if %{_use_d2x}
%define _requested_games d2x
%else
	return 1
%error You must enable at least one game.  Define use_d1x=1, use_d2x=1, or both.
%endif
%endif

%if !%{_use_sdl} && !%{_use_opengl}
%error You must enable at least one renderer.  Define use_sdl=1, use_opengl=1, or both.
%endif

# Use a function so that variables can be made local and `set` does not clobber
# global arguments.
build_with_use() {
	local ccache
	set --
	ccache="%{?ccache}%{?!ccache:$(type -p ccache)}"
	if [[ -n "$ccache" ]]; then
		set -- "ccache=$ccache"
	fi

%define arguments_dxx()	\\\
%if %{expand:%{_use_%{1}}}	\
	%if %{_use_opengl}	\
			%{1}_ogl_program_name=%{1}-rebirth-gl	\\\
	%endif	\
	%if %{_use_sdl}	\
			%{1}_sdl_program_name=%{1}-rebirth-sdl	\\\
	%endif	\
			%{1}_sharepath=%{expand:%{_%{1}_sharepath}}	\\\
%endif

	scons %{?__nprocs:"-j%__nprocs"}	\
		%{?_host:"CHOST=%_host"}	\
		"$@"	\
		builddir_prefix=build/	\
		%{arguments_dxx d1x}	\
		%{arguments_dxx d2x}	\
%if %{_use_sdl}
		sdl_opengl=0	\
		%_requested_games=sdl,	\
%endif
%if %{_use_opengl}
		%_requested_games=ogl,	\
%endif
		sdlmixer=1	\
		"prefix=%{_prefix}"	\
		"DESTDIR=%{buildroot}"	\
		verbosebuild=1	\
		install
}

build_with_use

%install
%if %{_use_d1x}
install -dm 755 %buildroot%_d1x_sharepath	\
			%buildroot%_d1x_sharepath/missions
%endif
%if %{_use_d2x}
install -dm 755 %buildroot%_d2x_sharepath	\
			%buildroot%_d2x_sharepath/missions
%endif

%if %{_use_d1x}
%files -n d1x-rebirth
%doc d1x-rebirth/d1x.ini COPYING.txt d1x-rebirth/RELEASE-NOTES.txt
%dir %_d1x_sharepath
%dir %_d1x_sharepath/missions

%if %{_use_sdl}
%files -n d1x-rebirth-sdl
%_bindir/d1x-rebirth-sdl
%endif

%if %{_use_opengl}
%files -n d1x-rebirth-gl
%_bindir/d1x-rebirth-gl
%endif
%endif

%if %{_use_d2x}
%files -n d2x-rebirth
%doc d2x-rebirth/d2x.ini COPYING.txt d2x-rebirth/RELEASE-NOTES.txt
%dir %_d2x_sharepath
%dir %_d2x_sharepath/missions

%if %{_use_sdl}
%files -n d2x-rebirth-sdl
%_bindir/d2x-rebirth-sdl
%endif

%if %{_use_opengl}
%files -n d2x-rebirth-gl
%_bindir/d2x-rebirth-gl
%endif
%endif
