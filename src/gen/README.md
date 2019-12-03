# Aseprite Code Generator

> Distributed under [MIT license](LICENSE.txt)

This utility generates source code from XML files. Its aim is to
convert XML files (dynamic data) to C++ files (static structures) that
can be checked in compile-time.  There are three areas of interest:

1. To create `ui::Widget`s subclasses from
   [data/widgets/*.xml](../../data/widgets/)
   files. In this way we can create wrappers that can access to each
   XML file directly in a easier way (e.g. one member for each widget
   with an `id` parameter on it).
2. To create configuration wrappers from a special
   `config-metadata.xml` file (so we can replace
   `get/set_config_int/bool/string()` function calls).  There is an
   ongoing `cfg` module to replace the whole reading/writing
   operations of user's settings/preferences.
3. To create a wrapper class for theme data access. From
   [data/skins/default/](../../data/skins/default/)
   we can create a C++ class with a member function to access
   each theme slice, color, style, etc.
