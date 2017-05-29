// Aseprite Gfx Library
// Copyright (C) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef GFX_HSL_H_INCLUDED
#define GFX_HSL_H_INCLUDED
#pragma once

#include "base/base.h"          // MID

namespace gfx {

class Rgb;

class Hsl {
public:
  Hsl()
    : m_hue(0.0)
    , m_saturation(0.0)
    , m_lightness(0.0)
  { }

  Hsl(double hue, double saturation, double lightness);

  Hsl(const Hsl& hsl)
    : m_hue(hsl.hue())
    , m_saturation(hsl.saturation())
    , m_lightness(hsl.lightness())
  { }

  // RGB to HSL conversion
  explicit Hsl(const Rgb& rgb);

  // Returns color's hue, a value from 0 to 360
  double hue() const { return m_hue; }

  // Returns color's saturation, a value from 0 to 100
  double saturation() const { return m_saturation; }

  // Returns color's lightness, a value from 0 to 100
  double lightness() const { return m_lightness; }

  // Integer getters, hue=[0,360), saturation=[0,100], value=[0,100]
  int hueInt() const;
  int saturationInt() const;
  int lightnessInt() const;

  void hue(double hue) {
    m_hue = MID(0.0, hue, 360.0);
  }

  void saturation(double saturation) {
    m_saturation = MID(0.0, saturation, 1.0);
  }

  void lightness(double lightness) {
    m_lightness = MID(0.0, lightness, 1.0);
  }

  // The comparison is done through the integer value of each component.
  bool operator==(const Hsl& other) const {
    return (hueInt() == other.hueInt() &&
            saturationInt() == other.saturationInt() &&
            lightnessInt() == other.lightnessInt());
  }

  bool operator!=(const Hsl& other) const {
    return !operator==(other);
  }

private:
  double m_hue;
  double m_saturation;
  double m_lightness;
};

} // namespace gfx

#endif
