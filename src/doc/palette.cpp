// Aseprite Document Library
// Copyright (c) 2020-2023 Igara Studio S.A.
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
#include "doc/palette_gradient_type.h"
#include "doc/remap.h"
#include "gfx/hsv.h"
#include "gfx/rgb.h"

#include <algorithm>
#include <limits>
#include <cmath>

namespace doc {

using namespace gfx;

enum class FitCriteria {
  OLD,
  RGB,
  linearizedRGB,
  CIEXYZ,
  CIELAB
};

Palette::Palette()
  : Palette(0, 256)
{
}

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

Palette& Palette::operator=(const Palette& that)
{
  m_frame = that.m_frame;
  m_colors = that.m_colors;
  m_names = that.m_names;
  m_filename = that.m_filename;
  m_comment = that.m_comment;

  ++m_modifications;
  return *this;
}

Palette* Palette::createGrayscale()
{
  Palette* graypal = new Palette(frame_t(0), 256);
  for (int c=0; c<256; c++)
    graypal->setEntry(c, rgba(c, c, c, 255));
  return graypal;
}

void Palette::resize(int ncolors, color_t color)
{
  ASSERT(ncolors >= 0);

  m_colors.resize(ncolors, color);
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

bool Palette::hasSemiAlpha() const
{
  for (int i=0; i<(int)m_colors.size(); ++i) {
    int a = rgba_geta(getEntry(i));
    if (a > 0 && a < 255)
      return true;
  }
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

void Palette::makeHueGradient(int from, int to)
{
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

  gfx::Hsv hsv1(gfx::Rgb(r1, g1, b1));
  gfx::Hsv hsv2(gfx::Rgb(r2, g2, b2));

  double h1 = hsv1.hue();
  double s1 = hsv1.saturation();
  double v1 = hsv1.value();

  double h2 = hsv2.hue();
  double s2 = hsv2.saturation();
  double v2 = hsv2.value();

  if (h2 >= h1) {
    if (h2-h1 > 180.0)
      h2 = h2 - 360.0;
  }
  else {
    if (h1-h2 > 180.0)
      h2 = h2 + 360.0;
  }

  gfx::Hsv hsv;
  for (i=from+1; i<to; ++i) {
    double t = double(i - from) / double(n);
    hsv.hue(h1 + (h2 - h1) * t);
    hsv.saturation(s1 + (s2 - s1) * t);
    hsv.value(v1 + (v2 - v1) * t);
    int alpha = int(a1 + double(a2 - a1) * t);
    gfx::Rgb rgb(hsv);
    setEntry(i, rgba(rgb.red(), rgb.green(), rgb.blue(), alpha));
  }
}

int Palette::findExactMatch(int r, int g, int b, int a, int mask_index) const
{
  for (int i=0; i<(int)m_colors.size(); ++i)
    if (getEntry(i) == rgba(r, g, b, a) && i != mask_index)
      return i;

  return -1;
}

bool Palette::findExactMatch(color_t color) const
{
  for (int i=0; i<(int)m_colors.size(); ++i) {
    if (getEntry(i) == color)
      return true;
  }
  return false;
}

//////////////////////////////////////////////////////////////////////
// Based on Allegro's bestfit_color

static std::vector<uint32_t> col_diff;
static uint32_t* col_diff_g;
static uint32_t* col_diff_r;
static uint32_t* col_diff_b;
static uint32_t* col_diff_a;

void Palette::initBestfit()
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

// Auxiliary function for rgbToOtherSpace()
static double f(double t)
{
  if (t > 0.00885645171)
    return std::pow(t, 0.3333333333333333);
  else
    return (t / 0.12841855 + 0.137931034);
}

// Auxiliary function for findBestfit()
static void rgbToOtherSpace(double& r, double& g, double& b, FitCriteria fc)
{
  if (fc == FitCriteria::RGB)
    return;
  double Rl, Gl, Bl;
  // Linearization:
  r = r / 255.0;
  g = g / 255.0;
  b = b / 255.0;
  if (r <= 0.04045)
    Rl = r / 12.92;
  else
    Rl = std::pow((r + 0.055) / 1.055, 2.4);
  if (g <= 0.04045)
    Gl = g / 12.92;
  else
    Gl = std::pow((g + 0.055) / 1.055, 2.4);
  if (b <= 0.04045)
    Bl = b / 12.92;
  else
    Bl = std::pow((b + 0.055) / 1.055, 2.4);
  if (fc == FitCriteria::linearizedRGB) {
    r = Rl;
    g = Gl;
    b = Bl;
    return;
  }
  // Conversion lineal RGB to CIE XYZ
  r = 41.24564*Rl + 35.75761 * Gl + 18.04375 * Bl;
  g = 21.26729*Rl + 71.51522 * Gl + 7.2175   * Bl;
  b = 1.93339*Rl  + 11.91920 * Gl + 95.03041 * Bl;
  switch (fc) {

    case FitCriteria::CIEXYZ:
      return;

    case FitCriteria::CIELAB: {
      // Converting CIEXYZ to CIELAB:
      // For Standard Illuminant D65:
      //  const double xn = 95.0489;
      //  const double yn = 100.0;
      //  const double zn = 108.884;
      double xxn = r / 95.0489;
      double yyn = g / 100.0;
      double zzn = b / 108.884;
      double fyyn = f(yyn);

      double Lstar = 116.0 * fyyn - 16.0;
      double aStar = 500.0 * (f(xxn) - fyyn);
      double bStar = 200.0 * (fyyn - f(zzn));

      r = Lstar;
      g = aStar;
      b = bStar;
      return;
    }
  }
}

int Palette::findBestfit(int r, int g, int b, int a, int mask_index) const
{
  ASSERT(r >= 0 && r <= 255);
  ASSERT(g >= 0 && g <= 255);
  ASSERT(b >= 0 && b <= 255);
  ASSERT(a >= 0 && a <= 255);

  FitCriteria fc = FitCriteria::OLD;

  if (fc == FitCriteria::OLD) {
    ASSERT(!col_diff.empty());

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

  if (a == 0 && mask_index >= 0)
    return mask_index;

  int bestfit = 0;
  double lowest = std::numeric_limits<double>::max();
  int size = m_colors.size();
  // Linearice:
  double x = double(r);
  double y = double(g);
  double z = double(b);

  rgbToOtherSpace(x, y, z, fc);

  for (int i=0; i<size; ++i) {
    color_t rgb = m_colors[i];
    double Xpal = double(rgba_getr(rgb));
    double Ypal = double(rgba_getg(rgb));
    double Zpal = double(rgba_getb(rgb));
    // Palette color conversion RGB-->XYZ and r,g,b is assumed CIE XYZ
    rgbToOtherSpace(Xpal, Ypal, Zpal, fc);
    double xDiff = x - Xpal;
    double yDiff = y - Ypal;
    double zDiff = z - Zpal;
    double aDiff = double(a - rgba_geta(rgb)) / 128.0;

    double diff = xDiff * xDiff + yDiff * yDiff + zDiff * zDiff + aDiff * aDiff;
    if (diff < lowest) {
      lowest = diff;
      bestfit = i;
    }
  }
  return bestfit;
}

int Palette::findMaskColor() const
{
  int size = m_colors.size();
  for (int i = 0; i < size; ++i) {
    if (m_colors[i] == 0)
      return i;
  }
  return -1;
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
