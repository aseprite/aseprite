# XXX build chm files
# XXX build devhelp files
# XXX resources (keyboard/language) - pretty useless except on DOS
# XXX install grabber.txt?
# XXX png plugin for grabber
# XXX what is fixbundle?
# XXX applehelp bundle?
# XXX projectbuilder application template?
# XXX allegro.m4?

# Ports that used to work which aren't supported yet:
#   Haiku/BeOS port
#   djgpp port
#   QNX port
#   BCC32
#   Watcom
#   DMC
#   PSP port?

#-----------------------------------------------------------------------------#
#
# CMake setup
#

cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are:
        None Debug Release RelWithDebInfo MinSizeRel Profile."
      FORCE)
endif()
# Restrict configuration types to the selected build type.
# Note: This needs to be done before the project command
set(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}" CACHE INTERNAL "internal")

#-----------------------------------------------------------------------------#
#
# Build options
#

# Set the project name.
project(ALLEGRO C CXX)

if(WIN32)
    if(MINGW)
        # As of CMake 2.6.2, support for building .rc files with mingw seems to
        # be broken (uses wrong command line syntax) so we have to set this
        # variable manually.
        set(CMAKE_RC_COMPILE_OBJECT
            "<CMAKE_RC_COMPILER> <FLAGS> -O coff -o <OBJECT> -i <SOURCE>")
        if(NOT CMAKE_RC_COMPILER OR "${CMAKE_RC_COMPILER}" STREQUAL "")
            string(REGEX REPLACE "gcc(.exe)?$" "windres"
                GUESS_RC_COMPILER ${CMAKE_C_COMPILER})
            find_program(CMAKE_RC_COMPILER ${GUESS_RC_COMPILER})
        endif()
    endif(MINGW)
    enable_language(RC OPTIONAL)
    message(STATUS "CMAKE_RC_COMPILER: ${CMAKE_RC_COMPILER}")
    if(NOT CMAKE_RC_COMPILER)
        message(FATAL_ERROR "Please set CMAKE_RC_COMPILER manually, e.g. *-windres.")
    endif()
endif(WIN32)

set(ALLEGRO_VERSION 4.4.2)
string(REGEX MATCH "^[0-9]+[.][0-9]+" ALLEGRO_SOVERSION ${ALLEGRO_VERSION})

# Mac OS X -compatibility_version.
# XXX doesn't seem to work
set(COMPAT_VERSION 4.4.0)

set(ALLEGRO_DLL_SHORTVER 44)

# Search in the `cmake' directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Search for C header files in these directories.
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR}/include)

# Put libraries into `lib'.
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

# Lists of all the source files.
include(FileList)

# Our own CMake macros and functions.
include(Common)

#-----------------------------------------------------------------------------#
#
# Build options
#

option(SHARED "Build shared library for main Allegro library" on)
set(BUILD_SHARED_LIBS ${SHARED})    # actual CMake variable

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
# Unix platform checks
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
    check_include_files(stdint.h ALLEGRO_HAVE_STDINT_H)
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

    # Hack to deal with Mac OS X 10.6.  NSQuickDrawView is not defined by
    # NSQuickDrawView.h when compiling in 64-bit mode, and 64-bit mode is the
    # default when compiling on Snow Leopard.
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL i386)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch i386")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch i386")
    endif()

    # The Mac port simply uses too many deprecated things.
    if(COMPILER_GCC)
        set(WFLAGS "${WFLAGS} -Wno-deprecated-declarations")
    endif(COMPILER_GCC)
endif(APPLE)

if(BEOS)
    if(CMAKE_SYSTEM_NAME STREQUAL Haiku)
        set(ALLEGRO_HAIKU 1)
    else(CMAKE_SYSTEM_NAME STREQUAL Haiku)
        set(ALLEGRO_BEOS 1)
    endif(CMAKE_SYSTEM_NAME STREQUAL Haiku)
    set(WFLAGS "-W -Wall -Wno-unused -Wno-multichar -Wno-ctor-dtor-privacy")
    set(WFLAGS_C_ONLY "")
