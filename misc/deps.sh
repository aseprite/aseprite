#! /bin/sh

GCC="gcc -MM"
CFLAGS="-I.					\
	-Isrc					\
	-Ithird_party/lua/include		\
	-Ithird_party/intl			\
	-Ithird_party/libpng			\
	-Ithird_party/zlib			\
	-Ithird_party/jpeg			\
	-Ithird_party/freetype/include		\
	-Ithird_party"

rm -f makefile.dep

$GCC $CFLAGS							     \
    src/*.c							     \
    src/commands/*.c						     \
    src/commands/fx/*.c						     \
    src/console/*.c						     \
    src/core/*.c						     \
    src/dialogs/*.c						     \
    src/effect/*.c						     \
    src/file/*.c						     \
    src/file/*/*.c						     \
    src/intl/*.c						     \
    src/jinete/*.c						     \
    src/modules/*.c						     \
    src/raster/*.c						     \
    src/script/*.c						     \
    src/util/*.c						     \
    src/widgets/*.c						     \
    src/widgets/editor/*.c					     \
    | sed -e 's/^\([a-z_\-]*\.o\)/obj\/mingw32\/\1/' >> makefile.dep
