#! /bin/sh

dir="`pwd`"
version=0.6
distdir=ase-$version

freetype_files="jinete/freetype/ChangeLog			     \
		jinete/freetype/descrip.mms			     \
		jinete/freetype/INSTALL				     \
		jinete/freetype/README				     \
		jinete/freetype/README.UNX			     \
		jinete/freetype/docs/*				     \
		jinete/freetype/include/*.h			     \
		jinete/freetype/include/freetype/*.h		     \
		jinete/freetype/include/freetype/cache/*.h	     \
		jinete/freetype/include/freetype/config/*.h	     \
		jinete/freetype/include/freetype/internal/*.h	     \
		jinete/freetype/src/autohint/*.[ch]		     \
		jinete/freetype/src/autohint/*.py		     \
		jinete/freetype/src/autohint/*.txt		     \
		jinete/freetype/src/base/*.[ch]			     \
		jinete/freetype/src/cache/*.[ch]		     \
		jinete/freetype/src/cff/*.[ch]			     \
		jinete/freetype/src/cid/*.[ch]			     \
		jinete/freetype/src/pcf/*.[ch]			     \
		jinete/freetype/src/psaux/*.[ch]		     \
		jinete/freetype/src/pshinter/*.[ch]		     \
		jinete/freetype/src/psnames/*.[ch]		     \
		jinete/freetype/src/raster/*.[ch]		     \
		jinete/freetype/src/sfnt/*.[ch]			     \
		jinete/freetype/src/smooth/*.[ch]		     \
		jinete/freetype/src/truetype/*.[ch]		     \
		jinete/freetype/src/type1/*.[ch]		     \
		jinete/freetype/src/winfonts/*.[ch]"

jinete_files="jinete/*.txt					     \
	      jinete/makefile.dj				     \
	      jinete/makefile.gcc				     \
	      jinete/makefile.lnx				     \
	      jinete/makefile.lst				     \
	      jinete/makefile.mgw				     \
	      jinete/docs/*.html				     \
	      jinete/docs/*.info				     \
	      jinete/docs/*.texi				     \
	      jinete/docs/*.txt					     \
	      jinete/examples/*.[ch]				     \
	      jinete/examples/*.jid				     \
	      jinete/examples/*.pcx				     \
	      jinete/examples/*.ttf				     \
	      jinete/examples/*.txt				     \
	      jinete/include/*.h				     \
	      jinete/include/jinete/*.h				     \
	      jinete/lib/*.txt					     \
	      jinete/lib/djgpp/*.txt				     \
	      jinete/lib/mingw32/*.txt				     \
	      jinete/lib/unix/*.txt				     \
	      jinete/obj/*.txt					     \
	      jinete/obj/djgpp/*.txt				     \
	      jinete/obj/mingw32/*.txt				     \
	      jinete/obj/unix/*.txt				     \
	      jinete/src/*.c					     \
	      jinete/src/themes/*.c				     \
	      jinete/src/themes/Makefile.icons			     \
	      jinete/src/themes/stand/*.pcx			     \
	      $freetype_files"

gfli_files="third_party/gfli/*.[ch]		\
	    third_party/gfli/README		\
	    third_party/gfli/TODO"

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

libart_files="third_party/libart_lgpl/AUTHORS	\
	      third_party/libart_lgpl/ChangeLog	\
	      third_party/libart_lgpl/COPYING	\
	      third_party/libart_lgpl/INSTALL	\
	      third_party/libart_lgpl/NEWS	\
	      third_party/libart_lgpl/README	\
	      third_party/libart_lgpl/*.[ch]"