endif(BEOS)

if(UNIX AND NOT APPLE AND NOT BEOS)
    set(ALLEGRO_UNIX 1)
endif()

if(PSP)
    set(ALLEGRO_PSP 1)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -G0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -G0 -fno-exceptions -fno-rtti")
    set(CMAKE_INSTALL_PREFIX ${PSP_PREFIX})
endif(PSP)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WFLAGS} ${WFLAGS_C_ONLY}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WFLAGS}")

if(NOT BUILD_SHARED_LIBS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DALLEGRO_STATICLINK")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DALLEGRO_STATICLINK")
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUGMODE=1")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUGMODE=1")

list(APPEND CMAKE_BUILD_TYPES Profile)
mark_as_advanced(
    CMAKE_C_FLAGS_PROFILE
    CMAKE_CXX_FLAGS_PROFILE
    CMAKE_EXE_LINKER_FLAGS_PROFILE
    )
if(COMPILER_GCC)
    set(CMAKE_C_FLAGS_PROFILE "-pg"
        CACHE STRING "profiling flags")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE}"
        CACHE STRING "profiling flags")
    set(CMAKE_EXE_LINKER_FLAGS_PROFILE "-pg"
        CACHE STRING "profiling flags")
endif(COMPILER_GCC)
if(COMPILER_MSVC)
    set(CMAKE_C_FLAGS_PROFILE "-Gd -Ox"
        CACHE STRING "profiling flags")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE}"
        CACHE STRING "profiling flags")
    set(CMAKE_EXE_LINKER_FLAGS_PROFILE "-profile"
        CACHE STRING "profiling flags")
endif(COMPILER_MSVC)

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
# Unix modules
#

option(WANT_MODULES "Use modules for drivers (Unix)" on)

if(WANT_MODULES AND ALLEGRO_UNIX)
    set(ALLEGRO_WITH_MODULES 1)
    set(ALLEGRO_MODULES_PATH
        ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}/allegro/${ALLEGRO_VERSION})
    install(FILES ${CMAKE_SOURCE_DIR}/modules.lst
            DESTINATION "${ALLEGRO_MODULES_PATH}"
            )
endif(WANT_MODULES AND ALLEGRO_UNIX)

function(add_our_module nm files)
    if(ALLEGRO_WITH_MODULES)
        add_library(${nm} MODULE ${files})
        set_target_properties(${nm}
            PROPERTIES
            PREFIX ""
            COMPILE_FLAGS -DALLEGRO_MODULE
            )
        target_link_libraries(${nm} ${ARGN})
        install(TARGETS ${nm}
                DESTINATION "${ALLEGRO_MODULES_PATH}"
                )
    endif(ALLEGRO_WITH_MODULES)
endfunction(add_our_module)

#-----------------------------------------------------------------------------#
#
# Platform drivers
#

set(PLATFORM_SOURCES)
set(PLATFORM_LIBS)              # always link with these libraries
set(PLATFORM_LIBS_NON_MODULES)  # only link with these if modules disabled
set(PLATFORM_LIBS_MODULES)      # only link with these if modules enabled

# -- Unix --

option(WANT_OSS "Build OSS support" on)
option(WANT_ALSA "Build ALSA support" on)
option(WANT_JACK "Build JACK support" on)
option(WANT_SGIAUDIO "Build SGI audio support" on)

