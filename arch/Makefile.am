SUBDIRS = dos linux ogl sdl win32

noinst_LIBRARIES = libarch.a
INCLUDES = -I $(top_srcdir)/includes -I $(top_srcdir)/main

libarch_a_SOURCES = \
dos_init.c dos_vesa.c dos_findfile.c dos_dpmi.c dos_ipx.c \
linux_init.c linux_findfile.c linux_net.c linux_ipx_bsd.c \
linux_ipx_lin.c linux_ipx_udp.c \
sdl_init.c sdl_timer.c

SUFFIXES = .asm
%.o: %.asm
	$(NASM) $(NASMFLAGS) $< -o $@

if ENV_DJGPP
libarch_a_LIBADD += dos_timer.o dos_modex.o
endif

EXTRA_DIST = \
dos_tweak.inc dos_vgaregs.inc \
dos_modex.asm dos_timer.asm
