// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_MODULES_PALETTES_H_INCLUDED
#define APP_MODULES_PALETTES_H_INCLUDED
#pragma once

namespace doc {
  class Palette;
}

namespace app {
  using namespace doc;

  int init_module_palette();
  void exit_module_palette();

  Palette* get_default_palette();
  Palette* get_current_palette();

  void set_default_palette(Palette* palette);
  bool set_current_palette(const Palette* palette, bool forced);
  void set_black_palette();

} // namespace app

#endif
