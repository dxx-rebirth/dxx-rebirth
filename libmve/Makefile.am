EXTRA_PROGRAMS = mveplayer

mveplayer_SOURCES = decoder16.c decoder8.c mve_audio.c mve_main.c mvelib.c mveplay.c

mveplayer_MANS = mveplayer.1


noinst_LIBRARIES = libmve.a

INCLUDES = -I$(top_srcdir)/include

libmve_a_SOURCES = decoder16.c decoder8.c mve_audio.c mvelib.c mveplay.c

EXTRA_DIST = decoders.h mve_audio.h mvelib.h $(mveplayer_MANS)
