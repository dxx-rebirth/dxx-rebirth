SUBDIRS = 2d 3d maths mem cfile iff texmap misc input video sound arch main

EXTRA_SUBDIRS = includes tools unused

dist-hook:
	list='$(EXTRA_SUBDIRS)'; for subdir in $$list; do \
	 ( cd $(srcdir) && tar --exclude=CVS/ -cf - $$subdir ) \
	  | ( cd $(distdir) && tar xf - ) \
	done
	$(MAKE) changelog
	cp -a ChangeLog $(distdir)

changelog::
	rm -f $(srcdir)/ChangeLog
	touch $(srcdir)/ChangeLog
	$(srcdir)/tools/cvs2cl/cvs2cl.pl -b --utc
	rm -f ChangeLog.bak

EXTRA_DIST = \
ChangeLog \
djgpp.bat djgpp.sh \
autogen.sh cygconf \
cvshowto.txt readme.txt license.txt thinking.txt installation.txt mingw32.txt
