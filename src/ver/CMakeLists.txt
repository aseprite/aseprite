# ASEPRITE
# Copyright (C) 2020  Igara Studio S.A.

# Version of Aseprite
set(VERSION "1.x-dev")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/generated_version.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/generated_version.h")

# Custom Aseprite website (for testing server-side)
if(NOT "${CUSTOM_WEBSITE_URL}" STREQUAL "")
  add_definitions(-DCUSTOM_WEBSITE_URL="${CUSTOM_WEBSITE_URL}")
endif()

add_library(ver-lib info.c ${gen_ver_fn})
add_dependencies(ver-lib generated_version_h)
target_include_directories(ver-lib PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