if(ALLEGRO_UNIX) # not MACOSX
    find_package(Threads)
    if(NOT CMAKE_USE_PTHREADS_INIT)
        message(FATAL_ERROR "Unix port requires pthreads support.")
    endif()
    set(ALLEGRO_HAVE_LIBPTHREAD 1)
    list(APPEND PLATFORM_LIBS m ${CMAKE_THREAD_LIBS_INIT})
    list(APPEND PLATFORM_SOURCES ${ALLEGRO_SRC_UNIX_FILES})

    find_library(DLOPEN_LIBRARY dl)
    mark_as_advanced(DLOPEN_LIBRARY)
    if(DLOPEN_LIBRARY)
        list(APPEND PLATFORM_LIBS_MODULES ${DLOPEN_LIBRARY})
    endif()

    if(ALLEGRO_HAVE_POSIX_MONOTONIC_CLOCK)
        list(APPEND PLATFORM_LIBS ${RT_LIBRARY})
    endif(ALLEGRO_HAVE_POSIX_MONOTONIC_CLOCK)

    if(WANT_OSS)
        include(AllegroFindOSS)
        if(OSS_FOUND)
            set(ALLEGRO_WITH_OSSDIGI 1)
            set(ALLEGRO_WITH_OSSMIDI 1)
            include_directories(SYSTEM ${OSS_INCLUDE_DIR})
        endif(OSS_FOUND)
    endif(WANT_OSS)

    if(WANT_ALSA)
        pkg_check_modules(ALSA alsa)
        if(ALSA_FOUND)
            # ALSA 0.5 is beyond obsolete.
            set(ALLEGRO_ALSA_VERSION 9)
            set(ALLEGRO_WITH_ALSADIGI 1)
            set(ALLEGRO_WITH_ALSAMIDI 1)
            include_directories(SYSTEM ${ALSA_INCLUDE_DIRS})
            list(APPEND PLATFORM_LIBS_NON_MODULES ${ALSA_LIBRARIES})
            add_our_module(alleg-alsadigi src/unix/alsa9.c ${ALSA_LIBRARIES})
            add_our_module(alleg-alsamidi src/unix/alsamidi.c ${ALSA_LIBRARIES})
        endif(ALSA_FOUND)
    endif(WANT_ALSA)

    if(WANT_JACK)
        pkg_check_modules(JACK jack)
        if(JACK_FOUND)
            set(ALLEGRO_WITH_JACKDIGI 1)
            include_directories(SYSTEM ${JACK_INCLUDE_DIRS})
            list(APPEND PLATFORM_LIBS_NON_MODULES ${JACK_LIBRARIES})
            add_our_module(alleg-jack src/unix/jack.c ${JACK_LIBRARIES})
        endif(JACK_FOUND)
    endif(WANT_JACK)

    # This is untested.
    if(WANT_SGIAUDIO)
        check_library_exists(audio alOpenPort "" SGIAUDIO_FOUND)
        if(SGIAUDIO_FOUND)
            set(ALLEGRO_WITH_SGIALDIGI 1)
            list(APPEND PLATFORM_LIBS_NON_MODULES audio)
            add_our_module(alleg-sgialdigi src/unix/sgial.c audio)
        endif(SGIAUDIO_FOUND)
    endif(WANT_SGIAUDIO)

    # aRts is obsolete
    # esd is obsolete
endif(ALLEGRO_UNIX)

# -- X11 --

option(WANT_X11 "Want X11 support (Unix)" on)

if(ALLEGRO_UNIX AND WANT_X11)
    find_package(X11)
    if(X11_FOUND)
        set(ALLEGRO_WITH_XWINDOWS 1)
    endif()
endif()

