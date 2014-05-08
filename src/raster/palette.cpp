/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "raster/palette.h"

#include "base/path.h"
#include "base/string.h"
#include "gfx/hsv.h"
#include "gfx/rgb.h"
#include "raster/conversion_alleg.h"
#include "raster/file/col_file.h"
#include "raster/file/gpl_file.h"
#include "raster/image.h"

#include <algorithm>

#include <allegro.h>            // TODO Remove this dependency

namespace raster {

using namespace gfx;

Palette::Palette(FrameNumber frame, int ncolors)
  : Object(OBJECT_PALETTE)
{
  ASSERT(ncolors >= 0 && ncolors <= MaxColors);

  m_frame = frame;
  m_colors.resize(ncolors);
  m_modifications = 0;

  std::fill(m_colors.begin(), m_colors.end(), rgba(0, 0, 0, 255));
}

Palette::Palette(const Palette& palette)
  : Object(palette)
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
  Palette* graypal = new Palette(FrameNumber(0), MaxColors);
  for (int c=0; c<MaxColors; c++)
    graypal->setEntry(c, rgba(c, c, c, 255));
  return graypal;
}

void Palette::resize(int ncolors)
{
  ASSERT(ncolors >= 0 && ncolors <= MaxColors);

  int old_size = m_colors.size();
  m_colors.resize(ncolors);

  if ((int)m_colors.size() > old_size) {
    // Fill new colors with black
    std::fill(m_colors.begin()+old_size,
              m_colors.begin()+m_colors.size(),
              rgba(0, 0, 0, 255));
  }

  ++m_modifications;
}

void Palette::addEntry(color_t color)
{
  resize(size()+1);
  setEntry(size()-1, color);
}

void Palette::setFrame(FrameNumber frame)
{
  ASSERT(frame >= 0);

  m_frame = frame;
}

void Palette::setEntry(int i, color_t color)
{
  ASSERT(i >= 0 && i < size());

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
    if (to) *to = max-1;
  }

  return diff;
}

bool Palette::isBlack() const
{
  for (size_t c=0; c<m_colors.size(); ++c)
    if (getEntry(c) != rgba(0, 0, 0, 255))
      return false;

  return true;
}

void Palette::makeBlack()
{
  std::fill(m_colors.begin(), m_colors.end(), rgba(0, 0, 0, 255));
  ++m_modifications;
}

// Creates a linear ramp in the palette.
void Palette::makeHorzRamp(int from, int to)
{
  int r, g, b;
  int r1, g1, b1;
  int r2, g2, b2;
  int i, n;

  ASSERT(from >= 0 && from <= 255);
  ASSERT(to >= 0 && to <= 255);

  if (from > to)
    std::swap(from, to);

  n = to - from;
  if (n < 2)
    return;

  r1 = rgba_getr(getEntry(from));
  g1 = rgba_getg(getEntry(from));
  b1 = rgba_getb(getEntry(from));
  r2 = rgba_getr(getEntry(to));
  g2 = rgba_getg(getEntry(to));
  b2 = rgba_getb(getEntry(to));

  for (i=from+1; i<to; ++i) {
    r = r1 + (r2-r1) * (i-from) / n;
    g = g1 + (g2-g1) * (i-from) / n;
    b = b1 + (b2-b1) * (i-from) / n;
    setEntry(i, rgba(r, g, b, 255));
  }
}

void Palette::makeVertRamp(int from, int to, int columns)
{
  int r, g, b;
  int r1, g1, b1;
  int r2, g2, b2;
  int y, ybeg, yend, n;
  int offset;

  ASSERT(from >= 0 && from <= 255);
  ASSERT(to >= 0 && to <= 255);
  ASSERT(columns >= 1 && columns <= MaxColors);

  /* both indices have to be in the same column */
  ASSERT((from % columns) == (to % columns));

  if (from > to)
    std::swap(from, to);

  ybeg = from/columns;
  yend = to/columns;
  n = yend - ybeg;
  if (n < 2)
    return;

  r1 = rgba_getr(getEntry(from));
  g1 = rgba_getg(getEntry(from));
  b1 = rgba_getb(getEntry(from));
  r2 = rgba_getr(getEntry(to));
  g2 = rgba_getg(getEntry(to));
  b2 = rgba_getb(getEntry(to));

  offset = from % columns;

  for (y=ybeg+1; y<yend; ++y) {
    r = r1 + (r2-r1) * (y-ybeg) / n;
    g = g1 + (g2-g1) * (y-ybeg) / n;
    b = b1 + (b2-b1) * (y-ybeg) / n;
    setEntry(y*columns+offset, rgba(r, g, b, 255));
  }
}

