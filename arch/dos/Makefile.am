EXTRA_SUBDIRS = allg_snd bak comm include mm_snd

noinst_LIBRARIES = libarch_dos.a

INCLUDES = -I$(top_srcdir)/arch/include -I$(top_srcdir)/include -I$(top_srcdir)/main -I$(srcdir)/include

libarch_dos_a_SOURCES = \
cdrom.c digi.c dpmi.c findfile.c gr.c init.c ipx.c joyc.c joydefs.c key.c mouse.c vesa.c

SUFFIXES = .asm
%.o: %.asm
	$(NASM) $(NASMFLAGS) $< -o $@

libarch_dos_a_LIBADD = timer.o modex.o

EXTRA_DIST = ${EXTRA_SUBDIRS} \
tweak.inc vgaregs.inc \
joy2.asm modex.asm timer.asm \
bcd.c digiallg.c digimm.c disk.c midiallg.c mono.c serial.c \
ipx.h mono.h
