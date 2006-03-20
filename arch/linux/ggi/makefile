TOPDIR = ../../..

include $(TOPDIR)/makefile.config

OBJECTS += init.$(OBJ)

ifdef GGI_VIDEO
OBJECTS += gr.$(OBJ)
endif

ifdef GII_INPUT
OBJECTS += key_arch.$(OBJ) mouse.$(OBJ) event.$(OBJ)
endif

OUTPUT = $(LIBDIR)ggi.$(ARC)

CFLAGS += -I$(TOPDIR)/main

include $(TOPDIR)/makefile.lib
