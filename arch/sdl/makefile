TOPDIR = ../..

include $(TOPDIR)/makefile.config

OBJECTS += init.$(OBJ)

ifdef SDL_AUDIO
OBJECTS += digi.$(OBJ)
endif

# ifdef SDL_INPUT
OBJECTS += event.$(OBJ) key_arch.$(OBJ) mouse.$(OBJ) clipboard.$(OBJ) joy.$(OBJ) joydefs.$(OBJ)
# endif

ifdef SDL_VIDEO
OBJECTS += gr.$(OBJ) 
endif

OUTPUT = $(LIBDIR)/sdl.$(LIB)

include $(TOPDIR)/makefile.lib
