// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/palette.h"

#include "doc/image.h"
#include "doc/remap.h"

#include <algorithm>
#include <limits>

namespace doc {

using namespace gfx;

Palette::Palette(frame_t frame, int ncolors)
  : Object(ObjectType::Palette)
{
  ASSERT(ncolors >= 0);

  m_frame = frame;
  m_colors.resize(ncolors);

  makeBlack();
  m_modifications = 0;
}

Palette::Palette(const Palette& palette)
  : Object(palette)
{
  m_frame = palette.m_frame;
  m_colors = palette.m_colors;
  m_modifications = 0;
}

Palette::Palette(const Palette& palette, const Remap& remap)
  : Object(palette)
{
  m_frame = palette.m_frame;

  resize(palette.size());
  for (int i=0; i<size(); ++i)
    setEntry(remap[i], palette.getEntry(i));

  m_modifications = 0;
}

Palette::~Palette()
{
}

Palette* Palette::createGrayscale()
{
  Palette* graypal = new Palette(frame_t(0), 256);
  for (int c=0; c<256; c++)
    graypal->setEntry(c, rgba(c, c, c, 255));
  return graypal;
}

void Palette::resize(int ncolors)
{
  ASSERT(ncolors >= 0);

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

void Palette::setFrame(frame_t frame)
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
  int c, diff = 0;
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
    if (from && *from < 0) *from = min;
    if (to) *to = max-1;
  }

  return diff;
}

bool Palette::isBlack() const
{
  for (std::size_t c=0; c<m_colors.size(); ++c)
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
void Palette::makeGradient(int from, int to)
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

int Palette::findExactMatch(int r, int g, int b) const
{
  for (int i=0; i<(int)m_colors.size(); ++i)
    if (getEntry(i) == rgba(r, g, b, 255))
      return i;

  return -1;
}

//////////////////////////////////////////////////////////////////////
// Based on Allegro's bestfit_color

static unsigned int col_diff[3*128];

static void bestfit_init()
{
  int i, k;

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
  lowest = std::numeric_limits<int>::max();

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

} // namespace doc
