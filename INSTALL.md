# Installation guide

> [!NOTE]
> If you're compiling an older version of Aseprite, follow its bundled `INSTALL.md`. Also, note that cross-compiling is not supported; compile on the same platform you intend to use Aseprite.

## Platforms

You should be able to compile Aseprite successfully on the following
platforms.

| OS | Dev Environment | SDK |
| --- | --- | --- |
| Windows 11 or 10 | Visual Studio Community 2022 | Windows 10 SDK |
| macOS Ventura 13.0.1 | Xcode 14.1 | macOS 11.3 SDK<br/>*Older versions might work.* |
| Linux | Clang v10.0 or newer |

> Newer versions may be compatible.

## 1. Get the source code

### Option 1: Manual
Go to the [Releases](https://github.com/aseprite/aseprite/releases) section and download *`Aseprite-vX.X.X-Source.zip`* file from the latest available release.

### Option 2: Git
Clone the repository and all its submodules using the following command:

```shell
git clone --recursive https://github.com/aseprite/aseprite.git
```

#### To update an existing clone:
```shell
cd aseprite
git pull
git submodule update --init --recursive
```

## 2. Dependencies

To compile Aseprite you will need:

| Package | Version | Note |
| - | - | - |
| [CMake](https://cmake.org) | Latest *(minimum v3.16)* | Make available in PATH environment variable |
| [Ninja](https://ninja-build.org) | Latest | Make available in PATH environment variable |
| [aseprite/Skia](https://github.com/aseprite/skia/releases) | Skia-m102 | See their release notes |

> Visit each link for more information on how to download pre-built packages or how to compile them yourself.

### 2.1 Additional dependencies *(platform-specific)*

<details>
<summary><h4>For Windows</h4></summary>

- [Visual Studio Community 2022](https://visualstudio.microsoft.com/downloads/)

Using `Visual Studio Installer`, install the `Desktop development with C++` item and its following components:

- Windows 10 SDK (10.0.18362.0)
- MSVC v143 - VS 2022 C++ x64/x86 build tools *(or newer)*

</details>

<details>
<summary><h4>For macOS</h4></summary>

You will need `macOS 11.3 SDK` and `Xcode 13.1`

*Older versions might work*.
</details>

<details>
<summary><h4>For Linux</h4></summary>

- Clang v10.0 or newer

##### Ubuntu/Debian

```shell
sudo apt-get install -y g++ clang libc++-dev libc++abi-dev cmake ninja-build libx11-dev libxcursor-dev libxi-dev libgl1-mesa-dev libfontconfig1-dev
```

If Clang in your distribution is older than v10.0, use `clang-10` packages or newer:

```shell
sudo apt-get install -y clang-10 libc++-10-dev libc++abi-10-dev
```

##### Fedora

```shell
sudo dnf install -y gcc-c++ clang libcxx-devel cmake ninja-build libX11-devel libXcursor-devel libXi-devel mesa-libGL-devel fontconfig-devel
```

##### Arch

```shell
sudo pacman -S gcc clang libc++ cmake ninja libx11 libxcursor mesa-libgl fontconfig
```

##### SUSE

```shell
sudo zypper install gcc-c++ clang libc++-devel libc++abi-devel cmake ninja libX11-devel libXcursor-devel libXi-devel Mesa-libGL-devel fontconfig-devel
```
</details>

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
        cmake -G Ninja -DLAF_BACKEND=skia ..

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

Open a command prompt window (`cmd.exe`) and call:

    call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64

The command above is required while using the 64-bit version of skia. When compiling with the 32-bit version, it is possible to open a [developer command prompt](https://docs.microsoft.com/en-us/dotnet/framework/tools/developer-command-prompt-for-vs) instead.

And then

    cd aseprite
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLAF_BACKEND=skia -DSKIA_DIR=C:\deps\skia -DSKIA_LIBRARY_DIR=C:\deps\skia\out\Release-x64 -DSKIA_LIBRARY=C:\deps\skia\out\Release-x64\skia.lib -G Ninja ..
    ninja aseprite

In this case, `C:\deps\skia` is the directory where Skia was compiled
or uncompressed.

### MinGW

We don't support MinGW compiler and it might bring some problems into
the compilation process. If you see that the detected C++ compiler by
cmake is `C:\MinGW\bin\c++.exe` or something similar, you have to get
rid of MinGW path (`C:\MinGW\bin`) from the `PATH` environment
variable and run cmake again from scratch, so the Visual Studio C++
compiler (`cl.exe`) is used instead.

You can define the `CMAKE_IGNORE_PATH` variable when running cmake for
the first time in case that you don't know or don't want to modify the
`PATH` variable, e.g.:

    cmake -DCMAKE_IGNORE_PATH=C:\MinGW\bin ...

More information in [issue #2449](https://github.com/aseprite/aseprite/issues/2449)

## macOS details

Run `cmake` with the following parameters and then `ninja`:

    cd aseprite
    mkdir build
    cd build
    cmake \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_OSX_ARCHITECTURES=x86_64 \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 \
      -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk \
      -DLAF_BACKEND=skia \
      -DSKIA_DIR=$HOME/deps/skia \
      -DSKIA_LIBRARY_DIR=$HOME/deps/skia/out/Release-x64 \
      -DSKIA_LIBRARY=$HOME/deps/skia/out/Release-x64/libskia.a \
      -G Ninja \
      ..
    ninja aseprite

In this case, `$HOME/deps/skia` is the directory where Skia was
compiled or downloaded.  Make sure that `CMAKE_OSX_SYSROOT` is
pointing to the correct SDK directory (in this case
`/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk`),
but it could be different in your Mac.

### Apple Silicon

If you running macOS on an ARM64/AArch64/Apple Silicon Mac (e.g. M1),
you can compile a native ARM64 version of Aseprite following similar 
steps as above but when we call `cmake`, we have some differences:

    cd aseprite
    mkdir build
    cd build
    cmake \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_OSX_ARCHITECTURES=arm64 \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
      -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk \
      -DLAF_BACKEND=skia \
      -DSKIA_DIR=$HOME/deps/skia \
      -DSKIA_LIBRARY_DIR=$HOME/deps/skia/out/Release-arm64 \
      -DSKIA_LIBRARY=$HOME/deps/skia/out/Release-arm64/libskia.a \
      -DPNG_ARM_NEON:STRING=on \
      -G Ninja \
      ..
      ninja aseprite

### Issues with Retina displays

If you have a Retina display, check the following issue:

  https://github.com/aseprite/aseprite/issues/589

## Linux details

You need to use clang and libc++ to compile Aseprite:

    cd aseprite
    mkdir build
    cd build
    export CC=clang
    export CXX=clang++
    cmake \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_CXX_FLAGS:STRING=-stdlib=libc++ \
      -DCMAKE_EXE_LINKER_FLAGS:STRING=-stdlib=libc++ \
      -DLAF_BACKEND=skia \
      -DSKIA_DIR=$HOME/deps/skia \
      -DSKIA_LIBRARY_DIR=$HOME/deps/skia/out/Release-x64 \
      -DSKIA_LIBRARY=$HOME/deps/skia/out/Release-x64/libskia.a \
      -G Ninja \
      ..
    ninja aseprite

In this case, `$HOME/deps/skia` is the directory where Skia was
compiled or uncompressed.

### GCC compiler

In case that you are using the pre-compiled Skia version, you must use
the clang compiler and libc++ to compile Aseprite. Only if you compile
Skia with GCC, you will be able to compile Aseprite with GCC, and this
is not recommended as you will have a performance penalty doing so.

# Using shared third party libraries

If you don't want to use the embedded code of third party libraries
(i.e. to use your installed versions), you can disable static linking
configuring each `USE_SHARED_` option.

After running `cmake -G`, you can edit `build/CMakeCache.txt` file,
and enable the `USE_SHARED_` flag (set its value to `ON`) of the
library that you want to be linked dynamically.
