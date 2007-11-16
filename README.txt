
 ASE - Allegro Sprite Editor
 Copyright (C) 2001-2005, 2007 by David A. Capello
 --------------------------------------------------------------
 See the "AUTHORS.txt" file for a complete list of contributors


===================================
COPYRIGHT
===================================

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
  for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc., 59
  Temple Place, Suite 330, Boston, MA  02111-1307  USA


===================================
INTRODUCTION
===================================

  ASE is a program specially designed with facilities to create
  animated sprites that can be used in some video game.  This program
  let you create from static images, to characters with movement,
  textures, patterns, backgrounds, logos, color palettes, and any
  other thing that you think.


===================================
FEATURES
===================================

  ASE gives to you the possibility to:

  - Edit sprites with layers and animation frames.

  - Edit RGB (with Alpha), Grayscale (with Alpha also) and Indexed
    images.

  - Control 256 color palettes completely.

  - Apply filters for different color effects (convolution matrix,
    color curve, etc.).

  - Load and save sprites in these formats: .BMP, .PCX, .TGA, .JPG,
    .GIF, .FLC, .FLI, and .ASE (ASE's special format).

  - Use bitmap's sequences (ani00.pcx, ani01.pcx, etc.) to save
    animations.

  - Drawing tools (dots, pencil, real-brush, floodfill, line, rectangle,
    ellipse), drawing modes (opaque, glass), and brushes types (circle,
    square, line).

  - Mask (selections) support.

  - Undo/Redo support for every operation.

  - Multiple editor support.

  - Draw with a customizable grid.

  - Unique tiled drawing mode to draw patterns and textures in seconds.

  - Scripting capabilities with Lua language (http://www.lua.org).


===================================
CONFIGURATION FILES
===================================

  In Windows and DOS platforms:

    ase.cfg			- Configuration
    data/matrices		- Convolutions matrices
    data/menus			- Menus
    data/scripts/*		- Scripts

  In Unix platforms, the configuration file is ~/.aserc, and the data/
  files are searched in these locations (in order of preference):

    $HOME/.ase/
    /usr/local/share/ase/
    data/

  See "src/core/dirs.c" for more information.


===================================
VERBOSE MODE
===================================

  When run "ase" with "-v" parameter, in Windows and DOS platforms the
  errors will be written in STDERR and a "logXXXX.txt" file in "ase/"
  directory is created with the same content.

  In others platforms (like Unix), that log file isn't created,
  because the use of STDERR is more common.

  See "src/core/core.c" for more information.


===================================
UPDATES
===================================

  The last packages of binaries and source code, you can found them
  from:

    http://sourceforge.net/project/showfiles.php?group_id=20848

  Also, if you want to get the last development version of ASE from
  the SVN repository, which is the version more prone to have errors,
  but is the more updated in the tools area, you can browse it file by
  file in this address:

    http://ase.svn.sourceforge.net/viewvc/ase/

  Or you can download it completelly to your disk with a program which
  control CVS, as follow-up:

    1) We must input to the repository in anonymous form (when you be
       asked for the password, just press <enter>):

         cvs -d :pserver:anonymous@cvs.ase.sourceforge.net:/cvsroot/ase login

    2) We make the first checkout, which means that we fetch a "fresh"
       copy of ASE:

         cvs -z3 -d :pserver:anonymous@cvs.ase.sourceforge.net:/cvsroot/ase checkout gnuase

    3) Then, whenever you want update your copy with the repository
       one, you will must execute in the gnuase/ directory:

         cvs update -Pd

  WARNING: When you obtain the CVS version, don't remove the CVS
  directories, they are for exclusive use of the cvs program.


===================================
CREDITS
===================================

  See the "AUTHORS.txt" file.


===================================
CONTACT INFO
===================================

  To request help, report bugs, send patches, etc., you can use the
  ase-help mailing list:

    ase-help@lists.sourceforge.net
    http://lists.sourceforge.net/lists/listinfo/ase-help/

  For more information, visit the official page of the project:

    http://ase.sourceforge.net


 ----------------------------------------------------------------------------
 Copyright (C) 2001-2005 by David A. Capello
