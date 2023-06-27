# Aseprite
# Copyright (C) 2018-2022  Igara Studio S.A.
# Copyright (C) 2001-2018  David Capello

cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # We use -std=c++17 instead of -std=gnu++17 in macOS
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(COMMAND cmake_policy)
  # CMP0003: Libraries linked via full path no longer produce linker search paths.
  #cmake_policy(SET CMP0003 NEW)
  if(CMAKE_MAJOR_VERSION GREATER 2)
    # CMP0046: Old behavior to silently ignore non-existent dependencies.
    cmake_policy(SET CMP0046 OLD)
  endif()
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are:
        None Debug Release RelWithDebInfo Profile."
      FORCE)
endif()

set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_SOURCE_DIR}/laf/cmake/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/laf/cmake/cxx_flag_overrides.cmake)

# Aseprite project
project(aseprite C CXX)

# Check repository status
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/laf/CMakeLists.txt)
  message(FATAL_ERROR "Your Aseprite repository is incomplete, initialize submodules using:\n  git submodule update --init --recursive")
endif()

# This is required for KDE/Qt destop integration, which sets
# BUILD_SHARED_LIBS to TRUE by default
set(BUILD_SHARED_LIBS off)

enable_testing()

######################################################################
# Options (these can be specified in cmake command line or modifying
# CMakeCache.txt)

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
option(ENABLE_WEBSOCKET    "Compile with websocket support" on)
option(ENABLE_TESTS        "Compile unit tests" off)
option(ENABLE_BENCHMARKS   "Compile benchmarks" off)
option(ENABLE_TRIAL_MODE   "Compile the trial version" off)
option(ENABLE_DRM          "Compile the DRM-enabled version (e.g. for automatic updates)" off)
option(ENABLE_STEAM        "Compile with Steam library" off)
option(ENABLE_DEVMODE      "Compile vesion for developers" off)
option(ENABLE_UI           "Compile UI (turn off to compile CLI-only version)" on)
option(FULLSCREEN_PLATFORM "Enable fullscreen by default" off)
option(ENABLE_CLANG_TIDY   "Enable static analysis" off)
option(ENABLE_CCACHE       "Use CCache to improve recompilation speed (optional)" on)
option(ENABLE_SENTRY       "Use Sentry SDK to report crashes" off)
option(ENABLE_WEBP         "Enable support to load/save .webp files" on)
option(ENABLE_PSD          "Enable experimental support for .psd files" off)
option(ENABLE_DESKTOP_INTEGRATION  "Enable desktop integration modules" off)
option(ENABLE_QT_THUMBNAILER       "Enable kde5/qt5 thumnailer" off)
set(CUSTOM_WEBSITE_URL "" CACHE STRING "Enable custom local webserver to check updates")

if(ENABLE_SENTRY)
  set(SENTRY_DIR "" CACHE STRING "Sentry native location")
  set(SENTRY_DSN "" CACHE STRING "Sentry Data Source Name URL")
  set(SENTRY_ENV "development" CACHE STRING "Sentry environment (production, testing, development, etc.)")
endif()

if(ENABLE_NEWS OR ENABLE_UPDATER OR ENABLE_DRM)
  set(REQUIRE_CURL ON)
else()
  set(REQUIRE_CURL OFF)
endif()

# Select libcurl's TLS backend according to target platform
if(REQUIRE_CURL AND ENABLE_DRM)
  if(APPLE)
    option(CMAKE_USE_SECTRANSP "enable Apple OS native SSL/TLS" ON)
  endif()
  if(WIN32)
    set(CMAKE_USE_SCHANNEL "enable Windows native SSL/TLS" ON)
  endif()
endif()

if(NOT LAF_OS_BACKEND)
  if(NOT ENABLE_UI)
    set(LAF_OS_BACKEND "none") # Without UI, we use the none backend
  else()
    set(LAF_OS_BACKEND "skia")
  endif()
endif()

if(ENABLE_DRM AND NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/drm/CMakeLists.txt)
  message(FATAL_ERROR "Your Aseprite repository is incomplete, clone the drm repository")
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

