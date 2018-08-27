# Aseprite
# Copyright (C) 2001-2018  David Capello

cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # We use -std=c++11 instead of -std=gnu++11 in macOS

if(COMMAND cmake_policy)
  # CMP0003: Libraries linked via full path no longer produce linker search paths.
  #cmake_policy(SET CMP0003 NEW)
  if(CMAKE_MAJOR_VERSION GREATER 2)
    # CMP0046: Old behavior to silently ignore non-existent dependencies.
    cmake_policy(SET CMP0046 OLD)
  endif()
endif(COMMAND cmake_policy)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are:
        None Debug Release RelWithDebInfo Profile."
      FORCE)
endif()

# Restrict configuration types to the selected build type.
# Note: This needs to be done before the project command
set(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}" CACHE INTERNAL "internal")

set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cxx_flag_overrides.cmake)

# Aseprite project
project(aseprite C CXX)

if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  # As we compile with CMAKE_OSX_DEPLOYMENT_TARGET=10.7, we have to
  # explicitly say that we want to use libc++ instead of the GNU libstdc++
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

# Check repository status
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/laf/CMakeLists.txt)
  message(FATAL_ERROR "Your Aseprite repository is incomplete, initialize submodules using:\n  git submodule update --init --recursive")
endif()

# This required for KDE/Qt destop integration, which sets BUILD_SHARED_LIBS to
# TRUE by default
set(BUILD_SHARED_LIBS off)

enable_testing()

######################################################################
# Options (these can be specified in cmake command line or modifying
# CMakeCache.txt)

option(WITH_WEBP_SUPPORT            "Enable support to load/save .webp files" on)
option(WITH_DESKTOP_INTEGRATION     "Enable desktop integration modules" off)
option(WITH_QT_THUMBNAILER          "Enable kde5/qt5 thumnailer" off)

option(USE_SHARED_CMARK   "Use your installed copy of cmark" off)
option(USE_SHARED_CURL    "Use your installed copy of curl" off)
option(USE_SHARED_GIFLIB  "Use your installed copy of giflib" off)
option(USE_SHARED_JPEGLIB "Use your installed copy of jpeglib" off)
option(USE_SHARED_ZLIB    "Use your installed copy of zlib" off)
option(USE_SHARED_LIBPNG  "Use your installed copy of libpng" off)
option(USE_SHARED_TINYXML "Use your installed copy of tinyxml" off)
option(USE_SHARED_PIXMAN  "Use your installed copy of pixman" off)
option(USE_SHARED_FREETYPE "Use shared FreeType library" off)
option(USE_SHARED_HARFBUZZ "Use shared HarfBuzz library" off)
option(ENABLE_ASEPRITE_EXE "Compile main Aseprite executable" on)
option(ENABLE_MEMLEAK      "Enable memory-leaks detector (only for developers)" off)
option(ENABLE_NEWS         "Enable the news in Home tab" on)
option(ENABLE_UPDATER      "Enable automatic check for updates" on)
option(ENABLE_SCRIPTING    "Compile with scripting support" on)
option(ENABLE_TESTS        "Compile unit tests" off)
option(ENABLE_BENCHMARKS   "Compile benchmarks" off)
option(ENABLE_TRIAL_MODE   "Compile the trial version" off)
option(ENABLE_STEAM        "Compile with Steam library" off)
option(ENABLE_DEVMODE      "Compile vesion for developers" off)
option(ENABLE_UI           "Compile UI (turn off to compile CLI-only version)" on)
option(FULLSCREEN_PLATFORM "Enable fullscreen by default" off)
set(CUSTOM_WEBSITE_URL "" CACHE STRING "Enable custom local webserver to check updates")

if(ENABLE_NEWS OR ENABLE_UPDATER)
  set(REQUIRE_CURL ON)
else()
  set(REQUIRE_CURL OFF)
endif()

if(NOT LAF_OS_BACKEND)
  if(NOT ENABLE_UI)
    set(LAF_OS_BACKEND "none") # Without UI, we use the none backend
  else()
    set(LAF_OS_BACKEND "skia")
  endif()
endif()

# Check valid gtk + libpng combination
if(LAF_OS_WITH_GTK)
  if(NOT USE_SHARED_LIBPNG)
    message(FATAL_ERROR "Cannot compile with gtk and static libpng, set USE_SHARED_LIBPNG=ON")
  endif()
  if(NOT USE_SHARED_HARFBUZZ)
    message(FATAL_ERROR "Cannot compile with gtk and static HarfBuzz, set USE_SHARED_HARFBUZZ=ON")
  endif()
