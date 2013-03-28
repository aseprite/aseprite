#-----------------------------------------------------------------------------#
#
# Build options
#

# Set the project name.
project(ALLEGRO C CXX)

set(ALLEGRO_VERSION 4.4.2)
string(REGEX MATCH "^[0-9]+[.][0-9]+" ALLEGRO_SOVERSION ${ALLEGRO_VERSION})

# Mac OS X -compatibility_version.
# XXX doesn't seem to work
set(COMPAT_VERSION 4.4.0)

set(ALLEGRO_DLL_SHORTVER 44)

# Search in the `cmake' directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Lists of all the source files.
include(FileList)

# Our own CMake macros and functions.
include(Common)

#-----------------------------------------------------------------------------#
#
# Build options
#

# On some 64-bit platforms, libraries should be installed into `lib64'
# instead of `lib'.  Set this to 64 to do that.
set(LIB_SUFFIX "" CACHE STRING "Suffix for 'lib' directories, e.g. '64'")

option(MAGIC_MAIN "Enable magic main (Unix)" off)

option(WANT_FRAMEWORKS "Want frameworks on Mac OS X" off)
option(WANT_EMBED
    "Make frameworks embeddable in application bundles (Mac OS X)" on)

set(FRAMEWORK_INSTALL_PREFIX "/Library/Frameworks" CACHE STRING
    "Directory in which to install Mac OS X frameworks")

#-----------------------------------------------------------------------------#
#
# Platform checks
#

include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(FindPkgConfig)
include(TestBigEndian)

if(UNIX)
    test_big_endian(ALLEGRO_BIG_ENDIAN)
    if(NOT ALLEGRO_BIG_ENDIAN)
        set(ALLEGRO_LITTLE_ENDIAN 1)
    endif(NOT ALLEGRO_BIG_ENDIAN)

    check_include_files(dirent.h ALLEGRO_HAVE_DIRENT_H)
    check_include_files(inttypes.h ALLEGRO_HAVE_INTTYPES_H)

    check_include_files(linux/awe_voice.h ALLEGRO_HAVE_LINUX_AWE_VOICE_H)
    check_include_files(linux/input.h ALLEGRO_HAVE_LINUX_INPUT_H)
    # On some systems including linux/joystick.h without sys/types.h results
    # in conflicting definitions of fd_set.
    check_include_files("sys/types.h;linux/joystick.h" ALLEGRO_HAVE_LINUX_JOYSTICK_H)
    check_include_files(linux/soundcard.h ALLEGRO_HAVE_LINUX_SOUNDCARD_H)
    check_include_files(machine/soundcard.h ALLEGRO_HAVE_MACHINE_SOUNDCARD_H)
    check_include_files(soundcard.h ALLEGRO_HAVE_SOUNDCARD_H)
    check_include_files(sys/io.h ALLEGRO_HAVE_SYS_IO_H)
    check_include_files(sys/stat.h ALLEGRO_HAVE_SYS_STAT_H)
    check_include_files(sys/time.h ALLEGRO_HAVE_SYS_TIME_H)
    check_include_files(sys/soundcard.h ALLEGRO_HAVE_SYS_SOUNDCARD_H)
    check_include_files(sys/utsname.h ALLEGRO_HAVE_SYS_UTSNAME_H)

    check_function_exists(getexecname ALLEGRO_HAVE_GETEXECNAME)
    check_function_exists(memcmp ALLEGRO_HAVE_MEMCMP)
    check_function_exists(mkstemp ALLEGRO_HAVE_MKSTEMP)
    check_function_exists(mmap ALLEGRO_HAVE_MMAP)
    check_function_exists(mprotect ALLEGRO_HAVE_MPROTECT)
    check_function_exists(sched_yield ALLEGRO_HAVE_SCHED_YIELD)
    check_function_exists(stricmp ALLEGRO_HAVE_STRICMP)
    check_function_exists(strlwr ALLEGRO_HAVE_STRLWR)
    check_function_exists(strupr ALLEGRO_HAVE_STRUPR)
    check_function_exists(sysconf ALLEGRO_HAVE_SYSCONF)

    check_c_source_compiles("
        #include <sys/procfs.h>
        #include <sys/ioctl.h>
        int main(void) {
            struct prpsinfo psinfo;
            ioctl(0, PIOCPSINFO, &psinfo);
            return 0;
        }"
        ALLEGRO_HAVE_SV_PROCFS_H
        )
    check_c_source_compiles("
        #include <sys/procfs.h>
        int main(void) {
            struct prpsinfo psinfo;
            psinfo.pr_argc = 0;
            return 0;
        }"
        ALLEGRO_HAVE_PROCFS_ARGCV
        )

    check_c_source_compiles("
        #include <unistd.h>
        #include <sys/mman.h>
        int main(void) {
            void *x = MAP_FAILED;
        }"
        MAP_FAILED_DEFINED)
    if(NOT MAP_FAILED_DEFINED)
        set(MAP_FAILED "((void *) -1)")
    endif()

    check_c_source_runs("
        static int notsupported = 1;
        void test_ctor (void) __attribute__((constructor));
        void test_ctor (void) { notsupported = 0; }
        int main(void) { return (notsupported); }
        "
        ALLEGRO_USE_CONSTRUCTOR)

    find_library(RT_LIBRARY rt)
    check_c_source_compiles("
        #include <time.h>
        int main(void) {
            struct timespec new_time_ns;
            clock_gettime(CLOCK_MONOTONIC, &new_time_ns);
            return 0;
        }"
        ALLEGRO_HAVE_POSIX_MONOTONIC_CLOCK
        )

    if(MAGIC_MAIN)
        set(ALLEGRO_WITH_MAGIC_MAIN 1)
    endif(MAGIC_MAIN)

    # XXX const
    # XXX inline
    # XXX size_t
