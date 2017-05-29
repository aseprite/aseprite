// Aseprite Gfx Library
// Copyright (C) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef GFX_HSV_H_INCLUDED
#define GFX_HSV_H_INCLUDED
#pragma once

#include "base/base.h"          // MID

namespace gfx {

class Rgb;

class Hsv {
public:
  Hsv()
    : m_hue(0.0)
    , m_saturation(0.0)
    , m_value(0.0)
  { }

  Hsv(double hue, double saturation, double value);

  Hsv(const Hsv& hsv)
    : m_hue(hsv.hue())
    , m_saturation(hsv.saturation())
    , m_value(hsv.value())
  { }

  // RGB to HSV conversion
  explicit Hsv(const Rgb& rgb);

  // Returns color's hue, a value from 0 to 360
  double hue() const { return m_hue; }

  // Returns color's saturation, a value from 0 to 100
  double saturation() const { return m_saturation; }

  // Returns color's brightness, a value from 0 to 100
  double value() const { return m_value; }

  // Integer getters, hue=[0,360), saturation=[0,100], value=[0,100]
  int hueInt() const;
  int saturationInt() const;
  int valueInt() const;

  void hue(double hue) {
    m_hue = MID(0.0, hue, 360.0);
  }

  void saturation(double saturation) {
    m_saturation = MID(0.0, saturation, 1.0);
  }

  void value(double value) {
    m_value = MID(0.0, value, 1.0);
  }

  // The comparison is done through the integer value of each component.
  bool operator==(const Hsv& other) const {
    return (hueInt() == other.hueInt() &&
            saturationInt() == other.saturationInt() &&
            valueInt() == other.valueInt());
  }

  bool operator!=(const Hsv& other) const {
    return !operator==(other);
  }

private:
  double m_hue;
  double m_saturation;
  double m_value;
};

} // namespace gfx

#endif
