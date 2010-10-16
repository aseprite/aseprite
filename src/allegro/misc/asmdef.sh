#!/bin/sh

# This script, by Neil Townsend, is for cross-compilers to parse the `asmdef.s'
# file and generate `asmdef.inc'.
# Updated by Peter Wang.

# Parse command-line arguments
if [ $# != 2 ]; then
  echo "usage: $0 asmdef.s asmdef.inc"
  exit 1
fi
s_file=$1
inc_file=$2

# 1. Get the comments
grep "/\*" $s_file | cut -f 2 -d '"' | cut -f 1 -d \\ > $inc_file
echo >> $inc_file

# 2. Get the real stuff
list=`grep "\.long" $s_file | sed -e 's/.*\.long//'`
if [ -z "$list" ]; then
  list=`grep "\.word" $s_file | sed -e 's/.*\.word//'`
fi
if [ -z "$list" ]; then
  echo "$0: unable to parse assembly file $s_file"
  exit 1
fi

# `list' is now a list of label-value pairs, terminated by a "0" label.
#
#   LC0 0 LC1 0 LC2 42 ... LCn v ... 0 0

# Set the positional parameters to `list'.
set -- $list

# Keep looping until we reach the "0" label.
while [ $1 != "0" ]; do
    label=$1:
    value=$2
    shift 2

    # Get the string corresponding to the label.
    # Remove quote marks and NUL terminator.
    n=`awk /$label/,/ascii\|string/ $s_file |\
       grep 'ascii\|string' |\
       cut -f 2 -d '"' |\
       cut -f 1 -d '\\'`

    if [ -z "$n" ]; then
      echo "$0: unable to parse assembly file $s_file"
      exit 1
    fi

    # The string is one of:
    #
    #	##FOO
    #	#ifdef BAR
    #	#endif BAR
    #	BAZ 
    #	NEWLINE (literally)
    #
    case $n in
      '##'*)
        n=`echo $n | cut -b 3-`
	echo "#ifndef $n" >> $inc_file
	echo "#define $n" >> $inc_file
	echo "#endif" >> $inc_file
	echo >> $inc_file
	;;

      '#'*)
        echo $n >> $inc_file
	;;

      NEWLINE)
	echo >> $inc_file
	;;

      *)
	echo "#define $n $value" >> $inc_file
	;;
    esac
done

echo >> $inc_file

