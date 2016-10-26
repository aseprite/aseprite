# Aseprite
# Copyright (C) 2001-2016  David Capello

######################################################################
# Common definitions for all Aseprite libraries/projects

add_definitions(-DHAVE_CONFIG_H)

if(ENABLE_MEMLEAK)
  add_definitions(-DMEMLEAK)
endif()

######################################################################
# Compiler-specific flags

if(MSVC)
  # As Skia is compiled with /GL flag (whole program optimization),
  # the linker prefer a /LTCG parameter to improve link times.
  if(USE_SKIA_BACKEND)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
  endif()

  # Do not link with libcmt.lib (to avoid duplicated symbols with msvcrtd.lib)
  if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT")
  endif()

  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
endif(MSVC)

if(ENABLE_UPDATER)
  add_definitions(-DENABLE_UPDATER)
endif()

# Directories where .h files can be found
include_directories(. .. ../third_party)

# Use patched version of Allegro 4 (with window resize support).
if(USE_ALLEG4_BACKEND AND NOT USE_SHARED_ALLEGRO4)
  add_subdirectory(allegro)
endif()

######################################################################
# Add C++11 support only for our code (avoid Allegro)

if(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-switch -std=gnu++11")
endif()

######################################################################
# pthread

if (CMAKE_USE_PTHREADS_INIT)
  list(APPEND PLATFORM_LIBS ${CMAKE_THREAD_LIBS_INIT})
endif()

######################################################################
# Custom Aseprite website (for testing server-side)

if(NOT "${CUSTOM_WEBSITE_URL}" STREQUAL "")
  add_definitions(-DCUSTOM_WEBSITE_URL="${CUSTOM_WEBSITE_URL}")
endif()

######################################################################
# Full-version or trial-mode?

if(NOT USE_SHARED_CURL AND CURL_STATICLIB)
  add_definitions(-DCURL_STATICLIB)
endif()

######################################################################
# Full-version or trial-mode?

if(NOT ENABLE_TRIAL_MODE)
  add_definitions(-DENABLE_SAVE -DENABLE_DATA_RECOVERY)
else()
  add_definitions(-DENABLE_TRIAL_MODE)
endif()

######################################################################
# Aseprite Libraries (in preferred order to be built)

# Disable observable tests
set(OBSERVABLE_TESTS OFF CACHE BOOL "Compile observable tests")
add_subdirectory(observable)
include_directories(observable)

# Disable clip examples
set(CLIP_EXAMPLES OFF CACHE BOOL "Compile clip examples")
add_subdirectory(clip)

# Disable undo tests
set(UNDO_TESTS OFF CACHE BOOL "Compile undo tests")
add_subdirectory(undo)

# Our base library
add_subdirectory(base)

# Directory where base/config.h file is located
include_directories(${BASE_INCLUDE_DIR})

add_subdirectory(cfg)
add_subdirectory(css)
add_subdirectory(doc)
add_subdirectory(filters)
add_subdirectory(fixmath)
add_subdirectory(flic)
add_subdirectory(gen)
add_subdirectory(gfx)
add_subdirectory(net)
add_subdirectory(render)
add_subdirectory(docio)
add_subdirectory(script)
add_subdirectory(she)
add_subdirectory(ui)

if(ENABLE_UPDATER)
  add_subdirectory(updater)
endif()

if(ENABLE_WEBSERVER)
  add_subdirectory(webserver)
endif()

if(ENABLE_STEAM)
  add_subdirectory(steam)
endif()

add_subdirectory(app)

######################################################################
# Copy data/ directory target

file(GLOB_RECURSE src_data_files
  RELATIVE ${CMAKE_SOURCE_DIR}/data/ "${CMAKE_SOURCE_DIR}/data/*.*")
foreach(fn ${src_data_files})
  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/bin/data/${fn}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/data/${fn} ${CMAKE_BINARY_DIR}/bin/data/${fn}
    MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/data/${fn})
  list(APPEND out_data_files ${CMAKE_BINARY_DIR}/bin/data/${fn})
endforeach()

add_custom_target(copy_data DEPENDS ${out_data_files})

######################################################################
# Aseprite application

if(WIN32)
  set(win32_resources
    main/resources_win32.rc
    main/settings.manifest)
endif()

if(UNIX AND NOT APPLE AND USE_ALLEG4_BACKEND)
  set(x11_resources main/xpm_icon.c)
endif()

add_executable(aseprite WIN32
  main/main.cpp
  ${win32_resources}
  ${x11_resources})
target_link_libraries(aseprite app-lib ${PLATFORM_LIBS})
add_dependencies(aseprite copy_data)

if(MSVC AND USE_SKIA_BACKEND)
  # Add support to expand filename wildcards in argc/argv
  set_target_properties(aseprite
    PROPERTIES LINK_FLAGS "-LINK wsetargv.obj -ENTRY:\"wWinMainCRTStartup\"")
endif()

install(TARGETS aseprite
  RUNTIME DESTINATION bin)

install(DIRECTORY ../data
  DESTINATION share/aseprite)

######################################################################
# Tests

if(ENABLE_TESTS)
  include(FindTests)

  find_tests(base base-lib)
  find_tests(gfx gfx-lib)
  find_tests(doc doc-lib)
  find_tests(render render-lib)
  find_tests(css css-lib)
  find_tests(ui ui-lib)
  find_tests(app/file app-lib)
  find_tests(app app-lib)
  find_tests(. app-lib)
endif()
