# ASEPRITE
# Copyright (C) 2020-present  Igara Studio S.A.

# Aseprite 2.0 version (see docs/aseprite2/PLAN.md, Phase 1)
set(VERSION "2.0")

option(UPDATE_VERSION_WITH_GIT "Append Git commit hash to the Aseprite 2.0 version" on)
if(UPDATE_VERSION_WITH_GIT)
  find_package(Git)
  if(Git_FOUND)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
      "${CMAKE_SOURCE_DIR}/.git/HEAD"
      "${CMAKE_SOURCE_DIR}/.git/index")
    execute_process(
      COMMAND git rev-parse --short HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE GIT_HASH
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET)
    if(GIT_HASH)
      # Keep it parseable by app.version (Lua) as "2.0-dev+<hash>"
      set(VERSION "${VERSION}-dev+${GIT_HASH}")
    else()
      set(VERSION "${VERSION}-dev")
    endif()
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
