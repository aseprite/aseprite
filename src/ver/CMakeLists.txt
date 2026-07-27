# ASEPRITE
# Copyright (C) 2020-present  Igara Studio S.A.

# Version of Aseprite
set(VERSION "1.x-dev")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/generated_version.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/generated_version.h")

add_library(ver-lib info.c ${gen_ver_fn})
target_include_directories(ver-lib PRIVATE .. "${CMAKE_CURRENT_BINARY_DIR}")

# Custom Aseprite website (for testing server-side)
if(NOT "${CUSTOM_WEBSITE_URL}" STREQUAL "")
  target_compile_definitions(ver-lib PRIVATE -DCUSTOM_WEBSITE_URL="${CUSTOM_WEBSITE_URL}")
endif()
