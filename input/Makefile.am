EXTRA_SUBDIRS = linux

INCLUDES = -I$(top_srcdir)/includes -I$(top_srcdir)/main -I$(top_srcdir)/arch/include

noinst_LIBRARIES = libinput.a

if ENV_DJGPP
libinput_a_SOURCES = dos_key.c dos_joyc.c dos_joydefs.c dos_mouse.c
else
if MINGW32
libinput_a_SOURCES = sdl_event.c sdl_key.c sdl_mouse.c sdl_joy.c sdl_joydefs.c
else
if USE_GGI
libinput_a_SOURCES = linux_joydefs.c linux_joystick.c \
ggi_event.c ggi_key.c ggi_mouse.c
else
if USE_SVGALIB
libinput_a_SOURCES = linux_joydefs.c linux_joystick.c \
svgalib_event.c svgalib_key.c svgalib_mouse.c svgalib_init.c
else
libinput_a_SOURCES = linux_joydefs.c linux_joystick.c \
sdl_event.c sdl_key.c sdl_mouse.c
endif
endif
endif
endif

EXTRA_libinput_a_SOURCES = \
dos_key.c dos_joyc.c dos_joydefs.c dos_mouse.c \
ggi_event.c ggi_key.c ggi_mouse.c \
linux_joydefs.c linux_joystick.c \
sdl_event.c sdl_key.c sdl_mouse.c sdl_joy.c sdl_joydefs.c \
svgalib_event.c svgalib_key.c svgalib_mouse.c svgalib_init.c

SUFFIXES = .asm
%.o: %.asm
	$(NASM) $(NASMFLAGS) $< -o $@

if ENV_DJGPP
libinput_a_LIBADD = dos_joy2.o
endif

EXTRA_DIST = \
dos_joy2.asm \
ggi_init.c

dist-hook:
	list='$(EXTRA_SUBDIRS)'; for subdir in $$list; do \
	 tar --exclude=CVS -cf - $$subdir | ( cd $(distdir) && tar xf - ) \
	done
