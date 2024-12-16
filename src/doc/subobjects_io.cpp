// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/subobjects_io.h"

#include "doc/cel.h"
#include "doc/cel_io.h"
#include "doc/image.h"
#include "doc/image_io.h"
#include "doc/layer.h"
#include "doc/layer_io.h"
#include "doc/sprite.h"

namespace doc {

using namespace doc;

SubObjectsFromSprite::SubObjectsFromSprite(Sprite* sprite) : m_sprite(sprite)
{
}

void SubObjectsFromSprite::addImageRef(const ImageRef& image)
{
  ASSERT(image);
  ASSERT(!getImageRef(image->id()));
  m_images.insert(std::make_pair(image->id(), image));
}

ImageRef SubObjectsFromSprite::getImageRef(ObjectId imageId)
{
  auto it = m_images.find(imageId);
  if (it != m_images.end()) {
    ImageRef image = it->second;
    ASSERT(image->id() == imageId);
    ASSERT(!m_sprite->getImageRef(imageId));
    return image;
  }
  else
    return m_sprite->getImageRef(imageId);
}

void SubObjectsFromSprite::addCelDataRef(const CelDataRef& celdata)
{
  ASSERT(celdata);
  ASSERT(!getCelDataRef(celdata->id()));
  m_celdatas.insert(std::make_pair(celdata->id(), celdata));
}

CelDataRef SubObjectsFromSprite::getCelDataRef(ObjectId celdataId)
{
  auto it = m_celdatas.find(celdataId);
  if (it != m_celdatas.end()) {
    CelDataRef celdata = it->second;
    ASSERT(celdata->id() == celdataId);
    ASSERT(!m_sprite->getCelDataRef(celdataId));
    return celdata;
  }
  else
    return m_sprite->getCelDataRef(celdataId);
}

} // namespace doc
