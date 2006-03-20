TOPDIR = ../../..

OBJECTS = dzcomm.$(OBJ) dzqueue.$(OBJ) commlib.$(OBJ) irq.$(OBJ) \
          irqwrap.$(OBJ)

OUTPUT = $(LIBDIR)comm.$(ARC)

%.o: %.s
	$(CC) $(CFLAGS) $(patsubst %.s,%.S,$<) -o $@


%.o: %.S
	$(CC) $(CFLAGS) $(patsubst %.s,%.S,$<) -o $@


include $(TOPDIR)/default.mak

CFLAGS += -Iinclude

