# ASEPRITE
# Copyright (C) 2001-2012  David Capello

find_path(V8_INCLUDE_DIR NAMES v8.h
  HINTS "$ENV{V8_DIR}/include")
find_library(V8_BASE NAMES v8_base
  HINTS "$ENV{V8_DIR}"
        "$ENV{V8_DIR}/lib"
        "$ENV{V8_DIR}/build/Release/lib")
find_library(V8_SNAPSHOT NAMES v8_snapshot
  HINTS "$ENV{V8_DIR}"
        "$ENV{V8_DIR}/lib"
        "$ENV{V8_DIR}/build/Release/lib")
mark_as_advanced(V8_INCLUDE_DIR V8_BASE V8_SNAPSHOT)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(V8 DEFAULT_MSG V8_BASE V8_SNAPSHOT V8_INCLUDE_DIR)

set(V8_LIBRARIES ${V8_BASE} ${V8_SNAPSHOT})
