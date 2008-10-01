/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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

#ifndef RASTER_PALETTE_H
#define RASTER_PALETTE_H

#include "jinete/jbase.h"
#include "raster/gfxobj.h"
#include <allegro/color.h>

#define MAX_PALETTE_COLORS	256

class Palette : public GfxObj
{
public:
  int frame;
  int ncolors;
  ase_uint32 color[MAX_PALETTE_COLORS];

  Palette(int frame, int ncolors);
  Palette(const Palette& palette);
  virtual ~Palette();
};

Palette* palette_new(int frame, int ncolors);
Palette* palette_new_copy(const Palette* pal);
void palette_free(Palette* pal);

void palette_black(Palette* pal);

void palette_copy_colors(Palette* pal, const Palette* src);
int palette_count_diff(const Palette* p1, const Palette* p2, int *from, int *to);

void palette_set_ncolors(Palette* pal, int ncolors);
void palette_set_frame(Palette* pal, int frame);

ase_uint32 palette_get_entry(const Palette* pal, int i);
void palette_set_entry(Palette* pal, int i, ase_uint32 color);

void palette_make_horz_ramp(Palette* palette, int from, int to);
void palette_make_vert_ramp(Palette* palette, int from, int to, int columns);
void palette_make_rect_ramp(Palette* palette, int from, int to, int columns);

struct RGB *palette_to_allegro(const Palette* pal, struct RGB *rgb);
Palette* palette_from_allegro(Palette* pal, const struct RGB *rgb);

Palette* palette_load(const char *filename);
bool palette_save(Palette* palette, const char *filename);

int palette_find_bestfit(const Palette* pal, int r, int g, int b);

#endif	/* RASTER_PALETTE_H */
