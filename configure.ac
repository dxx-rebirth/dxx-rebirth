# -*- mode: shell-script -*-
AC_INIT(main/inferno.c)
AC_PREREQ(2.52)

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_CONFIG_HEADER(conf.h)
AM_INIT_AUTOMAKE(d2x, 0.1.2)

AM_MAINTAINER_MODE

#kludge to keep autoconf from adding -g -O2
CFLAGS=" $CFLAGS"

AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_ISC_POSIX
AC_CHECK_TOOL(CC, gcc, :)
AC_CHECK_TOOL(RANLIB, ranlib, :)
AC_CHECK_TOOL(STRIP, strip, :)
AC_CHECK_TOOL(AR, ar, :)
AC_PROG_CC
AC_STDC_HEADERS
AC_PATH_PROGS(NASM, nasmw nasm, no)

AC_CYGWIN
AC_MINGW32
if test x$CYGWIN = xyes; then
    AC_MSG_WARN([Cygwin found, setting -mno-cygwin])
    CFLAGS="-mno-cygwin $CFLAGS"
    MINGW32=yes
fi

AM_CONDITIONAL(MINGW32, test x$MINGW32 = xyes)
if test x$MINGW32 = xyes; then
    GL_LIBS="opengl32 glu32"
else
    GL_LIBS="GL GLU"
fi

dnl Set $prefix and $exec_prefix to $ac_default_prefix if they are not set
test "x$prefix" = "xNONE" && prefix=$ac_default_prefix
test "x$exec_prefix" = "xNONE" && exec_prefix="${prefix}"

dnl Set default locations to find data files.
eval foo="$datadir"
default_sharepath="$foo/games/$PACKAGE"

CFLAGS="-Wall $CFLAGS"

dnl Enable editor build?
AC_ARG_ENABLE(editor,
  [  --enable-editor         Build editor? (not functional)],,)
if test x$enable_editor = xyes; then
    AC_DEFINE(EDITOR)
    D2X_MAIN_SUBDIRS=editor
fi
AM_CONDITIONAL(EDITOR, test x$enable_editor = xyes)

dnl Enable shareware build?
AC_ARG_ENABLE(shareware,
  [  --enable-shareware      Build for demo datafiles (not functional)],,)
if test x$enable_shareware = xyes; then
    AC_DEFINE(SHAREWARE)
    D2X_FEATURES="shareware ${D2X_FEATURES}"
fi
AM_CONDITIONAL(SHAREWARE, test x$enable_shareware = xyes)

dnl Enable movie trick?
AC_ARG_ENABLE(movies,
  [  --enable-movie-trick    Enable trick to show movies],,)
if test x$enable_movie_trick = xyes; then
    AC_DEFINE(MOVIE_TRICK)
    D2X_FEATURES="movie_trick ${D2X_FEATURES}"
fi
AM_CONDITIONAL(MOVIE_TRICK, test x$enable_movie_trick = xyes)

dnl Enable NMOVIES?
AC_ARG_ENABLE(nmovies,
  [  --enable-nmovies        Turn off movies (render endlevel flythrough)],,)
if test x$enable_nmovies = xyes; then
    AC_DEFINE(NMOVIES)
    D2X_FEATURES="nmovies ${D2X_FEATURES}"
fi
AM_CONDITIONAL(NMOVIES, test x$enable_nmovies = xyes)

dnl Enable cross-platform bitmap/palette loader?
AC_ARG_ENABLE(xploader,
  [  --enable-xploader       Enable cross-platform bitmap/palette loader ],,)
if test x$enable_xploader = xyes; then
    AC_DEFINE(PORTABLE_LOADER)
    D2X_FEATURES="xploader ${D2X_FEATURES}"
fi

dnl Count warnings as errors?
AC_ARG_ENABLE(werror,
  [  --enable-werror         Count compiler warning as errors ],,)
if test x$enable_werror = xyes; then
    CFLAGS="-Werror $CFLAGS"
    D2X_FEATURES="werror ${D2X_FEATURES}"
fi

