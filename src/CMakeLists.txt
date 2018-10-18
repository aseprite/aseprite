# Aseprite
# Copyright (C) 2001-2018  David Capello

######################################################################
# Compiler-specific flags

if(UNIX)
  # All warnings except for switch cases with missing enum items
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-switch")
endif()

if(MSVC)
  # As Skia is compiled with /GL flag (whole program optimization),
  # the linker prefer a /LTCG parameter to improve link times.
  if(LAF_OS_BACKEND STREQUAL "skia")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
  endif()

  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
endif(MSVC)

if(ENABLE_NEWS)
  add_definitions(-DENABLE_NEWS)
endif()

if(ENABLE_UPDATER)
  add_definitions(-DENABLE_UPDATER)
endif()

if(WIN32)
  # Needed to include icons in win32 .rc file
  include_directories(..)
endif()

######################################################################
# Custom Aseprite website (for testing server-side)

if(NOT "${CUSTOM_WEBSITE_URL}" STREQUAL "")
  add_definitions(-DCUSTOM_WEBSITE_URL="${CUSTOM_WEBSITE_URL}")
endif()

######################################################################
# With static libcurl

if(REQUIRE_CURL AND NOT USE_SHARED_CURL AND CURL_STATICLIB)
  add_definitions(-DCURL_STATICLIB)
endif()

######################################################################
# Special versions (full/trial, devmode, UI/CLI, etc.)

if(NOT ENABLE_TRIAL_MODE)
  add_definitions(-DENABLE_SAVE)
else()
  add_definitions(-DENABLE_TRIAL_MODE)
endif()

if(ENABLE_DEVMODE)
  add_definitions(-DENABLE_DEVMODE)
endif()

if(ENABLE_UI)
  add_definitions(-DENABLE_UI)
endif()

if(ENABLE_UI AND NOT ENABLE_TRIAL_MODE)
  set(ENABLE_DATA_RECOVERY on)
  add_definitions(-DENABLE_DATA_RECOVERY)
else()
  set(ENABLE_DATA_RECOVERY off)
endif()

if(ENABLE_SCRIPTING)
  # Needed for "app" and "main"
  add_definitions(-DENABLE_SCRIPTING)
endif()

######################################################################
# Aseprite Libraries (in preferred order to be built)

# Disable observable tests
set(OBSERVABLE_TESTS OFF CACHE BOOL "Compile observable tests")
add_subdirectory(observable)
include_directories(observable)

# Disable clip examples and tests
set(CLIP_EXAMPLES OFF CACHE BOOL "Compile clip examples")
set(CLIP_TESTS OFF CACHE BOOL "Compile clip tests")
set(CLIP_X11_PNG_LIBRARY "${PNG_LIBRARY}")
add_subdirectory(clip)

# Disable undo tests
set(UNDO_TESTS OFF CACHE BOOL "Compile undo tests")
add_subdirectory(undo)

add_subdirectory(cfg)
add_subdirectory(doc)
add_subdirectory(filters)
add_subdirectory(fixmath)
add_subdirectory(flic)
add_subdirectory(render)
add_subdirectory(dio)
add_subdirectory(ui)

if(REQUIRE_CURL)
  add_subdirectory(net)
endif()

if(GEN_EXE)
  add_executable(gen IMPORTED)
  set_target_properties(gen PROPERTIES IMPORTED_LOCATION ${GEN_EXE})
  set(GEN_DEP)
else()
  add_subdirectory(gen)
  set(GEN_EXE ${CMAKE_BINARY_DIR}/bin/gen)
  set(GEN_DEP gen)
endif()

if(ENABLE_UPDATER)
  add_subdirectory(updater)
endif()

if(ENABLE_STEAM)
  add_subdirectory(steam)
endif()

if(WITH_DESKTOP_INTEGRATION)
  add_subdirectory(desktop)
endif()

add_subdirectory(app)

######################################################################
# Copy data/ directory target

file(GLOB_RECURSE src_data_files
  RELATIVE ${SOURCE_DATA_DIR}/ "${SOURCE_DATA_DIR}/*.*")
foreach(fn ${src_data_files})
  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/bin/data/${fn}
    COMMAND ${CMAKE_COMMAND} -E copy ${SOURCE_DATA_DIR}/${fn} ${CMAKE_BINARY_DIR}/bin/data/${fn}
    MAIN_DEPENDENCY ${SOURCE_DATA_DIR}/${fn})
  list(APPEND out_data_files ${CMAKE_BINARY_DIR}/bin/data/${fn})
endforeach()

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/bin/data/README.md
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../README.md ${CMAKE_BINARY_DIR}/bin/data/README.md
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/../README.md)
list(APPEND out_data_files ${CMAKE_BINARY_DIR}/bin/data/README.md)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/bin/data/EULA.txt
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../EULA.txt ${CMAKE_BINARY_DIR}/bin/data/EULA.txt
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/../EULA.txt)
list(APPEND out_data_files ${CMAKE_BINARY_DIR}/bin/data/EULA.txt)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/bin/data/docs/LICENSES.md
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../docs/LICENSES.md ${CMAKE_BINARY_DIR}/bin/data/docs/LICENSES.md
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/../docs/LICENSES.md)
list(APPEND out_data_files ${CMAKE_BINARY_DIR}/bin/data/docs/LICENSES.md)

add_custom_target(copy_data DEPENDS ${out_data_files})

######################################################################
# Aseprite application

if(ENABLE_ASEPRITE_EXE)
  if(WIN32)
    set(win32_resources
      main/resources_win32.rc
      main/settings.manifest)
  endif()

  add_executable(aseprite main/main.cpp ${win32_resources})
  if(WIN32 AND ENABLE_UI)
    set_target_properties(aseprite PROPERTIES WIN32_EXECUTABLE true)
  endif()
  target_link_libraries(aseprite app-lib)
  add_dependencies(aseprite copy_data)

  if(LAF_OS_BACKEND_LINK_FLAGS)
    set_target_properties(aseprite PROPERTIES LINK_FLAGS
      ${LAF_OS_BACKEND_LINK_FLAGS})
  endif()

  install(TARGETS aseprite
    RUNTIME DESTINATION bin)

  install(DIRECTORY ../data
    DESTINATION share/aseprite)
endif()

######################################################################
# Tests

if(ENABLE_TESTS)
  include(FindTests)
  find_tests(doc doc-lib)
  find_tests(render render-lib)
  find_tests(ui ui-lib)
  find_tests(app/cli app-lib)
  find_tests(app/file app-lib)
  find_tests(app app-lib)
  find_tests(. app-lib)
endif()

if(ENABLE_BENCHMARKS)
  include(FindBenchmarks)
  find_benchmarks(doc doc-lib)
endif()
