/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef APP_COLOR_H_INCLUDED
#define APP_COLOR_H_INCLUDED

#include <string>

class Image;
class Layer;

class Color {
public:
  enum Type {
    MaskType,
    RgbType,
    HsvType,
    GrayType,
    IndexType,
  };

  // Default ctor is mask color
  Color()
    : m_value(fromMask().m_value) {
  }

  static Color fromMask();
  static Color fromRgb(int r, int g, int b);
  static Color fromHsv(int h, int s, int v);
  static Color fromGray(int g);
  static Color fromIndex(int index);

  static Color fromImage(int imgtype, int pixel);
  static Color fromImageGetPixel(Image* image, int x, int y);
  static Color fromString(const std::string& str);

  std::string toString() const;
  std::string toFormalString(int imgtype, bool long_format) const;

  bool operator==(const Color& other) const {
    return m_value == other.m_value;
  }

  bool operator!=(const Color& other) const {
    return m_value != other.m_value;
  }

  Type getType() const {
    return static_cast<Type>(m_value >> 24);
  }

  bool isValid() const;

  int getRed() const;
  int getGreen() const;
  int getBlue() const;
  int getHue() const;
  int getSaturation() const;
  int getValue() const;
  int getIndex() const;

private:
  Color(Type type, uint32_t data)
    : m_value((static_cast<int>(type) << 24) |
	      (data & 0xffffff)) {
    ASSERT((data & 0xff000000) == 0);
  }

  uint32_t getRgbData() const {
    return m_value & 0xffffff;
  }

  uint32_t getHsvData() const {
    return m_value & 0xffffff;
  }

  uint8_t getGrayData() const {
    return m_value & 0xff;
  }

  uint8_t getIndexData() const {
    return m_value & 0xff;
  }

  uint32_t m_value;
};

#endif

