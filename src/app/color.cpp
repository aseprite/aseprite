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

#include "config.h"

#include <string>
#include <cstdlib>
#include <sstream>
#include <iomanip>

#include "Vaca/String.h"

#include "app/color.h"
#include "app/color_utils.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "modules/palettes.h"

#define MAKE_DATA(c1,c2,c3)	((c3) << 16) | ((c2) << 8) | (c1)
#define GET_DATA_C1(c)		(((c) >> 0) & 0xff)
#define GET_DATA_C2(c)		(((c) >> 8) & 0xff)
#define GET_DATA_C3(c)		(((c) >> 16) & 0xff)

// static
Color Color::fromMask()
{
  return Color(Color::MaskType, 0);
}

// static
Color Color::fromRgb(int r, int g, int b)
{
  return Color(Color::RgbType, MAKE_DATA(r & 0xff, g & 0xff, b & 0xff));
}

// static
Color Color::fromHsv(int h, int s, int v)
{
  return Color(Color::HsvType, MAKE_DATA(h & 0xff, s & 0xff, v & 0xff));
}

// static
Color Color::fromGray(int g)
{
  return Color(Color::GrayType, g & 0xff);
}

// static
Color Color::fromIndex(int index)
{
  return Color(Color::IndexType, index & 0xff);
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
  int data;

  switch (getType()) {

    case Color::MaskType:
      result << "mask";
      break;

    case Color::RgbType:
      data = getRgbData();

      result << "rgb{"
	     << GET_DATA_C1(data) << ","
	     << GET_DATA_C2(data) << ","
	     << GET_DATA_C3(data) << "}";
      break;

    case Color::HsvType:
      data = getHsvData();

      result << "hsv{"
	     << GET_DATA_C1(data) << ","
	     << GET_DATA_C2(data) << ","
	     << GET_DATA_C3(data) << "}";
      break;

    case Color::GrayType:
      data = getGrayData();

      result << "gray{" << data << "}";
      break;

    case Color::IndexType:
      data = getIndexData();

      result << "index{" << data << "}";
      break;

  }

  return result.str();
}

std::string Color::toFormalString(int imgtype, bool long_format) const
{
  std::stringstream result;
  int data;

  // Long format
  if (long_format) {
    switch (getType()) {

      case Color::MaskType:
	result << "Mask";
	break;

      case Color::RgbType:
	data = getRgbData();
	if (imgtype == IMAGE_GRAYSCALE) {
	  result << "Gray "
		 << _graya_getv(color_utils::color_for_image(*this, imgtype));
	}
	else {
	  result << "RGB "
		 << GET_DATA_C1(data) << " "
		 << GET_DATA_C2(data) << " "
		 << GET_DATA_C3(data);

	  if (imgtype == IMAGE_INDEXED)
	    result << " Index "
		   << color_utils::color_for_image(*this, imgtype);
	}
	break;

      case Color::HsvType:
	data = getHsvData();
	if (imgtype == IMAGE_GRAYSCALE) {
	  result << "Gray " << GET_DATA_C3(data);
	}
	else {
	  result << "HSV "
		 << GET_DATA_C1(data) << " "
		 << GET_DATA_C2(data) << " "
		 << GET_DATA_C3(data);
  
	  if (imgtype == IMAGE_INDEXED)
	    result << " Index " << color_utils::color_for_image(*this, imgtype);
	}
	break;
	
      case Color::GrayType:
	data = getGrayData();
	result << "Gray " << data;
	break;

      case Color::IndexType:
	data = getIndexData();
	if (data >= 0 && data < (int)get_current_palette()->size()) {
	  ase_uint32 _c = get_current_palette()->getEntry(data);
	  result << "Index " << data
		 << " (RGB "
		 << (int)_rgba_getr(_c) << " "
		 << (int)_rgba_getg(_c) << " "
		 << (int)_rgba_getb(_c) << ")";
	}
	else {
	  result << "Index "
		 << data
		 << " (out of range)";
	}
	break;

      default:
	ASSERT(false);
	break;
    }
  }
  // Short format
  else {
    switch (getType()) {

      case Color::MaskType:
	result << "MASK";
	break;

      case Color::RgbType:
	data = getRgbData();
	if (imgtype == IMAGE_GRAYSCALE) {
	  result << "V " << _graya_getv(color_utils::color_for_image(*this, imgtype));
	}
	else {
	  result << "RGB " << std::hex << std::setfill('0')
		 << std::setw(2) << GET_DATA_C1(data)
		 << std::setw(2) << GET_DATA_C2(data)
		 << std::setw(2) << GET_DATA_C3(data);

	  if (imgtype == IMAGE_INDEXED) {
	    result << " (" << std::dec
		   << color_utils::color_for_image(*this, imgtype) << ")";
	  }
	}
	break;

      case Color::HsvType:
	data = getHsvData();
	if (imgtype == IMAGE_GRAYSCALE) {
	  result << "V " << GET_DATA_C3(data);
	}
	else {
	  result << "HSV " << std::hex << std::setfill('0')
		 << std::setw(2) << GET_DATA_C1(data)
		 << std::setw(2) << GET_DATA_C2(data)
		 << std::setw(2) << GET_DATA_C3(data);

	  if (imgtype == IMAGE_INDEXED) {
	    result << " (" << std::dec
		   << color_utils::color_for_image(*this, imgtype) << ")";
	  }
	}
	break;

      case Color::GrayType:
	data = getGrayData();
	result << "V " << data;
	break;

      case Color::IndexType:
	data = getIndexData();
	result << "I " << data;
	break;

      default:
	ASSERT(false);
	break;
    }
  }

  return result.str();
}