endif(UNIX)

check_include_files(stdint.h ALLEGRO_HAVE_STDINT_H)

#-----------------------------------------------------------------------------#
#
# Compiler and platform setup
#

option(STRICT_WARN "Halt at warnings" off)

set(INSTALL_PREFIX "")

if(CMAKE_COMPILER_IS_GNUCC)
    set(COMPILER_GCC 1)
    set(ALLEGRO_GCC 1)
    set(WFLAGS "-W -Wall -Wno-unused-parameter")
    set(WFLAGS_C_ONLY "-Wdeclaration-after-statement")
    if(STRICT_WARN)
        set(WFLAGS "${WFLAGS} -Werror -Wpointer-arith")
        set(WFLAGS_C_ONLY "${WFLAGS_C_ONLY} -Wmissing-declarations")
        set(WFLAGS_C_ONLY "${WFLAGS_C_ONLY} -Wstrict-prototypes")
    endif(STRICT_WARN)
endif(CMAKE_COMPILER_IS_GNUCC)

if(MINGW)
    set(ALLEGRO_MINGW32 1)

    # Guess MINGDIR from the value of CMAKE_C_COMPILER if it's not set.
    if("$ENV{MINGDIR}" STREQUAL "")
        string(REGEX REPLACE "/bin/[^/]*$" "" MINGDIR "${CMAKE_C_COMPILER}")
        message(STATUS "Guessed MinGW directory: ${MINGDIR}")
    else("$ENV{MINGDIR}" STREQUAL "")
        file(TO_CMAKE_PATH "$ENV{MINGDIR}" MINGDIR)
        message(STATUS "Using MINGDIR: ${MINGDIR}")
    endif("$ENV{MINGDIR}" STREQUAL "")

    # Search in MINGDIR for headers and libraries.
    set(CMAKE_PREFIX_PATH "${MINGDIR}")

    # Install to MINGDIR
    if(INSTALL_PREFIX STREQUAL "")
        set(CMAKE_INSTALL_PREFIX ${MINGDIR})
    else(INSTALL_PREFIX STREQUAL "")
        set(CMAKE_INSTALL_PREFIX ${INSTALL_PREFIX})
    endif(INSTALL_PREFIX STREQUAL "")

    message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
endif(MINGW)

