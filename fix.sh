#! /bin/bash

readln()
{
  echo -n "$1 [$2] "
  read ans
  if [ X"$ans" = X"" ] ; then ans="$2" ; fi
}

######################################################################
# platform

readln "What platform (unix/djgpp/mingw32)?" "unix"
platform=$ans

######################################################################
# debug info

readln "Do you want debug ASE (y/n)?" "n"
debug=$ans

######################################################################
# profile info

readln "Do you want profile ASE (y/n)?" "n"
profile=$ans

######################################################################
# memory leaks

readln "Do you want to check memory leaks (y/n)?" "n"
memleak=$ans

######################################################################
# prefix

if [ X"$platform" = X"unix" ] ; then
  readln "Where do you want install ASE by default?" "/usr/local"
  prefix=$ans
else
  prefix=""
fi

######################################################################
# show information

case "$platform" in
  "unix"    ) platform_name="Unix" ;;
  "djgpp"   ) platform_name="DOS (djgpp)" ;;
  "mingw32" ) platform_name="Windows (Mingw32)" ;;
  "*"       ) exit ;;
esac

echo "ASE configured:"

echo "  Platform:		$platform_name"

echo -n "  Debug suppport:	"
if [ X"$debug" = X"y" ] ; then echo "yes" ; else echo "no" ; fi

echo -n "  Profile suppport:	"
if [ X"$profile" = X"y" ] ; then echo "yes" ; else echo "no" ; fi

echo -n "  Check memory leaks:	"
if [ X"$memleak" = X"y" ] ; then echo "yes" ; else echo "no" ; fi

if [ X"$prefix" != X"" ] ; then
  echo "  Prefix:		$prefix"
fi

echo ""
readln "Is it right (y/n)?" "y"
if [ X"$ans" != X"y" ] ; then exit ; fi

######################################################################
# generate the makefile

case "$platform" in
  "unix"    ) makefile_name="makefile.lnx" ;;
  "djgpp"   ) makefile_name="makefile.dj" ;;
  "mingw32" ) makefile_name="makefile.mgw" ;;
  "*"       ) exit ;;
esac

gen_makefile()
{
  makefile=$1

  echo -n "creating $makefile..."

  if [ -f $makefile ] ; then
    mv $makefile $makefile~
    echo " (backup in $makefile~)"
  else
    echo ""
  fi

  echo "# Makefile for $platform_name generated with fix.sh" > $makefile
  echo "" >> $makefile
  if [ X"$2" = X"conf" ] ; then
      echo "CONFIGURED = 1" >> $makefile
      echo "" >> $makefile
  fi

  if [ X"$debug" == X"y" ] ; then echo -n "#" >> $makefile ; fi
  echo "RELEASE = 1" >> $makefile

  if [ X"$debug" != X"y" ] ; then echo -n "#" >> $makefile ; fi
  echo "DEBUGMODE = 1" >> $makefile

  if [ X"profile" != X"y" ] ; then echo -n "#" >> $makefile ; fi
  echo "PROFILE = 1" >> $makefile

  if [ X"memleak" != X"y" ] ; then echo -n "#" >> $makefile ; fi
  echo "MEMLEAK = 1" >> $makefile

  if [ X"$prefix" = X"" ] ; then echo -n "#" >> $makefile ; fi
  echo "DEFAULT_PREFIX = \"$prefix\"" >> $makefile

  echo "" >> $makefile
  echo "include $makefile_name" >> $makefile
}

gen_makefile makefile conf
