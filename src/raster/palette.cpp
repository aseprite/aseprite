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

Palette::Palette(int frame, size_t ncolors)
  : GfxObj(GFXOBJ_PALETTE)
{
  assert(ncolors >= 1 && ncolors <= 256);

  m_frame = frame;
  m_colors.resize(ncolors);
  m_modifications = 0;

  std::fill(m_colors.begin(), m_colors.end(), _rgba(0, 0, 0, 255));
}

Palette::Palette(const Palette& palette)
  : GfxObj(palette)
{
  m_frame = palette.m_frame;
  m_colors = palette.m_colors;
  m_modifications = 0;
}

Palette::~Palette()
{
}

Palette* Palette::createGrayscale()
{
  Palette* graypal = new Palette(0, 256);
  for (int c=0; c<256; c++)
    graypal->setEntry(c, _rgba(c, c, c, 255));
  return graypal;
}

void Palette::resize(size_t ncolors)
{
  assert(ncolors >= 1 && ncolors <= 256);

  size_t old_size = m_colors.size();
  m_colors.resize(ncolors);

  if (m_colors.size() > old_size) {
    // Fill new colors with black
    std::fill(m_colors.begin()+old_size,
	      m_colors.begin()+m_colors.size(),
	      _rgba(0, 0, 0, 255));
  }

  ++m_modifications;
}

void Palette::setFrame(int frame)
{
  assert(frame >= 0);

  m_frame = frame;
}

void Palette::setEntry(size_t i, ase_uint32 color)
{
  assert(i >= 0 && i < size());
  assert(_rgba_geta(color) == 255);

  m_colors[i] = color;
  ++m_modifications;
}

void Palette::copyColorsTo(Palette* dst) const
{
  dst->m_colors = m_colors;
  ++dst->m_modifications;
}

int Palette::countDiff(const Palette* other, int* from, int* to) const
{
  register int c, diff = 0;
  int min = MIN(this->m_colors.size(), other->m_colors.size());
  int max = MAX(this->m_colors.size(), other->m_colors.size());

  if (from) *from = -1;
  if (to) *to = -1;

  // Compare palettes
  for (c=0; c<min; ++c) {
    if (this->m_colors[c] != other->m_colors[c]) {
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

void Palette::makeBlack()
{
  std::fill(m_colors.begin(), m_colors.end(), _rgba(0, 0, 0, 255));
  ++m_modifications;
}

// Creates a linear ramp in the palette.
void Palette::makeHorzRamp(int from, int to)
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

  r1 = _rgba_getr(getEntry(from));
  g1 = _rgba_getg(getEntry(from));
  b1 = _rgba_getb(getEntry(from));
  r2 = _rgba_getr(getEntry(to));
  g2 = _rgba_getg(getEntry(to));
  b2 = _rgba_getb(getEntry(to));

  for (i=from+1; i<to; ++i) {
    r = r1 + (r2-r1) * (i-from) / n;
    g = g1 + (g2-g1) * (i-from) / n;
    b = b1 + (b2-b1) * (i-from) / n;
    setEntry(i, _rgba(r, g, b, 255));
  }
}

void Palette::makeVertRamp(int from, int to, int columns)
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

  r1 = _rgba_getr(getEntry(from));
  g1 = _rgba_getg(getEntry(from));
  b1 = _rgba_getb(getEntry(from));
  r2 = _rgba_getr(getEntry(to));
  g2 = _rgba_getg(getEntry(to));
  b2 = _rgba_getb(getEntry(to));

  offset = from % columns;

  for (y=ybeg+1; y<yend; ++y) {
    r = r1 + (r2-r1) * (y-ybeg) / n;
    g = g1 + (g2-g1) * (y-ybeg) / n;
    b = b1 + (b2-b1) * (y-ybeg) / n;
    setEntry(y*columns+offset, _rgba(r, g, b, 255));
  }
}

// Creates a rectangular ramp in the palette
void Palette::makeRectRamp(int from, int to, int columns)
{
  int x1, y1, x2, y2, y;

  assert(from >= 0 && from <= 255);
  assert(to >= 0 && to <= 255);
  assert(columns >= 1 && columns <= 256);
  
  x1 = from % columns;
  y1 = from / columns;
  x2 = to % columns;
  y2 = to / columns;

  makeVertRamp(from, y2*columns+x1, columns);
  if (x1 < x2) {
    makeVertRamp(y1*columns+x2, to, columns);
    if (x2 - x1 >= 2)
      for (y=y1; y<=y2; ++y)
	makeHorzRamp(y*columns+x1, y*columns+x2);
  }

}

/**
 * @param pal The ASE color palette to copy.
 * @param rgb An Allegro's PALETTE.
 *
 * @return The same @a rgb pointer specified in the parameters.
 */
void Palette::toAllegro(RGB *rgb) const
{
  int i;
  for (i=0; i<size(); ++i) {
    rgb[i].r = _rgba_getr(m_colors[i]) / 4;
    rgb[i].g = _rgba_getg(m_colors[i]) / 4;
    rgb[i].b = _rgba_getb(m_colors[i]) / 4;
  }
  for (; i<256; ++i) {
    rgb[i].r = 0;
    rgb[i].g = 0;
    rgb[i].b = 0;
  }
}

void Palette::fromAllegro(const RGB* rgb)
{
  int i;
  m_colors.resize(256);
  for (i=0; i<256; ++i) {
    m_colors[i] = _rgba(_rgb_scale_6[rgb[i].r],
			_rgb_scale_6[rgb[i].g],
			_rgb_scale_6[rgb[i].b], 255);
  }
  ++m_modifications;
}

Palette* Palette::load(const char *filename)
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

      pal = new Palette(0, 256);
      pal->fromAllegro(rgbpal);
    }
  }
  else if (ustricmp(ext, "col") == 0) {
    pal = load_col_file(filename);
  }

  return pal;
}

bool Palette::save(const char *filename) const
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

    toAllegro(rgbpal);

    success = (save_bitmap(filename, bmp, rgbpal) == 0);
    destroy_bitmap(bmp);
  }
  else if (ustricmp(ext, "col") == 0) {
    success = save_col_file(this, filename);
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

int Palette::findBestfit(int r, int g, int b) const
{
#ifdef __GNUC__
  register int bestfit asm("%eax");
#else
  register int bestfit;
#endif
  int coldiff, lowest;
  size_t i;

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
  while (i < size()) {
    ase_uint32 rgb = m_colors[i];

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
