TOPDIR = ../..

include $(TOPDIR)/makefile.config

OBJECTS += gr.$(OBJ) ogl.$(OBJ)

ifdef SDLGL_IO
OBJECTS += sdlgl.$(OBJ)
endif
ifdef WGL_IO
OBJECTS += wgl.$(OBJ)
endif

OUTPUT = $(LIBDIR)ogl.$(ARC)

CFLAGS += -I$(TOPDIR)/main

include $(TOPDIR)/makefile.lib
