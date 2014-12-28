// Aseprite Gfx Library
// Copyright (C) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/hsv.h"
#include "gfx/rgb.h"
#include <cmath>

namespace gfx {

using namespace std;

Hsv::Hsv(double hue, double saturation, double value)
  : m_hue(hue)
  , m_saturation(saturation)
  , m_value(value)
{
  while (m_hue < 0.0)
    m_hue += 360.0;
  m_hue = fmod(hue, 360.0);

  assert(hue        >= 0.0 && hue        <= 360.0);
  assert(saturation >= 0.0 && saturation <= 1.0);
  assert(value      >= 0.0 && value      <= 1.0);
}

// Reference: http://en.wikipedia.org/wiki/HSL_and_HSV
Hsv::Hsv(const Rgb& rgb)
{
  int M = rgb.maxComponent();
  int m = rgb.minComponent();
  int c = M - m;
  double chroma = double(c) / 255.0;
  double hue_prime = 0.0;
  double h, s, v;
  double r, g, b;

  v = double(M) / 255.0;

  if (c == 0) {
    h = 0.0; // Undefined Hue because max == min
    s = 0.0;
  }
  else {
    r = double(rgb.red())   / 255.0;
    g = double(rgb.green()) / 255.0;
    b = double(rgb.blue())  / 255.0;
    s = chroma / v;

    if (M == rgb.red()) {
      hue_prime = (g - b) / chroma;

      while (hue_prime < 0.0)
        hue_prime += 6.0;
      hue_prime = fmod(hue_prime, 6.0);
    }
    else if (M == rgb.green()) {
      hue_prime = ((b - r) / chroma) + 2.0;
    }
    else if (M == rgb.blue()) {
      hue_prime = ((r - g) / chroma) + 4.0;
    }

    h = hue_prime * 60.0;
  }

  m_hue = h;
  m_saturation = s;
  m_value = v;
}

int Hsv::hueInt() const
{
  return int(floor(m_hue + 0.5));
}

int Hsv::saturationInt() const
{
  return int(floor(m_saturation*100.0 + 0.5));
}

int Hsv::valueInt() const
{
  return int(floor(m_value*100.0 + 0.5));
}

} // namespace gfx