// Returns false only if the color is a index and it is outside the
// valid range (outside the maximum number of colors in the current
// palette)
bool Color::isValid() const
{
  switch (getType()) {

    case Color::IndexType: {
      int i = getIndexData();
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
      return GET_DATA_C1(getRgbData());

    case Color::HsvType: {
      int c = getHsvData();
      int h = GET_DATA_C1(c);
      int s = GET_DATA_C2(c);
      int v = GET_DATA_C3(c);
      hsv_to_rgb_int(&h, &s, &v);
      return h;
    }
      
    case Color::GrayType:
      return getGrayData();

    case Color::IndexType: {
      int i = getIndexData();
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
      return GET_DATA_C2(getRgbData());

    case Color::HsvType: {
      int c = getHsvData();
      int h = GET_DATA_C1(c);
      int s = GET_DATA_C2(c);
      int v = GET_DATA_C3(c);
      hsv_to_rgb_int(&h, &s, &v);
      return s;
    }
      
    case Color::GrayType:
      return getGrayData();

    case Color::IndexType: {
      int i = getIndexData();
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
      return GET_DATA_C3(getRgbData());

    case Color::HsvType: {
      int c = getHsvData();
      int h = GET_DATA_C1(c);
      int s = GET_DATA_C2(c);
      int v = GET_DATA_C3(c);
      hsv_to_rgb_int(&h, &s, &v);
      return v;
    }
      
    case Color::GrayType:
      return getGrayData();

    case Color::IndexType: {
      int i = getIndexData();
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

    case Color::RgbType: {
      int c = getRgbData();
      int r = GET_DATA_C1(c);
      int g = GET_DATA_C2(c);
      int b = GET_DATA_C3(c);
      rgb_to_hsv_int(&r, &g, &b);
      return r;
    }

    case Color::HsvType:
      return GET_DATA_C1(getHsvData());

    case Color::GrayType:
      return 0;

    case Color::IndexType: {
      int i = getIndexData();
      ASSERT(i >= 0 && i < get_current_palette()->size());

      ase_uint32 c = get_current_palette()->getEntry(i);
      int r = _rgba_getr(c);
      int g = _rgba_getg(c);
      int b = _rgba_getb(c);
      rgb_to_hsv_int(&r, &g, &b);
      return r;
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

    case Color::RgbType: {
      int c = getRgbData();
      int r = GET_DATA_C1(c);
      int g = GET_DATA_C2(c);
      int b = GET_DATA_C3(c);
      rgb_to_hsv_int(&r, &g, &b);
      return g;
    }

    case Color::HsvType:
      return GET_DATA_C2(getHsvData());

    case Color::GrayType:
      return 0;

    case Color::IndexType: {
      int i = getIndexData();
      ASSERT(i >= 0 && i < get_current_palette()->size());

      ase_uint32 c = get_current_palette()->getEntry(i);
      int r = _rgba_getr(c);
      int g = _rgba_getg(c);
      int b = _rgba_getb(c);
      rgb_to_hsv_int(&r, &g, &b);
      return g;
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

    case Color::RgbType: {
      int c = getRgbData();
      int r = GET_DATA_C1(c);
      int g = GET_DATA_C2(c);
      int b = GET_DATA_C3(c);
      rgb_to_hsv_int(&r, &g, &b);
      return b;
    }

    case Color::HsvType:
      return GET_DATA_C3(getHsvData());
      
    case Color::GrayType:
      return getGrayData();

    case Color::IndexType: {
      int i = getIndexData();
      ASSERT(i >= 0 && i < get_current_palette()->size());

      ase_uint32 c = get_current_palette()->getEntry(i);
      int r = _rgba_getr(c);
      int g = _rgba_getg(c);
      int b = _rgba_getb(c);
      rgb_to_hsv_int(&r, &g, &b);
      return b;
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
      return getGrayData();

    case Color::IndexType:
      return getIndexData();

  }

  ASSERT(false);
  return -1;
}
