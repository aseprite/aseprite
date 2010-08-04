/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <allegro.h>
#include <string.h>

#include "jinete/jlist.h"

#include "app.h"
#include "modules/palettes.h"
#include "raster/blend.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/sprite.h"

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
  ase_default_palette = new Palette(0, 256);
  ase_default_palette->fromAllegro(default_palette);

  ase_current_palette = new Palette(0, 256);
  ase_current_palette->fromAllegro(black_palette);

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

void set_default_palette(Palette* palette)
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

    // Change system color palette
    PALETTE allegPal;
    palette->toAllegro(allegPal);
    set_palette(allegPal);

    // Call slots in signals
    App::instance()->PaletteChange();

    ret = true;
  }

  return ret;
}

void set_black_palette()
{
  Palette* p = new Palette(0, 256);
  set_current_palette(p, true);
  delete p;
}

// Changes a color of the current system palette
void set_current_color(int index, int r, int g, int b)
{
  register int c;

  ASSERT(index >= 0 && index <= 255);
  ASSERT(r >= 0 && r <= 255);
  ASSERT(g >= 0 && g <= 255);
  ASSERT(b >= 0 && b <= 255);

  c = ase_current_palette->getEntry(index);

  if (_rgba_getr(c) != r ||
      _rgba_getg(c) != g ||
      _rgba_getb(c) != b) {
    RGB rgb;

    ase_current_palette->setEntry(index, _rgba(r, g, b, 255));

    rgb.r = r>>2;
    rgb.g = g>>2;
    rgb.b = b>>2;

    set_color(index, &rgb);
  }
}
