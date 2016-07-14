# Table of contents

* [Platforms](#platforms)
* [Get the source code](#get-the-source-code)
* [Dependencies](#dependencies)
  * [Windows dependencies](#windows-dependencies)
  * [Mac OS X dependencies](#mac-os-x-dependencies)
  * [Linux dependencies](#linux-dependencies)
* [Compiling](#compiling)
  * [Windows details](#windows-details)
  * [Mac OS X details](#mac-os-x-details)
    * [Issues with Retina displays](#issues-with-retina-displays)
  * [Linux details](#linux-details)
* [Using shared third party libraries](#using-shared-third-party-libraries)
  * [Linux issues](#linux-issues)
* [Building Skia dependency](#building-skia-dependency)
  * [Skia on Windows](#skia-on-windows)
  * [Skia on Mac OS X](#skia-on-mac-os-x)

# Platforms

You should be able to compile Aseprite successfully on the following
platforms:

* Windows 10 + VS2015 Community Edition + Windows 10 SDK
* Mac OS X 10.11.4 El Capitan + Xcode 7.3 + OS X 10.11 SDK + Skia (without GPU)
* Linux + gcc 4.8 with some C++11 support

# Get the source code

You can get the source code downloading a `Aseprite-v1.x-Source.zip`
file from the latest Aseprite release:

https://github.com/aseprite/aseprite/releases

Or you can clone the repository and all its submodules using the
following command:

    git clone --recursive https://github.com/aseprite/aseprite.git

To update an existing clone you can use the following commands:

    cd aseprite
    git pull
    git submodule update --init --recursive

You can use [Git for Windows](https://git-for-windows.github.io/) to
clone the repository on Windows.

# Dependencies

To compile Aseprite you will need:

* The latest version of [CMake](http://www.cmake.org/) (3.4 or greater)
* [Ninja](https://ninja-build.org) build system

Aseprite can be compiled with two different back-ends:

1. Allegro back-end (Windows, Linux): You will not need any extra
   library because the repository already contains a modified version
   of the Allegro library. This back-end is deprecated and will be
   removed in future versions. All new development is being done in
   the new Skia back-end.

2. Skia back-end (Windows, Mac OS X): You will need a compiled version
   of the Skia library. Please check the details about
   [how to build Skia](#building-skia-dependency) on your platform.

## Windows dependencies

First of all, you will need an extra little utility: `awk`, used to
compile the libpng library. You can get this utility from MSYS2
distributions like [MozillaBuild](https://wiki.mozilla.org/MozillaBuild).

After that you have to choose the back-end:

1. If you choose the Allegro back-end, you can jump directly to the
   [Compiling](#compiling) section.

2. If you choose the Skia back-end, you will need to
   [compile Skia](#skia-on-windows) before and then continue in the
   [Compiling](#compiling) section. Remember to check the
   [Windows details](#windows-details) section to know how to call
   `cmake` correctly.

## Mac OS X dependencies

On OS X you will need Mac OS X 10.11 SDK and Xcode 7.3 (maybe older
versions might work).

Also, you must compile [Skia](#skia-on-mac-os-x) before starting with
the [compilation](#compiling).

## Linux dependencies

You will need the following dependencies (Ubuntu, Debian):

    sudo apt-get update -qq
    sudo apt-get install -y g++ libx11-dev libxcursor-dev cmake ninja-build

The `libxcursor-dev` package is needed to
[hide the hardware cursor](https://github.com/aseprite/aseprite/issues/913).

# Compiling

1. [Get Aseprite code](#get-the-source-code), put it in a folder like
   `C:\aseprite`, and create a `build` directory inside to leave all
   the files that are result of the compilation process (`.exe`,
   `.lib`, `.obj`, `.a`, `.o`, etc).

        cd C:\aseprite
        mkdir build

   In this way, if you want to start with a fresh copy of Aseprite
   source code, you can remove the `build` directory and start again.

2. Enter in the new directory and execute `cmake`:

        cd C:\aseprite\build
        cmake -G Ninja ..

   Here `cmake` needs different options depending on your
   platform. You must check the details for
   [Windows](#windows-details), [OS X](#mac-os-x-details), and
   [Linux](#linux-details). Some `cmake` options can be modified using tools like
   [`ccmake`](https://cmake.org/cmake/help/latest/manual/ccmake.1.html)
   or [`cmake-gui`](https://cmake.org/cmake/help/latest/manual/cmake-gui.1.html).

3. After you have executed and configured `cmake`, you have to compile
   the project:

        cd C:\aseprite\build
        ninja aseprite

4. When `ninja` finishes the compilation, you can find the executable
   inside `C:\aseprite\build\bin\aseprite.exe`.

## Windows details

To choose the Skia back-end
([after you've compiled Skia](#skia-on-windows)) you can execute `cmake`
with the following arguments:

    cd aseprite
    mkdir build
    cd build
    cmake -DUSE_ALLEG4_BACKEND=OFF -DUSE_SKIA_BACKEND=ON -DSKIA_DIR=C:\deps\skia -G Ninja ..
    ninja aseprite

In this case, `C:\deps\skia` is the directory where Skia was compiled
as described in [Skia on Windows](#skia-on-windows) section.

## Mac OS X details

After [compiling Skia](#skia-on-mac-os-x), you should run `cmake` with
the following parameters and then `ninja`:

    cd aseprite
    mkdir build
    cd build
    cmake \
      -DCMAKE_OSX_ARCHITECTURES=x86_64 \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=10.7 \
      -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk \
      -DUSE_ALLEG4_BACKEND=OFF \
      -DUSE_SKIA_BACKEND=ON \
      -DSKIA_DIR=$HOME/deps/skia \
      -DWITH_HarfBuzz=OFF \
      -G Ninja \
      ..
    ninja aseprite

In this case, `$HOME/deps/skia` is the directory where Skia was
compiled as described in [Skia on Mac OS X](#skia-on-mac-os-x) section.

### Issues with Retina displays

If you have a Retina display, check the following issue:

  https://github.com/aseprite/aseprite/issues/589

## Linux details

On Linux you can specify a specific directory to install Aseprite
after a `ninja install` command. For example:

    cd aseprite
    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=~/software -G Ninja ..
    ninja aseprite

Then, you can invoke `ninja install` and it will copy the program in
the given location (e.g. `~/software/bin/aseprite` on Linux).

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

# Building Skia dependency

When you compile Aseprite with [Skia](https://skia.org) as back-end on
Windows or OS X, you need to compile a specific version of Skia. In
the following sections you will find straightforward steps to compile
Skia.

You can always check the
[official Skia instructions](https://skia.org/user/quick) and select
the OS you are building for. Aseprite uses the `aseprite-m53` Skia
branch from `https://github.com/aseprite/skia`.

## Skia on Windows

Download
[Google depot tools](https://storage.googleapis.com/chrome-infra/depot_tools.zip)
and uncompress it in some place like `C:\deps\depot_tools`.

Then open a command line follow these steps (for VS2015):

    call "%VS140COMNTOOLS%\vsvars32.bat"
    set PATH=C:\deps\depot_tools;%PATH%
    cd C:\deps\depot_tools
    gclient sync

(The `gclient` command might print an error like
`Error: client not configured; see 'gclient config'`.
Just ignore it.)

    cd C:\deps
    git clone https://github.com/aseprite/skia.git
    cd skia
    git checkout aseprite-m53
    python bin/sync-and-gyp

(The `bin/sync-and-gyp` will take some minutes because it downloads a
lot of packages, please wait and re-run the same command in case it fails.)

    ninja -C out/Release dm

More information about these steps in the
[official Skia documentation](https://skia.org/user/quick/windows).

## Skia on Mac OS X

These steps will create a `deps` folder in your home directory with a
couple of subdirectories needed to build Skia (you can change the
`$HOME/deps` with other directory). Some of these commands will take
several minutes to finish:

    mkdir $HOME/deps
    cd $HOME/deps
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
    git clone https://github.com/aseprite/skia.git
    export PATH="${PWD}/depot_tools:${PATH}"
    cd skia
    git checkout aseprite-m53
    python bin/sync-and-gyp
    ninja -C out/Release dm

After this you should have all Skia libraries compiled.  When you
[compile Aseprite](#compiling), remember to add
`-DSKIA_DIR=$HOME/deps/skia` parameter to your `cmake` call as
described in the [Mac OS X details](#mac-os-x-details) section.

More information about these steps in the
[official Skia documentation](https://skia.org/user/quick/macos).
