// Aseprite Document Library
// Copyright (c) 2020  Igara Studio S.A.
// Copyright (c) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/palette.h"

#include "base/base.h"
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
  m_colors.resize(ncolors, doc::rgba(0, 0, 0, 255));
  m_modifications = 0;
}

Palette::Palette(const Palette& palette)
  : Object(palette)
  , m_comment(palette.m_comment)
{
  m_frame = palette.m_frame;
  m_colors = palette.m_colors;
  m_modifications = 0;
}

Palette::Palette(const Palette& palette, const Remap& remap)
  : Object(palette)
  , m_comment(palette.m_comment)
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

  m_colors.resize(ncolors, doc::rgba(0, 0, 0, 255));
  ++m_modifications;
}

void Palette::addEntry(color_t color)
{
  resize(size()+1);
  setEntry(size()-1, color);
}

bool Palette::hasAlpha() const
{
  for (int i=0; i<(int)m_colors.size(); ++i)
    if (rgba_geta(getEntry(i)) < 255)
      return true;
  return false;
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
  int min = std::min(this->m_colors.size(), other->m_colors.size());
  int max = std::max(this->m_colors.size(), other->m_colors.size());

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
  int r, g, b, a;
  int r1, g1, b1, a1;
  int r2, g2, b2, a2;
  int i, n;

  ASSERT(from >= 0 && from < size());
  ASSERT(to >= 0 && to < size());

  if (from > to)
    std::swap(from, to);

  n = to - from;
  if (n < 2)
    return;

  r1 = rgba_getr(getEntry(from));
  g1 = rgba_getg(getEntry(from));
  b1 = rgba_getb(getEntry(from));
  a1 = rgba_geta(getEntry(from));

  r2 = rgba_getr(getEntry(to));
  g2 = rgba_getg(getEntry(to));
  b2 = rgba_getb(getEntry(to));
  a2 = rgba_geta(getEntry(to));

  for (i=from+1; i<to; ++i) {
    r = r1 + (r2-r1) * (i-from) / n;
    g = g1 + (g2-g1) * (i-from) / n;
    b = b1 + (b2-b1) * (i-from) / n;
    a = a1 + (a2-a1) * (i-from) / n;

    setEntry(i, rgba(r, g, b, a));
  }
}

int Palette::findExactMatch(int r, int g, int b, int a, int mask_index) const
{
  for (int i=0; i<(int)m_colors.size(); ++i)
    if (getEntry(i) == rgba(r, g, b, a) && i != mask_index)
      return i;

  return -1;
}

//////////////////////////////////////////////////////////////////////
// Based on Allegro's bestfit_color

static std::vector<uint32_t> col_diff;
static uint32_t* col_diff_g;
static uint32_t* col_diff_r;
static uint32_t* col_diff_b;
static uint32_t* col_diff_a;

static void initBestfit()
{
  col_diff.resize(4*128, 0);
  col_diff_g = &col_diff[128*0];
  col_diff_r = &col_diff[128*1];
  col_diff_b = &col_diff[128*2];
  col_diff_a = &col_diff[128*3];

  for (int i=1; i<64; ++i) {
    int k = i * i;
    col_diff_g[i] = col_diff_g[128-i] = k * 59 * 59;
    col_diff_r[i] = col_diff_r[128-i] = k * 30 * 30;
    col_diff_b[i] = col_diff_b[128-i] = k * 11 * 11;
    col_diff_a[i] = col_diff_a[128-i] = k * 8 * 8;
  }
}

int Palette::findBestfit(int r, int g, int b, int a, int mask_index) const
{
  ASSERT(r >= 0 && r <= 255);
  ASSERT(g >= 0 && g <= 255);
  ASSERT(b >= 0 && b <= 255);
  ASSERT(a >= 0 && a <= 255);

  if (col_diff.empty())
    initBestfit();

  r >>= 3;
  g >>= 3;
  b >>= 3;
  a >>= 3;

  // Mask index is like alpha = 0, so we can use it as transparent color.
  if (a == 0 && mask_index >= 0)
    return mask_index;

  int bestfit = 0;
  int lowest = std::numeric_limits<int>::max();
  int size = std::min(256, int(m_colors.size()));

  for (int i=0; i<size; ++i) {
    color_t rgb = m_colors[i];

    int coldiff = col_diff_g[((rgba_getg(rgb)>>3) - g) & 127];
    if (coldiff < lowest) {
      coldiff += col_diff_r[(((rgba_getr(rgb)>>3) - r) & 127)];
      if (coldiff < lowest) {
        coldiff += col_diff_b[(((rgba_getb(rgb)>>3) - b) & 127)];
        if (coldiff < lowest) {
          coldiff += col_diff_a[(((rgba_geta(rgb)>>3) - a) & 127)];
          if (coldiff < lowest && i != mask_index) {
            if (coldiff == 0)
              return i;

            bestfit = i;
            lowest = coldiff;
          }
        }
      }
    }
  }

  return bestfit;
}

void Palette::applyRemap(const Remap& remap)
{
  Palette original(*this);
  for (int i=0; i<size(); ++i)
    setEntry(remap[i], original.getEntry(i));
}

void Palette::setEntryName(const int i, const std::string& name)
{
  if (i >= int(m_names.size()))
    m_names.resize(i+1);
  m_names[i] = name;
}

const std::string& Palette::getEntryName(const int i) const
{
  if (i >= 0 && i < int(m_names.size()))
    return m_names[i];
  else {
    static std::string emptyString;
    return emptyString;
  }
}

} // namespace doc
