# ASE - Allegro Sprite Editor
# Copyright (C) 2001-2010  David Capello

add_library(loadpng
  loadpng.c
  savepng.c
  regpng.c)

set_target_properties(loadpng PROPERTIES
  COMPILE_FLAGS "-DDEBUGMODE -DALLEGRO_STATICLINK")
