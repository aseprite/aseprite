# ASEPRITE
# Copyright (C) 2001-2015  David Capello

include(CheckIncludeFiles)

include_directories(
  lib
  ${CMAKE_CURRENT_BINARY_DIR}           # For config.h
  )

check_include_files("dlfcn.h" HAVE_DLFCN_H)
check_include_files("fcntl.h" HAVE_FCNTL_H)
check_include_files("gl/gl.h" HAVE_GL_GL_H)
check_include_files("inttypes.h" HAVE_INTTYPES_H)
check_include_files("memory.h" HAVE_MEMORY_H)
check_include_files("rle.h" HAVE_RLE_H)
check_include_files("stdarg.h" HAVE_STDARG_H)
check_include_files("stdint.h" HAVE_STDINT_H)
check_include_files("stdlib.h" HAVE_STDLIB_H)
check_include_files("strings.h" HAVE_STRINGS_H)
check_include_files("string.h" HAVE_STRING_H)
check_include_files("sys/stat.h" HAVE_SYS_STAT_H)
check_include_files("sys/types.h" HAVE_SYS_TYPES_H)
check_include_files("unistd.h" HAVE_UNISTD_H)
check_include_files("varargs.h" HAVE_VARARGS_H)
check_include_files("X11/Xlib.h" HAVE_X11_XLIB_H)
check_include_files("X11/Xutil.h" HAVE_X11_XUTIL_H)

check_function_exists("vprintf" HAVE_VPRINTF)
check_function_exists("_doprnt" HAVE_DOPRNT)
check_function_exists("strdup" HAVE_STRDUP)

if(HAVE_STDINT_H OR HAVE_INTTYPES_H)
  set("UINT32" "uint32_t")
else()
  set("UINT32" "unsigned int")
endif()

configure_file(config.h.cmake config.h)

add_definitions(-DHAVE_CONFIG_H=1)

if(WIN32)
add_definitions(-DWINDOWS32=1)
endif()

add_library(giflib
  lib/dgif_lib.c
  lib/egif_lib.c
  lib/gif_err.c
  lib/gif_font.c
  lib/gif_hash.c
  lib/gifalloc.c
  lib/quantize.c)
