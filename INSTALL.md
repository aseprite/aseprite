# What platforms are supported?

You should be able to compile ASEPRITE successfully on the following
platforms:

* Windows + MSVC 2008 Express + DirectX SDK
* Linux + GCC
* Mac OS X

# How can I compile ASEPRITE?

The new build system for ASEPRITE is [CMake](http://www.cmake.org/).
The following are the steps to compile ASEPRITE (in this case we have
the source code in a directory called `aseprite-source`):

1. Make a build directory to leave all the files that are result of
   the compilation process (`.exe`, `.lib`, `.obj`, `.a`, `.o`, etc).

        C:\...\>cd aseprite-source
        C:\...\aseprite-source>mkdir build

   In this way, if you want to start with a fresh copy of ASEPRITE
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

4. When the project is compiled, you can copy `build/src/aseprite.exe`
   file to `aseprite-source` and execute it. If you have used a Visual
   Studio project, you can copy the whole `data/` directory to
   `build/src/RelWithDebInfo/` so you can run/debug the program from
   Visual Studio IDE.

# How to profile ASEPRITE?

You must compile with `Profile` configuration. For example on Linux:

    /.../aseprite-source/build$ cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE:STRING=Profile -DCOMPILER_GCC:BOOL=ON
