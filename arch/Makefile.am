SUBDIRS = @D2X_ARCH_SUBDIRS@
DIST_SUBDIRS = dos linux sdl win32
EXTRA_SUBDIRS = include

dist-hook:
	list='$(EXTRA_SUBDIRS)'; for subdir in $$list; do \
	 ( cd $(srcdir) && tar --exclude=CVS -cf - $$subdir ) \
	  | ( cd $(distdir) && tar xf - ) \
	done
