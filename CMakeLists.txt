# ASE - Allegro Sprite Editor
# Copyright (C) 2001-2010  David Capello

cmake_minimum_required(VERSION 2.6)

# Global project name
project(aseprite)

# Directories of third-party libraries
set(LIBFREETYPE_DIR	${CMAKE_SOURCE_DIR}/third_party/freetype)
set(LIBJPEG_DIR		${CMAKE_SOURCE_DIR}/third_party/jpeg)
set(LIBPNG_DIR		${CMAKE_SOURCE_DIR}/third_party/libpng)
set(LOADPNG_DIR		${CMAKE_SOURCE_DIR}/third_party/loadpng)
set(TINYXML_DIR		${CMAKE_SOURCE_DIR}/third_party/tinyxml)
set(VACA_DIR		${CMAKE_SOURCE_DIR}/third_party/vaca)
set(ZLIB_DIR		${CMAKE_SOURCE_DIR}/third_party/zlib)

# aseprite program
add_subdirectory(src)

# Third party libraries
add_subdirectory(third_party)
