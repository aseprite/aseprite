# Aseprite Base Library
# Copyright (c) 2001-2013 David Capello

include(CheckCSourceCompiles)

CHECK_C_SOURCE_COMPILES("
#include <unistd.h>
int main() {
  sched_yield();
  return 0;
}
" HAVE_SCHED_YIELD)

if(HAVE_SCHED_YIELD)
  add_definitions(-DHAVE_SCHED_YIELD)
endif()

set(BASE_SOURCES
  cfile.cpp
  chrono.cpp
  convert_to.cpp
  errno_string.cpp
  exception.cpp
  file_handle.cpp
  fs.cpp
  launcher.cpp
  mem_utils.cpp
  memory.cpp
  memory_dump.cpp
  mutex.cpp
  path.cpp
  program_options.cpp
  serialization.cpp
  sha1.cpp
  sha1_rfc3174.c
  split_string.cpp
  string.cpp
  system_console.cpp
  temp_dir.cpp
  thread.cpp
  trim_string.cpp
  version.cpp)

if(WIN32)
  set(BASE_SOURCES ${BASE_SOURCES}
    win32_exception.cpp)
endif()

add_library(base-lib ${BASE_SOURCES})
