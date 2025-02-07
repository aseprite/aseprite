// Aseprite Document Library
// Copyright (c) 2024 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/rgbmap_base.h"

#include <cmath>

namespace doc {

// Auxiliary function for rgbToOtherSpace()
double f(double t)
{
  if (t > 0.00885645171)
    return std::pow(t, 0.3333333333333333);
  else
    return (t / 0.12841855 + 0.137931034);
}

// Auxiliary function for findBestfit()
void RgbMapBase::rgbToOtherSpace(double& r, double& g, double& b) const
{
  if (m_fitCriteria == FitCriteria::RGB)
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
  if (m_fitCriteria == FitCriteria::linearizedRGB) {
    r = Rl;
    g = Gl;
    b = Bl;
    return;
  }
  // Conversion lineal RGB to CIE XYZ
  r = 41.24564 * Rl + 35.75761 * Gl + 18.04375 * Bl;
  g = 21.26729 * Rl + 71.51522 * Gl + 7.2175 * Bl;
  b = 1.93339 * Rl + 11.91920 * Gl + 95.03041 * Bl;
  switch (m_fitCriteria) {
    case FitCriteria::CIEXYZ: return;

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

int RgbMapBase::findBestfit(int r, int g, int b, int a, int mask_index) const
{
  ASSERT(r >= 0 && r <= 255);
  ASSERT(g >= 0 && g <= 255);
  ASSERT(b >= 0 && b <= 255);
  ASSERT(a >= 0 && a <= 255);

  if (m_fitCriteria == FitCriteria::DEFAULT)
    return m_palette->findBestfit(r, g, b, a, mask_index);

  if (a == 0 && mask_index >= 0)
    return mask_index;

  int bestfit = 0;
  double lowest = std::numeric_limits<double>::max();
  const int size = m_palette->size();
  // Linearice:
  double x = double(r);
  double y = double(g);
  double z = double(b);

  rgbToOtherSpace(x, y, z);

  for (int i = 0; i < size; ++i) {
    color_t rgb = m_palette->getEntry(i);
    double Xpal = double(rgba_getr(rgb));
    double Ypal = double(rgba_getg(rgb));
    double Zpal = double(rgba_getb(rgb));
    // Palette color conversion RGB-->XYZ and r,g,b is assumed CIE XYZ
    rgbToOtherSpace(Xpal, Ypal, Zpal);
    const double xDiff = x - Xpal;
    const double yDiff = y - Ypal;
    const double zDiff = z - Zpal;
    const double aDiff = double(a - rgba_geta(rgb)) / 128.0;

    double diff = xDiff * xDiff + yDiff * yDiff + zDiff * zDiff + aDiff * aDiff;
    if (diff < lowest && i != mask_index) {
      lowest = diff;
      bestfit = i;
    }
  }
  return bestfit;
}

} // namespace doc
