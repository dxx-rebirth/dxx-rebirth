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

ifdef D1XD3D
EXT=3d.exe
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
	$(LIBDIR)main.$(ARC)	\
	$(LIBDIR)3d.$(ARC)	\
	$(LIBDIR)2d.$(ARC)	\
	$(LIBDIR)iff.$(ARC)	\
	$(LIBDIR)maths.$(ARC)	\
	$(LIBDIR)cfile.$(ARC) \
	$(LIBDIR)mem.$(ARC)	\
	$(LIBDIR)misc.$(ARC)	\
	$(LIBDIR)texmap.$(ARC)	\
        $(LIBDIR)io.$(ARC)

#defines for dxbase mode

ifdef DXBASE
CFLAGS += -I$(DXBASE)/include
LINKLIBS += -L$(DXBASE)/lib
endif

ifdef D1XD3D
LIBS += $(LIBDIR)d3dframe.$(ARC)
endif

ifdef OGL
LIBS += $(LIBDIR)ogl.$(ARC)
endif

ifdef SDLGL_IO
LINKLIBS += -lGL -lGLU
endif

ifdef GLX_IO
LINKLIBS += -L/usr/X11R6/lib
ifndef OGL_RUNTIME
LINKLIBS += -lGL 
endif
LINKLIBS += -lGLU -lm -lX11 -lXext
endif

ifdef MAKE_EDITOR
NO_RL2 = 1
LIBS += $(LIBDIR)editor.$(ARC) $(LIBDIR)ui.$(ARC)
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
# LIBS += $(LIBDIR)d1x_res.$(OBJ)
endif

ifdef ENV_MSVC
LINKLIBS += dinput.lib ddraw.lib dsound.lib dxguid.lib wsock32.lib \
	winmm.lib user32.lib kernel32.lib gdi32.lib
# LIBS += $(LIBDIR)d1x.res
ifdef DEBUG
LINKLIBS += msvcrtd.lib
else
LINKLIBS += msvcrt.lib
endif
ifdef RELEASE
LFLAGS = -OPT:REF -PDB:NONE
else
LFLAGS = -DEBUG -DEBUGTYPE:CV -PDB:$(LIBDIR)
endif
endif

ifdef ENV_LINUX
ifdef SDL
LIBS += $(LIBDIR)sdl.$(ARC)
ifdef STATICSDL
LINKLIBS += $(STATICSDL) -ldl -L/usr/X11R6/lib -lX11 -lXext -lXxf86dga -lXxf86vm
else
LINKLIBS += -ldl -lSDL
endif
endif
ifdef SVGALIB
LIBS += $(LIBDIR)svgalib.$(ARC)
LINKLIBS += -lvga -lvgagl
endif
ifdef GGI
LIBS += $(LIBDIR)ggi.$(ARC)
endif
endif
ifdef SCRIPT
LIBS += $(LIBDIR)script.$(ARC)
E_CFLAGS += -DSCRIPT
ifdef GGI_VIDEO
LINKLIBS += -lggi
endif
ifdef GII_INPUT
LINKLIBS += -lgii
endif
ifdef GLIBC
LINKLIBS += -lpthread
endif
NASM = nasm -f elf -d__LINUX__
endif

default: $(OUTPUT)

$(DESTDIR):
	mkdir $(DESTDIR)

include $(TOPDIR)/makefile.rules

#rule for building the descent binary

ifdef ENV_MSVC
$(OUTPUT):  $(DESTDIR) $(SUBDIRS)
	link $(LFLAGS) -out:$(OUTPUT)  -machine:i386 -subsystem:console $(LIBS) $(LINKLIBS)
else
$(OUTPUT):  $(DESTDIR) $(SUBDIRS)
	$(CC) $(LFLAGS) -o $@ $(LIBS) $(LINKLIBS)
ifdef	RELEASE
ifndef DEBUGABLE
	strip --strip-all $(OUTPUT)
endif
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
	-rm $(OUTPUT)
	-rm $(IP_DAEMON_OUTPUT)
	$(CLEANSUBS)

depend:
	$(DEPSUBS)