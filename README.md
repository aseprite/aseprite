# Aseprite
*Copyright (C) 2001-2016 David Capello*

[![Build Status](https://travis-ci.org/aseprite/aseprite.svg)](https://travis-ci.org/aseprite/aseprite)
[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/aseprite/aseprite?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

> THIS PROGRAM IS DISTRIBUTED WITHOUT ANY WARRANTY<br/>
> See the [license section](#license) for more information.

## Introduction

**Aseprite** is an open source program to create animated sprites.
Its main features are:

* Sprites are composed by [**layers** &amp; **frames**](http://www.aseprite.org/docs/timeline/) (as separated concepts).
* Supported [color modes](http://www.aseprite.org/docs/color/): **RGBA**, **Indexed** (palettes up to 256
  colors), and Grayscale.
* Load/save sequence of **PNG** files and **GIF** animations (and
  FLC, FLI, JPG, BMP, PCX, TGA).
* Export/import animations to/from **Sprite Sheets**.
* **Tiled** drawing mode, useful to draw **patterns** and textures.
* **Undo/Redo** for every operation.
* Real-time **animation preview**.
* [**Multiple editors**](http://www.aseprite.org/docs/workspace/#drag-and-drop-tabs) support.
* Pixel-art specific tools like filled **Contour**, **Polygon**, [**Shading**](http://www.aseprite.org/docs/shading/) mode, etc.
* **Onion skinning**

## Issues

There is a list of
[Known Issues](https://github.com/aseprite/aseprite/issues) (things
to be fixed or that aren't yet implemented).

If you found a bug or have a new idea/feature for the program,
[you can report them](https://github.com/aseprite/aseprite/issues/new).

## Support

You can ask for help in:

* Our official support email address: [support@aseprite.org](mailto:support@aseprite.org)
* [Aseprite group/mailing list](http://groups.google.com/group/aseprite-discuss) ([subscribe](mailto:aseprite-discuss+subscribe@googlegroups.com))
* Social networks and community-driven places:
  [Twitter](https://twitter.com/aseprite/),
  [Facebook](https://facebook.com/aseprite/),
  [YouTube](https://www.youtube.com/user/aseprite),
  [Google+](https://plus.google.com/+AsepriteOrg/posts),
  [IRC](http://webchat.freenode.net/?channels=aseprite),
  [DeviantArt](https://aseprite.deviantart.com/).

## Authors

* David Capello [davidcapello@gmail.com](mailto:davidcapello@gmail.com) <br />
  Programmer, designer, and maintainer. <br />
  http://davidcapello.com/
* Ilija Melentijevic <br />
  New GUI skin for Aseprite v0.8. A lot of good ideas. <br />
  http://ilkke.blogspot.com/ <br />
  http://www.pixeljoint.com/p/9270.htm
* Contributors <br />
  http://www.aseprite.org/contributors/

Thanks to all the people who have contributed ideas, patches, bugs
report, feature requests, donations, and help me developing Aseprite.

## Credits

Aseprite includes color palettes created by:

* [Richard "DawnBringer" Fhager](http://pixeljoint.com/p/23821.htm) palettes, [16 colors](http://pixeljoint.com/forum/forum_posts.asp?TID=12795),  [32 colors](http://pixeljoint.com/forum/forum_posts.asp?TID=16247).
* [Arne Niklas Jansson](http://androidarts.com/) palettes, [16 colors](http://androidarts.com/palette/16pal.htm), [32 colors](http://wayofthepixel.net/index.php?topic=15824.msg144494).

It tries to replicate some pixel-art algorithms:

* [RotSprite](http://forums.sonicretro.org/index.php?showtopic=8848&st=15&p=159754&#entry159754) by Xenowhirl.
* [Pixel perfect drawing algorithm](http://deepnight.net/pixel-perfect-drawing/) by [Sébastien Bénard](https://twitter.com/deepnightfr) and [Carduus](https://twitter.com/CarduusHimself/status/420554200737935361).

And it uses the following third-party libraries:

* [Allegro 4](http://alleg.sourceforge.net/) - [allegro4 license](https://github.com/aseprite/aseprite/tree/master/docs/licenses/allegro4-LICENSE.txt)
* [FreeType](http://www.freetype.org/) - [FTL license](https://github.com/aseprite/aseprite/tree/master/docs/licenses/FTL.txt)
* [Google Test](https://github.com/google/googletest) - [gtest license](https://github.com/aseprite/aseprite/tree/master/docs/licenses/gtest-LICENSE.txt)
* [XFree86](http://www.x.org/) - [XFree86 license](https://github.com/aseprite/aseprite/tree/master/docs/licenses/XFree86-LICENSE.txt)
* [curl](http://curl.haxx.se/) - [curl license](https://github.com/aseprite/aseprite/tree/master/docs/licenses/curl-LICENSE.txt)
* [giflib](http://sourceforge.net/projects/giflib/) - [giflib license](https://github.com/aseprite/aseprite/tree/master/docs/licenses/giflib-LICENSE.txt)
* [libjpeg](http://www.ijg.org/) - [libjpeg license](https://github.com/aseprite/aseprite/tree/master/docs/licenses/libjpeg-LICENSE.txt)
* [libpng](http://www.libpng.org/pub/png/) - [libpng license](https://github.com/aseprite/aseprite/tree/master/docs/licenses/libpng-LICENSE.txt)
* [libwebp](https://developers.google.com/speed/webp/) - [libwebp license](https://chromium.googlesource.com/webm/libwebp/+/master/COPYING)
* [loadpng](http://tjaden.strangesoft.net/loadpng/) - [zlib license](https://github.com/aseprite/aseprite/tree/master/docs/licenses/ZLIB.txt)
* [mongoose](https://github.com/valenok/mongoose) - [MIT license](https://github.com/valenok/mongoose/blob/master/LICENSE)
* [pixman](http://www.pixman.org/) - [MIT license](http://cgit.freedesktop.org/pixman/plain/COPYING)
* [simpleini](https://github.com/aseprite/simpleini/) - [MIT license](https://github.com/aseprite/simpleini/blob/aseprite/LICENCE.txt)
* [tinyxml](http://www.sourceforge.net/projects/tinyxml) - [zlib license](https://github.com/aseprite/aseprite/tree/master/docs/licenses/ZLIB.txt)
* [zlib](http://www.gzip.org/zlib/) - [ZLIB license](https://github.com/aseprite/aseprite/tree/master/docs/licenses/ZLIB.txt)
* [modp_b64](https://github.com/aseprite/aseprite/tree/master/third_party/modp_b64/modp_b64.h) - [BSD license](https://github.com/aseprite/aseprite/tree/master/third_party/modp_b64/LICENSE)

## License

This program is distributed under three different licenses:

1. Source code is distributed under
   [GNU General Public License](LICENSE.txt) version 2,
   which means that compiled versions can be generated under GPL terms.
   (Useful for Linux distributions.)
2. Official releases are distributed under a [commercial EULA](EULA.txt).
3. And Steam releases are distributed under the terms of the
   [Steam Subscriber Agreement](http://store.steampowered.com/subscriber_agreement/).

As a side note, to distribute Aseprite under these licenses we don't
use third party GPL libraries.
