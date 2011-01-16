#!/bin/sh

# This script runs commands necessary to generate a Makefile for libgif.

echo "Warning: This script will run configure for you -- if you need to pass"
echo "  arguments to configure, please give them as arguments to this script."

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

THEDIR="`pwd`"
cd $srcdir

aclocal
autoheader
libtoolize --automake
automake --add-missing
autoconf
automake

cd $THEDIR

$srcdir/configure $*

exit 0
