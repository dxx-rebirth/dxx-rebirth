noinst_LIBRARIES = libinput.a
INCLUDES = -I $(top_srcdir)/includes -I $(top_srcdir)/main

libinput_a_SOURCES = sdl_event.c sdl_key.c sdl_mouse.c \
ggi_event.c ggi_key.c ggi_mouse.c \
dos_key.c dos_joyc.c dos_joydefs.c dos_mouse.c \
linux_joydefs.c linux_joystick.c \
svgalib_event.c svgalib_key.c svgalib_mouse.c

SUFFIXES = .asm
%.o: %.asm
	$(NASM) $(NASMFLAGS) $< -o $@

if ENV_DJGPP
libinput_a_LIBADD += dos_joy2.o
endif
