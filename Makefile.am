DIST_SUBDIRS = 2d 3d maths mem cfile console iff texmap misc arch main libmve utilities

SUBDIRS = ${DIST_SUBDIRS} @D2X_SUBDIRS@

EXTRA_SUBDIRS = include unused debian rpm VisualC VisualCE

bin_PROGRAMS = @TARGETS@

EXTRA_PROGRAMS = d2x d2x-sdl d2x-gl d2x-svga d2x-ggi

ARCH_LIBS = $(foreach subdir,${D2X_ARCH_SUBDIRS},arch/${subdir}/libarch_${subdir}.a)

if MINGW32
LD_KLUDGE = main/inferno.o
endif

if MACOSX
AM_LDFLAGS = -all_load
endif

if EDITOR
EDITOR_LIBS = main/editor/libeditor.a
endif

if CONSOLE
CONSOLE_LIBS = console/libconsole.a
endif

d2x_SOURCES =
d2x_sdl_SOURCES =
d2x_gl_SOURCES =
d2x_svga_SOURCES =
d2x_ggi_SOURCES =

d2x_LDADD = ${LD_KLUDGE} main/libmain.a ${EDITOR_LIBS} 3d/lib3d.a 2d/lib2d.a ${ARCH_LIBS} libmve/libmve.a mem/libmem.a cfile/libcfile.a iff/libiff.a texmap/libtexmap.a misc/libmisc.a maths/libmaths.a ${CONSOLE_LIBS}

if MINGW32
if USE_NETWORK
d2x_sdl_LDADD = ${d2x_LDADD} -lwinmm -lwsock32
else
d2x_sdl_LDADD = ${d2x_LDADD} -lwinmm
endif
else
d2x_sdl_LDADD = ${d2x_LDADD} -lm
endif
d2x_gl_LDADD = ${d2x_sdl_LDADD} ${OGL_LIBS}
d2x_svga_LDADD = ${d2x_sdl_LDADD} ${SVGA_LIBS}
d2x_ggi_LDADD = ${d2x_sdl_LDADD} ${GGI_LIBS}

dist-hook:
	rm -rf `find $(distdir) -name CVS`
#	rm -f `find $(distdir) -name Makefile`

pkgdatadir = @sharepath@
missiondir = ${pkgdatadir}/missions

CLEANFILES = @TARGETS@ Info.plist

EXTRA_DIST = ${EXTRA_SUBDIRS} \
ChangeLog \
README.utils \
djgpp.bat djgpp.sh \
autogen.sh cygconf \
cvshowto.txt readme.txt installation.txt compilerdefs.txt \
d2x-sdl.sh d2x-gl.sh
