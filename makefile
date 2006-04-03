TOPDIR = .

include $(TOPDIR)/makefile.config

SUBDIRS = arch maths 2d 3d texmap misc mem iff cfile main

#defines controlling the output filename

ifdef MAKE_EDITOR
SUBDIRS += editor ui 
D1XPREFIX = m
else
D1XPREFIX = d
endif

ifdef ENV_MINGW32
EXT=mw.exe
endif

ifdef SHAREWARE
SHRWR = sh
endif

ifdef DEBUG
DBG = db
endif

ifdef OGL
OPT = ogl
endif

OUTPUT = $(DESTDIR)/$(D1XPREFIX)1x$(D1XMAJOR)$(D1XMINOR)$(SHRWR)$(DBG)$(SND)$(OPT)$(EXT)

#libraries to link with

LIBS = \
	$(LIBDIR)/main.$(LIB)	\
	$(LIBDIR)/3d.$(LIB)	\
	$(LIBDIR)/2d.$(LIB)	\
	$(LIBDIR)/iff.$(LIB)	\
	$(LIBDIR)/maths.$(LIB)	\
	$(LIBDIR)/cfile.$(LIB) \
	$(LIBDIR)/mem.$(LIB)	\
	$(LIBDIR)/misc.$(LIB)	\
	$(LIBDIR)/texmap.$(LIB)	\
        $(LIBDIR)/io.$(LIB)

ifdef OGL
LIBS += $(LIBDIR)/ogl.$(LIB)
endif

ifdef MAKE_EDITOR
NO_RL2 = 1
LIBS += $(LIBDIR)/editor.$(LIB) $(LIBDIR)/ui.$(LIB)
E_CFLAGS += -DEDITOR -I.
ifdef ENV_LINUX
LINKLIBS += -lm
endif
endif

ifdef ENV_MINGW32
ifdef WGL_IO
ifndef OGL_RUNTIME
LIBS += -lopengl32
endif
LIBS += -lglu32 -lgdi32
endif
LIBS += -ldinput -lddraw -ldsound -ldxguid -lwsock32 -lwinmm -luser32 -lkernel32
LIBS += $(LIBDIR)/d1x_res.$(OBJ)
endif

ifdef ENV_LINUX
ifdef SDLGL_IO
LINKLIBS += -lGL -lGLU
endif
ifdef SDL
LIBS += $(LIBDIR)/sdl.$(LIB)
LINKLIBS += -ldl -lSDL
endif
endif
ifdef SCRIPT
LIBS += $(LIBDIR)/script.$(LIB)
E_CFLAGS += -DSCRIPT
ifdef GLIBC
LINKLIBS += -lpthread
endif
NASM = nasm -f elf -d__LINUX__
endif

default: $(OUTPUT)

$(DESTDIR):
ifeq ($(ENV_MINGW32),1)
ifneq ($(HAVE_MSYS),1)
	mkdir $(subst /,\,$(DESTDIR))
else
	mkdir $(DESTDIR)
endif
else
	mkdir $(DESTDIR)
endif

include $(TOPDIR)/makefile.rules

#rule for building the descent binary

$(OUTPUT):  $(DESTDIR) $(SUBDIRS)
ifdef SUPPORTS_NET_IP
	$(CXX) $(LFLAGS) -o $@ $(LIBS) $(LINKLIBS)
else
	$(CC) $(LFLAGS) -o $@ $(LIBS) $(LINKLIBS)
endif
ifdef	RELEASE
ifndef DEBUGABLE
	strip --strip-all $(OUTPUT)
endif
endif

#rule for building the ip daemon

ifdef SUPPORTS_NET_IP
IP_DAEMON_OUTPUT=$(DESTDIR)/$(D1XPREFIX)1x$(D1XMAJOR)$(D1XMINOR)$(DBG)$(EXT)_ip_daemon
IP_DAEMON_LIBS=$(TOPDIR)/main/$(OBJDIR)ipserver.$(OBJ) $(TOPDIR)/arch/linux/$(OBJDIR)arch_ip.$(OBJ) $(TOPDIR)/main/$(OBJDIR)ip_base.$(OBJ) $(TOPDIR)/arch/linux/$(OBJDIR)timer.$(OBJ) $(TOPDIR)/arch/linux/$(OBJDIR)mono.$(OBJ) $(TOPDIR)/main/$(OBJDIR)args.$(OBJ) $(TOPDIR)/mem/$(OBJDIR)mem.$(OBJ) $(TOPDIR)/misc/$(OBJDIR)strio.$(OBJ) $(TOPDIR)/misc/$(OBJDIR)strutil.$(OBJ) $(TOPDIR)/misc/$(OBJDIR)error.$(OBJ) $(TOPDIR)/lib/maths.a

ip_daemon: $(IP_DAEMON_OUTPUT)

$(IP_DAEMON_OUTPUT): $(SUBDIRS)
	$(CXX) $(LFLAGS) -o $(IP_DAEMON_OUTPUT) $(IP_DAEMON_LIBS)
endif

clean:
	$(CLEANSUBS)
	-rm $(OUTPUT)
	-rm $(IP_DAEMON_OUTPUT)
	-rm -r $(LIBDIR)/

depend:
	$(DEPSUBS)