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
  * [flic](flic/): Library to load/save FLI/FLC files
  * [gfx](gfx/): Abstract graphics structures like point, size, rectangle, region, color, etc.
  * [scripting](scripting/): JavaScript engine ([V8](https://code.google.com/p/v8/)).
  * [undo](undo/): Generic library to manage a history of undoable commands.

## Level 1

  * [cfg](cfg/) (base): Library to load/save .ini files.
  * [doc](doc/) (base, gfx): Document model library.
  * [gen](gen/) (base): Helper utility to generate C++ files from different XMLs.
  * [net](net/) (base): Networking library to send HTTP requests.
  * [she](she/) (base, gfx, allegro): A wrapper for the Allegro library.
  * [webserver](webserver/) (base): HTTP web server (based on [mongoose](https://github.com/valenok/mongoose))

## Level 2

  * [filters](filters/) (base, doc, gfx): Effects for images.
  * [render](render/) (base, gfx, doc): Library to render documents.
  * [ui](ui/) (base, gfx, she): Portable UI library (buttons, windows, text fields, etc.)
  * [updater](updater/) (base, net): Component to check for updates.

## Level 3

  * [iff](iff/) (base, doc, render): Image File Formats library (load/save documents).

## Level 4

  * [app](app/) (allegro, base, doc, filters, gfx, iff, render, scripting, she, ui, undo, updater, webserver)

## Level 5

  * [main](main/) (app, base, she, ui)

# Debugging Tricks

On Windows, you can use F5 to show the amount of used memory. Also
`Ctrl+Shift+Q` crashes the application in case that you want to test
the anticrash feature or your need a memory dump file.
