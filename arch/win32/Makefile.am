EXTRA_SUBDIRS = include d3dframe

noinst_LIBRARIES = libarch_win32.a

INCLUDES = -I$(top_srcdir)/arch/include -I$(top_srcdir)/includes -I$(top_srcdir)/main -I$(srcdir)/include

if USE_NETWORK
libarch_win32_a_SOURCES = findfile.c mingw_init.c winnet.c ipx_win.c
else
libarch_win32_a_SOURCES = findfile.c mingw_init.c
endif

EXTRA_DIST = \
d1x.ico d1x.rc d3d.ico debug.cpp descent.ico digi.c findfile.c glinit.c gr.c \
hmpfile.c init.c ipx_win.c joydefs.c joyhh.c key.c mono.c mouse.c palw32.c \
pch.cpp scene.cpp serial.c texture.cpp timer.c win32.c winmain.cpp winnet.c

dist-hook:
	list='$(EXTRA_SUBDIRS)'; for subdir in $$list; do \
	 ( cd $(srcdir) && tar --exclude=CVS -cf - $$subdir ) \
	  | ( cd $(distdir) && tar xf - ) \
	done
