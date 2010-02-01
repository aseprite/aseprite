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

#include <cassert>
#include <allegro.h>
#include <algorithm>

#include "raster/image.h"
#include "raster/palette.h"
#include "util/col_file.h"

//////////////////////////////////////////////////////////////////////

Palette::Palette(int frame, int ncolors)
  : GfxObj(GFXOBJ_PALETTE)
{
  assert(ncolors >= 1 && ncolors <= MAX_PALETTE_COLORS);

  this->frame = frame;
  this->ncolors = ncolors;

  std::fill(this->color,
	    this->color+MAX_PALETTE_COLORS,
	    _rgba(0, 0, 0, 255));
}

Palette::Palette(const Palette& palette)
  : GfxObj(palette)
{
  this->frame = palette.frame;
  this->ncolors = palette.ncolors;

  std::copy(palette.color,
	    palette.color+MAX_PALETTE_COLORS,
	    this->color);
}

Palette::~Palette()
{
}

//////////////////////////////////////////////////////////////////////

Palette* palette_new(int frame, int ncolors)
{
  return new Palette(frame, ncolors);
}

Palette* palette_new_copy(const Palette* pal)
{
  assert(pal);
  return new Palette(*pal);
}

void palette_free(Palette* palette)
{
  delete palette;
}

void palette_black(Palette* pal)
{
  int c;

  for (c=0; c<MAX_PALETTE_COLORS; ++c)
    pal->color[c] = 0;
}

/**
 * Copies the color of both palettes.
 */
void palette_copy_colors(Palette* pal, const Palette* src)
{
  int c;

  /* don't copy the frame property "pal->frame = src->frame;" !... */

  /* just copy the colors */
  pal->ncolors = src->ncolors;
  for (c=0; c<MAX_PALETTE_COLORS; ++c)
    pal->color[c] = src->color[c];
}

int palette_count_diff(const Palette* p1, const Palette* p2, int *from, int *to)
{
  register int c, diff = 0;
  int min = MIN(p1->ncolors, p2->ncolors);
  int max = MAX(p1->ncolors, p2->ncolors);

  if (from) *from = -1;
  if (to) *to = -1;

  /* compare palettes */
  for (c=0; c<min; ++c) {
    if (p1->color[c] != p2->color[c]) {
      if (from && *from < 0) *from = c;
      if (to) *to = c;
      ++diff;
    }
  }

  if (max != min) {
    diff += max - min;
    if (to) *to = max;
  }

  return diff;
}

void palette_set_ncolors(Palette* pal, int ncolors)
{
  int i;

  assert(pal != NULL);
  assert(ncolors >= 1 && ncolors <= MAX_PALETTE_COLORS);

  if (pal->ncolors != ncolors) {
    if (pal->ncolors > ncolors) {
      for (i=pal->ncolors-1; i>=ncolors; --i) {
	pal->color[i] = _rgba(0, 0, 0, 255);
      }
    }
    else {
      for (i=pal->ncolors; i<ncolors; ++i) {
	pal->color[i] = _rgba(0, 0, 0, 255);
      }
    }

    pal->ncolors = ncolors;
  }
}

void palette_set_frame(Palette* pal, int frame)
{
  assert(pal != NULL);
  assert(frame >= 0);

  pal->frame = frame;
}

ase_uint32 palette_get_entry(const Palette* pal, int i)
{
  assert(pal != NULL);
  assert(i >= 0 && i < MAX_PALETTE_COLORS);

  return pal->color[i];
}

void palette_set_entry(Palette* pal, int i, ase_uint32 color)
{
  assert(pal != NULL);
  assert(i >= 0 && i < pal->ncolors);
  assert(_rgba_geta(color) == 255);

  pal->color[i] = color;
}

/* creates a linear ramp in the palette */
void palette_make_horz_ramp(Palette* p, int from, int to)
{
  int r, g, b;
  int r1, g1, b1;
  int r2, g2, b2;
  int i, n;

  assert(from >= 0 && from <= 255);
  assert(to >= 0 && to <= 255);

  if (from > to) {
    i    = from;
    from = to;
    to   = i;
  }

  n = to - from;
  if (n < 2)
    return;

  r1 = _rgba_getr(p->color[from]);
  g1 = _rgba_getg(p->color[from]);
  b1 = _rgba_getb(p->color[from]);
  r2 = _rgba_getr(p->color[to]);
  g2 = _rgba_getg(p->color[to]);
  b2 = _rgba_getb(p->color[to]);

  for (i=from+1; i<to; ++i) {
    r = r1 + (r2-r1) * (i-from) / n;
    g = g1 + (g2-g1) * (i-from) / n;
    b = b1 + (b2-b1) * (i-from) / n;
    p->color[i] = _rgba(r, g, b, 255);
  }
}

void palette_make_vert_ramp(Palette* p, int from, int to, int columns)
{
  int r, g, b;
  int r1, g1, b1;
  int r2, g2, b2;
  int y, ybeg, yend, n;
  int offset;

  assert(from >= 0 && from <= 255);
  assert(to >= 0 && to <= 255);
  assert(columns >= 1 && columns <= 256);

  /* both indices have to be in the same column */
  assert((from % columns) == (to % columns));

  if (from > to) {
    y    = from;
    from = to;
    to   = y;
  }

  ybeg = from/columns;
  yend = to/columns;
  n = yend - ybeg;
  if (n < 2)
    return;

  r1 = _rgba_getr(p->color[from]);
  g1 = _rgba_getg(p->color[from]);
  b1 = _rgba_getb(p->color[from]);
  r2 = _rgba_getr(p->color[to]);
  g2 = _rgba_getg(p->color[to]);
  b2 = _rgba_getb(p->color[to]);

  offset = from % columns;

  for (y=ybeg+1; y<yend; ++y) {
    r = r1 + (r2-r1) * (y-ybeg) / n;
    g = g1 + (g2-g1) * (y-ybeg) / n;
    b = b1 + (b2-b1) * (y-ybeg) / n;
    p->color[y*columns+offset] = _rgba(r, g, b, 255);
  }
}

