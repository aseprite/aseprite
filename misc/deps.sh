#! /bin/sh

GCC="gcc -MM"
CFLAGS="-I.					\
	-Isrc					\
	-Ithird_party/lua/include		\
	-Ithird_party/intl			\
	-Ithird_party/libpng			\
	-Ithird_party/zlib			\
	-Ithird_party/jpeg			\
	-Ithird_party/tinyxml			\
	-Ithird_party/loadpng			\
	-Ithird_party/freetype/include		\
	-Ithird_party/vaca/include		\
	-Ithird_party"

rm -f makefile.dep

$GCC $CFLAGS							     \
    src/*.cpp							     \
    src/commands/*.cpp						     \
    src/commands/fx/*.cpp					     \
    src/core/*.cpp						     \
    src/dialogs/*.cpp						     \
    src/effect/*.cpp						     \
    src/file/*.cpp						     \
    src/file/*/*.cpp						     \
    src/intl/*.cpp						     \
    src/jinete/*.cpp						     \
    src/modules/*.cpp						     \
    src/raster/*.cpp						     \
    src/settings/*.cpp						     \
    src/tools/*.cpp						     \
    src/util/*.cpp						     \
    src/widgets/*.cpp						     \
    src/widgets/editor/*.cpp					     \
    | sed -e 's/^\([a-z_\-]*\)\.o/\$(OBJ_DIR)\/\1\$(OBJ)/' >> makefile.dep
