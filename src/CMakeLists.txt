# Aseprite
# Copyright (C) 2019-2024  Igara Studio S.A.
# Copyright (C) 2001-2018  David Capello

######################################################################
# Compiler-specific flags

if(UNIX)
  # All warnings except for switch cases with missing enum items
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-switch")

  # Prefer C++17 [[fallthrough]] attribute
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wimplicit-fallthrough")
endif()

if(MSVC)
  # As Skia is compiled with /GL flag (whole program optimization),
  # the linker prefer a /LTCG parameter to improve link times.
  if(LAF_OS_BACKEND STREQUAL "skia")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
  endif()

  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
  add_definitions(-wd4267) # disable warnings about signed/unsigned comparison
  add_definitions(-wd4244) # disable warnings about possible loss of data
else()
  # disable warnings about signed/unsigned comparison
  add_definitions(-Wno-sign-compare)
endif()

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

if(ENABLE_DRM)
  add_definitions(-DENABLE_DRM)
  add_definitions(-DENABLE_SAVE)
endif()

if(ENABLE_DEVMODE)
  add_definitions(-DENABLE_DEVMODE)
endif()

if(ENABLE_UI)
  add_definitions(-DENABLE_UI)
endif()

if(ENABLE_UI AND (NOT ENABLE_TRIAL_MODE OR ENABLE_DRM))
  set(ENABLE_DATA_RECOVERY on)
  add_definitions(-DENABLE_DATA_RECOVERY)
else()
  set(ENABLE_DATA_RECOVERY off)
endif()

if(ENABLE_SCRIPTING)
  # Needed for "app" and "main"
  add_definitions(-DENABLE_SCRIPTING)
endif()

if(ENABLE_WEBSOCKET)
  # Needed for "app" and "main"
  add_definitions(-DENABLE_WEBSOCKET)
endif()

######################################################################
# Aseprite Libraries (in preferred order to be built)

# Disable observable tests
set(OBSERVABLE_TESTS OFF CACHE BOOL "Compile observable tests")
add_subdirectory(observable)
include_directories(observable)

# Disable undo tests
set(UNDO_TESTS OFF CACHE BOOL "Compile undo tests")
add_subdirectory(undo)

add_subdirectory(cfg)
add_subdirectory(doc)
add_subdirectory(filters)
add_subdirectory(fixmath)
add_subdirectory(flic)
if(ENABLE_PSD)
  add_subdirectory(psd)
endif()
add_subdirectory(tga)
add_subdirectory(render)
add_subdirectory(dio)
add_subdirectory(ui)
add_subdirectory(ver)

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

if(ENABLE_DESKTOP_INTEGRATION)
  add_subdirectory(desktop)
endif()

add_subdirectory(app)

######################################################################
# Output bin/data/ directory where we are going to copy data files

set(DATA_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/data)

######################################################################
# Clone "strings" repo with translations into bin/data/strings.git

include(FetchContent)

find_package(Git)
if(GIT_FOUND)
  FetchContent_Declare(
    clone_strings
    GIT_REPOSITORY    https://github.com/aseprite/strings.git
    GIT_TAG           origin/main
    SOURCE_DIR        ${DATA_OUTPUT_DIR}/strings.git
    CONFIGURE_COMMAND ""
    BUILD_COMMAND     ""
    INSTALL_COMMAND   ""
    TEST_COMMAND      "")
  FetchContent_MakeAvailable(clone_strings)
  add_custom_target(clone_strings DEPENDS clone_strings)
else()
  add_custom_target(clone_strings)
endif()

######################################################################
# Copy data/ directory target into bin/data/

file(GLOB_RECURSE src_data_files
  RELATIVE ${SOURCE_DATA_DIR}/ "${SOURCE_DATA_DIR}/*.*")
foreach(fn ${src_data_files})
  add_custom_command(
    OUTPUT ${DATA_OUTPUT_DIR}/${fn}
    COMMAND ${CMAKE_COMMAND} -E copy ${SOURCE_DATA_DIR}/${fn} ${DATA_OUTPUT_DIR}/${fn}
    MAIN_DEPENDENCY ${SOURCE_DATA_DIR}/${fn})
  list(APPEND out_data_files ${DATA_OUTPUT_DIR}/${fn})
endforeach()

if(GIT_FOUND)
  # Copy original en.ini to strings.git/en.ini to keep it updated. We
  # have to manually sync the "en.ini" file in the "strings" repo from
  # the "aseprite" repo.
  add_custom_command(
    OUTPUT ${DATA_OUTPUT_DIR}/strings.git/en.ini
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SOURCE_DATA_DIR}/strings/en.ini ${DATA_OUTPUT_DIR}/strings.git/en.ini
    MAIN_DEPENDENCY ${SOURCE_DATA_DIR}/strings/en.ini)
  list(APPEND out_data_files ${DATA_OUTPUT_DIR}/strings.git/en.ini)
endif()

add_custom_command(
  OUTPUT ${DATA_OUTPUT_DIR}/README.md
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../README.md ${DATA_OUTPUT_DIR}/README.md
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/../README.md)
list(APPEND out_data_files ${DATA_OUTPUT_DIR}/README.md)

add_custom_command(
  OUTPUT ${DATA_OUTPUT_DIR}/EULA.txt
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../EULA.txt ${DATA_OUTPUT_DIR}/EULA.txt
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/../EULA.txt)
list(APPEND out_data_files ${DATA_OUTPUT_DIR}/EULA.txt)

add_custom_command(
  OUTPUT ${DATA_OUTPUT_DIR}/docs/LICENSES.md
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../docs/LICENSES.md ${DATA_OUTPUT_DIR}/docs/LICENSES.md
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/../docs/LICENSES.md)
list(APPEND out_data_files ${DATA_OUTPUT_DIR}/docs/LICENSES.md)

add_custom_target(copy_data DEPENDS clone_strings ${out_data_files})

######################################################################
# Aseprite application

if(ENABLE_ASEPRITE_EXE)
  set(main_target aseprite)

  if(WIN32)
    set(main_resources
      main/resources_win32.rc
      main/settings.manifest)
  endif()

  add_executable(${main_target}
    main/main.cpp
    ${main_resources})
  if(ENABLE_UI)
    if(WIN32)
      set_target_properties(${main_target} PROPERTIES WIN32_EXECUTABLE true)
    endif()
  endif()
  set_target_properties(${main_target} PROPERTIES LINK_FLAGS "${LAF_BACKEND_LINK_FLAGS}")
  target_link_libraries(${main_target} app-lib)
  add_dependencies(${main_target} copy_data)

  if(LAF_OS_BACKEND_LINK_FLAGS)
    set_target_properties(${main_target} PROPERTIES LINK_FLAGS
      ${LAF_OS_BACKEND_LINK_FLAGS})
  endif()

  install(TARGETS ${main_target}
    RUNTIME DESTINATION bin)

  install(DIRECTORY ../data
    DESTINATION share/aseprite)
endif()

######################################################################
# Tests

if(ENABLE_TESTS)
  include(FindTests)
  find_tests(doc doc-lib)
  find_tests(doc/algorithm doc-lib)
  find_tests(render render-lib)
  find_tests(ui ui-lib)
  find_tests(app/cli app-lib)
  find_tests(app/file app-lib)
  find_tests(app app-lib)
  find_tests(. app-lib)
endif()

if(ENABLE_BENCHMARKS)
  include(FindBenchmarks)
  find_benchmarks(app app-lib)
  find_benchmarks(doc doc-lib)
  find_benchmarks(doc/algorithm doc-lib)
  find_benchmarks(render render-lib)
endif()
