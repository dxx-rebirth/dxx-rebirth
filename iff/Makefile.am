EXTRA_SUBDIRS = archive

noinst_LIBRARIES = libiff.a
INCLUDES = -I $(top_srcdir)/includes

libiff_a_SOURCES = iff.c

dist-hook:
	list='$(EXTRA_SUBDIRS)'; for subdir in $$list; do \
	 tar --exclude=CVS -cf - $$subdir | ( cd $(distdir) && tar xf - ) \
	done
