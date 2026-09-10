# Aseprite
# Copyright (C) 2021-present  Igara Studio S.A.
# Copyright (C) 2001-2018  David Capello

set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/third_party")

set(CURL_DIR     "${THIRD_PARTY_DIR}/curl")
set(LIBPNG_DIR   "${THIRD_PARTY_DIR}/libpng")
set(LIBWEBP_DIR  "${THIRD_PARTY_DIR}/libwebp")
set(PIXMAN_DIR   "${THIRD_PARTY_DIR}/pixman")
set(FREETYPE_DIR "${THIRD_PARTY_DIR}/freetype2")
set(HARFBUZZ_DIR "${THIRD_PARTY_DIR}/harfbuzz")

# libpng
if(USE_SHARED_LIBPNG)
  find_package(PNG REQUIRED)
else()
  # Skia on Linux includes libpng symbols
  if(UNIX AND NOT APPLE AND LAF_BACKEND STREQUAL "skia")
    set(PNG_LIBRARY skia)
  else()
    set(SKIP_INSTALL_ALL ON)
    # We only need the static version of libpng
    set(PNG_SHARED OFF CACHE BOOL "Build shared lib")
    set(PNG_STATIC ON CACHE BOOL "Build static lib")
    set(PNG_TESTS OFF CACHE BOOL "Build libpng tests")
    add_subdirectory(${LIBPNG_DIR})
    add_library(PNG::PNG ALIAS png_static)

    set(PNG_LIBRARY png_static)
  endif()

  set(PNG_FOUND ON)
  set(PNG_LIBRARIES ${PNG_LIBRARY})
  set(PNG_INCLUDE_DIRS
    ${LIBPNG_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/third_party/libpng) # Libpng generated pnglibconf.h file
  set(PNG_INCLUDE_DIR ${PNG_INCLUDE_DIRS} CACHE PATH "")
  set(PNG_PNG_INCLUDE_DIR ${PNG_INCLUDE_DIRS} CACHE PATH "")
endif()

# tinyxml2
if(USE_SHARED_TINYXML)
  find_library(TINYXML_LIBRARY NAMES tinyxml2)
else()
  set(tinyxml2_BUILD_TESTING OFF CACHE BOOL "Build tests for tinyxml2")
  add_subdirectory(${THIRD_PARTY_DIR}/tinyxml2)

  set(TINYXML_LIBRARY tinyxml2)
endif()

# TinyEXIF
if(USE_SHARED_TINYEXIF)
  find_library(TINYEXIF_LIBRARY NAMES TinyEXIF)
else()
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "build as shared library")
  set(BUILD_STATIC_LIBS ON CACHE BOOL "build as static library")
  set(LINK_CRT_STATIC_LIBS OFF CACHE BOOL "link CRT static library")
  set(BUILD_DEMO OFF CACHE BOOL "build demo binary")
  add_subdirectory(${THIRD_PARTY_DIR}/TinyEXIF)
endif()
set(TINYEXIF_LIBRARY TinyEXIFstatic)

# pixman
if(USE_SHARED_PIXMAN)
  find_library(PIXMAN_LIBRARY NAMES pixman pixman-1)
  find_path(PIXMAN_INCLUDE_DIR NAMES pixman.h PATH_SUFFIXES pixman-1)
else()
  set(PIXMAN_LIBRARY pixman)
  set(PIXMAN_INCLUDE_DIR
    ${PIXMAN_DIR}/pixman
    ${CMAKE_BINARY_DIR}) # For pixman-version.h
  add_subdirectory(${THIRD_PARTY_DIR}/pixman-cmake)
endif()

# freetype
if(USE_SHARED_FREETYPE)
  find_package(Freetype REQUIRED)
