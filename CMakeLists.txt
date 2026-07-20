# Aseprite
# Copyright (C) 2018-present  Igara Studio S.A.
# Copyright (C) 2001-2018  David Capello

cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # We use -std=c++17 instead of -std=gnu++17 in macOS
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are:
        None Debug Release RelWithDebInfo Profile."
      FORCE)
endif()

set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_SOURCE_DIR}/laf/cmake/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/laf/cmake/cxx_flag_overrides.cmake)

# Aseprite project
project(aseprite C CXX)

# Check repository status
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/laf/CMakeLists.txt" OR
   NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/laf/clip/CMakeLists.txt")
  message(FATAL_ERROR
    "Your Aseprite repository looks incomplete. Please read the following "
    "instructions carefully:"
    "\n"
    "1) If you have downloaded Aseprite source using the "
    "\"Code > Download ZIP\" button from GitHub, you will not be able to "
    "compile Aseprite. You need to get the code with other methods (continue "
    "reading)."
    "\n"
    "2) You can correctly clone the GitHub repository with the "
    "\"--recursive\" argument:"
    "\n"
    "  git clone --recursive https://github.com/aseprite/aseprite.git"
    "\n"
    "3) If you have already cloned the repository and see this message, "
    "you still need to initialize submodules running the following command:"
    "\n"
    "  git submodule update --init --recursive"
    "\n"
    "4) Or if you want to download the full source code in a .zip file, "
    "go to the releases page and get the latest \"Aseprite-v1.x-Source.zip\":"
    "\n"
    "  https://github.com/aseprite/aseprite/releases"
    "\n"
    "More information in the building instructions:"
    "\n"
    "  https://github.com/aseprite/aseprite/blob/main/INSTALL.md#get-the-source-code")
endif()

# This is required for KDE/Qt destop integration, which sets
# BUILD_SHARED_LIBS to TRUE by default
set(BUILD_SHARED_LIBS off)

enable_testing()

######################################################################
# Options (these can be specified in cmake command line or modifying
# CMakeCache.txt)

option(USE_SHARED_CMARK   "Use your installed copy of cmark" off)
option(USE_SHARED_CURL    "Use your installed copy of curl" off)
option(USE_SHARED_GIFLIB  "Use your installed copy of giflib" off)
option(USE_SHARED_ZLIB    "Use your installed copy of zlib" off)
option(USE_SHARED_LIBPNG  "Use your installed copy of libpng" off)
option(USE_SHARED_TINYEXIF "Use your installed copy of TinyEXIF" off)
option(USE_SHARED_TINYXML "Use your installed copy of tinyxml" off)
option(USE_SHARED_PIXMAN  "Use your installed copy of pixman" off)
option(USE_SHARED_FREETYPE "Use shared FreeType library" off)
option(USE_SHARED_HARFBUZZ "Use shared HarfBuzz library" off)
option(ENABLE_ASEPRITE_EXE "Compile main Aseprite executable" on)
option(ENABLE_MEMLEAK      "Enable memory-leaks detector (only for developers)" off)
option(ENABLE_NEWS         "Enable the news in Home tab" on)
option(ENABLE_UPDATER      "Enable automatic check for updates" on)
option(ENABLE_SCRIPTING    "Compile with scripting support" on)
option(ENABLE_WEBSOCKET    "Compile with websocket support" on)
option(ENABLE_TESTS        "Compile unit tests" off)
option(ENABLE_BENCHMARKS   "Compile benchmarks" off)
option(ENABLE_TRIAL_MODE   "Compile the trial version" off)
option(ENABLE_DRM          "Compile the DRM-enabled version (e.g. for automatic updates)" off)
option(ENABLE_STEAM        "Compile with Steam library" off)
option(ENABLE_DEVMODE      "Compile vesion for developers" off)
option(ENABLE_I18N_STRINGS "Clone i18n strings repo (https://github.com/aseprite/strings) to bin/data/strings.git" off)
option(FULLSCREEN_PLATFORM "Enable fullscreen by default" off)
option(ENABLE_CLANG_TIDY   "Enable static analysis" off)
option(ENABLE_CCACHE       "Use CCache to improve recompilation speed (optional)" on)
option(ENABLE_SENTRY       "Use Sentry SDK to report crashes" off)
option(ENABLE_WEBP         "Enable support to load/save .webp files" on)
option(ENABLE_PSD          "Enable experimental support for .psd files" off)
option(ENABLE_DESKTOP_INTEGRATION  "Enable desktop integration modules" off)
option(ENABLE_QT_THUMBNAILER       "Enable kde5/qt5 thumnailer" off)
set(CUSTOM_WEBSITE_URL "" CACHE STRING "Enable custom local webserver to check updates")