if(MSVC)
    set(COMPILER_MSVC 1)
    set(ALLEGRO_MSVC 1)
    set(WFLAGS "/W3 -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE")
endif(MSVC)

if(WIN32)
    set(ALLEGRO_WINDOWS 1)
endif()

if(APPLE)
    set(ALLEGRO_MACOSX 1)
    set(ALLEGRO_DARWIN 1)
endif(APPLE)

if(UNIX AND NOT APPLE)
    set(ALLEGRO_UNIX 1)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WFLAGS} ${WFLAGS_C_ONLY}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WFLAGS}")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUGMODE=1")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUGMODE=1")

#-----------------------------------------------------------------------------#

# Not sure if we want to support disabling these any more.
set(ALLEGRO_COLOR8 1)
set(ALLEGRO_COLOR16 1)
set(ALLEGRO_COLOR24 1)
set(ALLEGRO_COLOR32 1)

set(ALLEGRO_NO_ASM 1)
# ALLEGRO_MMX left undefined
# ALLEGRO_SSE left undefined

#-----------------------------------------------------------------------------#
#
# Platform drivers
#

set(PLATFORM_SOURCES)

# -- Unix --

if(ALLEGRO_UNIX) # not MACOSX
    set(ALLEGRO_HAVE_LIBPTHREAD 1)
    list(APPEND PLATFORM_SOURCES ${ALLEGRO_SRC_UNIX_FILES})

    if(ALLEGRO_HAVE_POSIX_MONOTONIC_CLOCK)
        list(APPEND PLATFORM_LIBS ${RT_LIBRARY})
    endif(ALLEGRO_HAVE_POSIX_MONOTONIC_CLOCK)
endif(ALLEGRO_UNIX)

# -- X11 --

if(ALLEGRO_UNIX AND X11_FOUND)
  set(ALLEGRO_WITH_XWINDOWS 1)
endif()

if(ALLEGRO_WITH_XWINDOWS)
    list(APPEND PLATFORM_SOURCES ${ALLEGRO_SRC_X_FILES})

    if(X11_XShm_FOUND)
        set(ALLEGRO_XWINDOWS_WITH_SHM 1)
    endif()

    if(X11_Xcursor_FOUND)
        set(ALLEGRO_XWINDOWS_WITH_XCURSOR 1)
    endif()

    if(X11_Xpm_FOUND)
        set(ALLEGRO_XWINDOWS_WITH_XPM 1)
    endif()

    if(X11_xf86vmode_FOUND)
        set(ALLEGRO_XWINDOWS_WITH_XF86VIDMODE 1)
        list(APPEND PLATFORM_LIBS ${X11_Xxf86vm_LIB})
    endif()

    if(XIM_FOUND)
        set(ALLEGRO_XWINDOWS_WITH_XIM 1)
    endif(XIM_FOUND)

    if(XDGA_FOUND)
        set(ALLEGRO_XWINDOWS_WITH_XF86DGA2 1)
    endif()
endif(ALLEGRO_WITH_XWINDOWS)

# -- Windows --

if(WIN32)
    list(APPEND PLATFORM_SOURCES ${ALLEGRO_SRC_WIN_FILES})
endif(WIN32)

# -- Mac OS X --

if(ALLEGRO_MACOSX)
    list(APPEND PLATFORM_SOURCES ${ALLEGRO_SRC_MACOSX_FILES})
    list(APPEND PLATFORM_SOURCES src/macosx/main.m)
endif(ALLEGRO_MACOSX)

#-----------------------------------------------------------------------------#
#
# Generate and install headers
#

set(ALLEGRO_PUBLIC_HEADERS)

macro(add_headers location)
    list(APPEND ALLEGRO_PUBLIC_HEADERS ${ARGN})
    # We reuse MACOSX_PACKAGE_LOCATION on non-Mac platforms as well.
    set_source_files_properties(${ARGN}
        PROPERTIES
        MACOSX_PACKAGE_LOCATION Headers/${location}
        )
endmacro(add_headers)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/allegro/platform/alplatf.h.cmake
    ${CMAKE_BINARY_DIR}/include/allegro/platform/alplatf.h
    @ONLY
    )
