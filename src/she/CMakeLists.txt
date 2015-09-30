# SHE
# Copyright (C) 2012-2015  David Capello

set(SHE_SOURCES)

######################################################################
# Allegro 4 backend

if(USE_ALLEG4_BACKEND)
  add_definitions(-DUSE_KEY_POLLER)
  if(WIN32)
    find_package(DXGuid)
    if(NOT DXGUID_FOUND)
      if(MSVC)
        message(FATAL_ERROR "DirectX required for Windows port. You might need to add DirectX include and lib directories to your INCLUDE and LIB environment variables.")
      else()
        message(FATAL_ERROR "DirectX required for Windows port.")
      endif()
    endif()
    include_directories(SYSTEM ${DXGUID_INCLUDE_DIR})
  else()
    add_definitions(-DUSE_MOUSE_POLLER)
  endif()

  list(APPEND SHE_SOURCES
    alleg4/alleg_display.cpp
    alleg4/alleg_surface.cpp
    alleg4/clock.cpp
    alleg4/display_events.cpp
    alleg4/key_poller.cpp
    alleg4/mouse_poller.cpp
    alleg4/she.cpp)

  if(APPLE)
    if(NOT USE_SHARED_ALLEGRO4)
      list(APPEND SHE_SOURCES alleg4/app.mm)
    endif()
  endif()
endif()

######################################################################
# Skia backend

if(USE_SKIA_BACKEND)
  set(SKIA_DIR "" CACHE PATH "Skia source code directory")

  add_definitions(
    -DSK_INTERNAL
    -DSK_GAMMA_SRGB
    -DSK_GAMMA_APPLY_TO_A8
    -DSK_SCALAR_TO_FLOAT_EXCLUDED
    -DSK_ALLOW_STATIC_GLOBAL_INITIALIZERS=1
    -DSK_SUPPORT_GPU=1
    -DSK_SUPPORT_OPENCL=0
    -DSK_FORCE_DISTANCE_FIELD_TEXT=0
    -DGR_GL_FUNCTION_TYPE=__stdcall)

  if(WIN32)
    add_definitions(-DSK_BUILD_FOR_WIN32)
  elseif(APPLE)
    add_definitions(-DSK_BUILD_FOR_MAC)
    add_definitions(-Wno-ignored-attributes -Wno-unused-result)
  endif()

  if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(SKIA_BUILD_OUT_DIR "${SKIA_DIR}/out/Debug")
    add_definitions(-DSK_DEVELOPER=1)
  else()
    set(SKIA_BUILD_OUT_DIR "${SKIA_DIR}/out/Release")
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

  set(SKIA_LIBEGL_LIBRARY "")
  if(WIN32)
    find_library(SKIA_ETC1_LIBRARY libetc1 PATH "${SKIA_BUILD_OUT_DIR}/obj/gyp")
    find_library(SKIA_LIBSKKTX_LIBRARY libSkKTX PATH "${SKIA_BUILD_OUT_DIR}/obj/gyp")
    find_library(SKIA_OPENGL32_LIBRARY opengl32)
    find_library(SKIA_LIBEGL_LIBRARY libEGL.dll.lib PATH "${SKIA_BUILD_OUT_DIR}")
    if(SKIA_LIBEGL_LIBRARY)
      add_definitions(-DSK_ANGLE=1)
    endif()
  else()
    find_library(SKIA_ETC1_LIBRARY etc1 PATH "${SKIA_BUILD_OUT_DIR}")
    find_library(SKIA_LIBSKKTX_LIBRARY SkKTX PATH "${SKIA_BUILD_OUT_DIR}")
  endif()

  find_path(SKIA_CONFIG_INCLUDE_DIR SkUserConfig.h HINTS "${SKIA_DIR}/include/config")
  find_path(SKIA_CORE_INCLUDE_DIR SkCanvas.h HINTS "${SKIA_DIR}/include/core")
  find_path(SKIA_EFFECTS_INCLUDE_DIR SkBitmapSource.h HINTS "${SKIA_DIR}/include/effects")
  find_path(SKIA_GPU_INCLUDE_DIR SkGr.h HINTS "${SKIA_DIR}/include/gpu")
  find_path(SKIA_UTILS_INCLUDE_DIR SkRandom.h HINTS "${SKIA_DIR}/include/utils")

  set(SKIA_ANGLE_INCLUDE_DIR "")
  if(WIN32)
    find_path(SKIA_ANGLE_INCLUDE_DIR angle_gl.h HINTS "${SKIA_DIR}/third_party/externals/angle2/include")
  endif()

  include_directories(
    ${SKIA_CONFIG_INCLUDE_DIR}
    ${SKIA_CORE_INCLUDE_DIR}
    ${SKIA_GPU_INCLUDE_DIR}
    ${SKIA_PORTS_INCLUDE_DIR}
    ${SKIA_UTILS_INCLUDE_DIR}
    ${SKIA_ANGLE_INCLUDE_DIR})

  set(SKIA_LIBRARIES
    ${SKIA_ETC1_LIBRARY}
    ${SKIA_LIBSKKTX_LIBRARY}
    ${SKIA_OPENGL32_LIBRARY}
    ${SKIA_LIBEGL_LIBRARY}
    ${SKIA_CORE_LIBRARY}
    ${SKIA_EFFECTS_LIBRARY}
    ${SKIA_GPU_LIBRARY}
    ${SKIA_IMAGES_LIBRARY}
    ${SKIA_OPTS_LIBRARY}
    ${SKIA_OPTS_SSE41_LIBRARY}
    ${SKIA_OPTS_SSSE3_LIBRARY}
    ${SKIA_PORTS_LIBRARY}
    ${SKIA_SFNT_LIBRARY}
    ${SKIA_UTILS_LIBRARY}
    CACHE INTERNAL "Skia libraries")

  list(APPEND SHE_SOURCES
    skia/skia_display.cpp
    skia/she.cpp)

  if(WIN32)
    list(APPEND SHE_SOURCES
      skia/skia_window_win.cpp
      win/vk.cpp)
  endif()

  if(APPLE)
    list(APPEND SHE_SOURCES
      osx/app.mm
      osx/app_delegate.mm
      osx/event_queue.mm
      osx/window.mm
      skia/skia_window_osx.mm)
  endif()
endif()

######################################################################

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

target_link_libraries(she
  gfx-lib
  base-lib)

if(USE_ALLEG4_BACKEND)
  target_link_libraries(she
    ${LOADPNG_LIBRARY}
    ${LIBALLEGRO4_LINK_FLAGS}
    ${DXGUID_LIBRARIES})
endif()

if(USE_SKIA_BACKEND)
  target_link_libraries(she
    ${SKIA_LIBRARIES})
endif()
