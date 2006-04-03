TOPDIR = ../..

include $(TOPDIR)/makefile.config

OBJECTS = joystick.$(OBJ) init.$(OBJ) \
          joydefs.$(OBJ) mono.$(OBJ) \
          timer.$(OBJ)

ifdef NETWORK
OBJECTS += linuxnet.$(OBJ) ipx_bsd.$(OBJ) \
          ipx_kali.$(OBJ) ukali.$(OBJ) \
          serial.$(OBJ)
endif

ifdef SUPPORTS_NET_IP
OBJECTS += arch_ip.$(OBJ)
endif

ifdef ALSA_AUDIO
OBJECTS += alsadigi.$(OBJ)
endif

ifdef HMIPLAY
OBJECTS += hmiplay.$(OBJ)
else
OBJECTS += hmistub.$(OBJ)
endif

ifdef WANT_AWE32
CFLAGS += -DWANT_AWE32
endif
ifdef WANT_MPU401
CFLAGS += -DWANT_MPU401
endif

OUTPUT = $(LIBDIR)/io.$(LIB)

include $(TOPDIR)/makefile.lib