ase_files="all.h						     \
	   config.h						     \
	   ChangeLog						     \
	   COPYING						     \
	   fix.bat						     \
	   fix.sh						     \
	   makefile.cfg						     \
	   makefile.dj						     \
	   makefile.gcc						     \
	   makefile.lnx						     \
	   makefile.lst						     \
	   makefile.mgw						     \
	   *.txt						     \
	   data/aseicon.*					     \
	   data/convmatr.def					     \
	   data/gui-en.xml					     \
	   data/gui-es.xml					     \
	   data/fonts/*.pcx					     \
	   data/jids/*.jid					     \
	   data/palettes/*.col					     \
	   data/po/*.po						     \
	   data/scripts/*.lua					     \
	   data/scripts/examples/*.lua				     \
	   data/tips/*.pcx					     \
	   data/tips/tips.en					     \
	   data/tips/tips.es					     \
	   docs/*.html						     \
	   docs/*.info						     \
	   docs/*.pdf						     \
	   docs/*.texi						     \
	   docs/*.txt						     \
	   docs/files/*.txt					     \
	   docs/licenses/*.txt					     \
	   obj/*.txt						     \
	   obj/djgpp/*.txt					     \
	   obj/mingw32/*.txt					     \
	   obj/unix/*.txt					     \
	   src/*.[ch]						     \
	   src/*.rc						     \
	   src/commands/*.[ch]					     \
	   src/console/*.[ch]					     \
	   src/core/*.[ch]					     \
	   src/dialogs/*.[ch]					     \
	   src/dialogs/effect/*.[ch]				     \
	   src/effect/*.[ch]					     \
	   src/file/*.[ch]					     \
	   src/file/gif/*.[ch]					     \
	   src/intl/*.[ch]					     \
	   src/modules/*.[ch]					     \
	   src/raster/*.[ch]					     \
	   src/raster/examples/*.c				     \
	   src/raster/x86/*.s					     \
	   src/script/*.[ch]					     \
	   src/script/bindings.py				     \
	   src/util/*.[ch]					     \
	   src/widgets/*.[ch]					     \
	   src/widgets/editor/*.[ch]				     \
	   src/widgets/editor/*.txt				     \
	   third_party/*.txt"

######################################################################
# Source Distribution

# if [ ! -f $distdir.tar.gz ] ; then
if [ ! -f $distdir.zip ] ; then

cd "$dir/.."
mkdir "$dir/$distdir"
cp --parents							     \
    $jinete_files						     \
    $gfli_files							     \
    $lua_files							     \
    $libart_files						     \
    $ase_files							     \
    "$dir/$distdir"
cd "$dir"

# tar vczf $distdir.tar.gz $distdir
# tar vcjf $distdir.tar.bz2 $distdir
zip -r -9 $distdir.zip $distdir
rm -fr $distdir

fi

exit

######################################################################
# Files for binary distributions

function def_common_files()
{
  txt_files="							     \
$1/*.txt							     \
$1/COPYING							     \
$1/data/convmatr.def						     \
$1/data/fonts/*.txt						     \
$1/data/jids/*.jid						     \
$1/data/menus.en						     \
$1/data/menus.es						     \
$1/data/po/es.po						     \
$1/data/scripts/*.lua						     \
$1/data/scripts/examples/*.lua					     \
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
$1/data/fonts/*.ttf						     \
$1/data/palettes/*.col						     \
$1/data/tips/*.pcx						     \
$1/docs/*.pdf"
}

######################################################################
# Unix Distribution

# if [ ! -f $distdir-unix.tar.gz ] ; then

# cd $dir/..
# rm ase
# make -f makefile.lnx CONFIGURED=1 HAVE_LIBJPEG=1
# strip ase
# def_common_files .
# mkdir $dir/$distdir-unix
# cp -r --parents $txt_files $bin_files ase $dir/$distdir-unix

# cd $dir
# tar vczf $distdir-unix.tar.gz $distdir-unix
# rm -fr $distdir-unix

# fi

######################################################################
# DOS Distribution

# if [ ! -f $distdir-dos.zip ] ; then

# cd $dir/..
# rm ase.exe
# djgpp make -f makefile.dj CONFIGURED=1 HAVE_LIBJPEG=1
# djgpp strip ase.exe
# def_common_files .
# mkdir $dir/$distdir-dos
# cp -r --parents $txt_files $bin_files ase.exe cwsdpmi.doc cwsdpmi.exe $dir/$distdir-dos

# cd $dir
# def_common_files $distdir-dos
# zip -l -9 $distdir-dos.zip $txt_files
# zip -9 $distdir-dos.zip $bin_files		\
#     $distdir-dos/ase.exe			\
#     $distdir-dos/cwsdpmi.*
# rm -fr $distdir-dos

# fi

######################################################################
# Win32 Distribution

if [ ! -f $distdir-win32.zip ] ; then

cd "$dir/.."
rm ase
mingw32 make -f makefile.mgw CONFIGURED=1 HAVE_LIBJPEG=1
mingw32 strip ase.exe
def_common_files .
mkdir "$dir/$distdir-win32"
cp -r --parents $txt_files $bin_files ase.exe "$dir/$distdir-win32"
cp alleg42.dll "$dir/$distdir-win32"

cd "$dir"
def_common_files $distdir-win32
zip -l -9 $distdir-win32.zip $txt_files
zip -9 $distdir-win32.zip $bin_files		\
    $distdir-win32/ase.exe			\
    $distdir-win32/alleg42.dll
rm -fr $distdir-win32

fi
