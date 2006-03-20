TOPDIR = ../..

include $(TOPDIR)/defines.mak

OBJECTS = digi.$(OBJ) hmpfile.$(OBJ)  \
	joyhh.$(OBJ) key_arch.$(OBJ) mouse.$(OBJ) timer.$(OBJ) \
        mono.$(OBJ) joydefs.$(OBJ)
	
ifdef NETWORK
OBJECTS += ipx_win.$(OBJ) winnet.$(OBJ) serial.$(OBJ) arch_ip.$(OBJ)
endif

ifdef WGL_IO
OBJECTS += glinit.$(OBJ)
else
OBJECTS += palw32.$(OBJ) 
ifdef D1XD3D
OBJECTS += pch.$(OBJ) win32.$(OBJ) scene.$(OBJ) texture.$(OBJ) winmain.$(OBJ)
OBJECTS += debug.$(OBJ)
E_CFLAGS = -Id3dframe
SUBDIRS = d3dframe
else
OBJECTS += init.$(OBJ) gr.$(OBJ)
endif
endif

ifdef ENV_MSVC
MISCOUTPUT += $(LIBDIR)d1x.res
else
MISCOUTPUT += $(LIBDIR)d1x_res.$(OBJ)
endif

OUTPUT = $(LIBDIR)io.$(ARC)

CFLAGS += -I$(TOPDIR)/main -Iinclude

$(LIBDIR)d1x.res: d1x.rc
	rc /i include /fo $@ /r $<

$(LIBDIR)d1x_res.o: d1x_res.rc
	windres --include include -o $@ $<

include $(TOPDIR)/makefile.lib


