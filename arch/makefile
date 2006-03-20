TOPDIR = ..

include $(TOPDIR)/makefile.config

ifdef ENV_LINUX
SUBDIRS = linux
ifdef SDL
SUBDIRS += sdl
endif
endif

ifdef ENV_MINGW32
SUBDIRS = win32
endif

ifdef ENV_MSVC
SUBDIRS = win32
endif

ifdef OGL
SUBDIRS += ogl
endif

include $(TOPDIR)/makefile.lib
