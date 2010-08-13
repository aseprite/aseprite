#! /bin/sh

dir="`pwd`"
version=0.8.1-beta
distdir=ase-$version
zip="zip -9"
zip_recursive_flag="-r"

freetype_files="third_party/freetype/ChangeLog				\
		third_party/freetype/descrip.mms			\
		third_party/freetype/INSTALL				\
		third_party/freetype/README				\
		third_party/freetype/README.UNX				\
		third_party/freetype/docs/*				\
		third_party/freetype/include/*.h			\
		third_party/freetype/include/freetype/*.h		\
		third_party/freetype/include/freetype/cache/*.h		\
		third_party/freetype/include/freetype/config/*.h	\
		third_party/freetype/include/freetype/internal/*.h	\
		third_party/freetype/src/autohint/*.[ch]		\
		third_party/freetype/src/autohint/*.py			\
		third_party/freetype/src/autohint/*.txt			\
		third_party/freetype/src/base/*.[ch]			\
		third_party/freetype/src/cache/*.[ch]			\
		third_party/freetype/src/cff/*.[ch]			\
		third_party/freetype/src/cid/*.[ch]			\
		third_party/freetype/src/pcf/*.[ch]			\
		third_party/freetype/src/psaux/*.[ch]			\
		third_party/freetype/src/pshinter/*.[ch]		\
		third_party/freetype/src/psnames/*.[ch]			\
		third_party/freetype/src/raster/*.[ch]			\
		third_party/freetype/src/sfnt/*.[ch]			\
		third_party/freetype/src/smooth/*.[ch]			\
		third_party/freetype/src/truetype/*.[ch]		\
		third_party/freetype/src/type1/*.[ch]			\
		third_party/freetype/src/winfonts/*.[ch]"

jpeg_files="third_party/jpeg/*.[ch]		\
	    third_party/jpeg/*.log		\
	    third_party/jpeg/*.doc		\
	    third_party/jpeg/*.asm		\
	    third_party/jpeg/*.txt"

libart_files="third_party/libart_lgpl/AUTHORS	\
	      third_party/libart_lgpl/ChangeLog	\
	      third_party/libart_lgpl/COPYING	\
	      third_party/libart_lgpl/INSTALL	\
	      third_party/libart_lgpl/NEWS	\
	      third_party/libart_lgpl/README	\
	      third_party/libart_lgpl/*.[ch]"

libpng_files="third_party/libpng/*.[ch]		\
	      third_party/libpng/*.txt		\
	      third_party/libpng/LICENSE	\
	      third_party/libpng/README		\
	      third_party/libpng/TODO		\
	      third_party/libpng/Y2KINFO"

loadpng_files="third_party/loadpng/*.[ch]	\
	       third_party/loadpng/*.txt"

tinyxml_files="third_party/tinyxml/*.txt	\
	       third_party/tinyxml/*.cpp	\
	       third_party/tinyxml/*.h"

vaca_files="third_party/vaca/include/Vaca/*.h	\
	    third_party/vaca/src/*.cpp		\
	    third_party/vaca/src/allegro/*.h	\
	    third_party/vaca/src/std/*.h	\
	    third_party/vaca/src/unix/*.h	\
	    third_party/vaca/src/win32/*.h"

zlib_files="third_party/zlib/*.[ch]		\
	    third_party/zlib/*.txt		\
	    third_party/zlib/README"

