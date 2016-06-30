# Aseprite Base Library
# Copyright (c) 2001-2016 David Capello

include(CheckIncludeFiles)
include(CheckFunctionExists)
include(TestBigEndian)

check_include_files(stdint.h HAVE_STDINT_H)
check_include_files(dlfcn.h HAVE_DLFCN_H)
check_function_exists(sched_yield HAVE_SCHED_YIELD)

test_big_endian(ASEPRITE_BIG_ENDIAN)

if(NOT ASEPRITE_BIG_ENDIAN)
  set(ASEPRITE_LITTLE_ENDIAN TRUE)
endif()

# Generate config.h file
configure_file(${CMAKE_CURRENT_LIST_DIR}/config.h.cmakein
               ${CMAKE_CURRENT_BINARY_DIR}/base/config.h @ONLY)

set(BASE_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}
    CACHE INTERNAL "Extra include directory for base lib")
set(MODP_B64_DIR "${CMAKE_CURRENT_LIST_DIR}/../../third_party/modp_b64"
    CACHE STRING "modp_b64 directory")

include_directories(${BASE_INCLUDE_DIR})
include_directories(${MODP_B64_DIR})

set(BASE_SOURCES
  base64.cpp
  cfile.cpp
  chrono.cpp
  convert_to.cpp
  debug.cpp
  dll.cpp
  errno_string.cpp
  exception.cpp
  file_handle.cpp
  fs.cpp
  launcher.cpp
  log.cpp
  mem_utils.cpp
  memory.cpp
  memory_dump.cpp
  mutex.cpp
  path.cpp
  process.cpp
  program_options.cpp
  replace_string.cpp
  serialization.cpp
  sha1.cpp
  sha1_rfc3174.c
  split_string.cpp
  string.cpp
  system_console.cpp
  thread.cpp
  time.cpp
  trim_string.cpp
  version.cpp)

if(WIN32)
  set(BASE_SOURCES ${BASE_SOURCES}
    win32_exception.cpp)
endif()

# TODO remove dependency with observable library
add_library(base-lib ${BASE_SOURCES})
target_link_libraries(base-lib obs modp_b64)

if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  target_link_libraries(base-lib dbghelp shlwapi)
else()
  find_library(DL_LIBRARY NAMES dl)
  if(DL_LIBRARY)
    target_link_libraries(base-lib ${DL_LIBRARY})
  endif()
endif()
