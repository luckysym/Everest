#! /bin/sh

# autoscan
libtoolize -c
aclocal
autoheader
autoconf
automake -ac
