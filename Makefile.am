SUBDIRS = 2d 3d maths mem cfile iff texmap misc arch main

EXTRA_SUBDIRS = include tools unused debian rpm

bin_PROGRAMS = @TARGETS@

EXTRA_PROGRAMS = d2x d2x-sdl d2x-ogl d2x-svga d2x-ggi

CCLD = ${CC}
LDFLAGS = -export-dynamic
LINK = $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@

ARCH_LIBS = $(foreach subdir,${D2X_ARCH_SUBDIRS},arch/${subdir}/libarch_${subdir}.a)

if MINGW32
LD_KLUDGE = main/inferno.o
endif

if EDITOR
EDITOR_LIBS = main/editor/libeditor.a
endif

d2x_SOURCES =
d2x_sdl_SOURCES =
d2x_ogl_SOURCES =
d2x_svga_SOURCES =
d2x_ggi_SOURCES =

d2x_LDADD = ${LD_KLUDGE} main/libmain.a ${EDITOR_LIBS} 3d/lib3d.a 2d/lib2d.a ${ARCH_LIBS} mem/libmem.a cfile/libcfile.a iff/libiff.a texmap/libtexmap.a misc/libmisc.a maths/libmaths.a

if MINGW32
if USE_NETWORK
d2x_sdl_LDADD = ${d2x_LDADD} -lwsock32
else
d2x_sdl_LDADD = ${d2x_LDADD}
endif
else
d2x_sdl_LDADD = ${d2x_LDADD} -lm -ldl -lpthread
endif
d2x_ogl_LDADD = ${d2x_sdl_LDADD} ${OGL_LIBS}
d2x_svga_LDADD = ${d2x_sdl_LDADD} ${SVGA_LIBS}
d2x_ggi_LDADD = ${d2x_sdl_LDADD} ${GGI_LIBS}

dist-hook:
	rm -rf `find $(distdir) -name CVS`
#	rm -f `find $(distdir) -name Makefile`
	$(MAKE) changelog
	cp -a $(srcdir)/ChangeLog $(distdir)

changelog::
	rm -f $(srcdir)/ChangeLog
	touch $(srcdir)/ChangeLog
	cd $(srcdir) && tools/cvs2cl/cvs2cl.pl -b --utc
	rm -f $(srcdir)/ChangeLog.bak

EXTRA_DIST = ${EXTRA_SUBDIRS} \
ChangeLog \
djgpp.bat djgpp.sh \
autogen.sh cygconf \
cvshowto.txt readme.txt license.txt installation.txt mingw32.txt compilerdefs.txt \
d2x-sdl.sh d2x-ogl.sh
