# What platforms are supported?

You should be able to compile Aseprite successfully on the following
platforms:

* Windows + MSVC 2012 + DirectX SDK
* Mac OS X 10.8 Mountain Lion + Mac OS X 10.4 SDK universal
* Linux + GCC

# How can I compile Aseprite?

The new build system for Aseprite is [CMake](http://www.cmake.org/).
You will not need any extra library because the repository already
contains the source code of all dependencies, even a modified version
of the Allegro library is included in master branch.

The following are the steps to compile Aseprite (in this case we have
the source code in a directory called `aseprite-source`):

1. Make a build directory to leave all the files that are result of
   the compilation process (`.exe`, `.lib`, `.obj`, `.a`, `.o`, etc).

        C:\...\>cd aseprite-source
        C:\...\aseprite-source>mkdir build

   In this way, if you want to start with a fresh copy of Aseprite
   source code, you can remove the `build` directory and start again.

2. Enter in the new directory and execute cmake giving to it
   your compiler as generator:

        C:\...\aseprite-source>cd build

   If you have nmake (MSVC compilers):

        C:\...\aseprite-source\build>cmake .. -G "NMake Makefiles"

   If you have Visual Studio you can generate a solution:

        C:\...\aseprite-source\build>cmake .. -G "Visual Studio 8 2005"
        C:\...\aseprite-source\build>cmake .. -G "Visual Studio 9 2008"
        C:\...\aseprite-source\build>cmake .. -G "Visual Studio 10"

   If you are on Linux:

        /.../aseprite-source/build$ cmake .. -G "Unix Makefiles"

   If you have MinGW + MSYS:

        C:\...\aseprite-source\build>cmake .. -G "MSYS Makefiles"

   If you have MinGW + mingw-make:

        C:\...\aseprite-source\build>cmake .. -G "MinGW Makefiles"

   For more information in [CMake wiki](http://www.vtk.org/Wiki/CMake_Generator_Specific_Information).
    
3. After you have executed one of the `cmake .. -G <generator>`
   commands, you have to compile the project executing make, nmake,
   opening the solution, etc.

4. When the project is compiled, you can copy the resulting executable
   file (e.g. `build/src/aseprite.exe`) to `aseprite-source` and
   execute it. If you have used a Visual Studio project, you can copy
   the whole `data/` directory to `build/src/RelWithDebInfo/` so you
   can run/debug the program from Visual Studio IDE. On Linux, you can
   copy the `data/` directory in `~/.aseprite/` directory.

# How to use installed third party libraries?

If you don't want to use the embedded code of third party libraries
(i.e. to use your installed versions), you can disable static linking
configuring each `USE_SHARED_` option.

After running `cmake -G`, you edit `build/CMakeCache.txt` file, and
enable the `USE_SHARED_` flag (set its value to `ON`) of the library
that you want to be linked dynamically.

# How to profile Aseprite?

You must compile with `Profile` configuration. For example on Linux:

    /.../aseprite-source/build$ cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE:STRING=Profile -DCOMPILER_GCC:BOOL=ON

# How to compile Aseprite with Mac OS X?

You need the old Mac OS X 10.4 SDK universal, which can be obtained
from a Xcode 3.2 distribution. You can get it from Apple developer
website (you need to be registered):

  https://developer.apple.com/downloads/

Install the MacOSX10.4.Universal.pkg and run cmake with the following
parameters:

    -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=10.4
    -DCMAKE_OSX_SYSROOT:STRING=/SDKs/MacOSX10.4u.sdk
