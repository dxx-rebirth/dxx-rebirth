TOPDIR = ..

include $(TOPDIR)/makefile.config

ifdef NO_ASM
OBJECTS = fixc.$(OBJ) tables.$(OBJ) vecmat.$(OBJ) rand.$(OBJ)
else
E_CFLAGS = -DASM_VECMAT
OBJECTS = fix.$(OBJ) vecmat.$(OBJ) vecmata.$(OBJ) rand.$(OBJ)
endif

OUTPUT = $(LIBDIR)/maths.$(LIB)

include $(TOPDIR)/makefile.lib