if(ALLEGRO_WITH_XWINDOWS)
    include_directories(SYSTEM ${X11_INCLUDE_DIR})
    list(APPEND PLATFORM_SOURCES ${ALLEGRO_SRC_X_FILES})
    list(APPEND PLATFORM_LIBS ${X11_LIBRARIES})

    if(X11_XShm_FOUND)
        set(ALLEGRO_XWINDOWS_WITH_SHM 1)
        list(APPEND PLATFORM_LIBS ${X11_Xext_LIB})
    endif()

    if(X11_Xcursor_FOUND)
        set(ALLEGRO_XWINDOWS_WITH_XCURSOR 1)
        list(APPEND PLATFORM_LIBS ${X11_Xcursor_LIB})
    endif()

    if(X11_Xcursor_FOUND)
        set(ALLEGRO_XWINDOWS_WITH_XCURSOR 1)
        list(APPEND PLATFORM_LIBS ${X11_Xcursor_LIB})
    endif()

    if(X11_Xpm_FOUND)
        set(ALLEGRO_XWINDOWS_WITH_XPM 1)
        list(APPEND PLATFORM_LIBS ${X11_Xpm_LIB})
    endif()

    find_library(X11_Xxf86vm_LIB Xxf86vm ${X11_LIB_SEARCH_PATH})
    mark_as_advanced(X11_Xxf86vm_LIB)
    if(X11_xf86vmode_FOUND)
        set(ALLEGRO_XWINDOWS_WITH_XF86VIDMODE 1)
        list(APPEND PLATFORM_LIBS ${X11_Xxf86vm_LIB})
    endif()

    check_library_exists(X11 XOpenIM "${X11_LIB_SEARCH_PATH}" XIM_FOUND)
    if(XIM_FOUND)
        set(ALLEGRO_XWINDOWS_WITH_XIM 1)
    endif(XIM_FOUND)

    check_library_exists(Xxf86dga XDGAQueryExtension
        "${X11_LIB_SEARCH_PATH}" XDGA_FOUND)
    if(XDGA_FOUND)
        set(ALLEGRO_XWINDOWS_WITH_XF86DGA2 1)
        list(APPEND PLATFORM_LIBS_NON_MODULES Xxf86dga ${X11_LIBRARIES})
        add_our_module(alleg-dga2 src/x/xdga2.c Xxf86dga ${X11_LIBRARIES})
    endif()
endif(ALLEGRO_WITH_XWINDOWS)

# -- Linux console --

option(WANT_LINUX_CONSOLE "Want Linux console support (Linux)" off)
option(WANT_LINUX_VGA "Want Linux console VGA driver (Linux)" on)
option(WANT_LINUX_FBCON "Want Linux console fbcon driver (Linux)" on)
option(WANT_LINUX_SVGALIB "Want Linux console SVGAlib driver (Linux)" on)