endif()

######################################################################
# Profile build type

list(APPEND CMAKE_BUILD_TYPES Profile)
mark_as_advanced(
    CMAKE_C_FLAGS_PROFILE
    CMAKE_CXX_FLAGS_PROFILE
    CMAKE_EXE_LINKER_FLAGS_PROFILE)

if(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS_PROFILE "-pg"
        CACHE STRING "Profiling C flags")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE}"
        CACHE STRING "Profiling C++ flags")
    set(CMAKE_EXE_LINKER_FLAGS_PROFILE "-pg"
        CACHE STRING "Profiling linker flags")
endif()

if(MSVC)
    set(CMAKE_C_FLAGS_PROFILE "/MT /Zi /Ox /Gd"
        CACHE STRING "Profiling C flags")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE}"
        CACHE STRING "Profiling C++ flags")
    set(CMAKE_EXE_LINKER_FLAGS_PROFILE "/PROFILE /DEBUG"
        CACHE STRING "Profiling linker flags")
endif()

######################################################################
# Directories

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
# We need to specify the output for each configuration to make it work
# on Visual Studio solutions.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_PROFILE "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_PROFILE "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_PROFILE "${CMAKE_BINARY_DIR}/bin")

set(SOURCE_DATA_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/data)
set(CMARK_DIR           ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cmark)
set(CURL_DIR            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/curl)
set(GIFLIB_DIR          ${CMAKE_CURRENT_SOURCE_DIR}/third_party/giflib)
set(LIBJPEG_DIR         ${CMAKE_CURRENT_SOURCE_DIR}/third_party/jpeg)
set(LIBPNG_DIR          ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libpng)
set(LIBWEBP_DIR         ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libwebp)
set(PIXMAN_DIR          ${CMAKE_CURRENT_SOURCE_DIR}/third_party/pixman)
set(FREETYPE_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/freetype2)
set(HARFBUZZ_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/harfbuzz)
set(SIMPLEINI_DIR       ${CMAKE_CURRENT_SOURCE_DIR}/third_party/simpleini)
set(TINYXML_DIR         ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyxml)
set(ZLIB_DIR            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/zlib)

# Search in the "cmake" directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Put libraries into "lib".
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

######################################################################
# Common definitions to compile all sources (app code and third party)

# Debug C/C++ flags
if(CMAKE_BUILD_TYPE STREQUAL Debug)
  add_definitions(-DDEBUGMODE -D_DEBUG)
else()
  add_definitions(-DNDEBUG)
endif()

if(NOT USE_SHARED_CURL)
  set(CURL_STATICLIB ON BOOL)
endif()

# zlib
if(USE_SHARED_ZLIB)
  find_package(ZLIB REQUIRED)
else()
  set(ZLIB_FOUND)
  set(ZLIB_LIBRARY zlibstatic)
  set(ZLIB_LIBRARIES ${ZLIB_LIBRARY})
  set(ZLIB_INCLUDE_DIRS
    ${ZLIB_DIR}
    ${CMAKE_BINARY_DIR}/third_party/zlib) # Zlib generated zconf.h file
  set(ZLIB_INCLUDE_DIR ${ZLIB_INCLUDE_DIRS} CACHE PATH "")
endif()
include_directories(${ZLIB_INCLUDE_DIRS})

# libpng
if(USE_SHARED_LIBPNG)
  find_package(PNG REQUIRED)
  add_definitions(${PNG_DEFINITIONS})
