noinst_LIBRARIES = libtexmap.a
INCLUDES = -I $(top_srcdir)/includes -DFL1_WITH_FLAT

libtexmap_a_SOURCES = ntmap.c tmapflat.c

if USE_ASM
  SUFFIXES = .asm
  %.o: %.asm
	$(NASM) $(NASMFLAGS) $< -o $@
  libtexmap_a_SOURCES += tmapppro.S tmap_ll.asm tmap_flt.asm tmap_lin.asm tmapfade.asm
  libtexmap_a_LIBADD += tmapppro.o tmap_ll.o tmap_flt.o tmap_lin.o tmapfade.o
  ntmap.c tmapflat.c: tmapppro.o tmap_ll.o tmap_flt.o tmap_lin.o tmapfade.o
else
  libtexmap_a_SOURCES += scanline.c
  libtexmap_a_LIBADD += scanline.o
  ntmap.c tmapflat.c: scanline.o
endif
