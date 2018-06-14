# Aseprite Tests

Test suite for [Aseprite](https://github.com/aseprite/aseprite)
to avoid breaking backward compatibility.

This project is cloned by the
[.travis.yml](https://github.com/aseprite/aseprite/blob/master/.travis.yml) file
on Aseprite project to do several automated tests:

* Save/load file formats correctly. For this we have `.aseprite`, `.png`,
  `.gif`, etc. files [sprites](https://github.com/aseprite/tests/tree/master/sprites)
  folder.
* Test backward compatibility with [Aseprite CLI](https://www.aseprite.org/docs/cli/) options
* Future [scripting API](https://github.com/aseprite/api)
