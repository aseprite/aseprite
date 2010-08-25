# ASE - Allegro Sprite Editor
# Copyright (C) 2001-2010  David Capello

include_directories(. libpng zlib)

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

add_subdirectory(freetype)
add_subdirectory(jpeg)
add_subdirectory(libart_lgpl)
add_subdirectory(libpng)
add_subdirectory(loadpng)
add_subdirectory(tinyxml)
add_subdirectory(vaca)
add_subdirectory(zlib)
add_subdirectory(gtest)
