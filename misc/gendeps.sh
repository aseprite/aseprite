#! /bin/sh

# Jinete dependencies

GCC="gcc -MM"
CFLAGS="-Iinclude -Ifreetype/include"

cd jinete
$GCC $CFLAGS src/*.c						     \
    | sed -e 's/^\([a-z_\-]*\.o\)/obj\/mingw32\/\1/' > makefile.dep
cd ..

# ASE dependencies

CFLAGS="-I.					\
	-Isrc -Ijinete/include -Ilibase		\
	-Ithird_party/lua/include		\
	-Ithird_party/gfli			\
	-Ithird_party/intl			\
	-Ithird_party/libpng			\
	-Ithird_party/zlib			\
	-Ijinete/freetype/include		\
	-Ithird_party"

rm -f makefile.dep

$GCC $CFLAGS jinete/src/*.c					     \
    src/*.c							     \
    src/commands/*.c						     \
    src/commands/fx/*.c						     \
    src/console/*.c						     \
    src/core/*.c						     \
    src/dialogs/*.c						     \
    src/effect/*.c						     \
    src/file/*.c						     \
    src/intl/*.c						     \
    src/modules/*.c						     \
    src/raster/*.c						     \
    src/script/bindings.c src/script/script.c			     \
    src/util/*.c						     \
    src/widgets/*.c						     \
    src/widgets/editor/*.c					     \
    | sed -e 's/^\([a-z_\-]*\.o\)/obj\/mingw32\/\1/' >> makefile.dep