dnl Build with NDEBUG?
AC_ARG_ENABLE(debug,
  [  --disable-debug         Take out debug symbols, Asserts, Int3, etc ],,)
if test x$enable_debug = xno; then
    AC_DEFINE(NDEBUG)
    CFLAGS="-O2 $CFLAGS"
else
    CFLAGS="-g $CFLAGS"
    D2X_FEATURES="debug ${D2X_FEATURES}"
fi

dnl Build with RELEASE?
AC_ARG_ENABLE(release,
  [  --enable-release        Release build.  Removes debug/editor things ],,)
if test x$enable_release = xyes; then
    AC_DEFINE(RELEASE)
    D2X_FEATURES="release ${D2X_FEATURES}"
fi

AC_ARG_WITH(sharepath,
  [[  --with-sharepath=DIR    Use DIR for shared game data [DATADIR/games/d2x]]],
  sharepath=$withval, sharepath="auto")
if test "x$sharepath" = "xauto" -o "x$sharepath" = "xyes" -o "x$sharepath" = "x"; then
    sharepath="$default_sharepath"
fi
AC_DEFINE_UNQUOTED(SHAREPATH, "$sharepath")

# Test for DJGPP
AM_CONDITIONAL(DJGPP, test "$host_os" = "msdosdjgpp")
if test "$host_os" = "msdosdjgpp"; then

  CFLAGS="-I \$(top_srcdir)/arch/dos/include $CFLAGS"
  TARGETS=d2x
  D2X_ARCH_SUBDIRS=dos
else

  CFLAGS="-pipe $CFLAGS"

  dnl Check for SDL
  AM_PATH_SDL(1.1.7,
    :,
    AC_MSG_ERROR(SDL not found. Make sure sdl-config is in your PATH, or specify with --with-sdl-prefix)
  )
  CFLAGS="$SDL_CFLAGS $CFLAGS"
  LIBS="$SDL_LIBS $LIBS"

  TARGETS=d2x-sdl
  D2X_ARCH_SUBDIRS=sdl

  AC_CHECK_HEADERS(netipx/ipx.h)

  # Check for OpenGL
  AC_ARG_WITH(opengl,
    [  --with-opengl           Build OpenGL support ],,)
  if test x$with_opengl = xyes; then
    for lib in $GL_LIBS; do
	AC_CHECK_LIB($lib, main,
	    OGL_LIBS="${OGL_LIBS} -l$lib",
	    [AC_MSG_ERROR([$lib not found, --with-opengl cannot be used])
	    opengl=false],
	    ${OGL_LIBS}
	)
    done
    AC_DEFINE(OGL)
    TARGETS=d2x-ogl
    D2X_ARCH_SUBDIRS="ogl ${D2X_ARCH_SUBDIRS}"
  fi
  AC_SUBST(OGL_LIBS)
  AM_CONDITIONAL(USE_OPENGL, test x$with_opengl = xyes)

  # Check for GGI
  AC_ARG_WITH(ggi,
    [  --with-ggi              Build GGI support ],,)
  if test x$with_ggi = xyes; then
    AC_CHECK_LIB(ggi, ggiInit,
      GGI_LIBS="-lggi",
      [AC_MSG_ERROR([GGI not found, GGI version cannot be built]); ggi=false])
    AC_CHECK_LIB(gii, giiInit,
      GGI_LIBS="${GGI_LIBS} -lgii",
      [AC_MSG_ERROR([GII not found, GGI version cannot be built]); ggi=false])
    AC_DEFINE(GGI)
    TARGETS=d2x-ggi
    D2X_ARCH_SUBDIRS="ggi ${D2X_ARCH_SUBDIRS}"
  fi
  AC_SUBST(GGI_LIBS)
  AM_CONDITIONAL(USE_GGI, test x$with_ggi = xyes)

  # Check for SVGALib
  AC_ARG_WITH(svga,
    [  --with-svga             Build SVGALib support ],,)
  if test x$with_svga = xyes; then
    AC_CHECK_LIB(vga,vga_getmousetype,
      SVGA_LIBS="-lvga",
      [AC_MSG_ERROR([vga not found, SVGALib cannot be built])
       svga=false],
       )
    AC_CHECK_LIB(vgagl,gl_getcontext,
      SVGA_LIBS="${SVGA_LIBS} -lvgagl",
      [AC_MSG_ERROR([vgagl not found, SVGALib cannot be built])
       svga=false],
       -lvga)
    AC_DEFINE(SVGA)
    TARGETS=d2x-svga
    D2X_ARCH_SUBDIRS="svgalib ${D2X_ARCH_SUBDIRS}"
  fi
  AC_SUBST(SVGA_LIBS)
  AM_CONDITIONAL(USE_SVGALIB, test x$with_svga = xyes)

  if test x$MINGW32 = xyes; then
    D2X_ARCH_SUBDIRS="win32 ${D2X_ARCH_SUBDIRS}"
    AC_DEFINE(SDL_JOYSTICK)
  else
    CFLAGS="-I \$(top_srcdir)/arch/linux/include $CFLAGS"
    D2X_ARCH_SUBDIRS="linux ${D2X_ARCH_SUBDIRS}"

    dnl Check for SDL joystick
    AC_ARG_ENABLE(sdl-joystick,
      [  --enable-sdl-joystick   Use SDL joystick (required for mingw) ],,)
    if test x$enable_sdl_joystick = xyes; then
        AC_DEFINE(SDL_JOYSTICK)
        D2X_FEATURES="sdl_joystick ${D2X_FEATURES}"
    fi
    AM_CONDITIONAL(SDL_JOYSTICK, test x$enable_sdl_joystick = xyes)
  fi
