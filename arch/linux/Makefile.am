EXTRA_SUBDIRS = include

noinst_LIBRARIES = libarch_linux.a

INCLUDES = -I$(top_srcdir)/arch/include -I$(top_srcdir)/includes -I$(top_srcdir)/main -I$(srcdir)/include

if USE_NETWORK
libarch_linux_a_SOURCES = joydefs.c joystick.c findfile.c init.c \
ipx_bsd.c ipx_kali.c ipx_lin.c linuxnet.c ukali.c
else
libarch_linux_a_SOURCES = joydefs.c joystick.c findfile.c init.c
endif

EXTRA_libarch_linux_a_SOURCES = \
ipx_bsd.c ipx_kali.c ipx_lin.c ipx_udp.c linuxnet.c ukali.c

EXTRA_DIST = ${EXTRA_SUBDIRS} \
alsadigi.c  hmistub.c    serial.c  ukali.c \
hmiplay.c   mono.c      timer.c
