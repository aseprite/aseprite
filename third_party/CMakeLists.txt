# ASEPRITE
# Copyright (C) 2001-2013  David Capello

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
  add_subdirectory(libpng)
endif()

if(NOT USE_SHARED_GIFLIB)
  add_subdirectory(giflib)
endif()

if(NOT USE_SHARED_TINYXML)
  add_subdirectory(tinyxml)
endif()

if(NOT USE_SHARED_GTEST)
  add_subdirectory(gtest)
endif()

if(ENABLE_UPDATER AND NOT USE_SHARED_CURL)
  add_subdirectory(curl)
endif()

if(NOT USE_SHARED_LIBLOADPNG)
 add_subdirectory(loadpng)
endif()

if(ENABLE_WEBSERVER)
  add_subdirectory(mongoose)
endif()

if(NOT USE_SHARED_PIXMAN)
  add_subdirectory(pixman-cmake)
endif()
