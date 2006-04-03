TOPDIR = ../..

include $(TOPDIR)/makefile.config

OBJECTS = digi.$(OBJ) hmpfile.$(OBJ)  \
	joyhh.$(OBJ) key_arch.$(OBJ) mouse.$(OBJ) timer.$(OBJ) \
        mono.$(OBJ) joydefs.$(OBJ)
	
RESOURCES += d1x_res.$(OBJ)
	
ifdef NETWORK
OBJECTS += ipx_win.$(OBJ) winnet.$(OBJ) serial.$(OBJ) arch_ip.$(OBJ)
endif

ifdef WGL_IO
OBJECTS += glinit.$(OBJ)
else
OBJECTS += palw32.$(OBJ) 
OBJECTS += init.$(OBJ) gr.$(OBJ)
endif

OUTPUT = $(LIBDIR)/io.$(LIB)

include $(TOPDIR)/makefile.lib
