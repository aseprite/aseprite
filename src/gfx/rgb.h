// Aseprite Gfx Library
// Copyright (C) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef GFX_RGB_H_INCLUDED
#define GFX_RGB_H_INCLUDED
#pragma once

#include <cassert>

namespace gfx {

class Hsv;
class Hsl;

class Rgb {
public:
  Rgb()
    : m_red(0)
    , m_green(0)
    , m_blue(0)
  { }

  Rgb(int red, int green, int blue)
    : m_red(red)
    , m_green(green)
    , m_blue(blue)
  {
    assert(red   >= 0 && red   <= 255);
    assert(green >= 0 && green <= 255);
    assert(blue  >= 0 && blue  <= 255);
  }

  Rgb(const Rgb& rgb)
    : m_red(rgb.red())
    , m_green(rgb.green())
    , m_blue(rgb.blue())
  { }

  // Conversions
  explicit Rgb(const Hsv& hsv);
  explicit Rgb(const Hsl& hsl);

  int red() const {
    return m_red;
  }

  int green() const {
    return m_green;
  }

  int blue() const {
    return m_blue;
  }

  int maxComponent() const;
  int minComponent() const;

  void red(int red) {
    assert(red >= 0 && red <= 255);
    m_red = red;
  }

  void green(int green) {
    assert(green >= 0 && green <= 255);
    m_green = green;
  }

  void blue(int blue) {
    assert(blue >= 0 && blue <= 255);
    m_blue = blue;
  }

  bool operator==(const Rgb& other) const {
    return (m_red == other.m_red &&
            m_green == other.m_green &&
            m_blue == other.m_blue);
  }

  bool operator!=(const Rgb& other) const {
    return !operator==(other);
  }

private:
  int m_red;
  int m_green;
  int m_blue;
};

} // namespace gfx

#endif
