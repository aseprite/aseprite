# Use this command to build the Windows port of Allegro
# with a mingw cross compiler:
#
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchain-mingw.cmake .
#
# or for out of source:
#
#   cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-mingw.cmake ..
#
# You will need at least CMake 2.6.0.
#
# Adjust the following paths to suit your environment.
#
# You might want to set MINGDIR to prevent make install making a mess
# in your normal directories.
#
# This file was based on http://www.cmake.org/Wiki/CmakeMingw

# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)

# Assume the target architecture.
# XXX for some reason the value set here gets cleared before we reach the
# main CMakeLists.txt; see that file for a workaround.
# set(CMAKE_SYSTEM_PROCESSOR i686)

# Which compilers to use for C and C++, and location of target
# environment.
if(EXISTS /usr/i586-mingw32msvc)
    # First look in standard location as used by Debian/Ubuntu/etc.
    set(CMAKE_C_COMPILER i586-mingw32msvc-gcc)
    set(CMAKE_CXX_COMPILER i586-mingw32msvc-g++)
    set(CMAKE_FIND_ROOT_PATH /usr/i586-mingw32msvc)
elseif(EXISTS /opt/mingw)
    # Otherwise you can get a MinGW environment using the script at
    # <http://mingw-cross-env.nongnu.org>.  It downloads and builds MinGW and
    # most of the dependencies for you.  This is a suitable location.
    set(CMAKE_C_COMPILER /opt/mingw/usr/bin/i686-pc-mingw32-gcc)
    set(CMAKE_CXX_COMPILER /opt/mingw/usr/bin/i686-pc-mingw32-g++)
    set(CMAKE_FIND_ROOT_PATH /opt/mingw/usr/i686-pc-mingw32)
else()
    # Else fill in local path which the user will likely adjust.
    # This is the location assumed by <http://www.libsdl.org/extras/win32/cross/>
    set(CMAKE_C_COMPILER /usr/local/cross-tools/bin/i386-mingw32-gcc)
    set(CMAKE_CXX_COMPILER /usr/local/cross-tools/bin/i386-mingw32-g++)
    set(CMAKE_FIND_ROOT_PATH /usr/local/cross-tools)
endif()

# Adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Tell pkg-config not to look at the target environment's .pc files.
# Setting PKG_CONFIG_LIBDIR sets the default search directory, but we have to
# set PKG_CONFIG_PATH as well to prevent pkg-config falling back to the host's
# path.
set(ENV{PKG_CONFIG_LIBDIR} ${CMAKE_FIND_ROOT_PATH}/lib/pkgconfig)
set(ENV{PKG_CONFIG_PATH} ${CMAKE_FIND_ROOT_PATH}/lib/pkgconfig)

set(INSTALL_PREFIX ${CMAKE_FIND_ROOT_PATH})
if(ENV{MINGDIR} STREQUAL "")
    set(ENV{MINGDIR} ${CMAKE_FIND_ROOT_PATH})
endif()
