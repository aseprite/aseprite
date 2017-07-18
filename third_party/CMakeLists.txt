# ASEPRITE
# Copyright (C) 2001-2017  David Capello

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
    # Skia already includes webp library
    if(NOT USE_SKIA_BACKEND)
      add_subdirectory(libwebp-cmake)
    endif()
  endif()
endif()

if(NOT USE_SHARED_GIFLIB)
  add_subdirectory(giflib)
endif()

if(NOT USE_SHARED_TINYXML)
  add_subdirectory(tinyxml)
endif()

if(NOT USE_SHARED_CURL)
  set(BUILD_RELEASE_DEBUG_DIRS ON BOOL)
  set(CMAKE_USE_OPENSSL OFF CACHE BOOL "Use OpenSSL code. Experimental")
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

  if(NOT USE_SHARED_LIBPNG)
    add_dependencies(freetype png_static)
  endif()
endif()

add_subdirectory(harfbuzz-cmake)
add_subdirectory(simpleini)

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
set(ENABLE_TEST OFF CACHE BOOL "Enable unit and regression tests")
set(ENABLE_COVERAGE OFF CACHE BOOL "Enable code coverage (GCC only, automatically sets ENABLE_TEST to ON)")
set(ENABLE_LZMA OFF CACHE BOOL "Enable the use of the system LZMA library if found")
set(ENABLE_CNG OFF CACHE BOOL "Enable the use of CNG(Crypto Next Generation)")
add_subdirectory(libarchive)
target_include_directories(archive_static INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libarchive/libarchive>)

# benchmark
if(ENABLE_BENCHMARKS)
  add_subdirectory(benchmark)
endif()
