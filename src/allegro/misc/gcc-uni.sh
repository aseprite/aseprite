#!/bin/sh

# gcc-uni.sh 
# ----------
# By Matthew Leverton
#
# Builds a universal binary by a multi-step process to allow for individual
# options for both architectures. Its primary use is to be used as a wrapper
# for makefile based projects.
#
# Although untested, it should be able to build OS X 10.2 compatible builds.
# Note that you may need to install the various OS SDKs before this will
# work. gcc-3.3 is used for the PPC build. The active version of gcc is used
# for the Intel build. (Note that it must be at least version 4.)
#
# If the makefile has a CC variable, this is all that should be necessary:
#
# CC=/usr/bin/gcc-uni.sh
#

# set up defaults
mode=link
output=
cmd=

# check whether to use gcc or g++
# (using a symlink with g++ in name is recommended)
case "$0" in
	*g++*)
		gcc=g++
		;;
	*)
		gcc=gcc
		;;
esac

# which OSX to target (used for PPC)
OSX_TARGET=10.2

# which SDK to use (unused with PPC because gcc-3.3 doesn't know about it))
SDK_i386=/Developer/SDKs/MacOSX10.4u.sdk
SDK_PPC=

# i386 flags
CFLAGS_i386=" -isysroot $SDK_i386"
LDFLAGS_i386=" -isysroot $SDK_i386 -Wl,-syslibroot,$SDK_i386"

# ppc flags
CFLAGS_PPC=
LDFLAGS_PPC=

# Parse options:
#   -arch switches are ignored
#   looks for -c to enable the CFLAGS
#   looks for -o to determine the name of the output

if [ $# -eq 0 ]; then
	echo "This is a wrapper around gcc that builds universal binaries."
	echo "It can only be used to compile or link."
	exit 1
fi

# remember the arguments in case there's no output files
args=$*

while [ -n "$1" ]; do
	case "$1" in
		-arch)
			shift
			;;
		-c)
			mode=compile
			cmd="$cmd -c"
			;;
		-o)
			shift
			output="$1"
			;;
		*)
			cmd="$cmd $1"
			;;
	esac
    
	shift
done

# if no output file, just pass the original command as-is and hope for the best
if [ -z "$output" ]; then
	exec $gcc $args
fi

# figure out if we are compiling or linking
case "$mode" in
	link)
		FLAGS_i386="$LDFLAGS_i386"
		FLAGS_PPC="$LDFLAGS_PPC"
		;;
	compile)
		FLAGS_i386="$CFLAGS_i386"
		FLAGS_PPC="$CFLAGS_PPC"
		;;
	*)
		echo "internal error in gcc-uni.sh script"
		exit 1
		;;
esac

# TODO: use trap to cleanup

# build the i386 version
$gcc $cmd $FLAGS_i386 -arch i386 -o $output.i386
if [ $? -ne 0 ]; then
	exit 1
fi

# build the PPC version
MACOSX_DEPLOYMENT_TARGET=$OSX_TARGET /usr/bin/$gcc-3.3 $cmd $FLAGS_PPC -arch ppc -o $output.ppc
if [ $? -ne 0 ]; then
	rm -f $output.i386
	exit 1 
fi

# create the universal version
lipo -create $output.i386 $output.ppc -output $output
if [ $? -ne 0 ]; then
	rm -f $output.i386 $output.ppc
	exit 1 
fi

# cleanup
rm -f $output.i386 $output.ppc
