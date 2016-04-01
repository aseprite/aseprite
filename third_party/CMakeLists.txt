# ASEPRITE
# Copyright (C) 2001-2016  David Capello

include_directories(.)

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

if(NOT USE_SHARED_JPEGLIB)
  add_subdirectory(jpeg)
endif()

if(NOT USE_SHARED_ZLIB)
  set(SKIP_INSTALL_ALL on)
  set(ZLIB_LIBRARY zlibstatic)
  # Don't build zlib tests
  set(ZLIB_TESTS OFF CACHE BOOL "Build zlib tests")
  add_subdirectory(zlib)
endif()

if(NOT USE_SHARED_LIBPNG)
  set(SKIP_INSTALL_ALL ON)
  # We only need the static version of libpng
  set(PNG_SHARED OFF CACHE BOOL "Build shared lib")
  set(PNG_STATIC ON CACHE BOOL "Build static lib")
  set(PNG_TESTS OFF CACHE BOOL "Build libpng tests")
  set(ZLIB_INCLUDE_DIR "${ZLIB_INCLUDE_DIRS}")
  add_subdirectory(libpng)
endif()

if(WITH_WEBP_SUPPORT)
  if(NOT USE_SHARED_LIBWEBP)
    add_subdirectory(libwebp-cmake)
  endif()
endif()

if(NOT USE_SHARED_GIFLIB)
  add_subdirectory(giflib)
endif()

if(NOT USE_SHARED_TINYXML)
  add_subdirectory(tinyxml)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/gtest/CMakeLists.txt)
  # We don't use add_subdirectory(gtest) because the gtest
  # CMakeLists.txt modifies some MSVC flags.
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/gtest ${CMAKE_CURRENT_SOURCE_DIR}/gtest/include)
  add_library(gtest ${CMAKE_CURRENT_SOURCE_DIR}/gtest/src/gtest-all.cc)
else()
  message(FATAL_ERROR "gtest/gtest.h is missing. Initialize submodules: git submodule update --init --recursive")
endif()

if(NOT USE_SHARED_CURL)
  set(BUILD_RELEASE_DEBUG_DIRS ON BOOL)
  add_subdirectory(curl)
endif()

if(USE_ALLEG4_BACKEND AND NOT USE_SHARED_LIBLOADPNG)
  add_subdirectory(loadpng)
endif()

if(NOT USE_SHARED_PIXMAN)
  add_subdirectory(pixman-cmake)
endif()

if(NOT USE_SHARED_FREETYPE)
  set(SKIP_INSTALL_ALL on)

  # If we don't do this, the compilation fails because it looks like
  # ftgzip.c includes our zlib.h, but uses the zconf.h in the
  # freetype2 library.
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/freetype2/src/gzip/zlib.h)
    file(RENAME
      ${CMAKE_CURRENT_SOURCE_DIR}/freetype2/src/gzip/zlib.h
      ${CMAKE_CURRENT_SOURCE_DIR}/freetype2/src/gzip/zlib.h.included)
  endif()

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/freetype2/src/gzip/zconf.h)
    file(RENAME
      ${CMAKE_CURRENT_SOURCE_DIR}/freetype2/src/gzip/zconf.h
      ${CMAKE_CURRENT_SOURCE_DIR}/freetype2/src/gzip/zconf.h.included)
  endif()

  add_subdirectory(freetype2)
endif()

add_subdirectory(simpleini)
add_subdirectory(modp_b64)
