TOPDIR = ../../..

include $(TOPDIR)/makefile.config

OBJECTS += init.$(OBJ)

ifdef SVGALIB_VIDEO
OBJECTS += gr.$(OBJ)
endif

ifdef SVGALIB_INPUT
OBJECTS += mouse.$(OBJ) key_arch.$(OBJ) event.$(OBJ)
endif

OUTPUT = $(LIBDIR)svgalib.$(ARC)

CFLAGS += -I$(TOPDIR)/main

include $(TOPDIR)/makefile.lib