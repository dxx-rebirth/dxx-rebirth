EXTRA_SUBDIRS = include

noinst_LIBRARIES = libarch_linux.a

INCLUDES = -I$(top_srcdir)/includes -I$(top_srcdir)/main -I$(srcdir)/include

if USE_NETWORK
libarch_linux_a_SOURCES = findfile.c ipx_bsd.c ipx_kali.c ipx_lin.c linuxnet.c ukali.c
else
libarch_linux_a_SOURCES = findfile.c
endif

EXTRA_libarch_linux_a_SOURCES = \
ipx_bsd.c ipx_kali.c ipx_lin.c linuxnet.c ukali.c

EXTRA_DIST = \
alsadigi.c  hmistub.c    serial.c  ukali.c \
hmiplay.c   mono.c      timer.c

dist-hook:
	list='$(EXTRA_SUBDIRS)'; for subdir in $$list; do \
	 ( cd $(srcdir) && tar --exclude=CVS -cf - $$subdir ) \
	  | ( cd $(distdir) && tar xf - ) \
	done