ase_files="config.h				\
	   COPYING				\
	   NEWS.txt				\
	   README.html				\
	   TODO.txt				\
	   fix.bat				\
	   fix.sh				\
	   makefile.cfg				\
	   makefile.gcc				\
	   makefile.linux			\
	   makefile.lst				\
	   makefile.macosx			\
	   makefile.mingw			\
	   makefile.vc				\
	   data/convmatr.def			\
	   data/*.xml				\
	   data/fonts/*.pcx			\
	   data/icons/ase*.ico			\
	   data/icons/ase*.png			\
	   data/palettes/*.col			\
	   data/skins/*/*.pcx			\
	   data/skins/*/*.png			\
	   data/skins/*/*.xml			\
	   data/widgets/*.xml			\
	   docs/*.pdf				\
	   docs/files/*.txt			\
	   docs/licenses/*.txt			\
	   misc/deps.sh				\
	   misc/dist.sh				\
	   misc/etags.sh			\
	   obj/*.txt				\
	   obj/mingw32/*.txt			\
	   obj/msvc/*.txt			\
	   obj/unix/*.txt			\
	   src/*.cpp				\
	   src/*.h				\
	   src/*.rc				\
	   src/commands/*.cpp			\
	   src/commands/*.h			\
	   src/commands/fx/*.cpp		\
	   src/commands/fx/*.h			\
	   src/core/*.cpp			\
	   src/core/*.h				\
	   src/dialogs/*.cpp			\
	   src/dialogs/*.h			\
	   src/effect/*.cpp			\
	   src/effect/*.h			\
	   src/file/*.cpp			\
	   src/file/*.h				\
	   src/file/fli/*.cpp			\
	   src/file/fli/*.h			\
	   src/file/fli/README			\
	   src/file/gif/*.cpp			\
	   src/file/gif/*.h			\
	   src/intl/*.cpp			\
	   src/intl/*.h				\
	   src/jinete/*.cpp			\
	   src/jinete/*.h			\
 	   src/jinete/themes/*.cpp		\
 	   src/jinete/themes/*.h		\
	   src/jinete/themes/Makefile.icons	\
	   src/jinete/themes/stand/*.pcx	\
	   src/modules/*.cpp			\
	   src/modules/*.h			\
	   src/raster/*.cpp			\
	   src/raster/*.h			\
	   src/settings/*.cpp			\
	   src/settings/*.h			\
	   src/tests/*.cpp			\
	   src/tests/*.h			\
	   src/tests/jinete/*.cpp		\
	   src/tests/jinete/*.jid		\
	   src/tests/jinete/*.pcx		\
	   src/tests/jinete/*.ttf		\
	   src/tests/jinete/*.txt		\
	   src/tests/raster/*.cpp		\
	   src/tools/*.cpp			\
	   src/tools/*.h			\
	   src/util/*.cpp			\
	   src/util/*.h				\
	   src/widgets/*.cpp			\
	   src/widgets/*.h			\
	   src/widgets/editor/*.cpp		\
	   src/widgets/editor/*.h		\
	   src/widgets/editor/*.txt		\
	   third_party/*.txt"

######################################################################
# Source Distribution

if [ ! -f $distdir.zip ] ; then

cd "$dir/.."
mkdir "$dir/$distdir"

cp --parents					\
    $freetype_files				\
    $jpeg_files					\
    $libart_files				\
    $libpng_files				\
    $loadpng_files				\
    $tinyxml_files				\
    $vaca_files					\
    $zlib_files					\
    $ase_files					\
    "$dir/$distdir"

cd "$dir"

# tar vczf $distdir.tar.gz $distdir
# tar vcjf $distdir.tar.bz2 $distdir
$zip $zip_recursive_flag $distdir.zip $distdir
rm -fr $distdir

fi

######################################################################
# Files for binary distributions

function def_common_files()
{
  txt_files="							     \
$1/NEWS.txt							     \
$1/README.html							     \
$1/COPYING							     \
$1/data/convmatr.def						     \
$1/data/*.xml						     	     \
$1/docs/files/*.txt						     \
$1/docs/licenses/*.txt						     \
$1/data/widgets/*.xml"

  bin_files="							     \
$1/data/aseicon.*						     \
$1/data/palettes/*.col						     \
$1/data/icons/ase*.ico						     \
$1/data/icons/ase*.png						     \
$1/data/skins/*/*.pcx						     \
$1/data/skins/*/*.png						     \
$1/data/skins/*/*.xml						     \
$1/docs/*.pdf"
}

######################################################################
# Win32 Distribution

if [ ! -f $distdir-win32.zip ] ; then

cd "$dir/.."
#make -f makefile.vc CONFIGURED=1 RELEASE=1 STATIC_ALLEG_LINK=1 clean
#make -f makefile.vc CONFIGURED=1 RELEASE=1 STATIC_ALLEG_LINK=1
def_common_files .
mkdir "$dir/$distdir-win32"

# For Allegro dll / C Runtime dll
#cp -r --parents $txt_files $bin_files aseprite.exe alleg44.dll msvcr90.dll "$dir/$distdir-win32"

# For Allegro static / static C runtime dll (use /MT to compile Allegro)
cp -r --parents $txt_files $bin_files aseprite.exe "$dir/$distdir-win32"

cd "$dir"
def_common_files $distdir-win32
$zip $distdir-win32.zip $txt_files

# Dynamic version of DLLs
#$zip $distdir-win32.zip $bin_files		\
#    $distdir-win32/aseprite.exe \
#    $distdir-win32/alleg44.dll \
#    $distdir-win32/msvcr90.dll

# Static version
$zip $distdir-win32.zip $bin_files \
    $distdir-win32/aseprite.exe

rm -fr $distdir-win32

fi