else()
  set(PNG_FOUND ON)
  set(PNG_LIBRARY png_static)
  set(PNG_LIBRARIES ${PNG_LIBRARY})
  set(PNG_INCLUDE_DIRS
    ${LIBPNG_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/third_party/libpng) # Libpng generated pnglibconf.h file
  set(PNG_INCLUDE_DIR ${PNG_INCLUDE_DIRS} CACHE PATH "")
  set(PNG_PNG_INCLUDE_DIR ${PNG_INCLUDE_DIRS} CACHE PATH "")
endif()
include_directories(${PNG_INCLUDE_DIRS})
add_definitions(-DPNG_NO_MMX_CODE) # Do not use MMX optimizations in PNG code

# libwebp
if(WITH_WEBP_SUPPORT)
  set(WEBP_LIBRARIES webp webpdemux webpmux)
  set(WEBP_INCLUDE_DIR ${LIBWEBP_DIR}/src)
  include_directories(${WEBP_INCLUDE_DIR})
endif()

# tinyxml
if(USE_SHARED_TINYXML)
  find_library(TINYXML_LIBRARY NAMES tinyxml)
  find_path(TINYXML_INCLUDE_DIR NAMES tinyxml.h)
else()
  set(TINYXML_LIBRARY tinyxml)
  set(TINYXML_INCLUDE_DIR ${TINYXML_DIR})
endif()
include_directories(${TINYXML_INCLUDE_DIR})

# pixman
if(USE_SHARED_PIXMAN)
  find_library(PIXMAN_LIBRARY NAMES pixman pixman-1)
  find_path(PIXMAN_INCLUDE_DIR NAMES pixman.h PATH_SUFFIXES pixman-1)
else()
  set(PIXMAN_LIBRARY pixman)
  set(PIXMAN_INCLUDE_DIR
    ${PIXMAN_DIR}/pixman
    ${CMAKE_BINARY_DIR}) # For pixman-version.h
endif()
include_directories(${PIXMAN_INCLUDE_DIR})

# freetype
if(USE_SHARED_FREETYPE)
  find_package(Freetype REQUIRED)
else()
  set(FREETYPE_FOUND ON)
  set(FREETYPE_LIBRARY freetype)
  set(FREETYPE_LIBRARIES ${FREETYPE_LIBRARY})
  set(FREETYPE_INCLUDE_DIRS ${FREETYPE_DIR}/include)
endif()
include_directories(${FREETYPE_INCLUDE_DIRS})

# harfbuzz
if(USE_SHARED_HARFBUZZ)
  find_package(HarfBuzz)
else()
  set(HARFBUZZ_LIBRARIES harfbuzz)
  set(HARFBUZZ_INCLUDE_DIRS ${HARFBUZZ_DIR}/src)
endif()
include_directories(${HARFBUZZ_INCLUDE_DIRS})

if(USE_SHARED_GIFLIB)
  find_package(GIF REQUIRED)
else()
  set(GIF_LIBRARY giflib)
  set(GIF_LIBRARIES ${GIF_LIBRARY})
  set(GIF_INCLUDE_DIR ${GIFLIB_DIR}/lib)
  set(GIF_INCLUDE_DIRS ${GIF_INCLUDE_DIR})
endif()
include_directories(${GIF_INCLUDE_DIRS})

if(USE_SHARED_JPEGLIB)
  find_package(JPEG REQUIRED)
else()
  set(JPEG_FOUND ON)
  set(JPEG_INCLUDE_DIR ${LIBJPEG_DIR})
  set(JPEG_LIBRARY jpeg CACHE FILEPATH "")
  set(JPEG_LIBRARIES ${JPEG_LIBRARY})
  set(JPEG_INCLUDE_DIRS ${JPEG_INCLUDE_DIR})
endif()
include_directories(${JPEG_INCLUDE_DIRS})

if(USE_SHARED_CMARK)
  find_library(CMARK_LIBRARIES NAMES cmark)
  find_path(CMARK_INCLUDE_DIRS NAMES cmark.h)
else()
  add_definitions(-DCMARK_STATIC_DEFINE)
  set(CMARK_LIBRARIES libcmark_static)
  include_directories(${CMAKE_CURRENT_BINARY_DIR}/third_party/cmark)
endif()

if(REQUIRE_CURL)
  if(USE_SHARED_CURL)
    find_package(CURL REQUIRED)
  else()
    set(CURL_LIBRARIES libcurl)
    set(CURL_INCLUDE_DIRS ${CURL_DIR}/include)
  endif()
  include_directories(${CURL_INCLUDE_DIRS})
endif()

# simpleini
include_directories(${SIMPLEINI_DIR})

# Third parties
add_subdirectory(third_party)

# LAF libraries + Aseprite are compiled with config.h
include_directories(src)
add_definitions(-DHAVE_CONFIG_H)
if(ENABLE_MEMLEAK)
  add_definitions(-DLAF_MEMLEAK)
endif()

set(LAF_WITH_TESTS ${ENABLE_TESTS} CACHE BOOL "Enable LAF tests")
set(UNDO_TESTS     ${ENABLE_TESTS} CACHE BOOL "Enable undo tests")

add_subdirectory(laf)
add_subdirectory(src)
