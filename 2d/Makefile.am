noinst_LIBRARIES = lib2d.a
INCLUDES = -I $(top_srcdir)/includes

lib2d_a_SOURCES = \
2dsline.c  box.c     disc.c    ibitblt.c  pcx.c    rect.c   scalec.c \
bitblt.c   canvas.c  font.c    line.c	  pixel.c  rle.c    tmerge.c \
bitmap.c   circle.c  gpixel.c  palette.c  poly.c 

if USE_ASM
  SUFFIXES = .asm
  %.o: %.asm
	$(NASM) $(NASMFLAGS) $< -o $@
	
  lib2d_a_SOURCES += linear.asm tmerge_a.asm
  lib2d_a_LIBADD += linear.o tmerge_a.o
  2dsline.c: linear.o tmerge_a.o
endif

