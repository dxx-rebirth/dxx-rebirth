#!/bin/sh
set -e
aclocal -I macros
autoheader
automake --add-missing
autoconf
#./configure "$@"
echo "Now you are ready to run ./configure"
