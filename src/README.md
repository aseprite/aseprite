# Aseprite Source Code

If you are here is because you want to learn about Aseprite source
code. We'll try to write in these `README.md` files a summary of each
module/library.

# Modules & Libraries

Aseprite is separated in the following layers/modules:

## Level 0: Completely independent modules

These libraries are easy to be used and embedded in other software
because they don't depend on any other component.

  * [clip](https://github.com/aseprite/clip): Clipboard library.
  * [fixmath](fixmath/): Fixed point operations (original code from Allegro code by Shawn Hargreaves).
  * [flic](https://github.com/aseprite/flic): Library to load/save FLI/FLC files.
  * laf/[base](https://github.com/aseprite/laf/tree/main/base): Core/basic stuff, multithreading, utf8, sha1, file system, memory, etc.
  * laf/[gfx](https://github.com/aseprite/laf/tree/main/gfx): Abstract graphics structures like point, size, rectangle, region, color, etc.
  * [observable](https://github.com/aseprite/observable): Signal/slot functions.
  * [scripting](scripting/): JavaScript engine.
  * [steam](steam/): Steam API wrapper to avoid static linking to the .lib file.
  * [undo](https://github.com/aseprite/undo): Generic library to manage a history of undoable commands.

## Level 1

  * [cfg](cfg/) (base): Library to load/save .ini files.
  * [gen](gen/) (base): Helper utility to generate C++ files from different XMLs.
  * [net](net/) (base): Networking library to send HTTP requests.
  * laf/[os](https://github.com/aseprite/laf/tree/main/os) (base, gfx, wacom): OS input/output.

## Level 2

  * [doc](doc/) (base, fixmath, gfx): Document model library.
  * [ui](ui/) (base, gfx, os): Portable UI library (buttons, windows, text fields, etc.)
  * [updater](updater/) (base, cfg, net): Component to check for updates.

## Level 3

  * [dio](dio/) (base, doc, fixmath, flic): Load/save sprites/documents.
  * [filters](filters/) (base, doc, gfx): Effects for images.
  * [render](render/) (base, doc, gfx): Library to render documents.

## Level 4

  * [app](app/) (base, doc, dio, filters, fixmath, flic, gfx, pen, render, scripting, os, ui, undo, updater)
  * [desktop](desktop/) (base, doc, dio, render): Integration with the desktop (Windows Explorer, Finder, GNOME, KDE, etc.)

## Level 5

  * [main](main/) (app, base, os, ui)

# Debugging Tricks

When Aseprite is compiled with `ENABLE_DEVMODE`, you have the
following extra commands/features available:

* `F5`: On Windows shows the amount of used memory.
* `F1`: Switch between new/old/shader renderers.
* `Ctrl+F1`: Switch/test Screen/UI Scaling values.
* `Ctrl+Alt+Shift+Q`: crashes the application in case that you want to
  test the anticrash feature or your need a memory dump file.
* `Ctrl+Alt+Shift+R`: recover the active document from the data
  recovery store.
* `aseprite.ini`: `[perf] show_render_time=true` shows a performance
  clock in the Editor.

In Debug mode (`_DEBUG`):

* [`TRACEARGS`](https://github.com/aseprite/laf/blob/f3222bdee2d21556e9da55343e73803c730ecd97/base/debug.h#L40):
  in debug mode, it prints in the terminal/console each given argument

# Detect Platform

You can check the platform using some `laf` macros:

    #if LAF_WINDOWS
      // ...
    #elif LAF_MACOS
      // ...
    #elif LAF_LINUX
      // ...
    #endif

Or using platform-specific macros:

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
