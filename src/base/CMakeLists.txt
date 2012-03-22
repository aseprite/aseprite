# ASEPRITE
# Copyright (C) 2001-2012  David Capello

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

add_library(base-lib
  chrono.cpp
  convert_to.cpp
  errno_string.cpp
  exception.cpp
  mem_utils.cpp
  memory.cpp
  memory_dump.cpp
  mutex.cpp
  path.cpp
  serialization.cpp
  sha1.cpp
  sha1_rfc3174.c
  split_string.cpp
  string.cpp
  thread.cpp
  version.cpp)
