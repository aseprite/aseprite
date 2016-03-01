# Platforms

You should be able to compile Aseprite successfully on the following
platforms:

* Windows + VS2013 + DirectX SDK
* Mac OS X 10.8 Mountain Lion + Xcode 5.1.1 + Mac OS X 10.4 SDK universal
* Linux + gcc with some C++11 support, this port is not compiled
  regularly so you can expect some errors in the master branch.

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

# Compiling

Aseprite uses the latest version of [CMake](http://www.cmake.org/)
(3.0) as its build system. You will not need any extra library
because the repository already contains the source code of all
dependencies, even a modified version of the Allegro library is
included in master branch.

The following are the steps to compile Aseprite (in this case we have
the repository clone in a directory called `aseprite`):

1. Make a build directory to leave all the files that are result of
   the compilation process (`.exe`, `.lib`, `.obj`, `.a`, `.o`, etc).

        C:\...\>cd aseprite
        C:\...\aseprite>mkdir build

   In this way, if you want to start with a fresh copy of Aseprite
   source code, you can remove the `build` directory and start again.

2. Enter in the new directory and execute cmake giving to it
   your compiler as generator:

        C:\...\aseprite>cd build

   If you have nmake (MSVC compilers):

        C:\...\aseprite\build>cmake -G "NMake Makefiles" ..

   If you have Visual Studio you can generate a solution:

        C:\...\aseprite\build>cmake -G "Visual Studio 12 2013" ..

   If you are on Linux:

        /.../aseprite/build$ cmake -G "Unix Makefiles" ..

   For more information in [CMake wiki](http://www.vtk.org/Wiki/CMake_Generator_Specific_Information).

   Additionally you can change build settings by passing them on the
   command line, like so:

        /.../aseprite/build$ cmake -DCMAKE_INSTALL_PREFIX=~/software ..

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

## Linux details

You will need the following dependencies:

    sudo apt-get update -qq
    sudo apt-get install -y g++ libx11-dev libxcursor-dev cmake ninja-build

The `libxcursor-dev` package is needed to hide the hardware cursor:

    https://github.com/aseprite/aseprite/issues/913

## Mac OS X details

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

### Issues with Retina displays

If you have a Retina display, check this issue:

  https://github.com/aseprite/aseprite/issues/589

# Using shared third party libraries

If you don't want to use the embedded code of third party libraries
(i.e. to use your installed versions), you can disable static linking
configuring each `USE_SHARED_` option.

After running `cmake -G`, you can edit `build/CMakeCache.txt` file,
and enable the `USE_SHARED_` flag (set its value to `ON`) of the
library that you want to be linked dynamically.

# Profiling

You must compile with `Profile` configuration. For example on Linux:

    /.../aseprite/build$ cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE:STRING=Profile -DCOMPILER_GCC:BOOL=ON