add_headers(allegro/platform
    ${CMAKE_BINARY_DIR}/include/allegro/platform/alplatf.h
    )

if(UNIX)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/include/allegro/platform/alunixac.h.cmake
        ${CMAKE_BINARY_DIR}/include/allegro/platform/alunixac.h
        )
    add_headers(allegro/platform
        ${CMAKE_BINARY_DIR}/include/allegro/platform/alunixac.h
        )
endif(UNIX)

add_headers(""               include/allegro.h)
add_headers(allegro          ${ALLEGRO_INCLUDE_ALLEGRO_FILES})
add_headers(allegro/inline   ${ALLEGRO_INCLUDE_ALLEGRO_INLINE_FILES})
add_headers(allegro/internal ${ALLEGRO_INCLUDE_ALLEGRO_INTERNAL_FILES})
add_headers(allegro/platform ${ALLEGRO_INCLUDE_ALLEGRO_PLATFORM_FILES})

# Platform-specific top-level headers.
if(ALLEGRO_LINUX)
    add_headers("" include/linalleg.h)
endif(ALLEGRO_LINUX)
if(APPLE)
    add_headers("" include/osxalleg.h)
endif(APPLE)
if(WIN32)
    add_headers("" include/winalleg.h)
endif(WIN32)
if(ALLEGRO_WITH_XWINDOWS)
    add_headers("" include/xalleg.h)
endif(ALLEGRO_WITH_XWINDOWS)

#-----------------------------------------------------------------------------#
#
# Main library
#

# On Unix/Mac we don't ever use a -static postfix.
if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(BUILD_TYPE_SUFFIX -debug)
elseif(CMAKE_BUILD_TYPE STREQUAL Profile)
    set(BUILD_TYPE_SUFFIX -profile)
else()
    set(BUILD_TYPE_SUFFIX "")
endif()

if(APPLE)
    # CMake 2.6.2 onwards passes these flags automatically.
    if(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} STREQUAL 2.6 AND
            ${CMAKE_PATCH_VERSION} LESS 2)
        set(ALLEGRO_LINK_FLAGS
            "-compatibility_version ${COMPAT_VERSION} -current_version ${ALLEGRO_VERSION}")
    endif()
endif(APPLE)

add_our_library(allegro
    ${ALLEGRO_SRC_FILES}
    ${ALLEGRO_SRC_C_FILES}
    ${PLATFORM_SOURCES}
    ${ALLEGRO_PUBLIC_HEADERS}
    )

set(allegro_OUTPUT_NAME alleg)

set_target_properties(allegro
    PROPERTIES
    COMPILE_FLAGS "-DALLEGRO_SRC"
    LINK_FLAGS "${ALLEGRO_LINK_FLAGS}"
    OUTPUT_NAME ${allegro_OUTPUT_NAME}
    VERSION ${ALLEGRO_VERSION}
    SOVERSION ${ALLEGRO_SOVERSION}
    )

if(WIN32)
  # Add a -static postfix on the main library for Windows.
  # In earlier versions of Allegro this was "_s".
  set_target_properties(allegro
    PROPERTIES
    POSTFIX "-static"
    DEBUG_POSTFIX "-debug-static"
    PROFILE_POSTFIX "-profile-static"
    )
endif(WIN32)

set_our_framework_properties(allegro Allegro)

set(link_with ${PLATFORM_LIBS})
target_link_libraries(allegro ${link_with})

sanitize_cmake_link_flags(static_link_with ${link_with})
set_target_properties(allegro
    PROPERTIES
    static_link_with "${static_link_with}"
    )

if(WANT_FRAMEWORKS)
    add_our_library(alleg-main STATIC
        src/macosx/main.m
        )
    set_target_properties(alleg-main
        PROPERTIES
        OUTPUT_NAME alleg-main${BUILD_TYPE_SUFFIX}
        COMPILE_FLAGS -DALLEGRO_SRC
        )
    target_link_libraries(allegro alleg-main)
endif(WANT_FRAMEWORKS)

#-----------------------------------------------------------------------------#
