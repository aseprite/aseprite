# ASEPRITE
# Copyright (C) 2001-2018  David Capello

include_directories(.)

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

if(NOT USE_SHARED_JPEGLIB)
  add_subdirectory(jpeg)
endif()

if(NOT USE_SHARED_ZLIB)
  set(SKIP_INSTALL_ALL on)
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
  add_subdirectory(libpng)
endif()

if(NOT USE_SHARED_GIFLIB)
  set(GIFLIB_UTILS OFF CACHE BOOL "Build giflib utils")
  add_subdirectory(giflib)
endif()

if(WITH_WEBP_SUPPORT)
  add_subdirectory(libwebp)
endif()

if(NOT USE_SHARED_TINYXML)
  add_subdirectory(tinyxml)
endif()

if(REQUIRE_CURL AND NOT USE_SHARED_CURL)
  set(BUILD_RELEASE_DEBUG_DIRS ON BOOL)
  set(CMAKE_USE_OPENSSL OFF CACHE BOOL "Use OpenSSL code. Experimental")
  add_subdirectory(curl)
endif()

if(NOT USE_SHARED_PIXMAN)
  add_subdirectory(pixman-cmake)
endif()

if(NOT USE_SHARED_FREETYPE)
  set(SKIP_INSTALL_ALL on)
  set(WITH_BZip2 OFF CACHE BOOL "")

  add_subdirectory(freetype2)

  if(NOT USE_SHARED_LIBPNG)
    add_dependencies(freetype ${PNG_LIBRARIES})
  endif()
endif()

if(NOT USE_SHARED_HARFBUZZ)
  if(NOT USE_SHARED_FREETYPE)
    set(ENV{FREETYPE_DIR} ${FREETYPE_DIR})
  endif()
  set(HB_HAVE_FREETYPE ON CACHE BOOL "Enable freetype interop helpers")
  set(HB_HAVE_GRAPHITE2 OFF CACHE BOOL "Enable Graphite2 complementary shaper")
  set(HB_BUILTIN_UCDN ON CACHE BOOL "Use HarfBuzz provided UCDN")
  set(HB_HAVE_GLIB OFF CACHE BOOL "Enable glib unicode functions")
  set(HB_HAVE_ICU OFF CACHE BOOL "Enable icu unicode functions")
  set(HB_HAVE_CORETEXT OFF CACHE BOOL "Enable CoreText shaper backend on macOS")
  set(HB_HAVE_UNISCRIBE OFF CACHE BOOL "Enable Uniscribe shaper backend on Windows")
  set(HB_HAVE_DIRECTWRITE OFF CACHE BOOL "Enable DirectWrite shaper backend on Windows")
  add_subdirectory(harfbuzz)
endif()

add_subdirectory(simpleini)
add_subdirectory(fmt)

# Add cmark without tests
if(NOT USE_SHARED_CMARK)
  set(CMARK_TESTS OFF CACHE BOOL "Build cmark tests and enable testing")
  add_subdirectory(cmark)
  set(cmark_headers
    ${CMAKE_CURRENT_SOURCE_DIR}/cmark/src/cmark.h
    ${CMAKE_CURRENT_BINARY_DIR}/cmark/src/cmark_export.h
    ${CMAKE_CURRENT_BINARY_DIR}/cmark/src/cmark_version.h)
  foreach(fn ${cmark_headers})
    get_filename_component(fn_name "${fn}" NAME)
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cmark/${fn_name}
      COMMAND ${CMAKE_COMMAND} -E copy ${fn} ${CMAKE_CURRENT_BINARY_DIR}/cmark/${fn_name}
      MAIN_DEPENDENCY ${fn})
    list(APPEND copy_cmark_headers ${CMAKE_CURRENT_BINARY_DIR}/cmark/${fn_name})
  endforeach()
  add_custom_target(copy_cmark_headers DEPENDS ${copy_cmark_headers})
  add_dependencies(libcmark_static copy_cmark_headers)
endif()

# JSON
add_subdirectory(json11)

# libarchive
set(ENABLE_WERROR OFF CACHE BOOL "Treat warnings as errors - default is ON for Debug, OFF otherwise.")
set(ENABLE_TEST OFF CACHE BOOL "Enable unit and regression tests")
set(ENABLE_COVERAGE OFF CACHE BOOL "Enable code coverage (GCC only, automatically sets ENABLE_TEST to ON)")
set(ENABLE_LZMA OFF CACHE BOOL "Enable the use of the system LZMA library if found")
set(ENABLE_CNG OFF CACHE BOOL "Enable the use of CNG(Crypto Next Generation)")
set(ENABLE_BZip2 OFF CACHE BOOL "Enable the use of the system BZip2 library if found")
set(ENABLE_LIBXML2 OFF CACHE BOOL "Enable the use of the system libxml2 library if found")
add_subdirectory(libarchive)
target_include_directories(archive_static INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libarchive/libarchive>)

# benchmark
if(ENABLE_BENCHMARKS)
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Enable testing of the benchmark library.")
  add_subdirectory(benchmark)
endif()

# tinyexpr
add_library(tinyexpr tinyexpr/tinyexpr.c)
target_include_directories(tinyexpr PUBLIC tinyexpr)

# lua
if(ENABLE_SCRIPTING)
  add_library(lua
    lua/lapi.c
    lua/lcode.c
    lua/lctype.c
    lua/ldebug.c
    lua/ldo.c
    lua/ldump.c
    lua/lfunc.c
    lua/lgc.c
    lua/llex.c
    lua/lmem.c
    lua/lobject.c
    lua/lopcodes.c
    lua/lparser.c
    lua/lstate.c
    lua/lstring.c
    lua/ltable.c
    lua/ltm.c
    lua/lundump.c
    lua/lvm.c
    lua/lzio.c
    lua/ltests.c)
  add_library(lauxlib lua/lauxlib.c)
  add_library(lualib
    lua/lbaselib.c
    lua/lcorolib.c
    lua/ldblib.c
    lua/linit.c
    lua/liolib.c
    lua/lmathlib.c
    lua/loadlib.c
    lua/loslib.c
    lua/lstrlib.c
    lua/ltablib.c
    lua/lutf8lib.c)
  target_compile_definitions(lua PUBLIC LUA_FLOORN2I=1)
  target_include_directories(lua PUBLIC lua)
  target_include_directories(lauxlib PUBLIC lua)
  target_include_directories(lualib PUBLIC lua)
  target_link_libraries(lauxlib lua)
endif()