if(WANT_LINUX_CONSOLE AND CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(ALLEGRO_LINUX 1)
    list(APPEND PLATFORM_SOURCES ${ALLEGRO_SRC_LINUX_FILES})

    if(WANT_LINUX_VGA)
        set(ALLEGRO_LINUX_VGA 1)
        add_our_module(alleg-vga src/misc/vga.c)
    endif()

    if(WANT_LINUX_FBCON)
        check_c_source_compiles("
            #include <linux/fb.h>
            int main(void) {
                int x = FB_SYNC_ON_GREEN;
                return 0;
            }"
            CAN_FBCON)
        if(CAN_FBCON)
            set(ALLEGRO_LINUX_FBCON 1)
            add_our_module(alleg-fbcon src/linux/fbcon.c)
        endif()
    endif()

    if(WANT_LINUX_SVGALIB)
        find_package(Svgalib)
        if(SVGALIB_FOUND)
            set(ALLEGRO_LINUX_SVGALIB 1)
            include_directories(SYSTEM ${SVGALIB_INCLUDE_DIR})

            set(CMAKE_REQUIRED_LIBRARIES ${SVGALIB_LIBRARY})
            check_c_source_compiles("
                #include <vga.h>
                int main(void) {
                    int x = vga_version;
                    return 0;
                }"
                ALLEGRO_LINUX_SVGALIB_HAVE_VGA_VERSION)

            list(APPEND PLATFORM_LIBS_NON_MODULES ${SVGALIB_LIBRARY})
            add_our_module(alleg-svgalib src/linux/svgalib.c
                ${SVGALIB_LIBRARY})
        endif()
    endif()
endif(WANT_LINUX_CONSOLE AND CMAKE_SYSTEM_NAME STREQUAL Linux)

# -- Windows --

if(WIN32)
    find_package(DDraw)
    find_package(DInput)
    find_package(DSound)
    find_package(DXGuid)

    if(NOT DDRAW_FOUND OR NOT DINPUT_FOUND OR NOT DSOUND_FOUND OR NOT DXGUID_FOUND)
        if(MSVC)
            message(FATAL_ERROR
                "DirectX required for Windows port. You might need to add DirectX include and lib directories to your INCLUDE and LIB environment variables.
    ")
        elseif(MINGW)
            message(FATAL_ERROR
                "DirectX required for Windows port. Get it from the Allegro web site (dx80_mgw.zip).")
        else()
            message(FATAL_ERROR "DirectX required for Windows port.")
        endif()
    endif()

    include_directories(SYSTEM
        ${DDRAW_INCLUDE_DIR}
        ${DINPUT_INCLUDE_DIR}
        ${DSOUND_INCLUDE_DIR}
        ${DXGUID_INCLUDE_DIR}
        )
    list(APPEND PLATFORM_SOURCES ${ALLEGRO_SRC_WIN_FILES})
    list(APPEND PLATFORM_LIBS
        kernel32
        user32
        gdi32
        comdlg32
        ole32
        ${DINPUT_LIBRARIES}
        ${DDRAW_LIBRARIES}
        ${DXGUID_LIBRARIES}
        winmm
        ${DSOUND_LIBRARIES}
        )
endif(WIN32)

# -- Haiku/BeOS --

if(BEOS)
    list(APPEND PLATFORM_SOURCES ${ALLEGRO_SRC_BEOS_FILES})
    list(APPEND PLATFORM_LIBS game midi device)
endif(BEOS)

# -- Mac OS X --

if(ALLEGRO_MACOSX)
    list(APPEND PLATFORM_SOURCES ${ALLEGRO_SRC_MACOSX_FILES})
    if(NOT SHARED)
	list(APPEND PLATFORM_SOURCES src/macosx/main.m)
    endif(NOT SHARED)

    find_library(COCOA_LIBRARY Cocoa)
    find_library(CARBON_LIBRARY Carbon)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREAUDIO_LIBRARY CoreAudio)
    find_library(AUDIOUNIT_LIBRARY AudioUnit)
    find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)
    find_package(QuickTime)
    mark_as_advanced(COCOA_LIBRARY CARBON_LIBRARY IOKIT_LIBRARY)
    mark_as_advanced(COREAUDIO_LIBRARY AUDIOUNIT_LIBRARY AUDIOTOOLBOX_LIBRARY)
    mark_as_advanced(QUICKTIME_INCLUDE_DIR QUICKTIME_LIBRARY)

    list(APPEND PLATFORM_LIBS
	${COCOA_LIBRARY}
	${CARBON_LIBRARY}
	${IOKIT_LIBRARY}
	${COREAUDIO_LIBRARY}
	${AUDIOUNIT_LIBRARY}
	${AUDIOTOOLBOX_LIBRARY}
	${QUICKTIME_LIBRARY}
	)
endif(ALLEGRO_MACOSX)

# -- PSP --

if(PSP)
    include_directories(SYSTEM ${PSPSDK_PATH}/include)
    link_directories(${PSPSDK_PATH}/lib)
    list(APPEND PLATFORM_SOURCES ${ALLEGRO_SRC_PSP_FILES})
    list(APPEND PLATFORM_LIBS
    m pspaudio pspgu psprtc pspdebug pspdisplay pspge pspctrl pspsdk c pspnet pspnet_inet pspnet_resolver psputility pspuser pspkernel)
endif(PSP)

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
    ${CMAKE_SOURCE_DIR}/include/allegro/platform/alplatf.h.cmake
    ${CMAKE_BINARY_DIR}/include/allegro/platform/alplatf.h
    @ONLY
    )
add_headers(allegro/platform
    ${CMAKE_BINARY_DIR}/include/allegro/platform/alplatf.h
    )

