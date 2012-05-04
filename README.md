# ASEPRITE
*Copyright (C) 2001-2012 David Capello*

> THIS PROGRAM IS DISTRIBUTED WITHOUT ANY WARRANTY<br/>
> See the license section for more information.

## Introduction

**ASEPRITE** is an open source program to create animated
sprites. Sprites are little images that can be used in your website or
in a video game. You can draw characters with movement, intros,
textures, patterns, backgrounds, logos, color palettes, isometric
levels, etc.

What makes ASEPRITE different? It focuses on pixel editing, to do
pixel-art. Indeed, it isn't a photo retouching tool or a vector
graphics editor. Mainly it is a tool to create tiny animations
pixel-by-pixel.

## Features

The main features of ASEPRITE are:

* Sprites are composed by **layers** &amp; **frames**.
* Supported color modes: **RGBA**, **Indexed** (palettes up to 256
  colors), and Grayscale.
* Supported image formats (load/save): **PNG**, **GIF**, JPG, FLC,
  FLI, BMP, PCX, TGA, and ASE (special format).
* Load and save animations in a **sequence of bitmaps**
  (e.g. frame1.png, frame2.png, etc.).
* **Tiled** drawing mode, useful to draw **patterns** and textures.
* **Undo/Redo** for every operation.
* **Multiple editors** support. You can split an editor horizontally
  or vertically multiple times to edit and view the same sprite with
  different zooms, or different sprites.
* Import/Export **Sprite Sheets**.
* Pixel art specific tools like filled **Contour** &amp; **Polygon**.
* **Onion skinning**

## Configuration files

In Windows XP/Vista/7 the main configuration file is `aseprite.ini`
which is saved in the same folder of `aseprite.exe` executable file
(in this way ASEPRITE is a portable application, i.e. you can
transport a copy of the program in your USB drive).

The following is a list of all configuration files that you could
modify (it is not recommended to do so, but is useful if you want to
super-customize ASEPRITE):

    aseprite.ini          Program configuration
    data/gui.xml          Menus, shortcuts, and tools
    data/convmatr.def     Convolutions matrices
    data/skins/*.*        ASEPRITE skins
    data/widgets/*.xml    XML files with dialogs

In GNU/Linux, the configuration file is ~/.asepriterc, and the data/
files are searched in these locations (in priority order):

    $HOME/.aseprite/
    ../share/aseprite/
    ./data/

## Contact Information

You can report problems (bugs) or features in the Google Code project:

* [Bugs](http://code.google.com/p/aseprite/issues/entry)
* [Request features](http://code.google.com/p/aseprite/issues/entry?template=New%20feature)
* [ASEPRITE Group](http://groups.google.com/group/aseprite-discuss) ([subscribe](mailto:aseprite-discuss+subscribe@googlegroups.com))

For more information, visit the official page of the project:

   http://www.aseprite.org/

## License

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the [GNU General Public License](docs/licenses/GPL.txt)
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA

## Authors

David Capello [davidcapello@gmail.com](mailto:davidcapello@gmail.com)
> Programmer, designer, and maintainer. <br />
  http://dacap.com.ar/

Ilija Melentijevic
> New GUI skin (from ASEPRITE 0.8). A lot of good ideas. <br />
  http://ilkke.blogspot.com/ <br />
  http://www.pixeljoint.com/p/9270.htm

Contributors
> http://code.google.com/p/aseprite/people/list

Thanks to all the people who have contributed ideas, patches, bugs
report, feature requests, donations, and help me developing ASEPRITE.

## Credits

ASEPRITE uses libraries or parts of the original source code
of the following projects created by third-parties:

* [Allegro 4](http://alleg.sourceforge.net/) - [allegro4 license](docs/licenses/allegro4-LICENSE.txt)
* [AllegroFont](http://chernsha.sitesled.com/)  - [FTL license](docs/licenses/FTL.txt)
* [curl](http://curl.haxx.se/) - [curl license](docs/licenses/curl-LICENSE.txt)
* [FreeType](http://www.freetype.org/) - [FTL license](docs/licenses/FTL.txt)
* [giflib](http://sourceforge.net/projects/giflib/) - [giflib license](docs/licenses/giflib-LICENSE.txt)
* [GIMP](http://www.gimp.org/) - [GPL license](docs/licenses/GPL.txt)
* [GTK+](http://www.gtk.org/) - [LGPL license](docs/licenses/LGPL-2.1.txt)
* [Google Test](http://code.google.com/p/googletest/) - [gtest license](docs/licenses/gtest-LICENSE.txt)
* [libart](http://www.levien.com/libart/) - [LGPL license](docs/licenses/LGPL-2.0.txt)
* [libjpeg](http://www.ijg.org/) - [libjpeg license](docs/licenses/libjpeg-LICENSE.txt)
* [libpng](http://www.libpng.org/pub/png/) - [libpng license](docs/licenses/libpng-LICENSE.txt)
* [loadpng](http://tjaden.strangesoft.net/loadpng/) - [zlib license](docs/licenses/ZLIB.txt)
* [tinyxml](http://www.sourceforge.net/projects/tinyxml) - [zlib license](docs/licenses/ZLIB.txt)
* [XFree86](http://www.x.org/) - [XFree86 license](docs/licenses/XFree86-LICENSE.txt)
* [zlib](http://www.gzip.org/zlib/) - [ZLIB license](docs/licenses/ZLIB.txt)

Other parts of code by:

* Gary Oberbrunner: Code to quantize RGB images with ordered dither method.
