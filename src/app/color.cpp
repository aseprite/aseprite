/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "config.h"

#include <string>
#include <cstdlib>
#include <sstream>
#include <iomanip>

#include "app/color.h"
#include "app/color_utils.h"
#include "gfx/hsv.h"
#include "gfx/rgb.h"
#include "modules/palettes.h"
#include "raster/image.h"
#include "raster/palette.h"

using namespace gfx;

// static
Color Color::fromMask()
{
  return Color(Color::MaskType);
}

// static
Color Color::fromRgb(int r, int g, int b)
{
  Color color(Color::RgbType);
  color.m_value.rgb.r = r;
  color.m_value.rgb.g = g;
  color.m_value.rgb.b = b;
  return color;
}

// static
Color Color::fromHsv(int h, int s, int v)
{
  Color color(Color::HsvType);
  color.m_value.hsv.h = h;
  color.m_value.hsv.s = s;
  color.m_value.hsv.v = v;
  return color;
}

// static
Color Color::fromGray(int g)
{
  Color color(Color::GrayType);
  color.m_value.gray = g;
  return color;
}

// static
Color Color::fromIndex(int index)
{
  Color color(Color::IndexType);
  color.m_value.index = index;
  return color;
}

// static
Color Color::fromImage(int imgtype, int c)
{
  Color color = Color::fromMask();

  switch (imgtype) {

    case IMAGE_RGB:
      if (_rgba_geta(c) > 0) {
	color = Color::fromRgb(_rgba_getr(c),
			       _rgba_getg(c),
			       _rgba_getb(c));
      }
      break;

    case IMAGE_GRAYSCALE:
      if (_graya_geta(c) > 0) {
	color = Color::fromGray(_graya_getv(c));
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
  if ((x >= 0) && (y >= 0) && (x < image->w) && (y < image->h))
    return Color::fromImage(image->imgtype, image_getpixel(image, x, y));
  else
    return Color::fromMask();
}

// static
Color Color::fromString(const std::string& str)
{
  Color color = Color::fromMask();

  if (str != "mask") {
    if (str.find("rgb{") == 0 ||
	str.find("hsv{") == 0) {
      int c = 0, table[3] = { 0, 0, 0 };
      int i = 4, j;
      
      while ((j = str.find_first_of(",}", i)) != std::string::npos) {
	std::string element = str.substr(i, j - i);
	if (c < 3)
	  table[c++] = std::strtol(element.c_str(), NULL, 10);
	if (c >= 3)
	  break;
	i = j+1;
      }
      
      if (str[0] == 'r')
	color = Color::fromRgb(table[0], table[1], table[2]);
      else
	color = Color::fromHsv(table[0], table[1], table[2]);
    } 
    else if (str.find("gray{") == 0) {
      color = Color::fromGray(std::strtol(str.c_str()+5, NULL, 10));
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
	     << m_value.rgb.b << "}";
      break;

    case Color::HsvType:
      result << "hsv{"
	     << m_value.hsv.h << ","
	     << m_value.hsv.s << ","
	     << m_value.hsv.v << "}";
      break;

    case Color::GrayType:
      result << "gray{" << m_value.gray << "}";
      break;

    case Color::IndexType:
      result << "index{" << m_value.index << "}";
      break;

  }

  return result.str();
}

std::string Color::toFormalString(int imgtype, bool long_format) const
{
  std::stringstream result;

  // Long format
  if (long_format) {
    switch (getType()) {

      case Color::MaskType:
	result << "Mask";
	break;

      case Color::RgbType:
	if (imgtype == IMAGE_GRAYSCALE) {
	  result << "Gray " << getGray();
	}
	else {
	  result << "RGB "
		 << m_value.rgb.r << " "
		 << m_value.rgb.g << " "
		 << m_value.rgb.b;

	  if (imgtype == IMAGE_INDEXED)
	    result << " Index "
		   << color_utils::color_for_image(*this, imgtype);
	}
	break;

      case Color::HsvType:
	if (imgtype == IMAGE_GRAYSCALE) {
	  result << "Gray " << getGray();
	}
	else {
	  result << "HSB "
		 << m_value.hsv.h << "\xB0 "
		 << m_value.hsv.s << " "
		 << m_value.hsv.v;
  
	  if (imgtype == IMAGE_INDEXED)
	    result << " Index " << color_utils::color_for_image(*this, imgtype);
	}
	break;
	
      case Color::GrayType:
	result << "Gray " << m_value.gray;
	break;

      case Color::IndexType: {
	int i = m_value.index;
	if (i >= 0 && i < (int)get_current_palette()->size()) {
	  uint32_t _c = get_current_palette()->getEntry(i);
	  result << "Index " << i
		 << " (RGB "
		 << (int)_rgba_getr(_c) << " "
		 << (int)_rgba_getg(_c) << " "
		 << (int)_rgba_getb(_c) << ")";
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
  }
  // Short format
  else {
    switch (getType()) {

      case Color::MaskType:
	result << "Mask";
	break;

      case Color::RgbType:
	if (imgtype == IMAGE_GRAYSCALE) {
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
	if (imgtype == IMAGE_GRAYSCALE) {
	  result << "Gry-" << getGray();
	}
	else {
	  result << m_value.hsv.h << "\xB0"
		 << m_value.hsv.s << ","
		 << m_value.hsv.v;
	}
	break;

      case Color::GrayType:
	result << "Gry-" << m_value.gray;
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
	m_value.rgb.b == other.m_value.rgb.b;

    case Color::HsvType:
      return
	m_value.hsv.h == other.m_value.hsv.h &&
	m_value.hsv.s == other.m_value.hsv.s &&
	m_value.hsv.v == other.m_value.hsv.v;

    case Color::GrayType:
      return m_value.gray == other.m_value.gray;

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
      return m_value.gray;

    case Color::IndexType: {
      int i = m_value.index;
      ASSERT(i >= 0 && i < get_current_palette()->size());

      return _rgba_getr(get_current_palette()->getEntry(i));
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
      return m_value.gray;

    case Color::IndexType: {
      int i = m_value.index;
      ASSERT(i >= 0 && i < get_current_palette()->size());

      return _rgba_getg(get_current_palette()->getEntry(i));
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
      return m_value.gray;

    case Color::IndexType: {
      int i = m_value.index;
      ASSERT(i >= 0 && i < get_current_palette()->size());

      return _rgba_getb(get_current_palette()->getEntry(i));
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
      ASSERT(i >= 0 && i < get_current_palette()->size());

      uint32_t c = get_current_palette()->getEntry(i);

      return Hsv(Rgb(_rgba_getr(c),
		     _rgba_getg(c),
		     _rgba_getb(c))).hueInt();
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
      ASSERT(i >= 0 && i < get_current_palette()->size());

      uint32_t c = get_current_palette()->getEntry(i);

      return Hsv(Rgb(_rgba_getr(c),
		     _rgba_getg(c),
		     _rgba_getb(c))).saturationInt();
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
      return 100 * m_value.gray / 255;

    case Color::IndexType: {
      int i = m_value.index;
      ASSERT(i >= 0 && i < get_current_palette()->size());

      uint32_t c = get_current_palette()->getEntry(i);

      return Hsv(Rgb(_rgba_getr(c),
		     _rgba_getg(c),
		     _rgba_getb(c))).valueInt();
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
      return m_value.gray;

    case Color::IndexType: {
      int i = m_value.index;
      ASSERT(i >= 0 && i < get_current_palette()->size());

      uint32_t c = get_current_palette()->getEntry(i);

      return 255 * Hsv(Rgb(_rgba_getr(c),
			   _rgba_getg(c),
			   _rgba_getb(c))).valueInt() / 100;
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
      PRINTF("Getting `index' from a RGB color\n"); // TODO
      ASSERT(false);
      break;

    case Color::HsvType:
      PRINTF("Getting `index' from a HSV color\n"); // TODO
      ASSERT(false);
      break;

    case Color::GrayType:
      return m_value.gray;

    case Color::IndexType:
      return m_value.index;

  }

  ASSERT(false);
  return -1;
}
