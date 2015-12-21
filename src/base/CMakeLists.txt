# Aseprite Base Library
# Copyright (c) 2001-2015 David Capello

include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)
include(CheckCSourceRuns)

check_c_source_compiles("
#include <stdint.h>
int main() {
  return 0;
}
" HAVE_STDINT_H)

check_c_source_compiles("
#include <unistd.h>
int main() {
  sched_yield();
  return 0;
}
" HAVE_SCHED_YIELD)

check_c_source_runs("
int main() {
  unsigned long value = 0x40302010ul;
  return (((unsigned char*)&value)[0] == 0x10) ? 0: 1;
}
" ASEPRITE_LITTLE_ENDIAN)

if(NOT ASEPRITE_LITTLE_ENDIAN)
  set(ASEPRITE_BIG_ENDIAN TRUE)
endif()

# Generate config.h file
configure_file(${CMAKE_CURRENT_LIST_DIR}/config.h.cmakein
               ${CMAKE_CURRENT_BINARY_DIR}/base/config.h @ONLY)

set(BASE_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}
    CACHE INTERNAL "Extra include directory for base lib")

include_directories(${BASE_INCLUDE_DIR})

set(BASE_SOURCES
  cfile.cpp
  chrono.cpp
  connection.cpp
  convert_to.cpp
  debug.cpp
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

add_library(base-lib ${BASE_SOURCES})

if(WIN32)
  target_link_libraries(base-lib dbghelp shlwapi)
endif()