elseif(NOT LAF_BACKEND STREQUAL "skia")
  set(SKIP_INSTALL_ALL on)
  set(WITH_BZip2 OFF CACHE BOOL "")
  add_subdirectory(${FREETYPE_DIR})

  # For include/freetype/config/ftconfig.h and ftoption.h files.
  target_include_directories(freetype BEFORE PUBLIC
    ${CMAKE_BINARY_DIR}/third_party/freetype2/include)

  if(NOT USE_SHARED_LIBPNG)
    add_dependencies(freetype ${PNG_LIBRARIES})
  endif()

  set(FREETYPE_FOUND ON)
  set(FREETYPE_LIBRARY freetype)
  set(FREETYPE_LIBRARIES ${FREETYPE_LIBRARY})
  set(FREETYPE_INCLUDE_DIRS ${FREETYPE_DIR}/include)
endif()

# harfbuzz
if(USE_SHARED_HARFBUZZ)
  find_package(HarfBuzz)
elseif(NOT LAF_BACKEND STREQUAL "skia")
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
  add_subdirectory(${HARFBUZZ_DIR})
  add_library(HarfBuzz::HarfBuzz ALIAS harfbuzz)

  set(HARFBUZZ_FOUND ON)
  set(HARFBUZZ_LIBRARIES harfbuzz)
  set(HARFBUZZ_INCLUDE_DIRS "${HARFBUZZ_DIR}/src")
endif()

# freetype + harfbuzz
if(HARFBUZZ_FOUND AND NOT USE_SHARED_FREETYPE AND NOT LAF_BACKEND STREQUAL "skia")
  target_link_libraries(freetype PRIVATE ${HARFBUZZ_LIBRARIES})
  target_include_directories(freetype PRIVATE ${HARFBUZZ_INCLUDE_DIRS})
endif()

if(USE_SHARED_GIFLIB)
  find_package(GIF REQUIRED)
else()
  set(GIFLIB_UTILS OFF CACHE BOOL "Build giflib utils")
  add_subdirectory(${THIRD_PARTY_DIR}/giflib)

  set(GIF_FOUND ON)
  set(GIF_LIBRARY giflib)
  set(GIF_LIBRARIES ${GIF_LIBRARY})
  set(GIF_INCLUDE_DIR ${GIFLIB_DIR})
  set(GIF_INCLUDE_DIRS ${GIF_INCLUDE_DIR})
endif()

# libjpeg-turbo
find_package(JpegTurbo)

# cmark
if(USE_SHARED_CMARK)
  find_library(CMARK_LIBRARIES NAMES cmark)
  find_path(CMARK_INCLUDE_DIRS NAMES cmark.h)
else()
  # Add cmark without tests
  set(CMARK_PROGRAM OFF CACHE BOOL "Build cmark executable")
  set(CMARK_TESTS OFF CACHE BOOL "Build cmark tests and enable testing")
  add_definitions(-DCMARK_STATIC_DEFINE)
  add_subdirectory(${THIRD_PARTY_DIR}/cmark)

  set(CMARK_LIBRARIES cmark)
endif()

