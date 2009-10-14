/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#include <assert.h>
#include <allegro.h>
#include <string.h>

#include "jinete/jlist.h"

#include "core/app.h"
#include "modules/palettes.h"
#include "raster/blend.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/sprite.h"

RGB_MAP *orig_rgb_map = NULL;	/* color map for the original palette
				   (not for the mapped-palette) */
COLOR_MAP *orig_trans_map = NULL;

/**
 * The default color palette.
 */
static Palette *ase_default_palette = NULL;

/**
 * Current original palette (you can use _current_palette from Allegro
 * to refer to the current system "mapped-palette").
 */
static Palette *ase_current_palette = NULL;

/* current rgb map */
static RGB_MAP *my_rgb_map = NULL;
static int regen_my_rgb_map = FALSE;

/* system color (in order of importance) */
static ase_uint32 ase_system_color[] =
{
  _rgba(255, 255, 255, 255),	/* background */
  _rgba(0,     0,   0, 255),	/* foreground */
  _rgba(128, 128, 128, 255),	/* disabled */
  _rgba(210, 200, 190, 255),	/* face */
  _rgba( 44,  76, 145, 255),	/* selected */
  _rgba(250, 240, 230, 255),	/* hotface */
  _rgba(255, 255,   0, 255),	/* yellow */
  _rgba(  0,   0, 255, 255),	/* blue */
  _rgba(255, 255, 200, 255),	/* yellow for tooltips */
};

#define SYSTEM_COLORS   (sizeof(ase_system_color) / sizeof(RGB))

int init_module_palette()
{
  orig_rgb_map = jnew(RGB_MAP, 1);
  orig_trans_map = jnew(COLOR_MAP, 1);
  my_rgb_map = jnew(RGB_MAP, 1);

  rgb_map = my_rgb_map;
  color_map = NULL;

  ase_default_palette = palette_new(0, MAX_PALETTE_COLORS);
  palette_from_allegro(ase_default_palette, default_palette);

  ase_current_palette = palette_new(0, MAX_PALETTE_COLORS);
  palette_from_allegro(ase_current_palette, black_palette);

  return 0;
}

void exit_module_palette()
{
  rgb_map = NULL;

  if (ase_default_palette != NULL)
    palette_free(ase_default_palette);

  if (ase_current_palette != NULL)
    palette_free(ase_current_palette);

  jfree(my_rgb_map);
  jfree(orig_trans_map);
  jfree(orig_rgb_map);
}

Palette *get_current_palette()
{
  return ase_current_palette;
}

Palette *get_default_palette()
{
  return ase_default_palette;
}

void set_default_palette(Palette *palette)
{
  palette_copy_colors(ase_default_palette, palette);
}

/**
 * Changes the current system palette. Triggers the APP_PALETTE_CHANGE event
 *
 * @param palette If "palette" is NULL will be used the default one
 * (ase_default_palette)
 */
bool set_current_palette(Palette *_palette, bool forced)
{
  Palette *palette = _palette ? _palette: ase_default_palette;
  bool ret = false;

  /* have changes */
  if (forced ||
      palette_count_diff(palette, ase_current_palette, NULL, NULL) > 0) {
    PALETTE rgbpal;
    int i, j, k;

    /* copy current palette */
    palette_copy_colors(ase_current_palette, palette);

    /* create a RGB map for the original palette */
    create_rgb_table(orig_rgb_map, palette_to_allegro(palette, rgbpal), NULL);

    /* create a transparency-map with the original palette */
    rgb_map = orig_rgb_map;
    create_trans_table(orig_trans_map, rgbpal, 128, 128, 128, NULL);
    rgb_map = my_rgb_map;

    /* create the mapped-palette fixing up the _index_cmap[] to map
       similar colors... */

    /* fill maps with default values */
    for (i=0; i<256; i++)
      _index_cmap[i] = i;

    /* a mapped-palette is only necessary for 8 bpp screen */
    if (screen != NULL && bitmap_color_depth(screen) == 8) {
      bool found[SYSTEM_COLORS];

      /* first of all, we can search for system colors in the palette */
      for (i=0; i<SYSTEM_COLORS; i++) {
	found[i] = FALSE;

	for (j=1; j<256; j++) {
	  if ((_rgba_getr(ase_system_color[i])>>2) == (_rgba_getr(palette->color[j])>>2) &&
	      (_rgba_getg(ase_system_color[i])>>2) == (_rgba_getg(palette->color[j])>>2) &&
	      (_rgba_getb(ase_system_color[i])>>2) == (_rgba_getb(palette->color[j])>>2)) {
	    found[i] = TRUE;
	    break;
	  }
	}
      }

      /* after that, we can search for repeated colors in the system
	 palette, to make a mapping between two same colors */
      for (i=0, j=255; i<SYSTEM_COLORS; i++) {
	if (found[i])
	  continue;

	for (; j>=1; j--) {
	  for (k=1; k<j; k++) {
	    /* if the "j" color is the same as "k" color... */
	    if (palette->color[j] == palette->color[k]) {
	      /* ...then we can paint "j" using "k" */
	      _index_cmap[j] = k;
	      rgbpal[j].r = _rgba_getr(ase_system_color[i]) >> 2;
	      rgbpal[j].g = _rgba_getg(ase_system_color[i]) >> 2;
	      rgbpal[j].b = _rgba_getb(ase_system_color[i]) >> 2;
	      --j;
	      goto next;
	    }
	  }
	}
      next:;
      }
    }

    /* create a map for the mapped-palette */
    create_rgb_table(my_rgb_map, rgbpal, NULL);
    set_palette(rgbpal);	/* change system color palette */

    // call hooks
    App::instance()->trigger_event(AppEvent::PaletteChange);

    ret = TRUE;
  }

  return ret;
}

void set_black_palette()
{
  Palette *p = palette_new(0, MAX_PALETTE_COLORS);
  set_current_palette(p, TRUE);
  palette_free(p);
}

/* changes a color of the current system palette */
void set_current_color(int index, int r, int g, int b)
{
  register int c;

  assert(index >= 0 && index <= 255);
  assert(r >= 0 && r <= 255);
  assert(g >= 0 && g <= 255);
  assert(b >= 0 && b <= 255);

  c = ase_current_palette->color[index];

  if (_rgba_getr(c) != r ||
      _rgba_getg(c) != g ||
      _rgba_getb(c) != b) {
    RGB rgb;

    ase_current_palette->color[index] = _rgba(r, g, b, 255);

    if (screen && bitmap_color_depth(screen) == 8 &&
	/* this color is mapped? */
	_index_cmap[index] != index) {
      /* ok, restore the it to point to the original index */
      _index_cmap[index] = index;
    }

    rgb.r = r>>2;
    rgb.g = g>>2;
    rgb.b = b>>2;

    set_color(index, &rgb);
  }
}

//////////////////////////////////////////////////////////////////////

CurrentSpriteRgbMap::CurrentSpriteRgbMap()
{
  rgb_map = orig_rgb_map;
}

CurrentSpriteRgbMap::~CurrentSpriteRgbMap()
{
  rgb_map = my_rgb_map;

  if (regen_my_rgb_map) {
    regen_my_rgb_map = false;
    create_rgb_table(my_rgb_map, _current_palette, NULL);
  }
}
