// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/cel_data.h"

#include "gfx/rect.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace doc {

CelData::CelData(const ImageRef& image)
  : WithUserData(ObjectType::CelData)
  , m_image(image)
  , m_position(0, 0)
  , m_opacity(255)
{
}

CelData::CelData(const CelData& celData)
  : WithUserData(ObjectType::CelData)
  , m_image(celData.m_image)
  , m_position(celData.m_position)
  , m_opacity(celData.m_opacity)
{
}

void CelData::setImage(const ImageRef& image)
{
  ASSERT(image.get());

  m_image = image;
}

} // namespace doc