# curl
if(REQUIRE_CURL)
  if(USE_SHARED_CURL)
    find_package(CURL REQUIRED)
  else()
    if(WIN32)
      set(CURL_USE_SCHANNEL ON)
      set(CURL_USE_MBEDTLS OFF CACHE BOOL "")
    else()
      # mbedtls
      set(USE_STATIC_MBEDTLS_LIBRARY ON CACHE BOOL "")
      set(USE_SHARED_MBEDTLS_LIBRARY OFF CACHE BOOL "")
      set(ENABLE_TESTING OFF CACHE BOOL "")
      set(ENABLE_PROGRAMS OFF CACHE BOOL "")
      set(GEN_FILES OFF CACHE BOOL "")
      add_subdirectory("${THIRD_PARTY_DIR}/mbedtls")

      set(MbedTLS_CONFIG ON)
      set(MbedTLS_VERSION 4.2.0)
      add_library(MbedTLS::tfpsacrypto ALIAS tfpsacrypto)
      add_library(CURL::mbedtls ALIAS mbedtls)

      set(CURL_USE_MBEDTLS ON CACHE BOOL "")
    endif()

    # curl
    set(BUILD_CURL_EXE OFF CACHE BOOL "")
    set(BUILD_RELEASE_DEBUG_DIRS ON CACHE BOOL "")
    set(CURL_BROTLI OFF CACHE BOOL "")
    set(CURL_BUILD_EVERYTHING OFF CACHE BOOL "")
    set(CURL_DISABLE_INSTALL ON CACHE BOOL "")
    set(CURL_ENABLE_EXPORT_TARGET OFF CACHE BOOL "")
    set(CURL_STATICLIB ON CACHE BOOL "")
    set(CURL_USE_LIBPSL OFF CACHE BOOL "")
    set(CURL_USE_LIBSSH2 OFF CACHE BOOL "")
    set(CURL_USE_OPENSSL OFF CACHE BOOL "")
    set(CURL_USE_PKGCONFIG OFF CACHE BOOL "")
    set(CURL_ZSTD OFF CACHE BOOL "")
    set(USE_LIBIDN2 OFF CACHE BOOL "")
    set(USE_NGHTTP2 OFF CACHE BOOL "")
    add_subdirectory(${CURL_DIR})

    set(CURL_FOUND ON)
    set(CURL_LIBRARY libcurl)
    set(CURL_LIBRARIES libcurl)
    set(CURL_INCLUDE_DIRS ${CURL_DIR}/include)
  endif()
endif()

# libwebp
if(ENABLE_WEBP)
  # Use libwebp from Skia
  if(LAF_BACKEND STREQUAL "skia")
    find_library(WEBP_LIBRARIES webp
      NAMES libwebp # required for Windows
      PATHS "${SKIA_LIBRARY_DIR}" NO_DEFAULT_PATH)
    set(WEBP_INCLUDE_DIR "${SKIA_DIR}/third_party/externals/libwebp/src")
    if(WEBP_LIBRARIES)
      set(WEBP_FOUND ON)
    else()
      set(WEBP_FOUND OFF)
    endif()
  else()
    set(WEBP_BUILD_EXTRAS OFF CACHE BOOL "Build extras.")
    set(WEBP_BUILD_ANIM_UTILS OFF CACHE BOOL "Build animation utilities.")
    set(WEBP_BUILD_CWEBP OFF CACHE BOOL "Build the cwebp command line tool.")
    set(WEBP_BUILD_DWEBP OFF CACHE BOOL "Build the dwebp command line tool.")
    set(WEBP_BUILD_GIF2WEBP OFF CACHE BOOL "Build the gif2webp conversion tool.")
    set(WEBP_BUILD_IMG2WEBP OFF CACHE BOOL "Build the img2webp animation tool.")
    set(WEBP_BUILD_VWEBP OFF CACHE BOOL "Build the vwebp viewer tool.")
    set(WEBP_BUILD_WEBPINFO OFF CACHE BOOL "Build the webpinfo command line tool.")
    set(WEBP_BUILD_WEBPMUX OFF CACHE BOOL "Build the webpmux command line tool.")

    add_subdirectory(${LIBWEBP_DIR})

    set(WEBP_FOUND ON)
    set(WEBP_LIBRARIES webp webpdemux libwebpmux)
    set(WEBP_INCLUDE_DIR ${LIBWEBP_DIR}/src)

    if(NOT USE_SHARED_LIBPNG)
      add_dependencies(webp ${PNG_LIBRARY})
      add_dependencies(webpdemux ${PNG_LIBRARY})
      add_dependencies(libwebpmux ${PNG_LIBRARY})
    endif()
  endif()
endif()

# simpleini
add_subdirectory(${THIRD_PARTY_DIR}/simpleini)

# fmt
add_subdirectory(${THIRD_PARTY_DIR}/fmt)

# JSON
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
if(ENABLE_DEVMODE)
  set(JSON_Diagnostics ON CACHE INTERNAL "")
