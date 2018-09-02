# Installing D1X-Rebirth and D2X-Rebirth
## Building

D1X-Rebirth and D2X-Rebirth (generically DXX-Rebirth) are built by an SConstruct script.  DXX-Rebirth runs on Windows (XP and later), Linux (x86 and amd64), recent Mac OS X, and Raspberry Pi.  Other targets may also work, but are not tracked by the core team.  If you maintain a working target not listed here, please file an issue to have it included in this list.

The DXX-Rebirth maintainers have no control over the sites linked below.  The maintainers are not responsible for the safety or correct operation of the prerequisites listed below.  Unless specified otherwise, the maintainers of DXX-Rebirth have not verified that the links are current, safe, or produce a working environment.

### Prerequisites

* [Python 2.x](https://www.python.org/) to run [scons](https://www.scons.org/), the processor for SConstruct scripts.
[Python 2.7](https://www.python.org/downloads/release/python-2714/) is recommended.
As of this writing, **scons** does not support Python 3.x, but has plans to do so.
* C++ compiler with support for selected C++11 features.  One of:
    * [gcc](https://gcc.gnu.org/) 4.9.4 or later
    * [clang](https://clang.llvm.org/) 3.3 or later
    * Microsoft Visual Studio is **not** supported at this time.
	  Support for Microsoft Visual Studio will be added when it
	  implements sufficient C++11 features for the code to build with
	  few or no modifications.
* [SDL 1.2](https://www.libsdl.org/).
SDL 2 is **not** supported at this time.
SDL 2 support is planned.
If you would like to help, please post patches that resolve build failures that occur when SDL 2 is activated.
Patches must not break support for SDL 1.
* [PhysicsFS](https://icculus.org/physfs/).
PhysFS 2.x is recommended.

Optional, but recommended:

* [SDL\_mixer 1.2](https://www.libsdl.org/projects/SDL_mixer/).
SDL\_mixer 2 is **not** supported at this time.
* C++ compiler with support for selected C++14 features.  One of:
    * [gcc](https://gcc.gnu.org/) 4.9.4 or later
    * [clang](https://clang.llvm.org/) 3.4 or later

Unless otherwise noted, using the newest release available is recommended.  For example, prefer gcc-5.4 to gcc-4.9, even though both should work.

DXX-Rebirth can be built on one system to run on a different system, such as using Linux to build for Windows (a "cross-compiled build").  The sections below specify where to get prerequisites for a build meant to run on the system where it is built (a "native build").

#### Windows
Where possible, Windows users should try to obtain a compiled package, rather than locally compiling from source.  To build from source, read on.

If you are not sure whether your system is Windows x86 or Windows x64, use the packages for Windows x86.  Systems running Windows x64 support running Windows x86 programs, but Windows x86 systems do not run Windows x64 programs.

* [Python x86 MSI](https://www.python.org/ftp/python/2.7.14/python-2.7.14.msi) |
[Python x64 MSI](https://www.python.org/ftp/python/2.7.14/python-2.7.14.amd64.msi)
* [SCons](https://prdownloads.sourceforge.net/scons/scons-2.3.6-setup.exe)
* C++ compiler
    * mingw-gcc: [Getting Started](http://www.mingw.org/wiki/Getting_Started) |
	[Direct download](https://sourceforge.net/projects/mingw/files/latest/download)
    * [clang](https://llvm.org/releases/3.6.2/LLVM-3.6.2-win32.exe)
	([.sig](https://llvm.org/releases/3.6.2/LLVM-3.6.2-win32.exe.sig))
* [SDL 1.2 x86 zip](https://www.libsdl.org/release/SDL-1.2.15-win32.zip) |
[SDL 1.2 x64 zip](https://www.libsdl.org/release/SDL-1.2.15-win32-x64.zip)
* No published PhysFS package for Windows is known.
You must [build it](https://hg.icculus.org/icculus/physfs/raw-file/bf155bd2127b/INSTALL.txt)
from [source](https://icculus.org/physfs/downloads/physfs-2.0.3.tar.bz2).
You may be able avoid building from source by copying PhysFS DLLs from a
previous Rebirth release and installing current PhysFS headers.
However, building from source is recommended to ensure a consistent
environment.

#### Linux
Install the listed prerequisites through your system package manager.

##### Arch
* **pacman -S
 base-devel
 scons
 sdl
 sdl\_mixer
 physfs**

##### Fedora
* **yum install
 gcc-c++
 scons
 SDL-devel
 SDL\_mixer-devel
 physfs-devel**

##### Gentoo
* **emerge --ask --verbose --noreplace
 dev-util/scons
 media-libs/libsdl
 media-libs/sdl-mixer
 dev-games/physfs**

##### Ubuntu
* **apt-get install
 build-essential
 scons
 libsdl1.2-dev
 libsdl-mixer1.2-dev
 libphysfs-dev**

#### Mac OS X
Where possible, Mac users should try to obtain a compiled package, rather than locally compiling from source.  To build from source, read on.

Install the listed prerequisites through your preferred package manager.

PhysicsFS must be installed as a separate dynamic library.  It is not supported as an OS X Framework.  The required files will be installed into the correct locations by the PhysicsFS installer; refer to the relevant instructions.

The Mac OS X Command Line Tools are required.  Install them by running

    xcode-select --install

from the Terminal.  This may need to be done after each major OS upgrade as well.

DXX-Rebirth can be built from the Terminal (via SCons) without Xcode; to build using Xcode requires Xcode to be installed.

##### [Homebrew](https://github.com/Homebrew/homebrew/)
* **brew install
 gcc5
 scons
 sdl
 sdl\_mixer
 physfs**

### Building
Once prerequisites are installed, run **scons** *options* to build.  By default, both D1X-Rebirth and D2X-Rebirth are built.  To build only D1X-Rebirth, run **scons d1x=1**.  To build only D2X-Rebirth, run **scons d2x=1**.

If unspecified, **SConstruct** uses $CXX for the compiler, $CPPFLAGS for preprocessor options, $CXXFLAGS for C++ compiler options, and $LDFLAGS for linker options.  You may override any or all of these choices by setting options of the corresponding name: **scons CXX=/opt/my/c++ 'CXXFLAGS=-O0 -ggdb'**.  **SConstruct** supports numerous options for adjusting build details.  Run **scons -h** to see them.  Option names are case sensitive.  Commonly used options include:

* **CXX=**_path_ - path to C++ compiler
* **CPPFLAGS='**_flags_**'** - flags for C preprocessor
* **CXXFLAGS='**_flags_**'** - flags for C++ compiler
* **LDFLAGS='**_flags_**'** - flags for linker
* **lto=1** - enable Link Time Optimization
* **sdlmixer=1** - enable support for SDL\_mixer
* **builddir=**_path_ - set directory for build outputs; defaults to "."
* **builddir\_prefix=**_path_ - Developer option; set builddir to builddir\_prefix plus a path derived from build options.
Use this to build multiple targets without picking specific paths for each one.
The generated path is stable across multiple runs with the same options.
Packaging scripts should use **builddir** with manually chosen directories.
* **verbosebuild=1** - show commands executed instead of short descriptions of the commands
* **prefix=**_path_ - (Linux only); equivalent to **--prefix** in Autoconf
* **sharepath=**_path_ - (Linux only); sets system directory to search for game data

The build system supports building multiple targets in parallel.  This is primarily useful for developers, but can also be used by packagers to create secondary builds with different features enabled.  To use it, run **scons** *game*=*profile[,profile...]*.  **SConstruct** will search each profile for the known options.  The first match wins.  For example:

        scons dxx=gcc5,e, d2x=gcc49,sdl, \
            gcc49_CXX=/path/to/gcc-4.9 \
            gcc5_CXX=/path/to/gcc-5 \
            e_editor=1 sdl_opengl=0

This tells **SConstruct** to build both games (**dxx**) with the profiles **gcc5**, **e**, *empty* and also to build D2X-Rebirth (**d2x**) with the profiles **gcc49**, **sdl**, *empty*.  Profiles **gcc49** and **gcc5** define private values for **CXX**, so the default value of **CXX** is ignored.  Profile **e** enables the **editor** option, which builds features used by players who want to create their own levels.  Profile **sdl** sets the **opengl** option to false, which produces a build that uses only the software renderer.  Profile *empty* is the default namespace, so CPPFLAGS, CXXFLAGS, etc. are found when it is searched.  Since these values were not assigned, they are drawn from the corresponding environment variables.

The build system supports specifying a group of closely related targets.  This is mostly redundant on shells with brace expansion support, but can be easier to type.  For example:

        scons builddir_prefix=build/ \
			dxx=gcc5+gcc49,prof1,prof2,prof3,

This is equivalent to the shell brace expansion:

        scons builddir_prefix=build/ \
			dxx={gcc5,gcc49},prof1,prof2,prof3,

or

        scons builddir_prefix=build/ \
			dxx=gcc5,prof1,prof2,prof3, \
			dxx=gcc49,prof1,prof2,prof3,

Profile addition can be stacked: **scons dxx=a+b,c+d,e+f** is equivalent to **scons dxx=a,c,e dxx=a,d,e dxx=b,c,e dxx=b,d,e dxx=a,c,f dxx=a,d,f dxx=b,c,f dxx=b,d,f**.

#### Configuration
**SConstruct** runs tests to check for common problems and available functionality.  **SConstruct** will automatically enable available compiler features.  **SConstruct** will not automatically enable optional features which require an external library.  If the feature is enabled, either by default or by the provided options, and the library is present, it will be used.  If the feature is enabled, and the library is absent, the build fails.  If the feature is disabled, either by default or by the provided options, the library check is skipped and the feature is not used.

If required functionality is missing, **SConstruct** will stop the build and print a diagnostic message.  A stop at this stage indicates an environment problem.  If any target fails to configure, no target will be built.

Before reporting an issue, clear any applicable caches (**ccache -C**; **rm .sconsign.dblite**) and reproduce the failure.  The DXX-Rebirth maintainers may be able to help you resolve environment problems if you cannot solve them on your own.  If you need help, please post the full output of running **scons** and the contents of **sconf.log** from your build directory.

#### Compiling
After **SConstruct** finishes the configure tests, **scons** will compile and link the program.  Failures at this stage may indicate a bug that should be reported.  Broken environments are usually caught by **SConstruct** checks before the build began.  If the compilation succeeds, the output files will be found in game-specific directories.  If the profile specified editor features, then **-editor** is appended to the filename.

The output path can be overridden with the **SConstruct** option **program\_name**.  If **program\_name** is set, it overrides the game directory prefix and the optional **-editor** modification.  It does not override the output suffix (**.exe** for Windows, *empty* for Linux).

Windows output with **program\_name** unset:

* *build-directory*/d1x-rebirth/d1x-rebirth*[-editor]*.exe
* *build-directory*/d2x-rebirth/d2x-rebirth*[-editor]*.exe

Windows output with **d1x=1 program\_name**=**d1x-local**:

* *build-directory*/d1x-local.exe

Linux output with **program\_name** unset:

* *build-directory*/d1x-rebirth/d1x-rebirth*[-editor]*
* *build-directory*/d2x-rebirth/d2x-rebirth*[-editor]*

#### Installing
For Windows and Linux, DXX-Rebirth installs only the main game binary.  The binary can be run from anywhere and can be installed by copying the game binary.  The game does not inspect the name of its binary.  You may rename the output after compilation without affecting the game.

As a convenience, if **register\_install\_target=True**, **SConstruct** registers a pseudo-target named **install** which copies the compiled files to *DESTDIR*__/__*BINDIR*.  By default, **register\_install\_target=True**, *DESTDIR* is *empty*, and *BINDIR* is *PREFIX*__/bin__, which expands to **/usr/local/bin**.

DXX-Rebirth [requires game data](https://www.dxx-rebirth.com/game-content/) to play.  The build system has no support for interacting with game data.  You can get [Descent 1 PC shareware data](https://www.dxx-rebirth.com/download/dxx/content/descent-pc-shareware.zip) and [Descent 2 PC demo data](https://www.dxx-rebirth.com/download/dxx/content/descent2-pc-demo.zip) from the DXX-Rebirth website.  Full game data is supported (and recommended), but is not freely available.  You can [buy full Descent 1 game data](https://www.gog.com/game/descent) and/or [buy full Descent 2 game data](https://www.gog.com/game/descent_2) from GOG.com.  Historically, both Descent 1 and Descent 2 were sold as a single unit.  After a nearly two-year hiatus from sale, the games returned to GOG.com in November 2017 as separate units.  DXX-Rebirth contains engines for both games.  Each engine works for its respective game without the data from the other, so players who wish to purchase only one game may do so.