// Creates a rectangular ramp in the palette
void Palette::makeRectRamp(int from, int to, int columns)
{
  int x1, y1, x2, y2, y;

  ASSERT(from >= 0 && from <= 255);
  ASSERT(to >= 0 && to <= 255);
  ASSERT(columns >= 1 && columns <= MaxColors);

  if (from > to)
    std::swap(from, to);

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

//////////////////////////////////////////////////////////////////////
// Sort

SortPalette::SortPalette(Channel channel, bool ascending)
{
  m_channel = channel;
  m_ascending = ascending;
  m_chain = NULL;
}

SortPalette::~SortPalette()
{
  delete m_chain;
}

void SortPalette::addChain(SortPalette* chain)
{
  if (m_chain)
    m_chain->addChain(chain);
  else
    m_chain = chain;
}

bool SortPalette::operator()(color_t c1, color_t c2)
{
  int value1 = 0, value2 = 0;

  switch (m_channel) {

    case SortPalette::RGB_Red:
      value1 = rgba_getr(c1);
      value2 = rgba_getr(c2);
      break;

    case SortPalette::RGB_Green:
      value1 = rgba_getg(c1);
      value2 = rgba_getg(c2);
      break;

    case SortPalette::RGB_Blue:
      value1 = rgba_getb(c1);
      value2 = rgba_getb(c2);
      break;

    case SortPalette::HSV_Hue:
    case SortPalette::HSV_Saturation:
    case SortPalette::HSV_Value: {
      Hsv hsv1(Rgb(rgba_getr(c1),
                   rgba_getg(c1),
                   rgba_getb(c1)));
      Hsv hsv2(Rgb(rgba_getr(c2),
                   rgba_getg(c2),
                   rgba_getb(c2)));

      switch (m_channel) {
        case SortPalette::HSV_Hue:
          value1 = hsv1.hueInt();
          value2 = hsv2.hueInt();
          break;
        case SortPalette::HSV_Saturation:
          value1 = hsv1.saturationInt();
          value2 = hsv2.saturationInt();
          break;
        case SortPalette::HSV_Value:
          value1 = hsv1.valueInt();
          value2 = hsv2.valueInt();
          break;
        default:
          ASSERT(false);
          break;
      }
      break;
    }

    case SortPalette::HSL_Lightness: {
      value1 = (std::max(rgba_getr(c1), std::max(rgba_getg(c1), rgba_getb(c1))) +
                std::min(rgba_getr(c1), std::min(rgba_getg(c1), rgba_getb(c1)))) / 2;
      value2 = (std::max(rgba_getr(c2), std::max(rgba_getg(c2), rgba_getb(c2))) +
                std::min(rgba_getr(c2), std::min(rgba_getg(c2), rgba_getb(c2)))) / 2;
      break;
    }

    case SortPalette::YUV_Luma: {
      value1 = (rgba_getr(c1)*299 + rgba_getg(c1)*587 + rgba_getb(c1)*114); // do not /1000 (so we get more precission)
      value2 = (rgba_getr(c2)*299 + rgba_getg(c2)*587 + rgba_getb(c2)*114);
      break;
    }

  }

  if (!m_ascending)
    std::swap(value1, value2);

  if (value1 < value2) {
    return true;
  }
  else if (value1 == value2) {
    if (m_chain)
      return m_chain->operator()(c1, c2);
  }

  return false;
}

struct PalEntryWithIndex {
  int index;
  color_t color;
};

struct PalEntryWithIndexPredicate {
  SortPalette* sort_palette;
  PalEntryWithIndexPredicate(SortPalette* sort_palette)
    : sort_palette(sort_palette) { }

  bool operator()(const PalEntryWithIndex& a, const PalEntryWithIndex& b) {
    return sort_palette->operator()(a.color, b.color);
  }
};

void Palette::sort(int from, int to, SortPalette* sort_palette, std::vector<int>& mapping)
{
  if (from == to)               // Just do nothing
    return;

  ASSERT(from < to);

  std::vector<PalEntryWithIndex> temp(to-from+1);
  for (int i=0; i<(int)temp.size(); ++i) {
    temp[i].index = from+i;
    temp[i].color = m_colors[from+i];
  }

  std::sort(temp.begin(), temp.end(), PalEntryWithIndexPredicate(sort_palette));

  // Default mapping table (no-mapping)
  mapping.resize(MaxColors);
  for (int i=0; i<MaxColors; ++i)
    mapping[i] = i;

  for (int i=0; i<(int)temp.size(); ++i) {
    m_colors[from+i] = temp[i].color;
    mapping[from+i] = temp[i].index;
  }
}

// End of Sort stuff
//////////////////////////////////////////////////////////////////////

Palette* Palette::load(const char *filename)
{
  std::string ext = base::string_to_lower(base::get_file_extension(filename));
  Palette* pal = NULL;

  if (ext == "png" ||
      ext == "pcx" ||
      ext == "bmp" ||
      ext == "tga" ||
      ext == "lbm") {
    PALETTE rgbpal;
    BITMAP* bmp;

    bmp = load_bitmap(filename, rgbpal);
    if (bmp) {
      destroy_bitmap(bmp);

      pal = new Palette(FrameNumber(0), MaxColors);
      convert_palette_from_allegro(rgbpal, pal);
    }
  }
  else if (ext == "col") {
    pal = raster::file::load_col_file(filename);
  }
  else if (ext == "gpl") {
    pal = raster::file::load_gpl_file(filename);
  }

  if (pal)
    pal->setFilename(filename);

  return pal;
}

bool Palette::save(const char *filename) const
{
  std::string ext = base::string_to_lower(base::get_file_extension(filename));
  bool success = false;

  if (ext == "png" ||
      ext == "pcx" ||
      ext == "bmp" ||
      ext == "tga") {
    PALETTE rgbpal;
    BITMAP* bmp;
    int c, x, y;

    bmp = create_bitmap_ex(8, 16, 16);
    for (y=c=0; y<16; y++)
      for (x=0; x<16; x++)
        putpixel(bmp, x, y, c++);

    convert_palette_to_allegro(this, rgbpal);

    success = (save_bitmap(filename, bmp, rgbpal) == 0);
    destroy_bitmap(bmp);
  }
  else if (ext == "col") {
    success = raster::file::save_col_file(this, filename);
  }
  else if (ext == "gpl") {
    success = raster::file::save_gpl_file(this, filename);
  }

  return success;
}

//////////////////////////////////////////////////////////////////////
// Based on Allegro's bestfit_color

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

int Palette::findBestfit(int r, int g, int b, int mask_index) const
{
#ifdef __GNUC__
  register int bestfit asm("%eax");
#else
  register int bestfit;
#endif
  int i, coldiff, lowest;

  ASSERT(r >= 0 && r <= 255);
  ASSERT(g >= 0 && g <= 255);
  ASSERT(b >= 0 && b <= 255);

  if (col_diff[1] == 0)
    bestfit_init();

  bestfit = 0;
  lowest = INT_MAX;

  r >>= 3;
  g >>= 3;
  b >>= 3;

  i = 0;
  while (i < size()) {
    color_t rgb = m_colors[i];

    coldiff = (col_diff + 0) [ ((rgba_getg(rgb)>>3) - g) & 0x7F ];
    if (coldiff < lowest) {
      coldiff += (col_diff + 128) [ ((rgba_getr(rgb)>>3) - r) & 0x7F ];
      if (coldiff < lowest) {
        coldiff += (col_diff + 256) [ ((rgba_getb(rgb)>>3) - b) & 0x7F ];
        if (coldiff < lowest && i != mask_index) {
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

} // namespace raster
