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
  * [clip](https://github.com/aseprite/clip): Clipboard library.
  * [fixmath](fixmath/): Fixed point operations (original code from Allegro code by Shawn Hargreaves).
  * [flic](https://github.com/aseprite/flic): Library to load/save FLI/FLC files.
  * [gfx](gfx/): Abstract graphics structures like point, size, rectangle, region, color, etc.
  * [laf](https://github.com/aseprite/laf): Core/basic stuff, multithreading, utf8, sha1, file system, memory, etc.
  * [observable](https://github.com/dacap/observable): Signal/slot functions.
  * [scripting](scripting/): JavaScript engine.
  * [steam](steam/): Steam API wrapper to avoid static linking to the .lib file.
  * [undo](https://github.com/aseprite/undo): Generic library to manage a history of undoable commands.
  * [wacom](wacom/): Wacom definitions to use Wintab API.

## Level 1

  * [cfg](cfg/) (base): Library to load/save .ini files.
  * [gen](gen/) (base): Helper utility to generate C++ files from different XMLs.
  * [net](net/) (base): Networking library to send HTTP requests.
  * [webserver](webserver/) (base): HTTP web server

## Level 2

  * [doc](doc/) (base, fixmath, gfx): Document model library.
  * [she](she/) (allegro, base, gfx, wacom): A wrapper for the Allegro library.

## Level 3

  * [filters](filters/) (base, doc, gfx): Effects for images.
  * [render](render/) (base, doc, gfx): Library to render documents.
  * [ui](ui/) (base, gfx, she): Portable UI library (buttons, windows, text fields, etc.)
  * [updater](updater/) (base, cfg, net): Component to check for updates.

## Level 4

  * [docio](docio/) (base, flic): Load/save documents.

## Level 5

  * [app](app/) (allegro, base, doc, docio, filters, fixmath, flic, gfx, pen, render, scripting, she, ui, undo, updater, webserver)

## Level 6

  * [main](main/) (app, base, she, ui)

# Debugging Tricks

When Aseprite is compiled with `ENABLE_DEVMODE`, you have the
following extra commands available:

* `F5`: On Windows shows the amount of used memory.
* `F1`: Switch/test Screen/UI Scaling values.
* `Ctrl+Alt+Shift+Q`: crashes the application in case that you want to
  test the anticrash feature or your need a memory dump file.
* `Ctrl+Alt+Shift+R`: recover the active document from the data
  recovery store.

# Detect Platform

You can check the platform using the following checks:

    #ifdef _WIN32
      #ifdef _WIN64
        // Windows x64
      #else
        // Windows x86
      #endif
    #elif defined(__APPLE__)
        // macOS
    #else
        // Linux
    #endif