/* creates a rectangular ramp in the palette */
void palette_make_rect_ramp(Palette* p, int from, int to, int columns)
{
  int x1, y1, x2, y2, y;

  assert(from >= 0 && from <= 255);
  assert(to >= 0 && to <= 255);
  assert(columns >= 1 && columns <= 256);
  
  x1 = from % columns;
  y1 = from / columns;
  x2 = to % columns;
  y2 = to / columns;

  palette_make_vert_ramp(p, from, y2*columns+x1, columns);
  if (x1 < x2) {
    palette_make_vert_ramp(p, y1*columns+x2, to, columns);
    if (x2 - x1 >= 2)
      for (y=y1; y<=y2; ++y)
	palette_make_horz_ramp(p, y*columns+x1, y*columns+x2);
  }

}

/**
 * @param pal The ASE color palette to copy.
 * @param rgb An Allegro's PALETTE.
 *
 * @return The same @a rgb pointer specified in the parameters.
 */
RGB *palette_to_allegro(const Palette* pal, RGB *rgb)
{
  int i;
  for (i=0; i<MAX_PALETTE_COLORS; ++i) {
    rgb[i].r = _rgba_getr(pal->color[i]) / 4;
    rgb[i].g = _rgba_getg(pal->color[i]) / 4;
    rgb[i].b = _rgba_getb(pal->color[i]) / 4;
  }
  return rgb;
}

Palette* palette_from_allegro(Palette* pal, const struct RGB *rgb)
{
  int i;
  pal->ncolors = MAX_PALETTE_COLORS;
  for (i=0; i<MAX_PALETTE_COLORS; ++i) {
    pal->color[i] = _rgba(_rgb_scale_6[rgb[i].r],
			  _rgb_scale_6[rgb[i].g],
			  _rgb_scale_6[rgb[i].b], 255);
  }
  return pal;
}

Palette* palette_load(const char *filename)
{
  Palette* pal = NULL;
  char ext[64];

  ustrcpy(ext, get_extension(filename));

  if ((ustricmp(ext, "png") == 0) ||
      (ustricmp(ext, "pcx") == 0) ||
      (ustricmp(ext, "bmp") == 0) ||
      (ustricmp(ext, "tga") == 0) ||
      (ustricmp(ext, "lbm") == 0)) {
    PALETTE rgbpal;
    BITMAP *bmp;

    bmp = load_bitmap(filename, rgbpal);
    if (bmp) {
      destroy_bitmap(bmp);

      pal = palette_new(0, MAX_PALETTE_COLORS);
      palette_from_allegro(pal, rgbpal);
    }
  }
  else if (ustricmp(ext, "col") == 0) {
    pal = load_col_file(filename);
  }

  return pal;
}

bool palette_save(Palette* pal, const char *filename)
{
  bool success = false;
  char ext[64];

  ustrcpy(ext, get_extension(filename));

  if ((ustricmp(ext, "png") == 0) ||
      (ustricmp(ext, "pcx") == 0) ||
      (ustricmp(ext, "bmp") == 0) ||
      (ustricmp(ext, "tga") == 0)) {
    PALETTE rgbpal;
    BITMAP *bmp;
    int c, x, y;

    bmp = create_bitmap_ex(8, 16, 16);
    for (y=c=0; y<16; y++)
      for (x=0; x<16; x++)
	putpixel(bmp, x, y, c++);

    palette_to_allegro(pal, rgbpal);

    success = (save_bitmap(filename, bmp, rgbpal) == 0);
    destroy_bitmap(bmp);
  }
  else if (ustricmp(ext, "col") == 0) {
    success = save_col_file(pal, filename);
  }

  return success;
}

/**********************************************************************/
/* Based on Allegro's bestfit_color */

static unsigned int col_diff[3*128]; 

static void bestfit_init()
{
  register int i, k;

  for (i=1; i<64; i++) {
    k = i * i;
    col_diff[0  +i] = col_diff[0  +128-i] = k * (59 * 59);
    col_diff[128+i] = col_diff[128+128-i] = k * (30 * 30);
    col_diff[256+i] = col_diff[256+128-i] = k * (11 * 11);
  }
}

int palette_find_bestfit(const Palette* pal, int r, int g, int b)
{
#ifdef __GNUC__
  register int bestfit asm("%eax");
#else
  register int bestfit;
#endif
  int i, coldiff, lowest;

  assert(r >= 0 && r <= 255);
  assert(g >= 0 && g <= 255);
  assert(b >= 0 && b <= 255);

  if (col_diff[1] == 0)
    bestfit_init();

  bestfit = 0;
  lowest = INT_MAX;

  r >>= 3;
  g >>= 3;
  b >>= 3;

  i = 1;
  while (i<PAL_SIZE) {
    ase_uint32 rgb = pal->color[i];

    coldiff = (col_diff + 0) [ ((_rgba_getg(rgb)>>3) - g) & 0x7F ];
    if (coldiff < lowest) {
      coldiff += (col_diff + 128) [ ((_rgba_getr(rgb)>>3) - r) & 0x7F ];
      if (coldiff < lowest) {
	coldiff += (col_diff + 256) [ ((_rgba_getb(rgb)>>3) - b) & 0x7F ];
	if (coldiff < lowest) {
	  bestfit = i;
	  if (coldiff == 0)
	    return bestfit;
	  lowest = coldiff;
	}
      }
    }
    i++;
  }

  return bestfit;
}
