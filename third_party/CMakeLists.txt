# ASEPRITE
# Copyright (C) 2001-2012  David Capello

include_directories(.)

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

if(NOT USE_SHARED_JPEGLIB)
  add_subdirectory(jpeg)
endif()

if(NOT USE_SHARED_ZLIB)
  include_directories(zlib)
  add_subdirectory(zlib)
endif()

if(NOT USE_SHARED_LIBPNG)
  include_directories(libpng)
  add_subdirectory(libpng)
endif()

if(ENABLE_UPDATER)
  add_subdirectory(curl)
endif()
add_subdirectory(giflib)
add_subdirectory(gtest)
add_subdirectory(freetype)
add_subdirectory(libart_lgpl)
add_subdirectory(loadpng)
add_subdirectory(tinyxml)