endif()

add_subdirectory(${THIRD_PARTY_DIR}/json)

# libarchive
set(HAVE_WCSCPY 1)
set(HAVE_WCSLEN 1)

set(ENABLE_WERROR OFF CACHE BOOL "Treat warnings as errors - default is ON for Debug, OFF otherwise.")
set(ENABLE_TEST OFF CACHE BOOL "Enable unit and regression tests")
set(ENABLE_COVERAGE OFF CACHE BOOL "Enable code coverage (GCC only, automatically sets ENABLE_TEST to ON)")
set(ENABLE_LZ4 OFF CACHE BOOL "Enable the use of the system LZ4 library if found")
set(ENABLE_LZO OFF CACHE BOOL "Enable the use of the system LZO library if found")
set(ENABLE_LZMA OFF CACHE BOOL "Enable the use of the system LZMA library if found")
set(ENABLE_ZSTD OFF CACHE BOOL "Enable the use of the system zstd library if found")
set(ENABLE_CNG OFF CACHE BOOL "Enable the use of CNG(Crypto Next Generation)")
set(ENABLE_BZip2 OFF CACHE BOOL "Enable the use of the system BZip2 library if found")
set(ENABLE_EXPAT OFF CACHE BOOL "Enable the use of the system EXPAT library if found")
set(ENABLE_LIBXML2 OFF CACHE BOOL "Enable the use of the system libxml2 library if found")
set(ENABLE_CAT OFF CACHE BOOL "Enable cat building")
set(ENABLE_TAR OFF CACHE BOOL "Enable tar building")
set(ENABLE_CPIO OFF CACHE BOOL "Enable cpio building")
set(ENABLE_LIBB2 OFF CACHE BOOL "Enable the use of the system LIBB2 library if found")
set(ENABLE_ICONV OFF CACHE BOOL "Enable iconv support")
set(WINDOWS_VERSION "WS08" CACHE STRING "Set Windows Vista as the target version for compiling libarchive (Windows only)" FORCE)
add_subdirectory(${THIRD_PARTY_DIR}/libarchive)
target_include_directories(archive_static INTERFACE
  $<BUILD_INTERFACE:${THIRD_PARTY_DIR}/libarchive/libarchive>)

# benchmark
if(ENABLE_BENCHMARKS)
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Enable testing of the benchmark library.")
  add_subdirectory(${THIRD_PARTY_DIR}/benchmark)
endif()

# tinyexpr
add_library(tinyexpr ${THIRD_PARTY_DIR}/tinyexpr/tinyexpr.c)
target_include_directories(tinyexpr PUBLIC ${THIRD_PARTY_DIR}/tinyexpr)

# cityhash
add_subdirectory(${THIRD_PARTY_DIR}/cityhash)

