noinst_LIBRARIES = libmaths.a
INCLUDES = -I $(top_srcdir)/includes 

if USE_ASM
libmaths_a_SOURCES = vecmat.c rand.c tables.c
else
libmaths_a_SOURCES = vecmat.c rand.c tables.c fixc.c
endif

SUFFIXES = .asm
%.o: %.asm
	$(NASM) $(NASMFLAGS) $< -o $@

if USE_ASM
libmaths_a_LIBADD = vecmata.o fix.o
endif

EXTRA_DIST = \
fix.asm vecmata.asm \
fixc.c rand.c tables.c vecmat.c
