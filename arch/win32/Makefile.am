EXTRA_SUBDIRS = include d3dframe

noinst_LIBRARIES = libarch_win32.a

INCLUDES = -I$(top_srcdir)/arch/include -I$(top_srcdir)/include -I$(top_srcdir)/main -I$(srcdir)/include

if USE_NETWORK
NETWORK_SRCS = ipx_mcast4.c ipx_win.c ipx_udp.c winnet.c
endif

libarch_win32_a_SOURCES = ${NETWORK_SRCS} hmpfile.c midi.c mingw_init.c

EXTRA_libarch_win32_a_SOURCES = ipx_mcast4.c ipx_win.c ipx_udp.c winnet.c

EXTRA_DIST = ${EXTRA_SUBDIRS} \
d1x.ico d1x.rc d3d.ico debug.cpp descent.ico digi.c glinit.c gr.c \
init.c joydefs.c joyhh.c key.c mono.c mouse.c palw32.c \
pch.cpp scene.cpp serial.c texture.cpp timer.c win32.c winmain.cpp \
ipx_drv.h
