SUBDIRS = 2d 3d maths mem cfile iff texmap misc input video sound arch main \
includes tools unused

changelog::
	-touch ChangeLog
	-tools/cvs2cl/cvs2cl.pl -b --utc
	-rm -f ChangeLog.bak

## to automatically rebuild aclocal.m4 if any of the macros in
## `macros/' change
#@MAINT@include macros/macros.dep
#@MAINT@macros/macros.dep: macros/Makefile.am
#@MAINT@ cd macros && $(MAKE) macros.dep

EXTRA_DIST = \
ChangeLog \
djgpp.bat djgpp.sh \
cvshowto.txt readme.txt
