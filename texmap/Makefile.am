noinst_LIBRARIES = libtexmap.a
INCLUDES = -I $(top_srcdir)/includes -DFL1_WITH_FLAT

libtexmap_a_SOURCES = ntmap.c tmapflat.c

SUFFIXES = .asm
%.o: %.asm
	$(NASM) $(NASMFLAGS) $< -o $@

if USE_ASM
libtexmap_a_LIBADD += tmapppro.o tmap_ll.o tmap_flt.o tmap_lin.o tmapfade.o
else
libtexmap_a_LIBADD += scanline.o
endif

EXTRA_DIST = \
tmap_flt.asm tmap_inc.asm tmap_lin.asm tmap_ll.asm tmap_per.asm tmapfade.asm \
tmappent.S tmapppro.S \
scanline.h texmapl.h \
scanline.c