if(UNIX)
    configure_file(
        ${CMAKE_SOURCE_DIR}/include/allegro/platform/alunixac.h.cmake
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
if(BEOS)
    add_headers("" include/bealleg.h)
endif(BEOS)

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

if(ALLEGRO_MODULES_PATH)
    set(ALLEGRO_MODULES_PATH_FLAG
        "-DALLEGRO_MODULES_PATH='\"${ALLEGRO_MODULES_PATH}\"'")
endif(ALLEGRO_MODULES_PATH)

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

if(WIN32 AND SHARED)
    set(allegro_OUTPUT_NAME alleg${ALLEGRO_DLL_SHORTVER})
else()
    set(allegro_OUTPUT_NAME alleg)
endif()

set_target_properties(allegro
    PROPERTIES
    COMPILE_FLAGS "-DALLEGRO_SRC ${ALLEGRO_MODULES_PATH_FLAG}"
    LINK_FLAGS "${ALLEGRO_LINK_FLAGS}"
    OUTPUT_NAME ${allegro_OUTPUT_NAME}
    VERSION ${ALLEGRO_VERSION}
    SOVERSION ${ALLEGRO_SOVERSION}
    )

if(WIN32)
    if(SHARED)
        # Make mingw not add lib prefix to DLL, to match MSVC.
        set_target_properties(allegro
            PROPERTIES
            PREFIX ""
            )
    else(SHARED)
        # Add a -static postfix on the main library for Windows.
        # In earlier versions of Allegro this was "_s".
        set_target_properties(allegro
            PROPERTIES
            POSTFIX "-static"
            DEBUG_POSTFIX "-debug-static"
            PROFILE_POSTFIX "-profile-static"
            )
    endif(SHARED)
endif(WIN32)

set_our_framework_properties(allegro Allegro)

set(link_with ${PLATFORM_LIBS})
if(ALLEGRO_WITH_MODULES)
    list(APPEND link_with ${PLATFORM_LIBS_MODULES})
else()
    list(APPEND link_with ${PLATFORM_LIBS_NON_MODULES})
endif()
target_link_libraries(allegro ${link_with})

sanitize_cmake_link_flags(static_link_with ${link_with})
set_target_properties(allegro
    PROPERTIES
    static_link_with "${static_link_with}"
    )

install_our_library(allegro)
if(NOT WANT_FRAMEWORKS)
    install_our_headers(${ALLEGRO_PUBLIC_HEADERS})
endif(NOT WANT_FRAMEWORKS)

if(APPLE AND SHARED OR WANT_FRAMEWORKS)
    add_our_library(alleg-main STATIC
	src/macosx/main.m
	)
    set_target_properties(alleg-main
	PROPERTIES
	OUTPUT_NAME alleg-main${BUILD_TYPE_SUFFIX}
	COMPILE_FLAGS -DALLEGRO_SRC
	)
    target_link_libraries(allegro alleg-main)
    install_our_library(alleg-main)
endif(APPLE AND SHARED OR WANT_FRAMEWORKS)

if(PSP)
    add_our_library(alleg-main STATIC
	src/psp/pmain.c
	)
    set_target_properties(alleg-main
	PROPERTIES
	OUTPUT_NAME alleg-main${BUILD_TYPE_SUFFIX}
	COMPILE_FLAGS -DALLEGRO_SRC
	)
    target_link_libraries(alleg-main pspuser pspkernel)
    target_link_libraries(allegro alleg-main)
    install_our_library(alleg-main)
endif(PSP)

#-----------------------------------------------------------------------------#
#
# allegro-config script
#

if(UNIX) # including MACOSX
    set(prefix "${CMAKE_INSTALL_PREFIX}")
    set(INCLUDE_PREFIX "${CMAKE_INSTALL_PREFIX}") # not sure what this is
    if(BUILD_SHARED_LIBS)
        set(LINK_WITH_STATIC_LIBS no)
    else()
        set(LINK_WITH_STATIC_LIBS yes)
    endif()
    set(LIB_TO_LINK alleg${BUILD_TYPE_SUFFIX})
    if(APPLE)
	set(LDFLAGS "-framework Cocoa -lalleg-main")
    else(APPLE)
	set(LDFLAGS "")
    endif(APPLE)
    sanitize_cmake_link_flags(LIBS
        ${PLATFORM_LIBS}
        ${PLATFORM_LIBS_NON_MODULES}
        ${PLATFORM_LIBS_MODULES}
        )

    if(APPLE)
        # XXX untested
        set(ACCEPT_FRAMEWORKS yes)
        set(FRAMEWORKS "-framework Cocoa -framework Allegro -lalleg-main${BUILD_TYPE_SUFFIX}")
    else()
        set(ACCEPT_FRAMEWORKS no)
        set(FRAMEWORKS "")
    endif()

    configure_file(
        ${CMAKE_SOURCE_DIR}/misc/allegro-config.in
        ${CMAKE_BINARY_DIR}/allegro-config
        @ONLY
        )

    install(PROGRAMS ${CMAKE_BINARY_DIR}/allegro-config
            DESTINATION bin
            )
endif(UNIX)

#-----------------------------------------------------------------------------#
#
# Addons
#

option(WANT_ALLEGROGL "Enable AllegroGL" on)
option(WANT_LOADPNG "Enable loadpng" on)
option(WANT_LOGG "Enable logg" on)
option(WANT_JPGALLEG "Enable JPGAlleg" on)

if(WANT_FRAMEWORKS)
    set(ADDON_LINKAGE SHARED)
else()
    set(ADDON_LINKAGE STATIC)
endif()

if(WANT_ALLEGROGL)
    add_subdirectory(addons/allegrogl)
endif()
if(WANT_LOADPNG)
    add_subdirectory(addons/loadpng)
endif()
if(WANT_LOGG)
    add_subdirectory(addons/logg)
endif()
if(WANT_JPGALLEG)
    add_subdirectory(addons/jpgalleg)
endif()

#-----------------------------------------------------------------------------#
#
# pkg-config files - after addons to get properties
#

set(PKG_CONFIG_FILES
    allegro
    allegrogl
    jpgalleg
    loadpng
    logg
    )

if(UNIX)
    set(prefix "${CMAKE_INSTALL_PREFIX}")
    set(INCLUDE_PREFIX "\${prefix}")
    set(bindir "\${exec_prefix}/bin")
    set(includedir "\${prefix}/include")
    set(libdir "\${exec_prefix}/lib${LIB_SUFFIX}")
    set(lib_type "${BUILD_TYPE_SUFFIX}")
    # We only use -static suffixes on Windows, for the main library.
    set(lib_linkage "")

    foreach(name ${PKG_CONFIG_FILES})
        set(target ${name})

        # For static linking: get extra libraries to link with.
        get_target_property(link_with ${target} static_link_with)

        configure_file(
            misc/${name}.pc.in
            ${LIBRARY_OUTPUT_PATH}/pkgconfig/${name}${lib_type}.pc
            @ONLY
            )
        install(FILES ${LIBRARY_OUTPUT_PATH}/pkgconfig/${name}${lib_type}.pc
                DESTINATION "lib${LIB_SUFFIX}/pkgconfig"
                )
    endforeach(name)
endif(UNIX)

#-----------------------------------------------------------------------------#
#
# Documentation
#

add_subdirectory(docs)

#-----------------------------------------------------------------------------#
#
# Samples - after addons because skater can use AllegroGL
#

option(WANT_EXAMPLES "Build example programs" on)

if(WANT_EXAMPLES)
    add_subdirectory(examples)
    add_subdirectory(demos/shooter)
    add_subdirectory(demos/skater)
endif(WANT_EXAMPLES)

#-----------------------------------------------------------------------------#
#
# Tools - after addons because of grabber plugins
#

option(WANT_TOOLS "Build tool programs" on)

if(WANT_TOOLS)
    add_subdirectory(tools)

    # Note to packagers: setup is mostly a historical footnote on modern
    # operating systems.  Do not feel the need to package it.
    add_subdirectory(setup)
endif(WANT_TOOLS)

#-----------------------------------------------------------------------------#
#
# Tests
#

option(WANT_TESTS "Build test programs" on)

if(WANT_TESTS)
    add_subdirectory(tests)
endif(WANT_TESTS)

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
