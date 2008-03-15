#! /bin/bash

readln()
{
  echo -n "$1 [$2] "
  read ans
  if [ "$ans" == "" ] ; then ans="$2" ; fi
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
# prefix

if [ "$platform" == "unix" ] ; then
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
if [ "$debug" == "y" ] ; then echo "yes" ; else echo "no" ; fi

echo -n "  Profile suppport:	"
if [ "$profile" == "y" ] ; then echo "yes" ; else echo "no" ; fi

if [ "$prefix" != "" ] ; then
  echo "  Prefix:		$prefix"
fi

echo ""
readln "Is it right (y/n)?" "y"
if [ $ans != "y" ] ; then exit ; fi

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
  if [ "$2" == "conf" ] ; then
      echo "CONFIGURED = 1" >> $makefile
      echo "" >> $makefile
  fi

  if [ "$debug" != "y" ] ; then echo -n "#" >> $makefile ; fi
  echo "DEBUGMODE = 1" >> $makefile

  if [ "profile" != "y" ] ; then echo -n "#" >> $makefile ; fi
  echo "PROFILE = 1" >> $makefile

  if [ "$prefix" == "" ] ; then echo -n "#" >> $makefile ; fi
  echo "DEFAULT_PREFIX = \"$prefix\"" >> $makefile

  echo "#USE_386_ASM = 1" >> $makefile
  echo "" >> $makefile
  echo "include $makefile_name" >> $makefile
}

gen_makefile makefile conf
