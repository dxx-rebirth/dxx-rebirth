EXTRA_SUBDIRS = allg_snd bak comm include mm_snd

noinst_LIBRARIES = libarch_dos.a

INCLUDES = -I$(top_srcdir)/arch/include -I$(top_srcdir)/includes -I$(top_srcdir)/main -I$(srcdir)/include

libarch_dos_a_SOURCES = \
dpmi.c findfile.c init.c ipx.c joyc.c joydefs.c key.c mouse.c vesa.c

SUFFIXES = .asm
%.o: %.asm
	$(NASM) $(NASMFLAGS) $< -o $@

if ENV_DJGPP
libarch_dos_a_LIBADD = timer.o modex.o
endif

EXTRA_DIST = ${EXTRA_SUBDIRS} \
tweak.inc vgaregs.inc \
modex.asm timer.asm \
bcd.c digiallg.c digimm.c disk.c midiallg.c mono.c serial.c \
ipx.h mono.h
