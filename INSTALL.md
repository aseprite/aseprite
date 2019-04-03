# Table of contents

* [Platforms](#platforms)
* [Get the source code](#get-the-source-code)
* [Dependencies](#dependencies)
  * [Windows dependencies](#windows-dependencies)
  * [macOS dependencies](#macos-dependencies)
  * [Linux dependencies](#linux-dependencies)
* [Compiling](#compiling)
  * [Windows details](#windows-details)
  * [macOS details](#macos-details)
    * [Issues with Retina displays](#issues-with-retina-displays)
  * [Linux details](#linux-details)
* [Using shared third party libraries](#using-shared-third-party-libraries)
* [Building Skia dependency](#building-skia-dependency)
  * [Skia on Windows](#skia-on-windows)
  * [Skia on macOS](#skia-on-macos)
  * [Skia on Linux](#skia-on-linux)

# Platforms

You should be able to compile Aseprite successfully on the following
platforms:

* Windows NT 10.0 + Visual Studio (2017 or 2019) Community Edition + Windows 10 SDK

   *(Windows NT 10.0 is a Windows 10, Windows Server 2016, or Windows Server 2019 operating system)*

* macOS 10.12.6 Sierra + Xcode 9.0 + macOS 10.13 SDK + Skia
* Linux + gcc 4.8 with some C++11 support

# Get the source code

You can get the source code downloading a `Aseprite-v1.x-Source.zip`
file from the latest Aseprite release (*in that case please follow the
compilation instructions inside the `.zip` file*):

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

* The latest version of [CMake](http://www.cmake.org/)

   *(3.14 or greater is best on Windows, to make sure you have the Visual Studio 2019 generators and the highest compatibility if/when compiling on Windows Server)*
   
* [Ninja](https://ninja-build.org) build system
* You will need a compiled version of the Skia library.
  Please check the details about [how to build Skia](#building-skia-dependency)
  on your platform.

## Windows dependencies

First of all, you will need:

* Windows NT 10.0 (Windows 10, Windows Server 2016, or Windows Server 2019)

   *(we don't support cross-compiling and don't know if this would be possible)*

* [Visual Studio Community Edition](https://www.visualstudio.com/downloads/) (Visual Studio 2017 can be found further down the page, if you prefer that over 2019)
* The [Desktop development with C++](https://docs.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=vs-2019) item from the Visual Studio Installer
* Windows 10 SDK (10.0.17763.0 or higher) item from the Visual Studio Installer

   *(the Windows 10 SDK installation is located in the "Optional" section and is usually selected by default when installing "Desktop development with C++")*

You will need to [compile Skia](#skia-on-windows) before and then
continue in the [Compiling](#compiling) section. Remember to check the
[Windows details](#windows-details) section to know how to call
`cmake` correctly.

**Special considerations for running CMake on Windows:**

* CMake should be at least 3.14 or greater for Windows, if using the Installer (MSI file)

   *(this was when the Visual Studio 2019 generators were added and is to make sure of the highest compatibility when compiling on Windows Server. Lastly, CMake has had it's Installer technology changed since 3.4 and lower, making updates from previous versions better)*

* The CMake download should be from the "win64-x64" platform, not the win32-x86 one
* If using the CMake Installer (MSI file), using the checkbox option "Add CMake to the system PATH for all users" will help greatly with speeding-up the setup process
* Another way to get CMake on your system is to use a package manager like [Chocolatey](https://chocolatey.org/packages/cmake)

**Special considerations for running Ninja on Windows:**

* It is OK to copy ninja.exe into your CMake "\bin" directory, to save inputting another Windows Environment Variable

   *(CMake's bin directory is installed to "C:\Program Files\CMake\bin" by default, if using the win64-x64 platform Installer (MSI file)*

* Depending on the unzipping tool used, your ninja.exe may be blocked from running correctly by Windows SmartScreen. If this is the case, go to the Properties dialog of ninja.exe and click the "Unblock" checkbox at the bottom to fix this.
* Another way to get Ninja on your system is to use a package manager like [Chocolatey](https://chocolatey.org/packages/ninja)

**Special considerations for running Python on Windows:**

* Python is not built-in for Windows operating systems. Regardless, Python 2.7.6 should come with Google depot tools. If you're having issues running 'python tools/git-sync-deps', when syncing Skia dependency packages, you may have forgot to add the depot_tools directory to your PATH.

## macOS dependencies

On macOS you will need macOS 10.12 SDK and Xcode 8.0 (older versions
might work).

You must also compile [Skia](#skia-on-macos) before starting with the
[compilation](#compiling).

## Linux dependencies

You will need the following dependencies on Ubuntu/Debian:

    sudo apt-get install -y g++ cmake ninja-build libx11-dev libxcursor-dev libgl1-mesa-dev libfontconfig1-dev

On Fedora:

    sudo yum install -y gcc-c++ cmake ninja-build libX11-devel libXcursor-devel mesa-libGL-devel fontconfig-devel

You must also compile [Skia](#skia-on-linux) before starting with the
[compilation](#compiling).

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
   [Windows](#windows-details), [macOS](#macos-details), and
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

After you've [compiled Skia](#skia-on-windows),
open a [developer command prompt](https://docs.microsoft.com/en-us/dotnet/framework/tools/developer-command-prompt-for-vs)
or in the command line (`cmd.exe`) call one of the following commands (depending on if you are running Visual Studio 2017 or Visual Studio 2019).

For Visual Studio 2017:

    call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" -arch=x64
	
For Visual Studio 2019:

    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64

And then

    cd aseprite
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLAF_OS_BACKEND=skia -DSKIA_DIR="C:\deps\skia" -DSKIA_OUT_DIR="C:\deps\skia\out\Release" -G Ninja ..
    ninja aseprite

In this case, `C:\deps\skia` is the directory where Skia was compiled
as described in [Skia on Windows](#skia-on-windows) section.

## macOS details

After [compiling Skia](#skia-on-macos), you should run `cmake` with
the following parameters and then `ninja`:

    cd aseprite
    mkdir build
    cd build
    cmake \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_OSX_ARCHITECTURES=x86_64 \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 \
      -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.14.sdk \
      -DLAF_OS_BACKEND=skia \
      -DSKIA_DIR=$HOME/deps/skia \
      -DSKIA_OUT_DIR=$HOME/deps/skia/out/Release \
      -G Ninja \
      ..
    ninja aseprite

In this case, `$HOME/deps/skia` is the directory where Skia was
compiled as described in [Skia on macOS](#skia-on-macos) section.
Make sure that `CMAKE_OSX_SYSROOT` is pointing to the correct SDK
directory (in this case
`/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.14.sdk`),
but it could be different in your Mac.

### Issues with Retina displays

If you have a Retina display, check the following issue:

  https://github.com/aseprite/aseprite/issues/589

## Linux details

First you have to [compile Skia](#skia-on-linux), then you should run
`cmake` with the following parameters and then `ninja`:

    cd aseprite
    mkdir build
    cd build
    cmake \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DLAF_OS_BACKEND=skia \
      -DSKIA_DIR=$HOME/deps/skia \
      -DSKIA_OUT_DIR=$HOME/deps/skia/out/Release \
      -G Ninja \
      ..
    ninja aseprite

In this case, `$HOME/deps/skia` is the directory where Skia was
compiled as described in [Skia on Linux](#skia-on-linux) section.

# Using shared third party libraries

If you don't want to use the embedded code of third party libraries
(i.e. to use your installed versions), you can disable static linking
configuring each `USE_SHARED_` option.

After running `cmake -G`, you can edit `build/CMakeCache.txt` file,
and enable the `USE_SHARED_` flag (set its value to `ON`) of the
library that you want to be linked dynamically.

# Building Skia dependency

When you compile Aseprite with [Skia](https://skia.org) as back-end on
Windows or macOS, you need to compile a specific version of Skia. In
the following sections you will find straightforward steps to compile
Skia.

You can always check the
[official Skia instructions](https://skia.org/user/build) and select
the OS you are building for. Aseprite uses the `aseprite-m71` Skia
branch from `https://github.com/aseprite/skia`.

## Skia on Windows

Download
[Google depot tools](https://storage.googleapis.com/chrome-infra/depot_tools.zip)
and uncompress it in some place like `C:\deps\depot_tools`.

[It's recommended to compile Skia with Clang](https://github.com/google/skia/blob/master/site/user/build.md#a-note-on-software-backend-performance)
to get better performance. So you will need to [download the "win64" build of Clang](http://releases.llvm.org/8.0.0/LLVM-8.0.0-win64.exe),
and install it into a folder like: `C:\deps\llvm` (a folder without whitespaces is preferable, but with the latest versions of Skia and Clang it is not required anymore).

Open a [developer command prompt](https://docs.microsoft.com/en-us/dotnet/framework/tools/developer-command-prompt-for-vs)
or command line (`cmd.exe`) and call one of the following commands (depending on if you are running Visual Studio 2017 or Visual Studio 2019).

For Visual Studio 2017:

    call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" -arch=x64
	
For Visual Studio 2019:

    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64

Then:

    set PATH=C:\deps\depot_tools;%PATH%
    cd C:\deps\depot_tools
    gclient sync

(The `gclient` command might print an error like
`Error: client not configured; see 'gclient config'`.
Just ignore it.)

    cd C:\deps
    git clone -b aseprite-m71 https://github.com/aseprite/skia.git
    cd skia
    python tools/git-sync-deps

(The `tools/git-sync-deps` will take some minutes because it downloads
a lot of packages, please wait and re-run the same command in case it
fails.)

Next, if you've downloaded at least Clang 7.0.0 or higher, install it using the checkbox option "Add LLVM to the system PATH for all users"

Otherwise, use this command to set Clang's bin directory on the PATH:

    set PATH=C:\deps\llvm\bin;%PATH%
	
Finally, after Clang is installed correctly, use one of the following commands (depending on if you are running Visual Studio 2017 or Visual Studio 2019).

For Visual Studio 2017:

    gn gen out/Release --args="is_official_build=true skia_use_system_expat=false skia_use_system_libjpeg_turbo=false skia_use_system_libpng=false skia_use_system_libwebp=false skia_use_system_zlib=false target_cpu=""x64"" cc=""clang"" cxx=""clang++"" clang_win=""c:\deps\llvm"""
    ninja -C out/Release skia

For Visual Studio 2019:

    gn gen out/Release --args="is_official_build=true skia_use_system_expat=false skia_use_system_libjpeg_turbo=false skia_use_system_libpng=false skia_use_system_libwebp=false skia_use_system_zlib=false target_cpu=""x64"" cc=""clang"" cxx=""clang++"" clang_win=""c:\deps\llvm"" win_vc=""C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC"""
    ninja -C out/Release skia

If you haven't installed Clang, and want to compile Skia with MSVC,
you can use one of the following commands instead (depending on if you are running Visual Studio 2017 or Visual Studio 2019).

   *(but to reiterate, this mothod is not recommended because the performance penalty is usually too great)*

For Visual Studio 2017:

    gn gen out/Release --args="is_official_build=true skia_use_system_expat=false skia_use_system_libjpeg_turbo=false skia_use_system_libpng=false skia_use_system_libwebp=false skia_use_system_zlib=false target_cpu=""x64"" cc=2017"
    ninja -C out/Release skia
	
For Visual Studio 2019:

    gn gen out/Release --args="is_official_build=true skia_use_system_expat=false skia_use_system_libjpeg_turbo=false skia_use_system_libpng=false skia_use_system_libwebp=false skia_use_system_zlib=false target_cpu=""x64"" win_vc=""C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC"""
    ninja -C out/Release skia

More information about these steps in the
[official Skia documentation](https://skia.org/user/build).

## Skia on macOS

These steps will create a `deps` folder in your home directory with a
couple of subdirectories needed to build Skia (you can change the
`$HOME/deps` with other directory). Some of these commands will take
several minutes to finish:

    mkdir $HOME/deps
    cd $HOME/deps
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
    git clone -b aseprite-m71 https://github.com/aseprite/skia.git
    export PATH="${PWD}/depot_tools:${PATH}"
    cd skia
    python tools/git-sync-deps
    gn gen out/Release --args="is_official_build=true skia_use_system_expat=false skia_use_system_icu=false skia_use_libjpeg_turbo=false skia_use_system_libpng=false skia_use_system_libwebp=false skia_use_system_zlib=false extra_cflags_cc=[\"-frtti\"]"
    ninja -C out/Release skia

After this you should have all Skia libraries compiled.  When you
[compile Aseprite](#compiling), remember to add
`-DSKIA_DIR=$HOME/deps/skia` parameter to your `cmake` call as
described in the [macOS details](#macos-details) section.

More information about these steps in the
[official Skia documentation](https://skia.org/user/build).

## Skia on Linux

These steps will create a `deps` folder in your home directory with a
couple of subdirectories needed to build Skia (you can change the
`$HOME/deps` with other directory). Some of these commands will take
several minutes to finish:

    mkdir $HOME/deps
    cd $HOME/deps
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
    git clone -b aseprite-m71 https://github.com/aseprite/skia.git
    export PATH="${PWD}/depot_tools:${PATH}"
    cd skia
    python tools/git-sync-deps
    gn gen out/Release --args="is_debug=false is_official_build=true skia_use_system_expat=false skia_use_system_icu=false skia_use_system_libjpeg_turbo=false skia_use_system_libpng=false skia_use_system_libwebp=false skia_use_system_zlib=false"
    ninja -C out/Release skia

After this you should have all Skia libraries compiled.  When you
[compile Aseprite](#compiling), remember to add
`-DSKIA_DIR=$HOME/deps/skia` parameter to your `cmake` call as
described in the [Linux details](#linux-details) section.

More information about these steps in the
[official Skia documentation](https://skia.org/user/build).
