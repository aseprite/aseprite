// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODULES_PALETTES_H_INCLUDED
#define APP_MODULES_PALETTES_H_INCLUDED
#pragma once

#include <string>

namespace doc {
class Palette;
}

namespace app {
using namespace doc;

int init_module_palette();
void exit_module_palette();

// Loads the default palette or creates it. Also it migrates the
// palette if the palette format changes, etc.
void load_default_palette();

Palette* get_default_palette();
Palette* get_current_palette();

void set_default_palette(const Palette* palette);
bool set_current_palette(const Palette* palette, bool forced);

std::string get_preset_palette_filename(const std::string& preset,
                                        const std::string& dot_extension);
std::string get_default_palette_preset_name();
std::string get_preset_palettes_dir();

} // namespace app

#endif
