if ENV_DJGPP

noinst_LIBRARIES = libarchdos.a
INCLUDES = -I $(top_srcdir)/includes -Iinclude -I $(top_srcdir)/main -I$(top_srcdir)/input/sdl/include

libarchdos_a_SOURCES = init.c dpmi.c findfile.c vesa.c


if USE_ASM
  SUFFIXES = .asm
  %.o: %.asm
	$(NASM) $(NASMFLAGS) $< -o $@
	
  libarchdos_a_SOURCES += timer.asm modex.asm
  libarchdos_a_LIBADD += timer.o modex.o
  init.c: timer.o modex.o
endif


endif
