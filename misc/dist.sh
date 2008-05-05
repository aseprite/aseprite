#! /bin/sh

dir="`pwd`"
version=0.6-beta2
distdir=ase-$version

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

lua_files="third_party/lua/COPYRIGHT		\
	   third_party/lua/HISTORY		\
	   third_party/lua/README		\
	   third_party/lua/doc/idx.html		\
	   third_party/lua/doc/index.html	\
	   third_party/lua/doc/manual.html	\
	   third_party/lua/include/*.h		\
	   third_party/lua/src/*.[ch]		\
	   third_party/lua/src/lib/*.[ch]	\
	   third_party/lua/src/lib/README"

zlib_files="third_party/zlib/*.[ch]		\
	    third_party/zlib/*.txt		\
	    third_party/zlib/README"

ase_files="config.h				\
	   ChangeLog				\
	   COPYING				\
	   fix.bat				\
	   fix.sh				\
	   makefile.cfg				\
	   makefile.dj				\
	   makefile.gcc				\
	   makefile.lnx				\
	   makefile.lst				\
	   makefile.mgw				\
	   *.txt				\
	   data/aseicon.*			\
	   data/convmatr.def			\
	   data/gui-en.xml			\
	   data/fonts/*.pcx			\
	   data/jids/*.jid			\
	   data/palettes/*.col			\
	   data/tips/*.pcx			\
	   data/tips/tips.en			\
	   data/tips/tips.es			\
	   docs/*.html				\
	   docs/*.txt				\
	   docs/files/*.txt			\
	   docs/licenses/*.txt			\
	   obj/*.txt				\
	   obj/djgpp/*.txt			\
	   obj/mingw32/*.txt			\
	   obj/unix/*.txt			\
	   src/*.[ch]				\
	   src/*.rc				\
	   src/commands/*.[ch]			\
	   src/commands/fx/*.[ch]		\
	   src/console/*.[ch]			\
	   src/core/*.[ch]			\
	   src/dialogs/*.[ch]			\
	   src/effect/*.[ch]			\
	   src/file/*.[ch]			\
	   src/file/fli/*.[ch]			\
	   src/file/fli/README			\
	   src/file/gif/*.[ch]			\
	   src/intl/*.[ch]			\
	   src/jinete/*.[ch]			\
	   src/jinete/themes/*.[ch]		\
	   src/jinete/themes/Makefile.icons	\
	   src/jinete/themes/stand/*.pcx	\
	   src/modules/*.[ch]			\
	   src/raster/*.[ch]			\
	   src/raster/x86/*.s			\
	   src/script/*.[ch]			\
	   src/script/*.py			\
	   src/test/*.[ch]			\
	   src/test/jinete/*.[ch]		\
	   src/test/jinete/*.jid		\
	   src/test/jinete/*.pcx		\
	   src/test/jinete/*.ttf		\
	   src/test/jinete/*.txt		\
	   src/test/raster/*.[ch]		\
	   src/util/*.[ch]			\
	   src/widgets/*.[ch]			\
	   src/widgets/editor/*.[ch]		\
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
    $lua_files					\
    $zlib_files					\
    $ase_files					\
    "$dir/$distdir"

cd "$dir"

# tar vczf $distdir.tar.gz $distdir
# tar vcjf $distdir.tar.bz2 $distdir
zip -r -9 $distdir.zip $distdir
rm -fr $distdir

fi

######################################################################
# Files for binary distributions

function def_common_files()
{
  txt_files="							     \
$1/AUTHORS.txt							     \
$1/LEGAL.txt							     \
$1/NEWS.txt							     \
$1/README.txt							     \
$1/WARNING.txt							     \
$1/COPYING							     \
$1/data/convmatr.def						     \
$1/data/jids/*.jid						     \
$1/data/gui-en.xml						     \
$1/data/tips/*.en						     \
$1/data/tips/*.es						     \
$1/docs/*.html							     \
$1/docs/*.info							     \
$1/docs/*.texi							     \
$1/docs/*.txt							     \
$1/docs/files/*.txt						     \
$1/docs/licenses/*.txt"

  bin_files="							     \
$1/data/aseicon.*						     \
$1/data/fonts/*.pcx						     \
$1/data/palettes/*.col						     \
$1/data/tips/*.pcx						     \
$1/docs/*.pdf"
}

######################################################################
# Win32 Distribution

if [ ! -f $distdir-win32.zip ] ; then

cd "$dir/.."
make -f makefile.mgw CONFIGURED=1 clean
make -f makefile.mgw CONFIGURED=1
strip -s aseprite.exe
def_common_files .
mkdir "$dir/$distdir-win32"
cp -r --parents $txt_files $bin_files aseprite.exe "$dir/$distdir-win32"

cd "$dir"
cp alleg42.dll "$dir/$distdir-win32"
def_common_files $distdir-win32
zip -9 $distdir-win32.zip $txt_files
zip -9 $distdir-win32.zip $bin_files		\
    $distdir-win32/aseprite.exe			\
    $distdir-win32/alleg42.dll
rm -fr $distdir-win32

fi
