INCLUDES = -I $(top_srcdir) -I$(top_srcdir)/includes -I$(top_srcdir)/main

CFLAGS += ${SDL_CFLAGS}

noinst_LIBRARIES = libsound.a

if ENV_DJGPP
libsound_a_SOURCES = dos_digi.c dos_cdrom.c
else
libsound_a_SOURCES = sdl_digi.c sdl_cdrom.c
endif

EXTRA_libsound_a_SOURCES = sdl_digi.c sdl_cdrom.c dos_digi.c dos_cdrom.c
