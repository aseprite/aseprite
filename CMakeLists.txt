# Aseprite
# Copyright (C) 2001-2015  David Capello
#
# Parts of this file come from the Allegro 4.4 CMakeLists.txt

# CMake setup
cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

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

option(USE_STATIC_LIBC    "Use static version of C and C++ runtimes" off)
option(USE_SHARED_CURL    "Use your installed copy of curl" off)
option(USE_SHARED_GIFLIB  "Use your installed copy of giflib" off)
option(USE_SHARED_JPEGLIB "Use your installed copy of jpeglib" off)
option(USE_SHARED_ZLIB    "Use your installed copy of zlib" off)
option(USE_SHARED_LIBPNG  "Use your installed copy of libpng" off)
option(USE_SHARED_LIBLOADPNG "Use your installed copy of libloadpng" off)
option(USE_SHARED_TINYXML "Use your installed copy of tinyxml" off)
option(USE_SHARED_PIXMAN  "Use your installed copy of pixman" off)
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
    set(CMAKE_C_FLAGS_PROFILE "/MD /Zi /Ox /Gd /D NDEBUG"
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
set(LIBFREETYPE_DIR     ${CMAKE_SOURCE_DIR}/third_party/freetype)
set(LIBJPEG_DIR         ${CMAKE_SOURCE_DIR}/third_party/jpeg)
set(LIBPNG_DIR          ${CMAKE_SOURCE_DIR}/third_party/libpng)
set(LOADPNG_DIR         ${CMAKE_SOURCE_DIR}/third_party/loadpng)
set(MONGOOSE_DIR        ${CMAKE_SOURCE_DIR}/third_party/mongoose)
set(PIXMAN_DIR          ${CMAKE_SOURCE_DIR}/third_party/pixman)
set(SIMPLEINI_DIR       ${CMAKE_SOURCE_DIR}/third_party/simpleini)
set(TINYXML_DIR         ${CMAKE_SOURCE_DIR}/third_party/tinyxml)
set(ZLIB_DIR            ${CMAKE_SOURCE_DIR}/third_party/zlib)

# Zlib generated zconf.h file
include_directories(${CMAKE_BINARY_DIR}/third_party/zlib)

# We need private libpng header files so it doesn't matter if we use
# the shared version of libpng or not, we need to find header files in
# "third_party/libpng"" directory.
include_directories(${LIBPNG_DIR})

# Search in the "cmake" directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Put libraries into "lib".
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

######################################################################
# Common definitions to compile all sources

# Do not use MMX optimizations in PNG code
add_definitions(-DPNG_NO_MMX_CODE)

# Debug C/C++ flags
if(CMAKE_BUILD_TYPE STREQUAL Debug)
  add_definitions(-DDEBUGMODE -D_DEBUG)
else()
  add_definitions(-DNDEBUG)
endif()

# Fix to compile gtest with VC11 (2012)
if(NOT USE_SHARED_GTEST)
  if (MSVC_VERSION EQUAL 1700)
    add_definitions(-D_VARIADIC_MAX=10)
  endif()
endif()

if(NOT USE_SHARED_CURL)
  set(CURL_STATICLIB ON BOOL)
endif()

######################################################################
# Allegro 4 backend

if(USE_ALLEG4_BACKEND)
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
  endif()
endif()

######################################################################
# Platform specific stuff

set(PLATFORM_LIBS)

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
  if(USE_ALLEG4_BACKEND)
    find_package(DXGuid)

    if(NOT DXGUID_FOUND)
      if(MSVC)
        message(FATAL_ERROR "DirectX required for Windows port. You might need to add DirectX include and lib directories to your INCLUDE and LIB environment variables.")
      else()
        message(FATAL_ERROR "DirectX required for Windows port.")
      endif()
    endif()

    include_directories(SYSTEM ${DXGUID_INCLUDE_DIR})
  else()
    set(DXGUID_LIBRARIES)
  endif()

  list(APPEND PLATFORM_LIBS
    kernel32 user32 gdi32 comdlg32 ole32 winmm
    shlwapi psapi wininet comctl32 dbghelp
    ${DXGUID_LIBRARIES})

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
