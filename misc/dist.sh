#! /bin/sh

dir="`pwd`"
version=$(cat ../config.h | grep VERSION | sed -e 's/.*\"\(.*\)\"/\1/g')
distdir=aseprite-$version
zip="zip -9"
zip_recursive_flag="-r"

if [ ! -f dist.sh ]; then
    echo You must run dist.sh from misc/ directory
    exit 1
fi

######################################################################
# Full source distribution (with 3rd party code)

if [ ! -f $distdir.zip ] ; then
    cd "$dir/.."
    mkdir "$dir/$distdir"
    cp --parents $(git ls-files) "$dir/$distdir"
    cd "$dir"

    $zip $zip_recursive_flag $distdir.zip $distdir
    rm -fr $distdir
fi

#####################################################################
# Minimal source distribution (without 3rd party code).
# Used to compile ASEPRITE with shared libraries.

if [ ! -f $distdir.tar.xz ] ; then
    cd "$dir/.."
    mkdir "$dir/$distdir"
    cp --parents $(git ls-files | grep -v -e ^third_party | grep -v -e ^src/allegro) "$dir/$distdir"
    cp --parents $(git ls-files third_party/loadpng) "$dir/$distdir"
    cp --parents third_party/CMakeLists.txt "$dir/$distdir"
    cd "$dir"

    tar vcfJ $distdir.tar.xz $distdir
    rm -fr $distdir
fi

######################################################################
# Files for binary distributions

function def_common_files()
{
  txt_files="                                                        \
$1/README.html                                                       \
$1/LICENSE.txt                                                       \
$1/data/convmatr.def                                                 \
$1/data/*.xml                                                        \
$1/docs/files/*.txt                                                  \
$1/docs/licenses/*.txt                                               \
$1/data/widgets/*.xml"

  bin_files="                                                        \
$1/data/palettes/*.col                                               \
$1/data/icons/ase*.ico                                               \
$1/data/icons/ase*.png                                               \
$1/data/skins/*/*.png                                                \
$1/data/skins/*/*.xml                                                \
$1/docs/*.pdf"
}

######################################################################
# Win32 Distribution

if [ ! -f $distdir-win32.zip ] ; then

    cd "$dir/.."
    def_common_files .
    mkdir "$dir/$distdir-win32"

    # For Allegro dll / C Runtime dll
    #cp -r --parents $txt_files $bin_files aseprite.exe alleg44.dll msvcr90.dll "$dir/$distdir-win32"

    # For Allegro static / static C runtime dll (use /MT to compile Allegro)
    cp -r --parents $txt_files $bin_files aseprite.exe "$dir/$distdir-win32"

    cd "$dir"
    def_common_files $distdir-win32
    $zip -l $distdir-win32.zip $txt_files

    # Dynamic version of DLLs
    #$zip $distdir-win32.zip $bin_files         \
    #    $distdir-win32/aseprite.exe \
    #    $distdir-win32/alleg44.dll \
    #    $distdir-win32/msvcr90.dll

    # Static version
    $zip $distdir-win32.zip $bin_files \
        $distdir-win32/aseprite.exe

    rm -fr $distdir-win32
fi