if(ENABLE_CCACHE)
  find_package(CCache)
  if(CCache_FOUND)
    # Use e.g. "ccache clang++" instead of "clang++"
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCache_EXECUTABLE}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK    "${CCache_EXECUTABLE}")
  endif()
endif()
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
elseif(NOT LAF_BACKEND STREQUAL "skia")
  set(FREETYPE_FOUND ON)
  set(FREETYPE_LIBRARY freetype)
  set(FREETYPE_LIBRARIES ${FREETYPE_LIBRARY})
  set(FREETYPE_INCLUDE_DIRS ${FREETYPE_DIR}/include)
endif()
include_directories(${FREETYPE_INCLUDE_DIRS})

# harfbuzz
if(USE_SHARED_HARFBUZZ)
  find_package(HarfBuzz)
elseif(NOT LAF_BACKEND STREQUAL "skia")
  set(HARFBUZZ_LIBRARIES harfbuzz)
  set(HARFBUZZ_INCLUDE_DIRS ${HARFBUZZ_DIR}/src)
endif()
include_directories(${HARFBUZZ_INCLUDE_DIRS})

if(USE_SHARED_GIFLIB)
  find_package(GIF REQUIRED)
else()
  set(GIF_LIBRARY giflib)
  set(GIF_LIBRARIES ${GIF_LIBRARY})
  set(GIF_INCLUDE_DIR ${GIFLIB_DIR})
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
  set(CMARK_LIBRARIES cmark_static)
endif()

if(REQUIRE_CURL)
  if(USE_SHARED_CURL)
    find_package(CURL REQUIRED)
  else()
    set(CURL_FOUND 1)
    set(CURL_LIBRARY libcurl)
    set(CURL_LIBRARIES libcurl)
    set(CURL_INCLUDE_DIRS ${CURL_DIR}/include)
  endif()
  include_directories(${CURL_INCLUDE_DIRS})
endif()

# simpleini
include_directories(${SIMPLEINI_DIR})

# Third parties
add_subdirectory(third_party)

if(ENABLE_MEMLEAK)
  add_definitions(-DLAF_MEMLEAK)
endif()

set(LAF_WITH_TESTS ${ENABLE_TESTS} CACHE BOOL "Enable LAF tests")
set(UNDO_TESTS     ${ENABLE_TESTS} CACHE BOOL "Enable undo tests")

if(ENABLE_DRM)
  add_subdirectory(drm)
endif()
add_subdirectory(laf)

# libwebp
if(ENABLE_WEBP)
  # Use libwebp from Skia
  if(LAF_BACKEND STREQUAL "skia")
    find_library(WEBP_LIBRARIES webp
      NAMES libwebp # required for Windows
      PATHS "${SKIA_LIBRARY_DIR}" NO_DEFAULT_PATH)
    set(WEBP_INCLUDE_DIR "${SKIA_DIR}/third_party/externals/libwebp/src")
  else()
    set(WEBP_LIBRARIES webp webpdemux libwebpmux)
    set(WEBP_INCLUDE_DIR ${LIBWEBP_DIR}/src)
  endif()
  include_directories(${WEBP_INCLUDE_DIR})
endif()
message(STATUS "aseprite libwebp: ${WEBP_LIBRARIES}")

# LAF libraries + Aseprite are compiled with config.h
target_include_directories(laf-base PUBLIC src)
target_compile_definitions(laf-base PUBLIC HAVE_CONFIG_H)

add_subdirectory(src)

######################################################################
# Using clang-tidy with cmake.
# Based on http://mariobadr.com/using-clang-tidy-with-cmake-36.html

if(ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE
    NAMES "clang-tidy"
    DOC "Path to clang-tidy executable")
  if(NOT CLANG_TIDY_EXE)
    message(STATUS "clang-tidy not found.")
  else()
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}")
  endif()

  if(CLANG_TIDY_EXE)
    set_target_properties(app-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(aseprite PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(cfg-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(clip PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(dio-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(doc-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(filters-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(fixmath-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(flic-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(gen PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(laf-base PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(laf-ft PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(laf-gfx PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(laf-os PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(net-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(obs PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(render-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(ui-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(undo PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(updater-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
  endif()
endif()
