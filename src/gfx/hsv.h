// ASE gfx library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GFX_HSV_H_INCLUDED
#define GFX_HSV_H_INCLUDED

#include <cassert>

namespace gfx {

class Rgb;

class Hsv
{
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

  double hue() const {
    return m_hue;
  }

  double saturation() const {
    return m_saturation;
  }

  double value() const {
    return m_value;
  }

  // Getters in "int" type, hue=[0,360), saturation=[0,100], value=[0,100]
  int hueInt() const        { return int(m_hue+0.5); }
  int saturationInt() const { return int(m_saturation*100.0+0.5); }
  int valueInt() const      { return int(m_value*100.0+0.5); }

  void hue(double hue) {
    assert(hue >= 0.0 && hue <= 360.0);
    m_hue = hue;
  }

  void saturation(double saturation) {
    assert(saturation >= 0.0 && saturation <= 1.0);
    m_saturation = saturation;
  }

  void value(double value) {
    assert(value >= 0.0 && value <= 1.0);
    m_value = value;
  }

  bool operator==(const Hsv& other) const {
    // Compare floating points as integers (saturation and value are compared in [0,255] range)
    return (int(m_hue+0.5) == int(other.m_hue+0.5) && 
	    int(m_saturation*255.0+0.5) == int(other.m_saturation*255.0+0.5) && 
	    int(m_value*255.0+0.5) == int(other.m_value*255.0+0.5));
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
