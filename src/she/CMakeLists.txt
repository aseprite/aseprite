# SHE
# Copyright (C) 2012-2015  David Capello

set(SHE_SOURCES)

if(USE_ALLEG4_BACKEND)
  list(APPEND SHE_SOURCES
    alleg4/clock.cpp
    alleg4/close_button.cpp
    alleg4/fontbmp.cpp
    alleg4/key_poller.cpp
    alleg4/mouse_poller.cpp
    alleg4/she.cpp)

  if(APPLE)
    if(NOT USE_SHARED_ALLEGRO4)
      list(APPEND SHE_SOURCES alleg4/app.mm)
    endif()
  endif()
endif()

if(USE_SKIA_BACKEND)
  set(SKIA_BUILD_DIR "" CACHE PATH "Skia build directory")

  if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(SKIA_BUILD_OUT_DIR "${SKIA_BUILD_DIR}/out/Debug")
  else()
    set(SKIA_BUILD_OUT_DIR "${SKIA_BUILD_DIR}/out/Release")
  endif()

  find_library(SKIA_CORE_LIBRARY skia_core PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_EFFECTS_LIBRARY skia_effects PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_IMAGES_LIBRARY skia_images PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_OPTS_LIBRARY skia_opts PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_OPTS_SSE41_LIBRARY skia_opts_sse41 PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_OPTS_SSSE3_LIBRARY skia_opts_ssse3 PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_PORTS_LIBRARY skia_ports PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_SFNT_LIBRARY skia_sfnt PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_GPU_LIBRARY skia_skgpu PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_UTILS_LIBRARY skia_utils PATH "${SKIA_BUILD_OUT_DIR}")

  if(WIN32)
    find_library(ETC1_LIBRARY libetc1 PATH "${SKIA_BUILD_OUT_DIR}/obj/gyp")
    find_library(LIBSKKTX_LIBRARY libSkKTX PATH "${SKIA_BUILD_OUT_DIR}/obj/gyp")
    find_library(OPENGL32_LIBRARY opengl32)
  else()
    set(ETC1_LIBRARY)
    set(LIBSKKTX_LIBRARY)
    # find_library(OPENGL32_LIBRARY glapi)
    set(OPENGL32_LIBRARY)
  endif()

  find_path(SKIA_CONFIG_INCLUDE_DIR SkUserConfig.h HINTS "${SKIA_BUILD_DIR}/include/config")
  find_path(SKIA_CORE_INCLUDE_DIR SkCanvas.h HINTS "${SKIA_BUILD_DIR}/include/core")
  find_path(SKIA_EFFECTS_INCLUDE_DIR SkBitmapSource.h HINTS "${SKIA_BUILD_DIR}/include/effects")
  find_path(SKIA_GPU_INCLUDE_DIR SkGr.h HINTS "${SKIA_BUILD_DIR}/include/gpu")
  find_path(SKIA_UTILS_INCLUDE_DIR SkRandom.h HINTS "${SKIA_BUILD_DIR}/include/utils")

  include_directories(
    ${SKIA_CONFIG_INCLUDE_DIR}
    ${SKIA_CORE_INCLUDE_DIR}
    ${SKIA_GPU_INCLUDE_DIR}
    ${SKIA_PORTS_INCLUDE_DIR}
    ${SKIA_UTILS_INCLUDE_DIR})

  set(SKIA_LIBRARIES
    ${ETC1_LIBRARY}
    ${LIBSKKTX_LIBRARY}
    ${OPENGL32_LIBRARY}
    ${SKIA_CORE_LIBRARY}
    ${SKIA_EFFECTS_LIBRARY}
    ${SKIA_GPU_LIBRARY}
    ${SKIA_IMAGES_LIBRARY}
    ${SKIA_OPTS_LIBRARY}
    ${SKIA_OPTS_SSE41_LIBRARY}
    ${SKIA_OPTS_SSSE3_LIBRARY}
    ${SKIA_PORTS_LIBRARY}
    ${SKIA_UTILS_LIBRARY}
    CACHE INTERNAL "Skia libraries")

  list(APPEND SHE_SOURCES
    skia/she.cpp)
endif()

if(WIN32)
  list(APPEND SHE_SOURCES
    win/clipboard.cpp
    win/native_dialogs.cpp)
endif()

if(APPLE)
  list(APPEND SHE_SOURCES
    osx/clipboard.mm
    osx/logger.mm
    osx/native_dialogs.mm)
endif()

add_library(she ${SHE_SOURCES})
