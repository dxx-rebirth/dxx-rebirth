noinst_LIBRARIES = libtexmap.a
INCLUDES = -I $(top_srcdir)/include -DFL1_WITH_FLAT

if USE_ASM
libtexmap_a_SOURCES = ntmap.c scanline.c tmapflat.c tmappent.S tmapppro.S
else
libtexmap_a_SOURCES = ntmap.c scanline.c tmapflat.c
endif

SUFFIXES = .asm
%.o: %.asm
	$(NASM) $(NASMFLAGS) $< -o $@

if USE_ASM
libtexmap_a_LIBADD = tmap_flt.o tmap_lin.o tmap_ll.o tmap_per.o tmapfade.o
endif

EXTRA_DIST = \
tmap_flt.asm tmap_inc.asm tmap_lin.asm tmap_ll.asm tmap_per.asm tmapfade.asm \
tmappent.S tmapppro.S \
scanline.h texmapl.h \
scanline.c
