EXTRA_SUBDIRS = include

noinst_LIBRARIES = libarch_linux.a

INCLUDES = -I$(top_srcdir)/arch/include -I$(top_srcdir)/includes -I$(top_srcdir)/main -I$(srcdir)/include


if USE_NETWORK
NETWORK_SRCS = ipx_bsd.c ipx_kali.c ipx_lin.c linuxnet.c ukali.c
endif

if !SDL_JOYSTICK
JOYSTICK_SRCS = joydefs.c joystick.c
endif

libarch_linux_a_SOURCES = ${NETWORK_SRCS} ${JOYSTICK_SRCS} findfile.c init.c

EXTRA_libarch_linux_a_SOURCES = \
ipx_bsd.c ipx_kali.c ipx_lin.c joydefs.c joystick.c linuxnet.c ukali.c

EXTRA_DIST = ${EXTRA_SUBDIRS} \
alsadigi.c hmistub.c hmiplay.c ipx_udp.c mono.c serial.c timer.c ukali.c
