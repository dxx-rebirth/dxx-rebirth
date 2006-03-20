TOPDIR = ../../..

E_CFLAGS = -Isound -I.

SOUND_OBJS = sound.o midi.o config.o snddrv.o \
	drv/sb.o drv/gus.o drv/adlib.o drv/emu8k.o drv/emu8kmid.o \
	drv/awedata.o \
	drv/dma.o \
	drv/mpu.o \
	digmid.o \
        mixer.o

OBJECTS = allg_snd.$(OBJ) $(addprefix sound/,$(SOUND_OBJS))

OUTPUT = $(LIBDIR)allg_snd.$(ARC)

include $(TOPDIR)/default.mak

CLEANOBJ = rm -f *.$(OBJ); rm -f sound/*.$(OBJ); rm -f sound/drv/*.$(OBJ)
