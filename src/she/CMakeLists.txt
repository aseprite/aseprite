# SHE
# Copyright (C) 2012-2016  David Capello

set(SHE_SOURCES
  common/freetype_font.cpp)

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
    -DSK_SUPPORT_OPENCL=0
    -DSK_FORCE_DISTANCE_FIELD_TEXT=0
    -DGR_GL_FUNCTION_TYPE=__stdcall)

  if(WIN32)
    add_definitions(-DSK_BUILD_FOR_WIN32)
  elseif(APPLE)
    add_definitions(-DSK_BUILD_FOR_MAC)
    add_definitions(-Wno-ignored-attributes -Wno-unused-result)

    # Use Automatic Reference Counting
    add_definitions(-fobjc-arc)
  else()
    add_definitions(-DSK_SAMPLES_FOR_X)
  endif()

  if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(SKIA_BUILD_OUT_DIR "${SKIA_DIR}/out/Debug")
    add_definitions(-DSK_DEVELOPER=1)
  else()
    set(SKIA_BUILD_OUT_DIR "${SKIA_DIR}/out/Release")
  endif()

  find_library(SKIA_CODEC_LIBRARY skia_codec PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_CORE_LIBRARY skia_core PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_DNG_SDK_LIBRARY dng_sdk PATHS "${SKIA_BUILD_OUT_DIR}" "${SKIA_BUILD_OUT_DIR}/obj/gyp" NO_DEFAULT_PATH)
  find_library(SKIA_EFFECTS_LIBRARY skia_effects PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_ETC1_LIBRARY etc1 NAMES libetc1 PATHS "${SKIA_BUILD_OUT_DIR}" "${SKIA_BUILD_OUT_DIR}/obj/gyp" NO_DEFAULT_PATH)
  find_library(SKIA_FLAGS_LIBRARY flags PATHS "${SKIA_BUILD_OUT_DIR}" "${SKIA_BUILD_OUT_DIR}/obj/gyp" NO_DEFAULT_PATH)
  find_library(SKIA_GIF_LIBRARY giflib PATHS "${SKIA_BUILD_OUT_DIR}" "${SKIA_BUILD_OUT_DIR}/obj/gyp" NO_DEFAULT_PATH)
  find_library(SKIA_IMAGES_LIBRARY skia_images PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_JPG_LIBRARY jpeg-turbo NAMES libjpeg-turbo PATHS "${SKIA_BUILD_OUT_DIR}" "${SKIA_BUILD_OUT_DIR}/obj/gyp" NO_DEFAULT_PATH)
  find_library(SKIA_OPTS_AVX_LIBRARY skia_opts_avx PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_OPTS_HSW_LIBRARY skia_opts_hsw PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_OPTS_LIBRARY skia_opts PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_OPTS_SSE41_LIBRARY skia_opts_sse41 PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_OPTS_SSE42_LIBRARY skia_opts_sse42 PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_OPTS_SSSE3_LIBRARY skia_opts_ssse3 PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_PIEX_LIBRARY piex PATHS "${SKIA_BUILD_OUT_DIR}" "${SKIA_BUILD_OUT_DIR}/obj/gyp" NO_DEFAULT_PATH)
  find_library(SKIA_PNG_LIBRARY png_static NAMES libpng_static PATHS "${SKIA_BUILD_OUT_DIR}" "${SKIA_BUILD_OUT_DIR}/obj/gyp" NO_DEFAULT_PATH)
  find_library(SKIA_PORTS_LIBRARY skia_ports PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_RAW_CODEC_LIBRARY raw_codec PATHS "${SKIA_BUILD_OUT_DIR}" "${SKIA_BUILD_OUT_DIR}/obj/gyp" NO_DEFAULT_PATH)
  find_library(SKIA_SFNT_LIBRARY skia_sfnt PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_SKKTX_LIBRARY SkKTX libSkKTX PATHS "${SKIA_BUILD_OUT_DIR}" "${SKIA_BUILD_OUT_DIR}/obj/gyp" NO_DEFAULT_PATH)
  find_library(SKIA_UTILS_LIBRARY skia_utils PATH "${SKIA_BUILD_OUT_DIR}")
  find_library(SKIA_WEBP_DEC_LIBRARY webp_dec NAMES libwebp_dec PATHS "${SKIA_BUILD_OUT_DIR}" "${SKIA_BUILD_OUT_DIR}/obj/gyp" NO_DEFAULT_PATH)
  find_library(SKIA_WEBP_DEMUX_LIBRARY webp_demux NAMES libwebp_demux PATHS "${SKIA_BUILD_OUT_DIR}" "${SKIA_BUILD_OUT_DIR}/obj/gyp" NO_DEFAULT_PATH)
  find_library(SKIA_WEBP_DSP_LIBRARY webp_dsp NAMES libwebp_dsp PATHS "${SKIA_BUILD_OUT_DIR}" "${SKIA_BUILD_OUT_DIR}/obj/gyp" NO_DEFAULT_PATH)
  find_library(SKIA_WEBP_DSP_ENC_LIBRARY webp_dsp_enc NAMES libwebp_dsp_enc PATHS "${SKIA_BUILD_OUT_DIR}" "${SKIA_BUILD_OUT_DIR}/obj/gyp" NO_DEFAULT_PATH)
  find_library(SKIA_WEBP_ENC_LIBRARY webp_enc NAMES libwebp_enc PATHS "${SKIA_BUILD_OUT_DIR}" "${SKIA_BUILD_OUT_DIR}/obj/gyp" NO_DEFAULT_PATH)
  find_library(SKIA_WEBP_UTILS_LIBRARY webp_utils NAMES libwebp_utils PATHS "${SKIA_BUILD_OUT_DIR}" "${SKIA_BUILD_OUT_DIR}/obj/gyp" NO_DEFAULT_PATH)
  find_library(SKIA_ZLIB_LIBRARY zlib PATHS "${SKIA_BUILD_OUT_DIR}" "${SKIA_BUILD_OUT_DIR}/obj/gyp" NO_DEFAULT_PATH)

  find_library(SKIA_GPU_LIBRARY skia_skgpu PATH "${SKIA_BUILD_OUT_DIR}")
  if(SKIA_GPU_LIBRARY)
    add_definitions(-DSK_SUPPORT_GPU=1)
    if(WIN32)
      find_library(SKIA_OPENGL_LIBRARY opengl32)
      find_library(SKIA_LIBEGL_LIBRARY libEGL.dll PATH "${SKIA_BUILD_OUT_DIR}")
      if(SKIA_LIBEGL_LIBRARY)
        add_definitions(-DSK_ANGLE=1)
      endif()
    else()
      find_library(SKIA_OPENGL_LIBRARY opengl NAMES GL)
    endif()
  else()
    add_definitions(-DSK_SUPPORT_GPU=0)
    set(SKIA_GPU_LIBRARY "")
    set(SKIA_OPENGL_LIBRARY "")
  endif()

  if(NOT SKIA_LIBEGL_LIBRARY)
    set(SKIA_LIBEGL_LIBRARY "")
  endif()

  find_path(SKIA_CONFIG_INCLUDE_DIR SkUserConfig.h HINTS "${SKIA_DIR}/include/config")
  find_path(SKIA_CORE_INCLUDE_DIR SkCanvas.h HINTS "${SKIA_DIR}/include/core")
  find_path(SKIA_UTILS_INCLUDE_DIR SkRandom.h HINTS "${SKIA_DIR}/include/utils")
  find_path(SKIA_CODEC_INCLUDE_DIR SkCodec.h HINTS "${SKIA_DIR}/include/codec")
  find_path(SKIA_EFFECTS_INCLUDE_DIR SkImageSource.h HINTS "${SKIA_DIR}/include/effects")
  if(SKIA_GPU_LIBRARY)
    find_path(SKIA_GPU_INCLUDE_DIR SkGr.h HINTS "${SKIA_DIR}/include/gpu")
    find_path(SKIA_ANGLE_INCLUDE_DIR angle_gl.h HINTS "${SKIA_DIR}/third_party/externals/angle2/include")
  endif()

  include_directories(
    ${SKIA_CONFIG_INCLUDE_DIR}
    ${SKIA_CORE_INCLUDE_DIR}
    ${SKIA_PORTS_INCLUDE_DIR}
    ${SKIA_UTILS_INCLUDE_DIR}
    ${SKIA_CODEC_INCLUDE_DIR})
  if(SKIA_GPU_LIBRARY)
    include_directories(${SKIA_GPU_INCLUDE_DIR})
    if(WIN32)
      include_directories(${SKIA_ANGLE_INCLUDE_DIR})
    endif()
  endif()

  set(SKIA_LIBRARIES
    ${SKIA_CORE_LIBRARY}
    ${SKIA_IMAGES_LIBRARY}
    ${SKIA_OPTS_LIBRARY}
    ${SKIA_OPTS_SSSE3_LIBRARY}
    ${SKIA_OPTS_SSE41_LIBRARY}
    ${SKIA_OPTS_SSE42_LIBRARY}
    ${SKIA_OPTS_HSW_LIBRARY}
    ${SKIA_OPTS_AVX_LIBRARY}
    ${SKIA_PORTS_LIBRARY}
    ${SKIA_EFFECTS_LIBRARY}
    ${SKIA_CODEC_LIBRARY}
    ${SKIA_RAW_CODEC_LIBRARY}
    ${SKIA_PIEX_LIBRARY}
    ${SKIA_DNG_SDK_LIBRARY}
    ${SKIA_GIF_LIBRARY}
    ${SKIA_JPG_LIBRARY}
    ${SKIA_WEBP_DEC_LIBRARY}
    ${SKIA_WEBP_DEMUX_LIBRARY}
    ${SKIA_WEBP_DSP_LIBRARY}
    ${SKIA_WEBP_DSP_ENC_LIBRARY}
    ${SKIA_WEBP_ENC_LIBRARY}
    ${SKIA_WEBP_UTILS_LIBRARY}
    ${SKIA_PNG_LIBRARY}
    ${SKIA_ZLIB_LIBRARY}
    ${SKIA_GPU_LIBRARY}
    # We've to include two times skia_core as a dependency as it looks
    # like Skia libraries depends one each other (core+ports+effects)
    ${SKIA_CORE_LIBRARY}
    ${SKIA_UTILS_LIBRARY}
    ${SKIA_SFNT_LIBRARY}
    ${SKIA_OPENGL_LIBRARY}
    ${SKIA_LIBEGL_LIBRARY}
    ${SKIA_SKKTX_LIBRARY}
    ${SKIA_ETC1_LIBRARY}
    ${SKIA_FLAGS_LIBRARY}
    CACHE INTERNAL "Skia libraries")

  list(APPEND SHE_SOURCES
    skia/skia_display.cpp
    skia/skia_surface.cpp
    skia/she.cpp)

  if(WIN32)
    list(APPEND SHE_SOURCES
      skia/skia_window_win.cpp
      win/pen.cpp
      win/vk.cpp
      win/window_dde.cpp)
  elseif(APPLE)
    list(APPEND SHE_SOURCES
      osx/app.mm
      osx/app_delegate.mm
      osx/event_queue.mm
      osx/view.mm
      osx/vk.mm
      osx/window.mm
      skia/skia_window_osx.mm)
  else()
    list(APPEND SHE_SOURCES
      skia/skia_window_x11.cpp
      x11/keys.cpp
      x11/x11.cpp)
  endif()