fi

dnl Check for network
AC_ARG_ENABLE(network,
  [  --disable-network       Do not build network/serial support ],,)
if test x$enable_network != xno; then
    AC_DEFINE(NETWORK)
    D2X_FEATURES="network ${D2X_FEATURES}"
fi
AM_CONDITIONAL(USE_NETWORK, test x$enable_network != xno)

dnl Check for assembler
AC_ARG_ENABLE(assembler,
  [  --disable-assembler     Do not use assembler optimization ],,)
if test x$enable_assembler = xno; then
    AC_DEFINE(NO_ASM)
    D2X_FEATURES="no_asm ${D2X_FEATURES}"
else
    if test x$NASM = xno; then
	AC_MSG_ERROR([nasm not found. Install it or use --disable-assembler])
    else
        NASMFLAGS="-i\$(srcdir)/ $NASMFLAGS"
        case $host_os in
          cygwin)
              NASMFLAGS="-f win32 $NASMFLAGS"
              ;;
	  msdosdjgpp)
	      NASMFLAGS="-f coff $NASMFLAGS"
	      ;;
          linux*)
              NASMFLAGS="-f elf -d__linux__ $NASMFLAGS"
              ;;
	  *)
	      AC_MSG_ERROR([Assembler not supported for this OS])
        esac
    fi
fi
AM_CONDITIONAL(USE_ASM, test x$enable_assembler != xno)

AM_PROG_AS

AC_SUBST(D2X_ARCH_SUBDIRS)

AC_SUBST(D2X_MAIN_SUBDIRS)

AC_SUBST(TARGETS)

AC_SUBST(NASMFLAGS)

AC_SUBST(CFLAGS)

AC_OUTPUT(
Makefile
2d/Makefile
3d/Makefile
arch/Makefile
arch/dos/Makefile
arch/ggi/Makefile
arch/linux/Makefile
arch/ogl/Makefile
arch/sdl/Makefile
arch/svgalib/Makefile
arch/win32/Makefile
cfile/Makefile
iff/Makefile
main/Makefile
main/editor/Makefile
maths/Makefile
mem/Makefile
misc/Makefile
texmap/Makefile
)

AC_MSG_RESULT([
    d2x has been configured successfully.

    Platform(s): $D2X_ARCH_SUBDIRS
    Features   : $D2X_FEATURES $D2X_MAIN_SUBDIRS

    Shared game data directory: $sharepath
])