if(ENABLE_SENTRY)
  set(SENTRY_DIR "" CACHE STRING "Sentry native location")
  set(SENTRY_DSN "" CACHE STRING "Sentry Data Source Name URL")
  set(SENTRY_ENV "development" CACHE STRING "Sentry environment (production, testing, development, etc.)")
endif()

if(ENABLE_NEWS OR ENABLE_UPDATER OR ENABLE_DRM)
  set(REQUIRE_CURL ON)
else()
  set(REQUIRE_CURL OFF)
endif()

# Default laf backend depends in the SKIA_DIR is specified (similar to
# laf/CMakeLists.txt logic)
if(NOT LAF_BACKEND)
  if(SKIA_DIR)
    set(LAF_BACKEND "skia")
  else()
    set(LAF_BACKEND "none")
  endif()
endif()

if(ENABLE_DRM AND NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/drm/CMakeLists.txt)
  message(FATAL_ERROR "Your Aseprite repository is incomplete, clone the drm repository")
endif()

######################################################################
# Profile build type

list(APPEND CMAKE_BUILD_TYPES Profile)
mark_as_advanced(
    CMAKE_C_FLAGS_PROFILE
    CMAKE_CXX_FLAGS_PROFILE
    CMAKE_EXE_LINKER_FLAGS_PROFILE)

if(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS_PROFILE "-pg"
        CACHE STRING "Profiling C flags")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE}"
        CACHE STRING "Profiling C++ flags")
    set(CMAKE_EXE_LINKER_FLAGS_PROFILE "-pg"
        CACHE STRING "Profiling linker flags")
endif()

if(MSVC)
    set(CMAKE_C_FLAGS_PROFILE "/MT /Zi /Ox /Gd"
        CACHE STRING "Profiling C flags")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE}"
        CACHE STRING "Profiling C++ flags")
    set(CMAKE_EXE_LINKER_FLAGS_PROFILE "/PROFILE /DEBUG"
        CACHE STRING "Profiling linker flags")
endif()

######################################################################
# Directories

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
# We need to specify the output for each configuration to make it work
# on Visual Studio solutions.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_PROFILE "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_PROFILE "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_PROFILE "${CMAKE_BINARY_DIR}/bin")

# "data/" directory
set(SOURCE_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")

# Search in the "cmake" directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(ENABLE_CCACHE)
  find_package(CCache)
  if(CCache_FOUND)
    # Use e.g. "ccache clang++" instead of "clang++"
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCache_EXECUTABLE}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK    "${CCache_EXECUTABLE}")
  endif()
endif()

# Put libraries into "lib".
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")

######################################################################
# Common definitions to compile all sources (app code and third party)

# Debug C/C++ flags
if(CMAKE_BUILD_TYPE STREQUAL Debug)
  add_definitions(-D_DEBUG)
else()
  add_definitions(-DNDEBUG)
endif()

######################################################################
# Third-party dependencies and laf library

if(ENABLE_MEMLEAK)
  add_definitions(-DLAF_MEMLEAK)
endif()

set(LAF_WITH_TESTS ${ENABLE_TESTS} CACHE BOOL "Enable LAF tests")
set(UNDO_TESTS     ${ENABLE_TESTS} CACHE BOOL "Enable undo tests")

include(cmake/FindBaseDeps.cmake)

if(ENABLE_DRM)
  add_subdirectory(drm)
endif()
add_subdirectory(laf)

include(cmake/FindDeps.cmake)

# Print paths to used libraries
message(STATUS "aseprite giflib: ${GIF_LIBRARY}")
message(STATUS "aseprite libwebp: ${WEBP_LIBRARIES}")
message(STATUS "aseprite libjpeg-turbo: ${LIBJPEG_TURBO_LIBRARY}")

# LAF libraries + Aseprite are compiled with config.h
target_include_directories(laf-base PUBLIC src)
target_compile_definitions(laf-base PUBLIC HAVE_CONFIG_H)

######################################################################
# Aseprite application

add_subdirectory(src)

######################################################################
# Using clang-tidy with cmake.
# Based on http://mariobadr.com/using-clang-tidy-with-cmake-36.html

if(ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE
    NAMES "clang-tidy"
    DOC "Path to clang-tidy executable")
  if(NOT CLANG_TIDY_EXE)
    message(STATUS "clang-tidy not found.")
  else()
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}")
  endif()

  if(CLANG_TIDY_EXE)
    set_target_properties(app-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(aseprite PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(cfg-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(clip PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(dio-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(doc-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(filters-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(fixmath-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(flic-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(gen PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(laf-base PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(laf-ft PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(laf-gfx PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(laf-os PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(net-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(obs PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(render-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(ui-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(undo PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    set_target_properties(updater-lib PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
  endif()
endif()
