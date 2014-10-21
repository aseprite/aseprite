// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/stock.h"

#include "doc/image.h"
#include "doc/sprite.h"

#include <cstring>

namespace doc {

Stock::Stock(Sprite* sprite, PixelFormat format)
  : Object(ObjectType::Stock)
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

} // namespace doc
