/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#include "raster/pixel_format.h"

#include <string>

namespace raster {
  class Image;
  class Layer;
}

namespace app {

  using namespace raster;

  class Color {
  public:
    enum Type {
      MaskType,
      RgbType,
      HsvType,
      GrayType,
      IndexType,
    };

    enum HumanReadableString {
      ShortHumanReadableString,
      LongHumanReadableString
    };

    // Default ctor is mask color
    Color() : m_type(MaskType) { }

    static Color fromMask();
    static Color fromRgb(int r, int g, int b);
    static Color fromHsv(int h, int s, int v); // h=[0,360], s=[0,100], v=[0,100]
    static Color fromGray(int g);
    static Color fromIndex(int index);

    static Color fromImage(PixelFormat pixelFormat, int pixel);
    static Color fromImageGetPixel(Image* image, int x, int y);
    static Color fromString(const std::string& str);

    std::string toString() const;
    std::string toHumanReadableString(PixelFormat format, HumanReadableString type) const;

    bool operator==(const Color& other) const;
    bool operator!=(const Color& other) const {
      return !operator==(other);
    }

    Type getType() const {
      return m_type;
    }

    bool isValid() const;

    // Getters
    int getRed() const;
    int getGreen() const;
    int getBlue() const;
    int getHue() const;
    int getSaturation() const;
    int getValue() const;
    int getGray() const;
    int getIndex() const;

  private:
    Color(Type type) : m_type(type) { }

    // Color type
    Type m_type;

    // Color value
    union {
      struct {
        int r, g, b;
      } rgb;
      struct {
        int h, s, v;
      } hsv;
      int gray;
      int index;
    } m_value;
  };

} // namespace app

#endif
