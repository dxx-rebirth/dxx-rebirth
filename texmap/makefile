TOPDIR = ..

include $(TOPDIR)/makefile.config

OBJECTS = \
 ntmap.$(OBJ) \
 scanline.$(OBJ)

ifdef OGL
NO_ASM = 1
else
OBJECTS += tmapflat.$(OBJ)
ifndef NO_ASM
 OBJECTS += \
  tmap_ll.$(OBJ) tmap_flt.$(OBJ) tmapfade.$(OBJ) tmap_lin.$(OBJ)
  OBJECTS+=tmapppro.$(OBJ) 
  OBJECTS+=tmappent.$(OBJ) 
  OBJECTS+=tmap_per.$(OBJ) 
endif
endif

OUTPUT = $(LIBDIR)/texmap.$(LIB)

include $(TOPDIR)/makefile.lib
