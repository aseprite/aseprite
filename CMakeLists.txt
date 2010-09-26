# ASE - Allegro Sprite Editor
# Copyright (C) 2001-2010  David Capello

cmake_minimum_required(VERSION 2.6)

######################################################################
# Global project name

project(aseprite)

######################################################################
# Options (these can be specified in cmake command line or modifying CMakeCache.txt)

option(USE_STATIC_LIBC "Use static version of C and C++ runtimes" off)
option(USE_STATIC_ALLEGRO "Use static version of Allegro library" off)

######################################################################
# Directories of third-party libraries

set(LIBFREETYPE_DIR	${CMAKE_SOURCE_DIR}/third_party/freetype)
set(LIBJPEG_DIR		${CMAKE_SOURCE_DIR}/third_party/jpeg)
set(LIBPNG_DIR		${CMAKE_SOURCE_DIR}/third_party/libpng)
set(LOADPNG_DIR		${CMAKE_SOURCE_DIR}/third_party/loadpng)
set(TINYXML_DIR		${CMAKE_SOURCE_DIR}/third_party/tinyxml)
set(VACA_DIR		${CMAKE_SOURCE_DIR}/third_party/vaca)
set(ZLIB_DIR		${CMAKE_SOURCE_DIR}/third_party/zlib)

######################################################################
# Common definitions to compile all sources

# Vaca is compiled with Allegro
add_definitions(-DVACA_ALLEGRO)

# Do not use MMX optimizations in PNG code
add_definitions(-DPNG_NO_MMX_CODE)

# Debug C/C++ flags
if(CMAKE_BUILD_TYPE STREQUAL Debug)
  add_definitions(-DDEBUGMODE -D_DEBUG)
else()
  add_definitions(-DNDEBUG)
endif()

# Use static Allegro library
if(USE_STATIC_ALLEGRO)
  add_definitions(-DALLEGRO_STATICLINK)
endif()

######################################################################
# Main ASE targets

add_subdirectory(src)

######################################################################
# Third party libraries

add_subdirectory(third_party)
