#!/bin/sh
set -e
touch ChangeLog
aclocal $ACLOCAL_FLAGS
autoheader
automake --add-missing
autoconf
#./configure "$@"
echo "Now you are ready to run ./configure"