endif()

######################################################################

if(WIN32)
  list(APPEND SHE_SOURCES
    win/native_dialogs.cpp)
endif()

if(APPLE)
  list(APPEND SHE_SOURCES
    osx/logger.mm
    osx/native_dialogs.mm)
endif()

if(WITH_GTK_FILE_DIALOG_SUPPORT AND UNIX AND NOT APPLE AND NOT BEOS)
  if(WITH_DEPRECATED_GLIB_SUPPORT)
    add_definitions(-DASEPRITE_DEPRECATED_GLIB_SUPPORT)
  endif()
  add_definitions(-DASEPRITE_WITH_GTK_FILE_DIALOG_SUPPORT)
  list(APPEND SHE_SOURCES
    gtk/native_dialogs.cpp)
endif()

add_library(she ${SHE_SOURCES})

target_link_libraries(she
  gfx-lib
  laf-base
  ${FREETYPE_LIBRARIES})

if(USE_ALLEG4_BACKEND)
  target_link_libraries(she
    ${LOADPNG_LIBRARY}
    ${LIBALLEGRO4_LINK_FLAGS}
    ${DXGUID_LIBRARIES}
    ${X11_LIBRARIES})
endif()

if(USE_SKIA_BACKEND)
  target_link_libraries(she
    ${SKIA_LIBRARIES})

  if(UNIX AND NOT APPLE)
    target_link_libraries(she fontconfig)
  endif()
endif()

if(WITH_GTK_FILE_DIALOG_SUPPORT)
  target_link_libraries(she
    ${GTKMM_LIBRARIES})
endif()
