noinst_LIBRARIES = libmaths.a
INCLUDES = -I $(top_srcdir)/includes 

libmaths_a_SOURCES = vecmat.c rand.c tables.c fixc.c

if USE_ASM
  SUFFIXES = .asm
  %.o: %.asm
	$(NASM) $(NASMFLAGS) $< -o $@

  libmaths_a_SOURCE += vecmata.asm fix.asm
  libmaths_a_LIBADD += vecmata.o fix.o
  vecmat.c rand.c tables.c: fix.o vecmata.o
endif
