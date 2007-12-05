
                    FreeType 2 compilation how-to
                    =============================


Introduction
------------

Welcome  to the  FreeType 2  library.   You'll find  in this  document
instructions on how to compile the library on your favorite platform.


I. QUICK COMMAND-LINE GUIDE
---------------------------

  THE FOLLOWING WILL  ONLY WORK WITH GNU MAKE,  IT WILL FAIL MISERABLY
  WITH OTHER MAKE TOOLS, FOR EXAMPLE "BSD MAKE".

  Install GNU Make  (version 3.78.1 or newer), then  try the following
  on Unix or any system with gcc:

      make    // this will setup the build
      make    // this will build the library

  On Win32 + Visual C++:

      make setup visualc    // setup the build for VisualC++ on Win32
      make                  // build the library

  Then, go to the `demos' directory and type

      make


  Note that on Unix, the  first `make' invocation will run a configure
  script (which is located  in `freetype2/builds/unix/'.  You can also
  pass parameters to this script with the CFG variable, as in:

      make CFG="--prefix=/usr/local"
      make


  If this doesn't work, read the following.


II. COMMAND-LINE COMPILATION
----------------------------

  Note that  if you do not want  to compile FreeType 2  from a command
  line shell, please skip to section III below (DETAILED COMPILATION).

  FreeType 2 includes a powerful and flexible build system that allows
  you to  easily compile it on  a great variety of  platforms from the
  command line.  To do so, just follow these simple instructions:

  a. Install GNU Make

    Because  GNU Make  is  the  only Make  tool  supported to  compile
    FreeType 2, you should install it on your machine.

    The FreeType 2 build system relies on many features special to GNU
    Make -- trying to build the  library with any other Make tool will
    *fail*.

    NEARLY ALL OTHER MAKE TOOLS WILL FAIL, INCLUDING "BSD MAKE", SO
    REALLY INSTALL A RECENT VERSION OF GNU MAKE ON YOUR SYSTEM!

    Make sure that you are invoking GNU Make from the command line, by
    typing something like:

        make -v

    to display its version number.

    VERSION 3.78.1 OR NEWER IS NEEDED!

  b. Invoke `make'

    Go to  the root  directory of FreeType  2, then simply  invoke GNU
    Make from the command line.   This will launch the FreeType 2 host
    platform  detection routines.   A summary  will be  displayed, for
    example, on Win32:


        ==============================================================
        FreeType build system -- automatic system detection

        The following settings are used:

          platform                     win32
          compiler                     gcc
          configuration directory      ./builds/win32
          configuration rules          ./builds/win32/w32-gcc.mk

        If this does not correspond to your system or settings please
        remove the file 'config.mk' from this directory then read the
        INSTALL file for help.

        Otherwise, simply type 'make' again to build the library.
        =============================================================


    If the detected settings correspond to your platform and compiler,
    skip to step e.  Note that if your platform is completely alien to
    the build system, the detected platform will be `ansi'.

  c. Configure the build system for a different compiler

    If the build system correctly detected your platform, but you want
    to use a different compiler  than the one specified in the summary
    (for most platforms, gcc is  the defaut compiler), invoke GNU Make
    with

         make setup <compiler>

    For example:

        to use Visual C++ on Win32, type: "make setup visualc"
        to use LCC-Win32 on Win32, type:  "make setup lcc"

    The  <compiler> name to  use is  platform-dependent.  The  list of
    available  compilers for  your  system is  available  in the  file
    `builds/<system>/detect.mk' (note  that we  hope to make  the list
    displayed at user demand in the final release).

    If you  are satisfied  by the new  configuration summary,  skip to
    step e.

  d. Configure the build system for an unknown platform/compiler

    The auto-detection/setup  phase of the build system  copies a file
    to the current directory under the name `config.mk'.

    For    example,    on    OS/2+gcc,    it   would    simply    copy
    `builds/os2/os2-gcc.mk' to `./config.mk'.

    If for  some reason your  platform isn't correctly  detected, copy
    manually the configuration sub-makefile to `./config.mk' and go to
    step e.

    Note  that  this file  is  a  sub-Makefile  used to  specify  Make
    variables  for compiler  and linker  invocation during  the build.
    You can  easily create your own  version from one  of the existing
    configuration files,  then copy it to the  current directory under
    the name `./config.mk'.

  e. Build the library

    The auto-detection/setup  phase should have  copied a file  in the
    current  directory,  called   `./config.mk'.  This  file  contains
    definitions of various Make  variables used to invoke the compiler
    and linker during the build.

    To  launch  the build,  simply  invoke  GNU  Make again:  The  top
    Makefile will detect the configuration file and run the build with
    it.

  f. Build the demonstration programs


III. DETAILED COMPILATION PROCEDURE
-----------------------------------

  If you don't  want to compile FreeType 2  from the command-line (for
  example if  you use a graphical IDE  on a Mac or  Windows), you will
  need to understand how the FreeType files are organized.

  FreeType 2  has a  very modular  design, and it  is made  of several
  components.  Each component must be compiled as a stand-alone object
  file, even  if it  is really  made of several  C source  files.  For
  example,  the `base  layer' component  is  made of  the following  C
  files:

    src/
      base/
        ftcalc.c    - computations
        ftextend.c  - extensions support
        ftlist.c    - simple list management
        ftobjs.c    - object management
        ftoutln.c   - simple outline processing
        ftstream.c  - stream input

  However, you can  create a single object file  by compiling the file
  `src/base/ftbase.c', which basically contains

      #include "ftcalc.c"
      #include "ftobjs.c"
      #include "ftstream.c"
      #include "ftlist.c"
      #include "ftoutln.c"
      #include "ftextend.c"

  Similarly, each component has a single `englobing' C file to compile
  it as a stand-alone object:

    src/autohint/autohint.c   - the autohinting module
    src/base/ftbase.c         - the base layer, high-level interface
    src/cache/ftcache.c       - a glyph and image caching system
                                (still experimental)
    src/cff/cff.c             - the OpenType font driver
    src/cid/type1cid.c        - the CID-keyed font driver
    src/psaux/psaux.c         - the PS support module
    src/psnames/psnames.c     - a support module to handle PS glyph
                                names
    src/raster1/raster1.c     - the monochrome raster module
    src/sfnt/sfnt.c           - the `sfnt' module
    src/smooth/smooth.c       - the anti-aliasing raster module
    src/truetype/truetype.c   - the TrueType font driver
    src/type1/type1.c         - the Type 1 font driver

  The last module of  FreeType 2, `winfonts' (implementing support for
  Windows FNT format), is a single file.

  To compile one component, do the following:

    - Add  the  top-level  `include'  directory  to  your  compilation
      include path

    - Add the `src/<component>'  directory to your compilation include
      path, or simply `cd' to the component's source directory.

    - Compile the component `source'  file (see list below); you don't
      need  to   be  in  the   component's  directory  if   you  added
      `src/<component>' to your include path.

  For example, the following line  can be used to compile the truetype
  driver on Unix:

     cd freetype2/
     cc -c -Iinclude -Isrc/truetype src/truetype/truetype.c

  Alternatively:

     cd freetype2/src/truetype
     cc -c -I../../include truetype.c

  Finally, FreeType 2 contains some other components:

     src/base/ftsystem.c  - system-specific memory and i/o support
     src/base/ftinit.c    - initialization layer
     src/base/ftdebug.c   - debugging component (empty in release
                            build)
     src/base/ftglyph.c   - optional convenience functions

  All font  drivers are optional.  The `sfnt',  `psaux', and `psnames'
  modules are mandatory for certain drivers.  However, you may need to
  update  the  list of  drivers  that  are  statically linked  to  the
  library,    which   is   located    in   the    configuration   file
  `include/freetype/config/ftmodule.h'.


IV. Support for flat-directory compilation
------------------------------------------

  It is  possible to  put all  FreeType 2 source  files into  a single
  directory, with the exception of the `include' hierarchy.

    1. Copy all files in current directory:

        cp freetype2/src/base/*.[hc] .
        cp freetype2/src/raster1/*.[hc] .
        cp freetype2/src/smooth/*.[hc] .
        etc.

    2. Compile sources:

        cc -c -Ifreetype2/include ftsystem.c
        cc -c -Ifreetype2/include ftinit.c
        cc -c -Ifreetype2/include ftdebug.c
        cc -c -Ifreetype2/include ftbase.c
        etc.

   You don't need to define the FT_FLAT_COMPILATION macro (as this was
   required in previous releases of FreeType 2).

End of file
