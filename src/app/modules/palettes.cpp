// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/modules/palettes.h"

#include "app/app.h"
#include "app/resource_finder.h"
#include "base/fs.h"
#include "base/path.h"
#include "doc/blend.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "doc/sprite.h"

#include <cstring>

namespace app {

/**
 * The default color palette.
 */
static Palette* ase_default_palette = NULL;

/**
 * Current original palette (you can use _current_palette from Allegro
 * to refer to the current system "mapped-palette").
 */
static Palette* ase_current_palette = NULL;

int init_module_palette()
{
  ase_default_palette = new Palette(frame_t(0), 256);
  ase_current_palette = new Palette(frame_t(0), 256);
  return 0;
}

void exit_module_palette()
{
  delete ase_default_palette;
  delete ase_current_palette;
}

Palette* get_current_palette()
{
  return ase_current_palette;
}

Palette* get_default_palette()
{
  return ase_default_palette;
}

void set_default_palette(const Palette* palette)
{
  palette->copyColorsTo(ase_default_palette);
}

/**
 * Changes the current system palette. Triggers the APP_PALETTE_CHANGE event
 *
 * @param palette If "palette" is NULL will be used the default one
 * (ase_default_palette)
 */
bool set_current_palette(const Palette *_palette, bool forced)
{
  const Palette* palette = _palette ? _palette: ase_default_palette;
  bool ret = false;

  // Have changes
  if (forced ||
      palette->countDiff(ase_current_palette, NULL, NULL) > 0) {
    // Copy current palette
    palette->copyColorsTo(ase_current_palette);

    // Call slots in signals
    App::instance()->PaletteChange();

    ret = true;
  }

  return ret;
}

void set_black_palette()
{
  Palette* p = new Palette(frame_t(0), 256);
  set_current_palette(p, true);
  delete p;
}

std::string get_preset_palette_filename(const std::string& preset)
{
  ResourceFinder rf;
  rf.includeUserDir(base::join_path("palettes", ".").c_str());
  std::string palettesDir = rf.getFirstOrCreateDefault();

  if (!base::is_directory(palettesDir))
    base::make_directory(palettesDir);

  return base::join_path(palettesDir, preset + ".gpl");
}

std::string get_default_palette_preset_name()
{
  return "default";
}

} // namespace app
