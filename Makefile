# Makefile emulation for the SCons build system

SCONSFLAGS="sdlmixer=1"

all:	build

info:
	@LANG=C svn info | grep '^Revision:' | cut -d' ' -f2 | xargs echo 'Based on revision'

build:
	scons $(SCONSFLAGS)

install:
	scons install

clean:
	scons -c

distclean:	clean
