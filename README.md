# Aseprite

[![Build Status](https://travis-ci.org/aseprite/aseprite.svg)](https://travis-ci.org/aseprite/aseprite)
[![Build status](https://ci.appveyor.com/api/projects/status/kdu2gt7ls014i25h?svg=true)](https://ci.appveyor.com/project/dacap/aseprite)
[![Discourse Community](https://img.shields.io/badge/discourse-community-brightgreen.svg?style=flat)](https://community.aseprite.org/)
[![Discord Server](https://discordapp.com/api/guilds/324979738533822464/embed.png)](https://discord.gg/Yb2CeX8)

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
* [**Onion skinning**](https://www.aseprite.org/docs/animation/#onion-skinning)

## Issues

There is a list of
[Known Issues](https://github.com/aseprite/aseprite/issues) (things
to be fixed or that aren't yet implemented).

If you found a bug or have a new idea/feature for the program,
[you can report them](https://github.com/aseprite/aseprite/issues/new).

## Support

You can ask for help in:

* [Aseprite Community](https://community.aseprite.org/)
* [Aseprite Discord Server](https://discord.gg/Yb2CeX8)
* Official support: [support@aseprite.org](mailto:support@aseprite.org)
* Social networks and community-driven places:
  [Twitter](https://twitter.com/aseprite/),
  [Facebook](https://facebook.com/aseprite/),
  [YouTube](https://www.youtube.com/user/aseprite),
  [Google+](https://plus.google.com/+AsepriteOrg/posts),
  [IRC](http://webchat.freenode.net/?channels=aseprite),
  [DeviantArt](https://aseprite.deviantart.com/).

## Authors

[Igara Studio](https://www.igarastudio.com/) is developing Aseprite:

* [David Capello](https://davidcapello.com/): Lead developer, fixing
  issues, new features, and user support.
* [Gaspar Capello](https://github.com/Gasparoken): Developer, fixing
  issues and new features.

## Credits

The default Aseprite theme was introduced in v0.8, created by:

* [Ilija Melentijevic](https://ilkke.net/)

Aseprite includes color palettes created by:

* [Richard "DawnBringer" Fhager](http://pixeljoint.com/p/23821.htm), [16 colors](http://pixeljoint.com/forum/forum_posts.asp?TID=12795),  [32 colors](http://pixeljoint.com/forum/forum_posts.asp?TID=16247).
* [Arne Niklas Jansson](http://androidarts.com/), [16 colors](http://androidarts.com/palette/16pal.htm), [32 colors](http://wayofthepixel.net/index.php?topic=15824.msg144494).
* [ENDESGA Studios](https://twitter.com/ENDESGA), [EDG16 and EDG32](https://forums.tigsource.com/index.php?topic=46126.msg1279124#msg1279124), and [other palettes](https://twitter.com/ENDESGA/status/865812366931353600)
* [Hyohnoo Games](https://twitter.com/Hyohnoo), [mail24](https://twitter.com/Hyohnoo/status/797472587974639616) palette.
* [Davit Masia](https://twitter.com/DavitMasia), [matriax8c](https://twitter.com/DavitMasia/status/834862452164612096) palette.
* [Javier Guerrero](https://twitter.com/Xavier_Gd), [nyx8](https://twitter.com/Xavier_Gd/status/868519467864686594) palette.
* [Adigun A. Polack](https://twitter.com/adigunpolack), [AAP-64](http://pixeljoint.com/pixelart/119466.htm), [AAP-Splendor128](http://pixeljoint.com/pixelart/120714.htm), [SimpleJPC-16](http://pixeljoint.com/pixelart/119844.htm), and [AAP-Micro12](http://pixeljoint.com/pixelart/121151.htm) palette.
* [PineTreePizza](https://twitter.com/PineTreePizza), [Rosy-42](https://twitter.com/PineTreePizza/status/1006536191955623938) palette

It tries to replicate some pixel-art algorithms:

* [RotSprite](http://forums.sonicretro.org/index.php?showtopic=8848&st=15&p=159754&#entry159754) by Xenowhirl.
* [Pixel perfect drawing algorithm](http://deepnight.net/pixel-perfect-drawing/) by [Sébastien Bénard](https://twitter.com/deepnightfr) and [Carduus](https://twitter.com/CarduusHimself/status/420554200737935361).

Thanks to [third-party open source projects](docs/LICENSES.md), to
[contributors](http://www.aseprite.org/contributors/), and all the
people who have contributed ideas, patches, bugs report, feature
requests, donations, and help me to develop Aseprite.

## License

This program is distributed under three different licenses:

1. Source code and official releases/binaries are distributed under
   our [End-User License Agreement for Aseprite (EULA)](EULA.txt). Please check
   that there are [modules/libraries in the source code](src/README.md) that
   are distributed under the MIT license
   (e.g. [laf](https://github.com/aseprite/laf),
   [clip](https://github.com/aseprite/clip),
   [undo](https://github.com/aseprite/undo),
   [observable](https://github.com/aseprite/observable),
   [ui](src/ui), etc.).
2. You can request a special
   [educational license](http://www.aseprite.org/faq/#is-there-an-educational-license)
   in case you are a teacher in an educational institution and want to
   use Aseprite in your classroom (in-situ).
3. Steam releases are distributed under the terms of the
   [Steam Subscriber Agreement](http://store.steampowered.com/subscriber_agreement/).

You can get more information about Aseprite license in the
[FAQ](http://www.aseprite.org/faq/#licensing-&-commercial).
