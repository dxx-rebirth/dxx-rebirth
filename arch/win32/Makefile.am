EXTRA_SUBDIRS = include d3dframe

noinst_LIBRARIES = libarch_win32.a

INCLUDES = -I$(top_srcdir)/arch/include -I$(top_srcdir)/include -I$(top_srcdir)/main -I$(srcdir)/include

if USE_NETWORK
NETWORK_SRCS = winnet.c ipx_win.c
#               arch_ip.cpp
endif

libarch_win32_a_SOURCES = ${NETWORK_SRCS} findfile.c mingw_init.c

EXTRA_libarch_win32_a_SOURCES = winnet.c ipx_win.c arch_ip.cpp

EXTRA_DIST = ${EXTRA_SUBDIRS} \
d1x.ico d1x.rc d3d.ico debug.cpp descent.ico digi.c findfile.c glinit.c gr.c \
hmpfile.c init.c joydefs.c joyhh.c key.c mono.c mouse.c palw32.c \
pch.cpp scene.cpp serial.c texture.cpp timer.c win32.c winmain.cpp
