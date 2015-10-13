// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/color.h"

#include "app/color_utils.h"
#include "app/modules/palettes.h"
#include "gfx/hsv.h"
#include "gfx/rgb.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "doc/primitives.h"

#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>

namespace app {

using namespace gfx;

// static
Color Color::fromMask()
{
  return Color(Color::MaskType);
}

// static
Color Color::fromRgb(int r, int g, int b, int a)
{
  Color color(Color::RgbType);
  color.m_value.rgb.r = r;
  color.m_value.rgb.g = g;
  color.m_value.rgb.b = b;
  color.m_value.rgb.a = a;
  return color;
}

// static
Color Color::fromHsv(int h, int s, int v, int a)
{
  Color color(Color::HsvType);
  color.m_value.hsv.h = h;
  color.m_value.hsv.s = s;
  color.m_value.hsv.v = v;
  color.m_value.hsv.a = a;
  return color;
}

// static
Color Color::fromGray(int g, int a)
{
  Color color(Color::GrayType);
  color.m_value.gray.g = g;
  color.m_value.gray.a = a;
  return color;
}

// static
Color Color::fromIndex(int index)
{
  assert(index >= 0);

  Color color(Color::IndexType);
  color.m_value.index = index;
  return color;
}

// static
Color Color::fromImage(PixelFormat pixelFormat, color_t c)
{
  Color color = app::Color::fromMask();

  switch (pixelFormat) {

    case IMAGE_RGB:
      if (rgba_geta(c) > 0) {
        color = Color::fromRgb(rgba_getr(c),
                               rgba_getg(c),
                               rgba_getb(c),
                               rgba_geta(c));
      }
      break;

    case IMAGE_GRAYSCALE:
      if (graya_geta(c) > 0) {
        color = Color::fromGray(graya_getv(c),
                                graya_geta(c));
      }
      break;

    case IMAGE_INDEXED:
      color = Color::fromIndex(c);
      break;
  }

  return color;
}

// static
Color Color::fromImageGetPixel(Image *image, int x, int y)
{
  if ((x >= 0) && (y >= 0) && (x < image->width()) && (y < image->height()))
    return Color::fromImage(image->pixelFormat(), doc::get_pixel(image, x, y));
  else
    return Color::fromMask();
}

// static
Color Color::fromString(const std::string& str)
{
  Color color = app::Color::fromMask();

  if (str != "mask") {
    if (str.find("rgb{") == 0 ||
        str.find("hsv{") == 0 ||
        str.find("gray{") == 0) {
      int c = 0, table[4] = { 0, 0, 0, 255 };
      std::string::size_type i = str.find_first_of('{')+1, j;

      while ((j = str.find_first_of(",}", i)) != std::string::npos) {
        std::string element = str.substr(i, j - i);
        if (c < 4)
          table[c++] = std::strtol(element.c_str(), NULL, 10);
        if (c >= 4)
          break;
        i = j+1;
      }

      if (str[0] == 'r')
        color = Color::fromRgb(table[0], table[1], table[2], table[3]);
      else if (str[0] == 'h')
        color = Color::fromHsv(table[0], table[1], table[2], table[3]);
      else if (str[0] == 'g')
        color = Color::fromGray(table[0], c >= 2 ? table[1]: 255);
    }
    else if (str.find("index{") == 0) {
      color = Color::fromIndex(std::strtol(str.c_str()+6, NULL, 10));
    }
  }

  return color;
}

std::string Color::toString() const
{
  std::stringstream result;

  switch (getType()) {

    case Color::MaskType:
      result << "mask";
      break;

    case Color::RgbType:
      result << "rgb{"
             << m_value.rgb.r << ","
             << m_value.rgb.g << ","
             << m_value.rgb.b << ","
             << m_value.rgb.a << "}";
      break;

    case Color::HsvType:
      result << "hsv{"
             << m_value.hsv.h << ","
             << m_value.hsv.s << ","
             << m_value.hsv.v << ","
             << m_value.hsv.a << "}";
      break;

    case Color::GrayType:
      result << "gray{"
             << m_value.gray.g << ","
             << m_value.gray.a << "}";
      break;

    case Color::IndexType:
      result << "index{" << m_value.index << "}";
      break;

  }

  return result.str();
}

std::string Color::toHumanReadableString(PixelFormat pixelFormat, HumanReadableString humanReadable) const
{
  std::stringstream result;

  if (humanReadable == LongHumanReadableString) {
    switch (getType()) {

      case Color::MaskType:
        result << "Mask";
        break;

      case Color::RgbType:
        if (pixelFormat == IMAGE_GRAYSCALE) {
          result << "Gray " << getGray();
        }
        else {
          result << "RGB "
                 << m_value.rgb.r << " "
                 << m_value.rgb.g << " "
                 << m_value.rgb.b;

          if (pixelFormat == IMAGE_INDEXED)
            result << " Index "
                   << color_utils::color_for_image(*this, pixelFormat);
        }
        break;

      case Color::HsvType:
        if (pixelFormat == IMAGE_GRAYSCALE) {
          result << "Gray " << getGray();
        }
        else {
          result << "HSB "
                 << m_value.hsv.h << "\xc2\xb0 "
                 << m_value.hsv.s << " "
                 << m_value.hsv.v;

          if (pixelFormat == IMAGE_INDEXED)
            result << " Index " << color_utils::color_for_image(*this, pixelFormat);
        }
        break;

      case Color::GrayType:
        result << "Gray " << m_value.gray.g;
        break;

      case Color::IndexType: {
        int i = m_value.index;
        if (i >= 0 && i < (int)get_current_palette()->size()) {
          uint32_t _c = get_current_palette()->getEntry(i);
          result << "Index " << i
                 << " (RGB "
                 << (int)rgba_getr(_c) << " "
                 << (int)rgba_getg(_c) << " "
                 << (int)rgba_getb(_c) << ")";
        }
        else {
          result << "Index "
                 << i
                 << " (out of range)";
        }
        break;
      }

      default:
        ASSERT(false);
        break;
    }

    result << " #" << std::hex << std::setfill('0')
           << std::setw(2) << getRed()
           << std::setw(2) << getGreen()
           << std::setw(2) << getBlue();
  }
  else if (humanReadable == ShortHumanReadableString) {
    switch (getType()) {

      case Color::MaskType:
        result << "Mask";
        break;

      case Color::RgbType:
        if (pixelFormat == IMAGE_GRAYSCALE) {
          result << "Gry-" << getGray();
        }
        else {
          result << "#" << std::hex << std::setfill('0')
                 << std::setw(2) << m_value.rgb.r
                 << std::setw(2) << m_value.rgb.g
                 << std::setw(2) << m_value.rgb.b;
        }
        break;

      case Color::HsvType:
        if (pixelFormat == IMAGE_GRAYSCALE) {
          result << "Gry-" << getGray();
        }
        else {
          result << m_value.hsv.h << "\xc2\xb0"
                 << m_value.hsv.s << ","
                 << m_value.hsv.v;
        }
        break;

      case Color::GrayType:
        result << "Gry-" << m_value.gray.g;
        break;

      case Color::IndexType:
        result << "Idx-" << m_value.index;
        break;

      default:
        ASSERT(false);
        break;
    }
  }

  return result.str();
}

bool Color::operator==(const Color& other) const
{
  if (getType() != other.getType())
    return false;

  switch (getType()) {

    case Color::MaskType:
      return true;

    case Color::RgbType:
      return
        m_value.rgb.r == other.m_value.rgb.r &&
        m_value.rgb.g == other.m_value.rgb.g &&
        m_value.rgb.b == other.m_value.rgb.b &&
        m_value.rgb.a == other.m_value.rgb.a;

    case Color::HsvType:
      return
        m_value.hsv.h == other.m_value.hsv.h &&
        m_value.hsv.s == other.m_value.hsv.s &&
        m_value.hsv.v == other.m_value.hsv.v &&
        m_value.hsv.a == other.m_value.hsv.a;

    case Color::GrayType:
      return
        m_value.gray.g == other.m_value.gray.g &&
        m_value.gray.a == other.m_value.gray.a;

    case Color::IndexType:
      return m_value.index == other.m_value.index;

    default:
      ASSERT(false);
      return false;
  }
}

// Returns false only if the color is a index and it is outside the
// valid range (outside the maximum number of colors in the current
// palette)
bool Color::isValid() const
{
  switch (getType()) {

    case Color::IndexType: {
      int i = m_value.index;
      return (i >= 0 && i < get_current_palette()->size());
    }

  }
  return true;
}

int Color::getRed() const
{
  switch (getType()) {

    case Color::MaskType:
      return 0;

    case Color::RgbType:
      return m_value.rgb.r;

    case Color::HsvType:
      return Rgb(Hsv(m_value.hsv.h,
                     double(m_value.hsv.s) / 100.0,
                     double(m_value.hsv.v) / 100.0)).red();

    case Color::GrayType:
      return m_value.gray.g;

    case Color::IndexType: {
      int i = m_value.index;
      if (i >= 0 && i < get_current_palette()->size())
        return rgba_getr(get_current_palette()->getEntry(i));
      else
        return 0;
    }

  }

  ASSERT(false);
  return -1;
}

int Color::getGreen() const
{
  switch (getType()) {

    case Color::MaskType:
      return 0;

    case Color::RgbType:
      return m_value.rgb.g;

    case Color::HsvType:
      return Rgb(Hsv(m_value.hsv.h,
                     double(m_value.hsv.s) / 100.0,
                     double(m_value.hsv.v) / 100.0)).green();

    case Color::GrayType:
      return m_value.gray.g;

    case Color::IndexType: {
      int i = m_value.index;
      if (i >= 0 && i < get_current_palette()->size())
        return rgba_getg(get_current_palette()->getEntry(i));
      else
        return 0;
    }

  }

  ASSERT(false);
  return -1;
}

int Color::getBlue() const
{
  switch (getType()) {

    case Color::MaskType:
      return 0;

    case Color::RgbType:
      return m_value.rgb.b;

    case Color::HsvType:
      return Rgb(Hsv(m_value.hsv.h,
                     double(m_value.hsv.s) / 100.0,
                     double(m_value.hsv.v) / 100.0)).blue();

    case Color::GrayType:
      return m_value.gray.g;

    case Color::IndexType: {
      int i = m_value.index;
      if (i >= 0 && i < get_current_palette()->size())
        return rgba_getb(get_current_palette()->getEntry(i));
      else
        return 0;
    }

  }

  ASSERT(false);
  return -1;
}

int Color::getHue() const
{
  switch (getType()) {

    case Color::MaskType:
      return 0;

    case Color::RgbType:
      return Hsv(Rgb(m_value.rgb.r,
                     m_value.rgb.g,
                     m_value.rgb.b)).hueInt();

    case Color::HsvType:
      return m_value.hsv.h;

    case Color::GrayType:
      return 0;

    case Color::IndexType: {
      int i = m_value.index;
      if (i >= 0 && i < get_current_palette()->size()) {
        uint32_t c = get_current_palette()->getEntry(i);
        return Hsv(Rgb(rgba_getr(c),
                       rgba_getg(c),
                       rgba_getb(c))).hueInt();
      }
      else
        return 0;
    }

  }

  ASSERT(false);
  return -1;
}

int Color::getSaturation() const
{
  switch (getType()) {

    case Color::MaskType:
      return 0;

    case Color::RgbType:
      return Hsv(Rgb(m_value.rgb.r,
                     m_value.rgb.g,
                     m_value.rgb.b)).saturationInt();

    case Color::HsvType:
      return m_value.hsv.s;

    case Color::GrayType:
      return 0;

    case Color::IndexType: {
      int i = m_value.index;
      if (i >= 0 && i < get_current_palette()->size()) {
        uint32_t c = get_current_palette()->getEntry(i);
        return Hsv(Rgb(rgba_getr(c),
                       rgba_getg(c),
                       rgba_getb(c))).saturationInt();
      }
      else
        return 0;
    }

  }

  ASSERT(false);
  return -1;
}

int Color::getValue() const
{
  switch (getType()) {

    case Color::MaskType:
      return 0;

    case Color::RgbType:
      return Hsv(Rgb(m_value.rgb.r,
                     m_value.rgb.g,
                     m_value.rgb.b)).valueInt();

    case Color::HsvType:
      return m_value.hsv.v;

    case Color::GrayType:
      return 100 * m_value.gray.g / 255;

    case Color::IndexType: {
      int i = m_value.index;
      if (i >= 0 && i < get_current_palette()->size()) {
        uint32_t c = get_current_palette()->getEntry(i);
        return Hsv(Rgb(rgba_getr(c),
                       rgba_getg(c),
                       rgba_getb(c))).valueInt();
      }
      else
        return 0;
    }

  }

  ASSERT(false);
  return -1;
}

int Color::getGray() const
{
  switch (getType()) {

    case Color::MaskType:
      return 0;

    case Color::RgbType:
      return 255 * Hsv(Rgb(m_value.rgb.r,
                           m_value.rgb.g,
                           m_value.rgb.b)).valueInt() / 100;

    case Color::HsvType:
      return 255 * m_value.hsv.v / 100;

    case Color::GrayType:
      return m_value.gray.g;

    case Color::IndexType: {
      int i = m_value.index;
      if (i >= 0 && i < get_current_palette()->size()) {
        uint32_t c = get_current_palette()->getEntry(i);
        return 255 * Hsv(Rgb(rgba_getr(c),
                             rgba_getg(c),
                             rgba_getb(c))).valueInt() / 100;
      }
      else
        return 0;
    }

  }

  ASSERT(false);
  return -1;
}

int Color::getIndex() const
{
  switch (getType()) {

    case Color::MaskType:
      return 0;

    case Color::RgbType:
    case Color::HsvType:
    case Color::GrayType: {
      int i = get_current_palette()->findExactMatch(getRed(), getGreen(), getBlue(), getAlpha(), -1);
      if (i >= 0)
        return i;
      else
        return get_current_palette()->findBestfit(getRed(), getGreen(), getBlue(), getAlpha(), 0);
    }

    case Color::IndexType:
      return m_value.index;

  }

  ASSERT(false);
  return -1;
}

int Color::getAlpha() const
{
  switch (getType()) {

    case Color::MaskType:
      return 0;

    case Color::RgbType:
      return m_value.rgb.a;

    case Color::HsvType:
      return m_value.hsv.a;

    case Color::GrayType:
      return m_value.gray.a;

    case Color::IndexType: {
      int i = m_value.index;
      if (i >= 0 && i < get_current_palette()->size())
        return rgba_geta(get_current_palette()->getEntry(i));
      else
        return 0;
    }

  }

  ASSERT(false);
  return -1;
}

} // namespace app
