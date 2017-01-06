# Aseprite
*Copyright (C) 2001-2017 David Capello*

[![Build Status](https://travis-ci.org/aseprite/aseprite.svg)](https://travis-ci.org/aseprite/aseprite)
[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/aseprite/aseprite?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

## Introduction

**Aseprite** is a program to create animated sprites. Its main
features are:

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
* [Aseprite Steam Community](https://steamcommunity.com/app/431730/discussions/)
* [Aseprite mailing list](http://groups.google.com/group/aseprite-discuss) ([subscribe](mailto:aseprite-discuss+subscribe@googlegroups.com))
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

Aseprite uses [several third-party open source projects](docs/LICENSES.md).

## License

This program is distributed under three different licenses:

1. Source code and official releases/binaries are distributed under
   our [End-User License Agreement for Aseprite (EULA)](EULA.txt). Please check
   that there are [modules/libraries in the source code](src/README.md) that
   are distributed under the MIT license
   (e.g. [laf](https://github.com/aseprite/laf),
   [clip](https://github.com/aseprite/clip),
   [she](https://github.com/aseprite/aseprite/tree/master/src/she),
   [gfx](src/gfx), [ui](src/ui), etc.).
2. You can request a special
   [educational license](http://www.aseprite.org/faq/#is-there-an-educational-license)
   in case you are a teacher in an educational institution and want to
   use Aseprite in your classroom (in-situ).
3. Steam releases are distributed under the terms of the
   [Steam Subscriber Agreement](http://store.steampowered.com/subscriber_agreement/).

You can get more information about Aseprite license in the
[FAQ](http://www.aseprite.org/faq/#licensing-&-commercial).
