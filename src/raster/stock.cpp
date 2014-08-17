/* Aseprite
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "raster/stock.h"

#include "raster/image.h"
#include "raster/sprite.h"

#include <cstring>

namespace raster {

Stock::Stock(Sprite* sprite, PixelFormat format)
  : Object(OBJECT_STOCK)
  , m_format(format)
  , m_sprite(sprite)
{
  // Image with index=0 is always NULL.
  m_image.push_back(NULL);
}

Stock::~Stock()
{
  for (int i=0; i<size(); ++i) {
    if (getImage(i))
      delete getImage(i);
  }
}

Sprite* Stock::sprite() const
{
  return m_sprite;
}

Image* Stock::getImage(int index) const
{
  ASSERT((index >= 0) && (index < size()));

  return m_image[index];
}

int Stock::addImage(Image* image)
{
  int i = m_image.size();
  try {
    m_image.resize(m_image.size()+1);
  }
  catch (...) {
    delete image;
    throw;
  }
  m_image[i] = image;

  fixupImage(image);
  return i;
}

void Stock::removeImage(Image* image)
{
  for (int i=0; i<size(); i++)
    if (m_image[i] == image) {
      m_image[i] = NULL;
      return;
    }

  ASSERT(false && "The specified image was not found");
}

void Stock::replaceImage(int index, Image* image)
{
  ASSERT((index > 0) && (index < size()));
  m_image[index] = image;

  fixupImage(image);
}

void Stock::fixupImage(Image* image)
{
  // Change the mask color of the added image to the sprite mask color.
  if (image)
    image->setMaskColor(m_sprite->transparentColor());
}

} // namespace raster
