#!/bin/sh
autoconf
export CONFIG_SHELL="bash"
echo "Please ignore any non-critical error messages in this script"
./configure "$@"

