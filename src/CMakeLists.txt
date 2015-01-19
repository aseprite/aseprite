# Aseprite
# Copyright (C) 2001-2015  David Capello

add_definitions(-DHAVE_CONFIG_H)

######################################################################
# Compiler-specific flags

if(MSVC)
  # Do not link with libcmt.lib (to avoid duplicated symbols with msvcrtd.lib)
  if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT")
  endif()

  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64 bits
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MACHINE:X64")
  else()
    # Add support for Windows XP with 5.01 subsystem
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MACHINE:X86 /SUBSYSTEM:WINDOWS,5.01")
  endif()
endif(MSVC)

# Libraries in this directory
set(aseprite_libraries
  app-lib
  fixmath-lib
  cfg-lib
  css-lib
  doc-lib
  render-lib
  scripting-lib
  undo-lib
  filters-lib
  ui-lib
  she
  gfx-lib
  base-lib)

# Directories where .h files can be found
include_directories(. .. ../third_party)

# Third-party libraries
if(USE_SHARED_ALLEGRO4)
  # Find the shared Allegro 4 library
  find_library(LIBALLEGRO4_LIBRARY alleg)
  find_path(LIBALLEGRO4_INCLUDE_DIR allegro.h)

  if(NOT LIBALLEGRO4_LIBRARY)
    message(FATAL_ERROR "Allegro 4 not found")
  endif()

  # Get flags to link programs using allegro-config program
  execute_process(COMMAND allegro-config --libs --shared
    OUTPUT_VARIABLE LIBALLEGRO4_LINK_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  set(LIBALLEGRO4_LINK_FLAGS ${LIBALLEGRO4_LINK_FLAGS} ${PLATFORM_LIBS})
  include_directories(${LIBALLEGRO4_INCLUDE_DIR})
else()
  add_definitions(-DALLEGRO4_WITH_RESIZE_PATCH)
  add_definitions(-DALLEGRO4_WITH_EXTRA_CURSORS)

  # Use patched version of Allegro 4 (with window resize support).
  add_subdirectory(allegro)
  set(libs3rdparty ${libs3rdparty} allegro)
endif()

if(USE_SHARED_JPEGLIB)
  find_package(JPEG)
  if(JPEG_FOUND)
    set(libs3rdparty ${libs3rdparty} ${JPEG_LIBRARIES})
    include_directories(${JPEG_INCLUDE_DIR})
  endif(JPEG_FOUND)
else()
  set(libs3rdparty ${libs3rdparty} jpeg)
  include_directories(${LIBJPEG_DIR})
endif()

if(USE_SHARED_ZLIB)
  find_package(ZLIB)
  if(ZLIB_FOUND)
    set(libs3rdparty ${libs3rdparty} ${ZLIB_LIBRARIES})
    include_directories(${ZLIB_INCLUDE_DIR})
  endif(ZLIB_FOUND)
else()
  set(libs3rdparty ${libs3rdparty} zlib)
  include_directories(${ZLIB_DIR})
endif()

if(USE_SHARED_LIBPNG)
  find_package(PNG)
  if(PNG_FOUND)
    set(libs3rdparty ${libs3rdparty} ${PNG_LIBRARIES})
    add_definitions(${PNG_DEFINITIONS})
    include_directories(${PNG_INCLUDE_DIR})
  endif(PNG_FOUND)
else()
  set(libs3rdparty ${libs3rdparty} libpng)
endif()

if(USE_SHARED_GIFLIB)
  find_package(GIF REQUIRED)
  if(GIF_FOUND)
    set(libs3rdparty ${libs3rdparty} ${GIF_LIBRARIES})
    include_directories(${GIF_INCLUDE_DIR})
  endif(GIF_FOUND)
else()
  set(libs3rdparty ${libs3rdparty} giflib)
  include_directories(${GIFLIB_DIR}/lib)
endif()

if(USE_SHARED_TINYXML)
  find_library(LIBTINYXML_LIBRARY NAMES tinyxml)
  find_path(LIBTINYXML_INCLUDE_DIR NAMES tinyxml.h)

  set(libs3rdparty ${libs3rdparty} ${LIBTINYXML_LIBRARY})
  include_directories(${LIBTINYXML_INCLUDE_DIR})
else()
  set(libs3rdparty ${libs3rdparty} tinyxml)
  include_directories(${TINYXML_DIR})
endif()

if(USE_SHARED_LIBLOADPNG)
  find_library(LIBLOADPNG_LIBRARY NAMES loadpng)
  find_path(LIBLOADPNG_INCLUDE_DIR NAMES loadpng.h)

  set(libs3rdparty ${libs3rdparty} ${LIBLOADPNG_LIBRARY})
  include_directories(${LIBLOADPNG_INCLUDE_DIR})
else()
  set(libs3rdparty loadpng ${libs3rdparty})
  include_directories(${LOADPNG_DIR})
endif()

if(USE_SHARED_PIXMAN)
  find_library(LIBPIXMAN_LIBRARY NAMES pixman)
  find_path(LIBPIXMAN_INCLUDE_DIR NAMES pixman.h)

  set(libs3rdparty ${libs3rdparty} ${LIBPIXMAN_LIBRARY})
  include_directories(${LIBPIXMAN_INCLUDE_DIR})
else()
  set(libs3rdparty ${libs3rdparty} pixman)
  include_directories(${PIXMAN_DIR}/pixman)
  include_directories(${CMAKE_BINARY_DIR})
endif()

set(libs3rdparty simpleini ${libs3rdparty})
include_directories(${SIMPLEINI_DIR})

######################################################################
# Add C++11 support only for our code (avoid Allegro)

if(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-switch -std=gnu++11")
endif()

######################################################################
# Updater

if (CMAKE_USE_PTHREADS_INIT)
  set(sys_libs ${sys_libs} ${CMAKE_THREAD_LIBS_INIT})
endif()

if(NOT "${CUSTOM_WEBSITE_URL}" STREQUAL "")
  add_definitions(-DCUSTOM_WEBSITE_URL="${CUSTOM_WEBSITE_URL}")
endif()

if(ENABLE_UPDATER)
  if(USE_SHARED_CURL)
    find_library(LIBCURL_LIBRARY NAMES curl)
    find_path(LIBCURL_INCLUDE_DIR NAMES curl/curl.h)

    set(libs3rdparty ${libs3rdparty} ${LIBCURL_LIBRARY})
    include_directories(${LIBCURL_INCLUDE_DIR})
  else()
    set(libs3rdparty ${libs3rdparty} libcurl)
    include_directories(${CURL_DIR}/include)
    if(CURL_STATICLIB)
      add_definitions(-DCURL_STATICLIB)
    endif()
  endif()

  set(aseprite_libraries ${aseprite_libraries} updater-lib net-lib)
  add_definitions(-DENABLE_UPDATER)

  add_subdirectory(net)
  add_subdirectory(updater)
endif()

if(ENABLE_WEBSERVER)
  set(libs3rdparty ${libs3rdparty} mongoose)
  include_directories(${MONGOOSE_DIR})

  set(aseprite_libraries ${aseprite_libraries} webserver-lib)
  add_definitions(-DENABLE_WEBSERVER)

  add_subdirectory(webserver)
endif()

# Full-version or trial-mode?
if(NOT ENABLE_TRIAL_MODE)
  add_definitions(-DENABLE_SAVE)
else()
  add_definitions(-DENABLE_TRIAL_MODE)
endif()

######################################################################
# Aseprite Libraries (in preferred order to be built)

add_subdirectory(base)
add_subdirectory(cfg)
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

add_subdirectory(app)

if(V8_FOUND)
list(APPEND libs3rdparty ${V8_LIBRARIES})
endif()

# All libraries for .exe files
set(all_libs ${aseprite_libraries} ${libs3rdparty} ${sys_libs})

if(LIBALLEGRO4_LINK_FLAGS)
  set(all_libs ${all_libs} ${LIBALLEGRO4_LINK_FLAGS})
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
endif(WIN32)

if(UNIX)
  set(x11_resources main/xpm_icon.c)
endif(UNIX)

add_executable(aseprite WIN32
  main/main.cpp
  ${win32_resources}
  ${x11_resources})
target_link_libraries(aseprite ${all_libs})
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

  if(NOT USE_SHARED_GTEST)
    # Add gtest include directory so we can #include <gtest/gtest.h> in tests source code
    include_directories(${CMAKE_SOURCE_DIR}/third_party/gtest/include)
  endif()

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

    target_link_libraries(${testname} gtest ${ARGV})
    if(LIBALLEGRO4_LINK_FLAGS)
      target_link_libraries(${testname} ${LIBALLEGRO4_LINK_FLAGS})
    endif()

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

find_tests(base base-lib ${sys_libs})
find_tests(undo undo-lib ${sys_libs})
find_tests(gfx gfx-lib base-lib ${libs3rdparty} ${sys_libs})
find_tests(doc doc-lib gfx-lib base-lib ${libs3rdparty} ${sys_libs})
find_tests(render render-lib doc-lib gfx-lib base-lib ${libs3rdparty} ${sys_libs})
find_tests(css css-lib gfx-lib base-lib ${libs3rdparty} ${sys_libs})
find_tests(ui ui-lib she gfx-lib base-lib ${libs3rdparty} ${sys_libs})
find_tests(app/file ${all_libs})
find_tests(app ${all_libs})
find_tests(. ${all_libs})

# To run tests
add_custom_target(run_all_tests DEPENDS ${all_runs})
add_custom_target(run_non_ui_tests DEPENDS ${non_ui_runs})
