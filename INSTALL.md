# Table of contents

* [Platforms](#platforms)
* [Get the source code](#get-the-source-code)
* [Dependencies](#dependencies)
  * [Linux dependencies](#linux-dependencies)
* [Compiling](#compiling)
* [Mac OS X details](#mac-os-x-details)
  * [Issues with Retina displays](#issues-with-retina-displays)
* [Using shared third party libraries](#using-shared-third-party-libraries)
  * [Linux issues](#linux-issues)

# Platforms

You should be able to compile Aseprite successfully on the following
platforms:

* Windows 10 + VS2015 Community Edition + Windows 10 SDK
* Mac OS X 10.11.4 El Capitan + Xcode 7.3 + OS X 10.11 SDK + Skia (without GPU)
* Linux + gcc 4.8 with some C++11 support

# Get the source code

At the moment the only way to compile Aseprite is cloning the Git
repository located here:

https://github.com/aseprite/aseprite

You can clone it using the following command (read-only URL):

    git clone --recursive https://github.com/aseprite/aseprite.git

To update an existing copy:

    cd aseprite
    git pull
    git submodule update --init --recursive

On Windows you can use programs like
[msysgit](http://msysgit.github.io/) to clone the repository.

# Dependencies

Aseprite uses the latest version of [CMake](http://www.cmake.org/)
(3.0 or greater) as its build system. Also we use
[Ninja](https://ninja-build.org) build files regularly instead of
Visual Studio or Xcode projects.

Aseprite can be compiled with two different back-ends:

1. Allegro back-end (Windows, Linux): You will not need any extra
   library because the repository already contains a modified version
   of the Allegro library. This back-end is only available for Windows
   and Linux and it'll be removed in following versions.

2. Skia back-end (Windows, OS X): You will need a compiled version of
   [Skia](http://skia.org/), `chrome/m50` branch, without GPU support,
   i.e.  compiled with `GYP_DEFINES='skia_gpu=0'`. When you compile
   Aseprite, you'll need to give some variables to CMake:
   `-DUSE_SKIA_ALLEG4=OFF`, `-DUSE_SKIA_BACKEND=ON`, and
   `-DSKIA_DIR=...` pointing to the Skia checkout directory. (Note:
   The GPU support is a work-in-progress, so next versions .)

## Linux dependencies

You will need the following dependencies:

    sudo apt-get update -qq
    sudo apt-get install -y g++ libx11-dev libxcursor-dev cmake ninja-build

The `libxcursor-dev` package is needed to
[hide the hardware cursor](https://github.com/aseprite/aseprite/issues/913).

# Compiling

The following are the steps to compile Aseprite (in this case we have
the repository clone in a directory called `aseprite`):

1. Make a build directory to leave all the files that are result of
   the compilation process (`.exe`, `.lib`, `.obj`, `.a`, `.o`, etc).

        C:\>cd aseprite
        C:\aseprite>mkdir build

   In this way, if you want to start with a fresh copy of Aseprite
   source code, you can remove the `build` directory and start again.

2. Enter in the new directory and execute cmake giving to it
   your compiler as generator:

        C:\aseprite>cd build

   If you have ninja:

        C:\aseprite\build>cmake -G Ninja ..

   If you have nmake (MSVC compilers):

        C:\aseprite\build>cmake -G "NMake Makefiles" ..

   If you have Visual Studio you can generate a solution:

        C:\aseprite\build>cmake -G "Visual Studio 12 2013" ..

   If you are on Linux:

        ~/aseprite/build$ cmake -G "Unix Makefiles" ..

   For more information in [CMake wiki](http://www.vtk.org/Wiki/CMake_Generator_Specific_Information).

   Additionally you can change build settings by passing them on the
   command line, like so:

        ~/aseprite/build$ cmake -DCMAKE_INSTALL_PREFIX=~/software ..

   or later on with a tool like
   [`ccmake`](https://cmake.org/cmake/help/latest/manual/ccmake.1.html)
   or
   [`cmake-gui`](https://cmake.org/cmake/help/latest/manual/cmake-gui.1.html).

3. After you have executed one of the `cmake -G <generator> ..`
   commands, you have to compile the project executing make, nmake,
   opening the solution, etc.

4. When the project is compiled, you can find the executable file
   inside `build/bin/aseprite.exe`. If you invoked `make install` it
   will be copied to an appropriate location
   (e.g. `/usr/local/bin/aseprite` on Linux).

# Mac OS X details

You need the old Mac OS X 10.4 SDK universal, which can be obtained
from Xcode 3.1 Developer Tools (Xcode 3.1 Developer DVD,
`xcode31_2199_developerdvd.dmg`). You can get it from Apple developer
website (you need to be registered):

  https://developer.apple.com/downloads/

Inside the `Packages` folder, there is a `MacOSX10.4.Universal.pkg`,
install it (it will be installed in `/SDKs/MacOSX10.4u.sdk`), and run
cmake with the following parameters:

    -DCMAKE_OSX_ARCHITECTURES:STRING=i386
    -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=10.4
    -DCMAKE_OSX_SYSROOT:STRING=/SDKs/MacOSX10.4u.sdk
    -DWITH_HarfBuzz=OFF

## Issues with Retina displays

If you have a Retina display, check this issue:

  https://github.com/aseprite/aseprite/issues/589

# Using shared third party libraries

If you don't want to use the embedded code of third party libraries
(i.e. to use your installed versions), you can disable static linking
configuring each `USE_SHARED_` option.

After running `cmake -G`, you can edit `build/CMakeCache.txt` file,
and enable the `USE_SHARED_` flag (set its value to `ON`) of the
library that you want to be linked dynamically.

## Linux issues

If you use the official version of Allegro 4.4 library (i.e. you
compile with `USE_SHARED_ALLEGRO4=ON`) you will experience a couple of
known issues solved in
[our patched version of Allegro 4.4 library](https://github.com/aseprite/aseprite/tree/master/src/allegro):

* You will
  [not be able to resize the window](https://github.com/aseprite/aseprite/issues/192)
  ([patch](https://github.com/aseprite/aseprite/commit/920f6275d55113507121afcbcda80adb44cc0563)).
* You will have problems
  [adding HSV colors in non-English systems](https://github.com/aseprite/aseprite/commit/27b55030e26e93c5e8d9e7e21206c8709d46ff22)
  using the warning icon.
