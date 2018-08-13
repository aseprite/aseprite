# SHE
# Copyright (C) 2012-2018  David Capello

# TODO the following variables should be available through options
#      in this file instead of the main Aseprite CMakeLists.txt:
#      - USE_ALLEG4_BACKEND
#      - USE_NONE_BACKEND
#      - USE_SKIA_BACKEND
#      - WITH_GTK_FILE_DIALOG_SUPPORT

set(SHE_SOURCES
  common/freetype_font.cpp
  draw_text.cpp
  system.cpp)

######################################################################
# Allegro 4 backend

if(USE_ALLEG4_BACKEND)
  add_definitions(-DUSE_KEY_POLLER -DUSE_MOUSE_POLLER)
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
  endif()

  list(APPEND SHE_SOURCES
    alleg4/alleg_display.cpp
    alleg4/alleg_surface.cpp
    alleg4/display_events.cpp
    alleg4/key_poller.cpp
    alleg4/mouse_poller.cpp
    alleg4/scancode.cpp
    alleg4/she.cpp)
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
    -DGR_GL_FUNCTION_TYPE=__stdcall
    # TODO change this to 1
    -DSK_SUPPORT_GPU=0)

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

  if(NOT SKIA_DIR)
    set(SKIA_OUT_DIR "" CACHE PATH "Skia output directory")
  else()
    if(CMAKE_BUILD_TYPE STREQUAL Debug)
      set(SKIA_OUT_DIR "${SKIA_DIR}/out/Debug" CACHE PATH "Skia output directory")
    else()
      set(SKIA_OUT_DIR "${SKIA_DIR}/out/Release" CACHE PATH "Skia output directory")
    endif()
  endif()

  find_library(SKIA_LIBRARY skia PATH "${SKIA_OUT_DIR}")
  if(WIN32)
    find_library(SKIA_OPENGL_LIBRARY opengl32)
  else()
    find_library(SKIA_OPENGL_LIBRARY opengl NAMES GL)
  endif()

  find_path(SKIA_CONFIG_INCLUDE_DIR SkUserConfig.h HINTS "${SKIA_DIR}/include/config")
  find_path(SKIA_CORE_INCLUDE_DIR SkCanvas.h HINTS "${SKIA_DIR}/include/core")
  find_path(SKIA_UTILS_INCLUDE_DIR SkRandom.h HINTS "${SKIA_DIR}/include/utils")
  find_path(SKIA_CODEC_INCLUDE_DIR SkCodec.h HINTS "${SKIA_DIR}/include/codec")
  find_path(SKIA_EFFECTS_INCLUDE_DIR SkImageSource.h HINTS "${SKIA_DIR}/include/effects")
  find_path(SKIA_GPU_INCLUDE_DIR GrContext.h HINTS "${SKIA_DIR}/include/gpu")
  find_path(SKIA_GPU2_INCLUDE_DIR gl/GrGLDefines.h HINTS "${SKIA_DIR}/src/gpu")
  find_path(SKIA_ANGLE_INCLUDE_DIR angle_gl.h HINTS "${SKIA_DIR}/third_party/externals/angle2/include")

  include_directories(
    ${SKIA_CONFIG_INCLUDE_DIR}
    ${SKIA_CORE_INCLUDE_DIR}
    ${SKIA_PORTS_INCLUDE_DIR}
    ${SKIA_UTILS_INCLUDE_DIR}
    ${SKIA_CODEC_INCLUDE_DIR}
    ${SKIA_GPU_INCLUDE_DIR}
    ${SKIA_GPU2_INCLUDE_DIR})
  if(WIN32)
    include_directories(${SKIA_ANGLE_INCLUDE_DIR})
  endif()

  set(SKIA_LIBRARIES
    ${SKIA_LIBRARY}
    ${SKIA_OPENGL_LIBRARY}
    CACHE INTERNAL "Skia libraries")

  list(APPEND SHE_SOURCES
    skia/skia_display.cpp
    skia/skia_surface.cpp
    skia/she.cpp)

  if(WIN32)
    list(APPEND SHE_SOURCES
      skia/skia_window_win.cpp
      win/keys.cpp
      win/pen.cpp
      win/winapi.cpp
      win/window.cpp
      win/window_dde.cpp)
  elseif(APPLE)
    list(APPEND SHE_SOURCES
      osx/app.mm
      osx/app_delegate.mm
      osx/event_queue.mm
      osx/keys.mm
      osx/view.mm
      osx/window.mm
      skia/skia_window_osx.mm)
  else()
    list(APPEND SHE_SOURCES
      skia/skia_window_x11.cpp
      x11/event_queue.cpp
      x11/keys.cpp
      x11/window.cpp
      x11/x11.cpp)
  endif()
endif()

######################################################################
# None backend

if(USE_NONE_BACKEND)
  list(APPEND SHE_SOURCES
    none/she.cpp)
endif()

######################################################################

if(WIN32)
  list(APPEND SHE_SOURCES
    win/native_dialogs.cpp)
endif()

if(APPLE)
  list(APPEND SHE_SOURCES
    osx/logger.mm
    osx/menus.mm
    osx/native_dialogs.mm)
endif()

if(WITH_GTK_FILE_DIALOG_SUPPORT AND UNIX AND NOT APPLE AND NOT BEOS)
  # Find gtkmm library
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK gtk+-3.0)
  include_directories(${GTK_INCLUDE_DIRS})
  link_directories(${GTK_LIBRARY_DIRS})

  add_definitions(-DASEPRITE_WITH_GTK_FILE_DIALOG_SUPPORT)
  list(APPEND SHE_SOURCES
    gtk/native_dialogs.cpp)
endif()

add_library(she ${SHE_SOURCES})

target_link_libraries(she
  ft-lib
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
    ${SKIA_LIBRARIES}
    ${X11_LIBRARIES})

  if(UNIX AND NOT APPLE)
    target_link_libraries(she fontconfig Xcursor)
  endif()
endif()

if(WITH_GTK_FILE_DIALOG_SUPPORT)
  target_link_libraries(she
    ${GTKMM_LIBRARIES})
endif()
