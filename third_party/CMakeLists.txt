# ASEPRITE
# Copyright (C) 2001-2013  David Capello


if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

if(NOT USE_SHARED_JPEGLIB)
  include_directories(jpeg)
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

if(NOT USE_SHARED_GIFLIB)
  include_directories(giflib)
  add_subdirectory(giflib)
endif()

if(NOT USE_SHARED_TINYXML)
  include_directories(tinyxml)
  add_subdirectory(tinyxml)
endif()

if(NOT USE_SHARED_GTEST)
  include_directories(gtest)
  add_subdirectory(gtest)
endif()

if(ENABLE_UPDATER AND NOT USE_SHARED_CURL)
  include_directories(curl)
  add_subdirectory(curl)
endif()

if(NOT USE_SHARED_LIBLOADPNG)
  include_directories(loadpng)
 add_subdirectory(loadpng)
endif()

if(ENABLE_WEBSERVER)
  include_directories(mongoose)
  add_subdirectory(mongoose)
endif()

if(NOT USE_SHARED_PIXMAN)
  include_directories(pixman-cmake)
  add_subdirectory(pixman-cmake)
endif()

add_subdirectory(simpleini)
include_directories(simpleini)
