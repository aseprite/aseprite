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
  include_directories(zlib)
  add_subdirectory(zlib)
endif()

if(NOT USE_SHARED_LIBPNG)
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
  add_subdirectory(freetype2)
endif()

add_subdirectory(simpleini)
add_subdirectory(modp_b64)
