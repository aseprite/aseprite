# Aseprite Source Code

If you are here is because you want to learn about Aseprite source
code. We'll try to write in these `README.md` files a summary of each
module/library.

# Modules & Libraries

Aseprite is separated in the following layers/modules:

## Level 0: Completely independent modules

These libraries are easy to be used and embedded in other software
because they don't depend on any other component.

  * [allegro](allegro/): Modified version of [Allegro](http://alleg.sourceforge.net/) library, used for keyboard/mouse input, and drawing 2D graphics on screen.
  * [base](base/): Core/basic stuff, multithreading, utf8, sha1, file system, memory, etc.
  * [css](css/): Pseudo-style sheet library.
  * [gfx](gfx/): Abstract graphics structures like point, size, rectangle, region, color, etc.
  * [scripting](scripting/): JavaScript engine ([V8](https://code.google.com/p/v8/)).
  * [undo](undo/): Generic library to manage undo history of undoable actions.

## Level 1

  * [cfg](cfg/) (base): Library to load/save .ini files.
  * [gen](gen/) (base): Helper utility to generate C++ files from different XMLs.
  * [net](net/) (base): Networking library to send HTTP requests.
  * [raster](raster/) (base, gfx): Library to handle graphics entities like sprites, images, frames.
  * [she](she/) (base, gfx, allegro): A wrapper for the Allegro library.
  * [webserver](webserver/) (base): HTTP web server (based on [mongoose](https://github.com/valenok/mongoose))

## Level 2

  * [doc](doc/) (raster, base, gfx): Document model library.
  * [filters](filters/) (base, gfx, raster): FX for raster images.
  * [ui](ui/) (base, gfx, she): Portable UI library (buttons, windows, text fields, etc.)
  * [updater](updater/) (base, net): Component to check for updates.

## Level 3

  * [iff](iff/) (base, doc): Image File Formats library (load/save documents).

## Level 4

  * [app](app/) (allegro, base, doc, filters, gfx, iff, raster, scripting, she, ui, undo, updater, webserver)

## Level 5

  * [main](main/) (app, base, she, ui)
