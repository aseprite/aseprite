# ASE - Allegro Sprite Editor
# Copyright (C) 2001-2012  David Capello
#
# Parts of this file come from the Allegro 4.4 CMakeLists.txt

# CMake setup
cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# CMP0003: Libraries linked via full path no longer produce linker search paths.
#if(COMMAND cmake_policy)
#    cmake_policy(SET CMP0003 NEW)
#endif(COMMAND cmake_policy)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are:
        None Debug Release RelWithDebInfo Profile."
      FORCE)
endif()

# Restrict configuration types to the selected build type.
# Note: This needs to be done before the project command
set(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}" CACHE INTERNAL "internal")

# ASE project
project(aseprite C CXX)

######################################################################
# Options (these can be specified in cmake command line or modifying
# CMakeCache.txt)

option(USE_STATIC_LIBC    "Use static version of C and C++ runtimes" off)
option(USE_SHARED_JPEGLIB "Use your installed copy of jpeglib" off)
option(USE_SHARED_ZLIB    "Use your installed copy of zlib" off)
option(USE_SHARED_LIBPNG  "Use your installed copy of libpng" off)
option(ENABLE_MEMLEAK     "Enable memory-leaks detector (only for developers)" off)
option(ENABLE_UPDATER     "Enable automatic check for updates" on)
option(FULLSCREEN_PLATFORM "Enable fullscreen by default" off)

######################################################################
# Profile build type

list(APPEND CMAKE_BUILD_TYPES Profile)
mark_as_advanced(
    CMAKE_C_FLAGS_PROFILE
    CMAKE_CXX_FLAGS_PROFILE
    CMAKE_EXE_LINKER_FLAGS_PROFILE
    )

if(COMPILER_GCC)
    set(CMAKE_C_FLAGS_PROFILE "-pg"
        CACHE STRING "profiling flags")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE}"
        CACHE STRING "profiling flags")
    set(CMAKE_EXE_LINKER_FLAGS_PROFILE "-pg"
        CACHE STRING "profiling flags")
endif(COMPILER_GCC)

if(COMPILER_MSVC)
    set(CMAKE_C_FLAGS_PROFILE "-Gd -Ox"
        CACHE STRING "profiling flags")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE}"
        CACHE STRING "profiling flags")
    set(CMAKE_EXE_LINKER_FLAGS_PROFILE "-profile"
        CACHE STRING "profiling flags")
endif(COMPILER_MSVC)

######################################################################
# Directories

set(CURL_DIR            ${CMAKE_SOURCE_DIR}/third_party/curl)
set(GIFLIB_DIR          ${CMAKE_SOURCE_DIR}/third_party/giflib)
set(LIBFREETYPE_DIR     ${CMAKE_SOURCE_DIR}/third_party/freetype)
set(LIBJPEG_DIR         ${CMAKE_SOURCE_DIR}/third_party/jpeg)
set(LIBPNG_DIR          ${CMAKE_SOURCE_DIR}/third_party/libpng)
set(LOADPNG_DIR         ${CMAKE_SOURCE_DIR}/third_party/loadpng)
set(TINYXML_DIR         ${CMAKE_SOURCE_DIR}/third_party/tinyxml)
set(ZLIB_DIR            ${CMAKE_SOURCE_DIR}/third_party/zlib)

# Allegro header files
include_directories(${CMAKE_SOURCE_DIR}/src/allegro/include)
include_directories(${CMAKE_BINARY_DIR}/include)

# Search in the "cmake" directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Put libraries into "lib".
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

######################################################################
# Common definitions to compile all sources

# Do not use MMX optimizations in PNG code
add_definitions(-DPNG_NO_MMX_CODE)

# Static Allegro (the code of Allegro library is embedded).
add_definitions(-DALLEGRO_STATICLINK)

# Debug C/C++ flags
if(CMAKE_BUILD_TYPE STREQUAL Debug)
  add_definitions(-DDEBUGMODE -D_DEBUG)
else()
  add_definitions(-DNDEBUG)
endif()

if(ENABLE_MEMLEAK)
  add_definitions(-DMEMLEAK)
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
  find_package(DXGuid)

  if(NOT DXGUID_FOUND)
    if(MSVC)
      message(FATAL_ERROR
        "DirectX required for Windows port. You might need to add DirectX include and lib directories to your INCLUDE and LIB environment variables.")
    elseif(MINGW)
      message(FATAL_ERROR
        "DirectX required for Windows port. Get it from the Allegro web site (dx80_mgw.zip).")
    else()
      message(FATAL_ERROR "DirectX required for Windows port.")
    endif()
  endif()

  include_directories(SYSTEM
    ${DXGUID_INCLUDE_DIR}
    )

  list(APPEND PLATFORM_LIBS
    kernel32
    user32
    gdi32
    comdlg32
    ole32
    ${DXGUID_LIBRARIES}
    winmm
    shlwapi
    psapi
    wininet
    dbghelp
    )

  add_definitions(-D_WIN32_WINNT=0x0500)

endif(WIN32)

# -- Mac OS X --

if(APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(CARBON_LIBRARY Carbon)
  find_library(IOKIT_LIBRARY IOKit)
  find_library(COREAUDIO_LIBRARY CoreAudio)
  find_library(AUDIOUNIT_LIBRARY AudioUnit)
  find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)
  find_package(QuickTime)
  mark_as_advanced(COCOA_LIBRARY CARBON_LIBRARY IOKIT_LIBRARY)
  mark_as_advanced(COREAUDIO_LIBRARY AUDIOUNIT_LIBRARY AUDIOTOOLBOX_LIBRARY)
  mark_as_advanced(QUICKTIME_INCLUDE_DIR QUICKTIME_LIBRARY)

  list(APPEND PLATFORM_LIBS
    ${COCOA_LIBRARY}
    ${CARBON_LIBRARY}
    ${IOKIT_LIBRARY}
    ${COREAUDIO_LIBRARY}
    ${AUDIOUNIT_LIBRARY}
    ${AUDIOTOOLBOX_LIBRARY}
    ${QUICKTIME_LIBRARY}
    )

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
