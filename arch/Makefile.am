SUBDIRS = include dos linux win32

noinst_LIBRARIES = libarch.a
INCLUDES = -I$(top_srcdir)/includes -I$(top_srcdir)/main -I$(top_srcdir)/arch/include

if ENV_DJGPP
libarch_a_SOURCES = dos_init.c dos_vesa.c dos_findfile.c dos_dpmi.c dos_ipx.c
else
if USE_NETWORK
if MINGW32
libarch_a_SOURCES = linux_init.c win32_findfile.c sdl_init.c sdl_timer.c
	linux_ipx_bsd.c linux_ipx_lin.c linux_ipx_udp.c linux_net.c
else
libarch_a_SOURCES = linux_init.c linux_findfile.c sdl_init.c sdl_timer.c \
	linux_ipx_bsd.c linux_ipx_lin.c linux_ipx_udp.c linux_net.c
endif
else
if MINGW32
libarch_a_SOURCES = linux_init.c win32_findfile.c sdl_init.c sdl_timer.c
else
libarch_a_SOURCES = linux_init.c linux_findfile.c sdl_init.c sdl_timer.c
endif
endif
endif

EXTRA_libarch_a_SOURCES = \
	dos_init.c dos_vesa.c dos_findfile.c dos_dpmi.c dos_ipx.c \
	linux_init.c linux_findfile.c \
	linux_net.c linux_ipx_bsd.c linux_ipx_lin.c linux_ipx_udp.c \
	sdl_init.c sdl_timer.c \
	win32_findfile.c

SUFFIXES = .asm
%.o: %.asm
	$(NASM) $(NASMFLAGS) $< -o $@

if ENV_DJGPP
libarch_a_LIBADD += dos_timer.o dos_modex.o
endif

EXTRA_DIST = \
dos_tweak.inc dos_vgaregs.inc \
dos_modex.asm dos_timer.asm
