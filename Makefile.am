SUBDIRS = 2d 3d maths mem cfile iff texmap misc input video sound arch main

## to automatically rebuild aclocal.m4 if any of the macros in
## `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@ cd macros && $(MAKE) macros.dep
