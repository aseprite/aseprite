# Aseprite
# Copyright (C) 2001-2014  David Capello

add_definitions(-DHAVE_CONFIG_H)

if(MSVC)
  # Do not link with libcmt.lib (to avoid duplicated symbols with msvcrtd.lib)
  if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT")
  endif()

  # Add support for Windows XP
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MACHINE:X86 /SUBSYSTEM:WINDOWS,5.01")
endif(MSVC)

# Libraries in this directory
set(aseprite_libraries app-lib css-lib doc-lib raster-lib
  scripting-lib undo-lib filters-lib ui-lib she gfx-lib base-lib)

# Directories where .h files can be found
include_directories(. .. ../third_party)

# Directory where generated files by "gen" utility will stay.
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Third-party libraries

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

  include_directories(${LIBALLEGRO4_INCLUDE_DIR})
else()
  # Use patched version of Allegro 4 (with window resize support).
  add_subdirectory(allegro)
  set(libs3rdparty ${libs3rdparty} allegro)
  add_definitions(-DALLEGRO4_WITH_RESIZE_PATCH)
endif()

######################################################################
# Aseprite Libraries (in preferred order to be built)

add_subdirectory(base)
add_subdirectory(css)
add_subdirectory(doc)
add_subdirectory(filters)
add_subdirectory(gen)
add_subdirectory(gfx)
add_subdirectory(raster)
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
# ASEPRITE application

if(WIN32)
  set(win32_resources main/resources_win32.rc)
endif(WIN32)

if(UNIX)
  set(x11_resources main/xpm_icon.c)
endif(UNIX)

add_executable(aseprite WIN32 main/main.cpp ${win32_resources} ${x11_resources})
target_link_libraries(aseprite ${all_libs})

install(TARGETS aseprite
  RUNTIME DESTINATION bin)

install(DIRECTORY ../data
  DESTINATION share/aseprite)

if(EXISTS ../docs/quickref.pdf)
  install(FILES ../docs/quickref.pdf
    DESTINATION share/aseprite/docs/quickref.pdf)
endif()

######################################################################
# Generate source files from widget XML files

file(GLOB widget_files ${CMAKE_SOURCE_DIR}/data/widgets/*.xml)
foreach(widget_file ${widget_files})
  get_filename_component(widget_name ${widget_file} NAME_WE)
  set(target_name generated_${widget_name})
  set(output_fn ${CMAKE_CURRENT_BINARY_DIR}/generated_${widget_name}.h)

  add_custom_command(
    OUTPUT ${output_fn}
    COMMAND gen/gen --input ${widget_file} --widgetid ${widget_name} > ${output_fn}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    MAIN_DEPENDENCY ${widget_file}
    DEPENDS gen)

  add_custom_target(${target_name} DEPENDS ${output_fn})

  set_source_files_properties(${target_name} PROPERTIES GENERATED TRUE)

  add_dependencies(app-lib ${target_name})
endforeach()

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
find_tests(gfx gfx-lib base-lib ${sys_libs})
find_tests(raster raster-lib gfx-lib base-lib ${libs3rdparty} ${sys_libs})
find_tests(css css-lib gfx-lib base-lib ${libs3rdparty} ${sys_libs})
find_tests(ui ui-lib she gfx-lib base-lib ${libs3rdparty} ${sys_libs})
find_tests(app/file ${all_libs})
find_tests(app ${all_libs})
find_tests(. ${all_libs})

# To run tests
add_custom_target(run_all_tests DEPENDS ${all_runs})
add_custom_target(run_non_ui_tests DEPENDS ${non_ui_runs})
