TOPDIR = ..

include $(TOPDIR)/makefile.config

OBJECTS =  \
        palette.$(OBJ) canvas.$(OBJ) \
	pixel.$(OBJ) rect.$(OBJ) 2dsline.$(OBJ) line.$(OBJ) \
	box.$(OBJ) poly.$(OBJ) disc.$(OBJ) gpixel.$(OBJ) \
	font.$(OBJ) bitmap.$(OBJ) bitblt.$(OBJ) pcx.$(OBJ) \
	rle.$(OBJ) ibitblt.$(OBJ) \
        circle.$(OBJ) tmerge.$(OBJ)

OBJECTS += scalec.$(OBJ)

ifndef NO_ASM
OBJECTS += linear.$(OBJ) tmerge_a.$(OBJ)
endif

OUTPUT = $(LIBDIR)2d.$(ARC)

include $(TOPDIR)/makefile.lib
