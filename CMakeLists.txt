# Aseprite
# Copyright (C) 2001-2015  David Capello
#
# Parts of this file come from the Allegro 4.4 CMakeLists.txt

# CMake setup
cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
enable_testing()

if(COMMAND cmake_policy)
  # CMP0003: Libraries linked via full path no longer produce linker search paths.
  #cmake_policy(SET CMP0003 NEW)
  if(CMAKE_MAJOR_VERSION GREATER 2)
    # CMP0046: Old behavior to silently ignore non-existent dependencies.
    cmake_policy(SET CMP0046 OLD)
  endif(CMAKE_MAJOR_VERSION GREATER 2)
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

# Aseprite project
project(aseprite C CXX)

######################################################################
# Options (these can be specified in cmake command line or modifying
# CMakeCache.txt)

option(WITH_WEBP_SUPPORT  "Enable support to load/save .webp files" off)

option(USE_STATIC_LIBC    "Use static version of C and C++ runtimes" off)
option(USE_SHARED_CURL    "Use your installed copy of curl" off)
option(USE_SHARED_GIFLIB  "Use your installed copy of giflib" off)
option(USE_SHARED_JPEGLIB "Use your installed copy of jpeglib" off)
option(USE_SHARED_ZLIB    "Use your installed copy of zlib" off)
option(USE_SHARED_LIBPNG  "Use your installed copy of libpng" off)
option(USE_SHARED_LIBLOADPNG "Use your installed copy of libloadpng" off)
option(USE_SHARED_LIBWEBP "Use your installed copy of libwebp" off)
option(USE_SHARED_TINYXML "Use your installed copy of tinyxml" off)
option(USE_SHARED_PIXMAN  "Use your installed copy of pixman" off)
option(USE_SHARED_FREETYPE "Use shared FreeType library" off)
option(USE_SHARED_ALLEGRO4 "Use shared Allegro 4 library (without resize support)" off)
option(USE_ALLEG4_BACKEND "Use Allegro 4 backend" on)
option(USE_SKIA_BACKEND   "Use Skia backend" off)
option(ENABLE_MEMLEAK     "Enable memory-leaks detector (only for developers)" off)
option(ENABLE_UPDATER     "Enable automatic check for updates" on)
option(ENABLE_WEBSERVER   "Enable support to run a webserver (for HTML5 gamedev)" off)
option(ENABLE_TRIAL_MODE  "Compile the trial version" off)
option(FULLSCREEN_PLATFORM "Enable fullscreen by default" off)
set(CUSTOM_WEBSITE_URL "" CACHE STRING "Enable custom local webserver to check updates")

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
    set(CMAKE_C_FLAGS_PROFILE "/MD /Zi /Ox /Gd"
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

set(CURL_DIR            ${CMAKE_SOURCE_DIR}/third_party/curl)
set(GIFLIB_DIR          ${CMAKE_SOURCE_DIR}/third_party/giflib)
set(LIBJPEG_DIR         ${CMAKE_SOURCE_DIR}/third_party/jpeg)
set(LIBPNG_DIR          ${CMAKE_SOURCE_DIR}/third_party/libpng)
set(LOADPNG_DIR         ${CMAKE_SOURCE_DIR}/third_party/loadpng)
set(LIBWEBP_DIR         ${CMAKE_SOURCE_DIR}/third_party/libwebp)
set(MONGOOSE_DIR        ${CMAKE_SOURCE_DIR}/third_party/mongoose)
set(PIXMAN_DIR          ${CMAKE_SOURCE_DIR}/third_party/pixman)
set(FREETYPE_DIR        ${CMAKE_SOURCE_DIR}/third_party/freetype2)
set(SIMPLEINI_DIR       ${CMAKE_SOURCE_DIR}/third_party/simpleini)
set(TINYXML_DIR         ${CMAKE_SOURCE_DIR}/third_party/tinyxml)
set(ZLIB_DIR            ${CMAKE_SOURCE_DIR}/third_party/zlib)
set(DUKTAPE_DIR         ${CMAKE_SOURCE_DIR}/third_party/duktape)

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

# Fix to compile gtest with VC11 (2012)
if(MSVC_VERSION EQUAL 1700)
  add_definitions(-D_VARIADIC_MAX=10)
endif()

if(NOT WIN32 AND NOT APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu89")
endif()

if(APPLE AND USE_SKIA_BACKEND)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
endif()

if(NOT USE_SHARED_CURL)
  set(CURL_STATICLIB ON BOOL)
endif()

# zlib
if(USE_SHARED_ZLIB)
  find_package(ZLIB)
  if(NOT ZLIB_FOUND)
    message(FATAL_ERROR "zlib not found")
  endif()
else()
  set(ZLIB_LIBRARIES zlib)
  set(ZLIB_INCLUDE_DIR ${ZLIB_DIR})
  # Zlib generated zconf.h file
  include_directories(${CMAKE_BINARY_DIR}/third_party/zlib)
endif()
include_directories(${ZLIB_INCLUDE_DIR})

# libpng
if(USE_SHARED_LIBPNG)
  find_package(PNG)
  if(NOT PNG_FOUND)
    message(FATAL_ERROR "libpng not found")
  endif()
  add_definitions(${PNG_DEFINITIONS})
else()
  set(PNG_LIBRARIES libpng)
  set(PNG_INCLUDE_DIR ${LIBPNG_DIR})
endif()
include_directories(${PNG_INCLUDE_DIR})
add_definitions(-DPNG_NO_MMX_CODE) # Do not use MMX optimizations in PNG code

# libwebp
if(WITH_WEBP_SUPPORT)
  if(USE_SHARED_LIBWEBP)
    find_package(PkgConfig)
    pkg_check_modules(WEBP libwebp)
    if(NOT WEBP_FOUND)
      message(FATAL_ERROR "libwebp not found")
    endif()
  else()
    set(WEBP_LIBRARIES webp)
    set(WEBP_INCLUDE_DIR ${LIBWEBP_DIR}/src)
  endif()
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
  set(PIXMAN_INCLUDE_DIR ${PIXMAN_DIR}/pixman)
  # For pixman-version.h
  include_directories(${CMAKE_BINARY_DIR})
endif()
include_directories(${PIXMAN_INCLUDE_DIR})

# freetype
if(USE_SHARED_FREETYPE)
  find_library(FREETYPE_LIBRARY NAMES freetype)
  find_path(FREETYPE_INCLUDE_DIR NAMES freetype.h)
else()
  set(FREETYPE_LIBRARY freetype)
  set(FREETYPE_INCLUDE_DIR ${FREETYPE_DIR}/include)
endif()
include_directories(${FREETYPE_INCLUDE_DIR})

if(USE_SHARED_GIFLIB)
  find_package(GIF REQUIRED)
  if(NOT GIF_FOUND)
    message(FATAL_ERROR "giflib not found")
  endif()
else()
  set(GIF_LIBRARIES giflib)
  set(GIF_INCLUDE_DIR ${GIFLIB_DIR}/lib)
endif()
include_directories(${GIF_INCLUDE_DIR})

if(USE_SHARED_JPEGLIB)
  find_package(JPEG)
  if(NOT JPEG_FOUND)
    message(FATAL_ERROR "jpeg not found")
  endif()
else()
  set(JPEG_LIBRARIES jpeg)
  set(JPEG_INCLUDE_DIR ${LIBJPEG_DIR})
endif()
include_directories(${JPEG_INCLUDE_DIR})

if(USE_ALLEG4_BACKEND)
  if(USE_SHARED_LIBLOADPNG)
    find_library(LOADPNG_LIBRARY NAMES loadpng)
    find_path(LOADPNG_INCLUDE_DIR NAMES loadpng.h)
  else()
    set(LOADPNG_LIBRARY loadpng)
    set(LOADPNG_INCLUDE_DIR ${LOADPNG_DIR})
  endif()
  include_directories(${LOADPNG_INCLUDE_DIR})
endif()

if(USE_SHARED_CURL)
  find_library(LIBCURL_LIBRARY NAMES curl)
  find_path(LIBCURL_INCLUDE_DIR NAMES curl/curl.h)
else()
  set(LIBCURL_LIBRARY libcurl)
  set(LIBCURL_INCLUDE_DIR ${CURL_DIR}/include)
endif()
include_directories(${LIBCURL_INCLUDE_DIR})

# mongoose
if(ENABLE_WEBSERVER)
  set(MONGOOSE_LIBRARY mongoose)
  include_directories(${MONGOOSE_DIR})
  add_definitions(-DENABLE_WEBSERVER)
endif()

# simpleini
set(SIMPLEINI_LIBRARY simpleini)
include_directories(${SIMPLEINI_DIR})

######################################################################
# Platform specific stuff

set(PLATFORM_LIBS)

# Allegro 4 backend
if(USE_ALLEG4_BACKEND)
  add_definitions(-DUSE_ALLEG4_BACKEND)

  if(USE_SHARED_ALLEGRO4)
    # Find the shared Allegro 4 library
    find_library(LIBALLEGRO4_LIBRARY alleg)
    find_path(LIBALLEGRO4_INCLUDE_DIR allegro.h)

    if(NOT LIBALLEGRO4_LIBRARY)
      message(FATAL_ERROR "Allegro 4 not found")
    endif()

    # Get flags to link programs using allegro-config program
    execute_process(COMMAND allegro-config --libs --shared
      OUTPUT_VARIABLE LIBALLEGRO4_LINK_FLAGS
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    set(LIBALLEGRO4_LINK_FLAGS ${LIBALLEGRO4_LINK_FLAGS} ${PLATFORM_LIBS})
    include_directories(${LIBALLEGRO4_INCLUDE_DIR})
  else()
    include_directories(${CMAKE_SOURCE_DIR}/src/allegro/include)
    include_directories(${CMAKE_BINARY_DIR}/include)

    add_definitions(-DALLEGRO4_WITH_RESIZE_PATCH)
    add_definitions(-DALLEGRO4_WITH_EXTRA_CURSORS)

    # Static Allegro (the code of Allegro library is embedded).
    add_definitions(-DALLEGRO_STATICLINK)

    set(LIBALLEGRO4_LINK_FLAGS allegro)
  endif()
endif()

# Skia backend
if(USE_SKIA_BACKEND)
  add_definitions(-DUSE_SKIA_BACKEND)
endif()

# -- Unix --

if(UNIX AND NOT APPLE AND NOT BEOS)
  # Pthreads
  find_package(Threads)
  if(NOT CMAKE_USE_PTHREADS_INIT)
    message(FATAL_ERROR "Unix port requires pthreads support.")
  endif()
  list(APPEND PLATFORM_LIBS m ${CMAKE_THREAD_LIBS_INIT})

  # X11
  find_package(X11)
  if(NOT X11_FOUND)
    message(FATAL_ERROR "Unix port requires X11 (e.g. libx11-dev).")
  endif()
  include_directories(SYSTEM ${X11_INCLUDE_DIR})
  list(APPEND PLATFORM_LIBS ${X11_LIBRARIES})

  if(X11_XShm_FOUND)
    list(APPEND PLATFORM_LIBS ${X11_Xext_LIB})
  endif()

  if(X11_Xcursor_FOUND)
    list(APPEND PLATFORM_LIBS ${X11_Xcursor_LIB})
  endif()

  if(X11_Xpm_FOUND)
    list(APPEND PLATFORM_LIBS ${X11_Xpm_LIB})
  endif()

  find_library(X11_Xxf86vm_LIB Xxf86vm ${X11_LIB_SEARCH_PATH})
  mark_as_advanced(X11_Xxf86vm_LIB)
  if(X11_xf86vmode_FOUND)
    list(APPEND PLATFORM_LIBS ${X11_Xxf86vm_LIB})
  endif()

  check_library_exists(X11 XOpenIM "${X11_LIB_SEARCH_PATH}" XIM_FOUND)
  check_library_exists(Xxf86dga XDGAQueryExtension
    "${X11_LIB_SEARCH_PATH}" XDGA_FOUND)

  if(XDGA_FOUND)
    list(APPEND PLATFORM_LIBS Xxf86dga ${X11_LIBRARIES})
  endif()
endif()

# -- Windows --

if(WIN32)
  list(APPEND PLATFORM_LIBS
    kernel32 user32 gdi32 comdlg32 ole32 winmm
    shlwapi psapi wininet comctl32 dbghelp)

  # Windows XP is the minimum supported platform.
  add_definitions(-D_WIN32_WINNT=0x0501 -DWINVER=0x0501)

  # We need Unicode support
  add_definitions(-DUNICODE -D_UNICODE)
endif(WIN32)

# -- Mac OS X --

if(APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(CARBON_LIBRARY Carbon)
  find_library(IOKIT_LIBRARY IOKit)
  mark_as_advanced(COCOA_LIBRARY CARBON_LIBRARY IOKIT_LIBRARY)

  list(APPEND PLATFORM_LIBS
    ${COCOA_LIBRARY}
    ${CARBON_LIBRARY}
    ${IOKIT_LIBRARY})

    # Hack to deal with Mac OS X 10.6.  NSQuickDrawView is not defined by
    # NSQuickDrawView.h when compiling in 64-bit mode, and 64-bit mode is the
    # default when compiling on Snow Leopard.
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL i386)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch i386")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch i386")
    endif()

    # The Mac port simply uses too many deprecated things.
    if(COMPILER_GCC)
        set(WFLAGS "${WFLAGS} -Wno-deprecated-declarations")
    endif(COMPILER_GCC)
endif(APPLE)

######################################################################
# Main ASE targets

add_subdirectory(src)

######################################################################
# Third party libraries

add_subdirectory(third_party)
