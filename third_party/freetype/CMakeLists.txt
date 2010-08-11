# ASE - Allegro Sprite Editor
# Copyright (C) 2001-2010  David Capello

include_directories(include)

add_library(freetype
  src/base/ftsystem.c
  src/base/ftdebug.c
  src/base/ftinit.c
  src/base/ftbase.c
  src/base/ftglyph.c
  src/base/ftmm.c
  src/base/ftbbox.c
  src/autohint/autohint.c
  src/cache/ftcache.c
  src/cff/cff.c
  src/cid/type1cid.c
  src/pcf/pcf.c
  src/psaux/psaux.c
  src/pshinter/pshinter.c
  src/psnames/psmodule.c
  src/raster/raster.c
  src/sfnt/sfnt.c
  src/smooth/smooth.c
  src/truetype/truetype.c
  src/type1/type1.c
  src/winfonts/winfnt.c)
