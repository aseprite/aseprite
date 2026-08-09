# ASEPRITE
# Copyright (C) 2020-present  Igara Studio S.A.

# Aseprite version
set(VERSION "1.x-dev")

option(UPDATE_VERSION_WITH_GIT "Update Aseprite version using Git commit" on)
if(UPDATE_VERSION_WITH_GIT)
  find_package(Git)
  if(Git_FOUND)
    execute_process(
      COMMAND git describe --tags --always --dirty
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX REPLACE "^v" "" VERSION "${VERSION}")
    string(REGEX REPLACE "-dev$" "" VERSION "${VERSION}")
    if(NOT VERSION MATCHES "^1.[0-9]")
      set(VERSION "1.x-${VERSION}")
    endif()
    set(VERSION "${VERSION}-dev")
  endif()
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/generated_version.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/generated_version.h")

add_library(ver-lib info.c ${gen_ver_fn})
target_include_directories(ver-lib PRIVATE .. "${CMAKE_CURRENT_BINARY_DIR}")

# Custom Aseprite website (for testing server-side)
if(NOT "${CUSTOM_WEBSITE_URL}" STREQUAL "")
  target_compile_definitions(ver-lib PRIVATE -DCUSTOM_WEBSITE_URL="${CUSTOM_WEBSITE_URL}")
endif()
