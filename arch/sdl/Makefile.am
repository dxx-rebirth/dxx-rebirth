EXTRA_SUBDIRS = include

noinst_LIBRARIES = libarch_sdl.a

INCLUDES = -I$(top_srcdir)/arch/include -I$(top_srcdir)/includes -I$(top_srcdir)/main

CFLAGS += ${SDL_CFLAGS}

if USE_OPENGL
if MINGW32
libarch_sdl_a_SOURCES = cdrom.c digi.c event.c init.c joy.c joydefs.c key.c mouse.c timer.c
else
libarch_sdl_a_SOURCES = cdrom.c digi.c event.c init.c key.c mouse.c timer.c
endif
else
if MINGW32
libarch_sdl_a_SOURCES = cdrom.c digi.c event.c gr.c init.c joy.c joydefs.c key.c mouse.c timer.c
else
libarch_sdl_a_SOURCES = cdrom.c digi.c event.c gr.c init.c key.c mouse.c timer.c
endif
endif