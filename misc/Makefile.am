noinst_LIBRARIES = libmisc.a
INCLUDES = -I $(top_srcdir)/include

libmisc_a_SOURCES = \
args.c	    d_delay.c  error.c	  strio.c \
d_io.c      hash.c     strutil.c  fileutil.c

# checker.c

#checker.o: checker.c
#	gcc -c -g $< -o $@ $(INCLUDES) $(CFLAGS) -I/usr/lib/checker/i686-pc-linux-gnu/include -I.. -I.

EXTRA_DIST = \
dos_disk.h parsarg.h \
byteswap.c errtest.c parsarg.c parstest.c
