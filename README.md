# Aseprite Tests

Test suite for [Aseprite](https://github.com/aseprite/aseprite)
to avoid breaking backward compatibility.

This project is cloned by the
[build.yml](https://github.com/aseprite/aseprite/blob/main/.github/workflows/build.yml) file
on Aseprite project to do several automated tests:

* Save/load file formats correctly. For this we have `.aseprite`, `.png`,
  `.gif`, etc. files [sprites](https://github.com/aseprite/tests/tree/main/sprites)
  folder.
* Test backward compatibility with [Aseprite CLI](https://www.aseprite.org/docs/cli/) options
* Future [scripting API](https://github.com/aseprite/api) using [scripts](https://github.com/aseprite/tests/tree/main/scripts)

## How to run tests?

You have to set the `ASEPRITE` environment variable pointing to the
Aseprite executable and then run `run-tests.sh` from Bash:

    export ASEPRITE=$HOME/your-aseprite-build/bin/aseprite
    git clone https://github.com/aseprite/tests.git
    cd tests
    bash run-tests.sh

You can filter some tests with a regex giving a parameter to
`run-tests.sh`, for example:

    run-tests.sh color

Should run all tests which have the `color` word in their name.
