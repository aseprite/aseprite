# Aseprite
# Copyright (C) 2001-2015  David Capello

######################################################################
# Common definitions for all Aseprite libraries/projects

add_definitions(-DHAVE_CONFIG_H)

if(ENABLE_MEMLEAK)
  add_definitions(-DMEMLEAK)
endif()

######################################################################
# Compiler-specific flags

if(MSVC)
  # Do not link with libcmt.lib (to avoid duplicated symbols with msvcrtd.lib)
  if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT")
  endif()

  if (CMAKE_CL_64)
    # 64 bits
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MACHINE:X64")
  else()
    # Add support for Windows XP with 5.01 subsystem
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MACHINE:X86 /SUBSYSTEM:WINDOWS,5.01")
  endif()

  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
endif(MSVC)

if(ENABLE_UPDATER)
  set(updater_lib updater-lib)
  add_definitions(-DENABLE_UPDATER)
else()
  set(updater_lib)
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

add_subdirectory(base)

# Directory where base/config.h file is located
include_directories(${BASE_INCLUDE_DIR})

add_subdirectory(cfg)
add_subdirectory(flic)
add_subdirectory(css)
add_subdirectory(doc)
add_subdirectory(render)
add_subdirectory(filters)
add_subdirectory(fixmath)
add_subdirectory(gen)
add_subdirectory(gfx)
add_subdirectory(scripting)
add_subdirectory(she)
add_subdirectory(ui)
add_subdirectory(undo)
add_subdirectory(net)

if(ENABLE_UPDATER)
  add_subdirectory(updater)
endif()

if(ENABLE_WEBSERVER)
  add_subdirectory(webserver)
endif()

add_subdirectory(app)

if(USE_SKIA_BACKEND)
  list(APPEND PLATFORM_LIBS ${SKIA_LIBRARIES})
endif()

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
  set(win32_resources main/resources_win32.rc)
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

install(TARGETS aseprite
  RUNTIME DESTINATION bin)

install(DIRECTORY ../data
  DESTINATION share/aseprite)

######################################################################
# Tests

function(find_tests dir dependencies)
  file(GLOB tests ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/*_tests.cpp)
  list(REMOVE_AT ARGV 0)

  # Add gtest include directory so we can #include <gtest/gtest.h> in tests source code
  include_directories(${CMAKE_SOURCE_DIR}/third_party/gtest/include)

  # See if the test is linked with "she" library.
  list(FIND dependencies she link_with_she)
  if(link_with_she)
    set(extra_definitions -DLINKED_WITH_SHE)
  endif()

  foreach(testsourcefile ${tests})
    get_filename_component(testname ${testsourcefile} NAME_WE)

    add_executable(${testname} ${testsourcefile})

    if(MSVC)
      # Fix problem compiling gen from a Visual Studio solution
      set_target_properties(${testname}
	PROPERTIES LINK_FLAGS -ENTRY:"mainCRTStartup")
    endif()

    target_link_libraries(${testname} gtest ${ARGV} ${PLATFORM_LIBS})

    if(extra_definitions)
      set_target_properties(${testname}
	PROPERTIES COMPILE_FLAGS ${extra_definitions})
    endif()

    add_custom_target(run_${testname}
      COMMAND ${testname}
      DEPENDS ${testname})

    set(local_runs ${local_runs} run_${testname})

    string(REGEX MATCH "_ui_tests" test_requires_ui ${testname})
    if (NOT test_requires_ui STREQUAL "_ui_tests")
      set(local_non_ui_runs ${local_non_ui_runs} run_${testname})
    endif()
  endforeach()
  set(all_runs ${all_runs} ${local_runs} PARENT_SCOPE)
  set(non_ui_runs ${non_ui_runs} ${local_non_ui_runs} PARENT_SCOPE)
endfunction()

find_tests(base base-lib)
find_tests(undo undo-lib)
find_tests(gfx gfx-lib)
find_tests(doc doc-lib)
find_tests(render render-lib)
find_tests(css css-lib)
find_tests(ui ui-lib)
find_tests(app/file app-lib)
find_tests(app app-lib)
find_tests(. app-lib)

# To run tests
add_custom_target(run_all_tests DEPENDS ${all_runs})
add_custom_target(run_non_ui_tests DEPENDS ${non_ui_runs})
