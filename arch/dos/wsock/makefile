TOPDIR = ../../..

OBJECTS = bootp.$(OBJ) debug.$(OBJ) dns.$(OBJ) vxd.$(OBJ) \
          vxdldr.$(OBJ) winsock.$(OBJ) wsock.$(OBJ) _wsock.$(OBJ)

OUTPUT = $(LIBDIR)wsock.$(ARC)

%.o: %.s
	$(CC) -g -Wall -c $(patsubst %.s,%.S,$<) -o $@

%.o: %.S
	$(CC) -g -Wall -c $(patsubst %.s,%.S,$<) -o $@

%.o: %.cc
	$(CC) -g -Wall -I../include -c $< -o $@

include $(TOPDIR)/default.mak