# lua
if(ENABLE_SCRIPTING)
  add_library(lua
    ${THIRD_PARTY_DIR}/lua/lapi.c
    ${THIRD_PARTY_DIR}/lua/lcode.c
    ${THIRD_PARTY_DIR}/lua/lctype.c
    ${THIRD_PARTY_DIR}/lua/ldebug.c
    ${THIRD_PARTY_DIR}/lua/ldo.c
    ${THIRD_PARTY_DIR}/lua/ldump.c
    ${THIRD_PARTY_DIR}/lua/lfunc.c
    ${THIRD_PARTY_DIR}/lua/lgc.c
    ${THIRD_PARTY_DIR}/lua/llex.c
    ${THIRD_PARTY_DIR}/lua/lmem.c
    ${THIRD_PARTY_DIR}/lua/lobject.c
    ${THIRD_PARTY_DIR}/lua/lopcodes.c
    ${THIRD_PARTY_DIR}/lua/lparser.c
    ${THIRD_PARTY_DIR}/lua/lstate.c
    ${THIRD_PARTY_DIR}/lua/lstring.c
    ${THIRD_PARTY_DIR}/lua/ltable.c
    ${THIRD_PARTY_DIR}/lua/ltm.c
    ${THIRD_PARTY_DIR}/lua/lundump.c
    ${THIRD_PARTY_DIR}/lua/lvm.c
    ${THIRD_PARTY_DIR}/lua/lzio.c
    ${THIRD_PARTY_DIR}/lua/ltests.c)
  add_library(lauxlib
    ${THIRD_PARTY_DIR}/lua/lauxlib.c)
  add_library(lualib
    ${THIRD_PARTY_DIR}/lua/lbaselib.c
    ${THIRD_PARTY_DIR}/lua/lcorolib.c
    ${THIRD_PARTY_DIR}/lua/ldblib.c
    ${THIRD_PARTY_DIR}/lua/linit.c
    ${THIRD_PARTY_DIR}/lua/liolib.c
    ${THIRD_PARTY_DIR}/lua/lmathlib.c
    ${THIRD_PARTY_DIR}/lua/loadlib.c
    ${THIRD_PARTY_DIR}/lua/loslib.c
    ${THIRD_PARTY_DIR}/lua/lstrlib.c
    ${THIRD_PARTY_DIR}/lua/ltablib.c
    ${THIRD_PARTY_DIR}/lua/lutf8lib.c)

  if(WIN32)
    # LUA_USE_WINDOWS is defined in luaconf.h when we compile with _WIN32
    #target_compile_definitions(lua PUBLIC LUA_USE_WINDOWS=1)
  elseif(APPLE)
    target_compile_definitions(lua PUBLIC LUA_USE_MACOSX=1)
  elseif(UNIX)
    target_compile_definitions(lua PUBLIC LUA_USE_LINUX=1)
  endif()

  # Compile Lua C files as C++ to control errors with exceptions and
  # have stack unwinding (i.e. calling destructors correctly).
  file(GLOB all_lua_source_files ${THIRD_PARTY_DIR}/lua/*.c)
  set_source_files_properties(${all_lua_source_files} PROPERTIES LANGUAGE CXX)

  target_compile_definitions(lua PUBLIC LUA_FLOORN2I=F2Ifloor)
  target_compile_definitions(lualib PRIVATE HAVE_SYSTEM)
  target_include_directories(lua PUBLIC ${THIRD_PARTY_DIR}/lua)
  target_include_directories(lauxlib PUBLIC ${THIRD_PARTY_DIR}/lua)
  target_include_directories(lualib PUBLIC ${THIRD_PARTY_DIR}/lua)
  target_link_libraries(lauxlib lua)
  target_link_libraries(lualib lua)

  # ixwebsocket
  if(ENABLE_WEBSOCKET)
    set(IXWEBSOCKET_INSTALL OFF CACHE BOOL "Install IXWebSocket")
    add_subdirectory(${THIRD_PARTY_DIR}/IXWebSocket)
  endif()

  set(CPPDAP_BUILD_TESTS OFF CACHE INTERNAL "")
  set(CPPDAP_BUILD_EXAMPLES OFF CACHE INTERNAL "")
  set(CPPDAP_INSTALL_VSCODE_EXAMPLES OFF CACHE INTERNAL "")
  set(CPPDAP_THIRD_PARTY_DIR "${THIRD_PARTY_DIR}" CACHE INTERNAL "")
  add_subdirectory(${THIRD_PARTY_DIR}/cppdap)

  set(BLAKE3_USE_TBB OFF CACHE INTERNAL "")
  set(BLAKE3_EXAMPLES OFF CACHE INTERNAL "")
  set(BLAKE3_TESTING OFF CACHE INTERNAL "")
  add_subdirectory(${THIRD_PARTY_DIR}/BLAKE3/c)
endif()

# qoi
add_library(qoi INTERFACE)
target_include_directories(qoi INTERFACE ${THIRD_PARTY_DIR}/qoi)
