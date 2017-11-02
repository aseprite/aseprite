# SHE - Simplified Hardware Entry-point

`she` is an abstraction layer to access in different way to the
hardware/operating system. It can be implemented with different
back-ends:

* Previous version were using Allegro 4 (it still uses Allegro 4 on Linux)
* Now we use our own implementation on Windows and macOS to handle
  events, and [Skia](https://skia.org/) to render graphics.
* Minimum Windows platform: Windows Vista
