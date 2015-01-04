// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/cel.h"

#include "gfx/rect.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace doc {

Cel::Cel(frame_t frame, const ImageRef& image)
  : Object(ObjectType::Cel)
  , m_layer(NULL)
  , m_frame(frame)
  , m_image(image)
  , m_position(0, 0)
  , m_opacity(255)
{
}

Cel::Cel(const Cel& cel)
  : Object(cel)
  , m_layer(NULL)
  , m_frame(cel.m_frame)
  , m_image(cel.m_image)
  , m_position(cel.m_position)
  , m_opacity(cel.m_opacity)
{
}

Cel::~Cel()
{
}

void Cel::setFrame(frame_t frame)
{
  ASSERT(m_layer == NULL);
  m_frame = frame;
}

void Cel::setImage(const ImageRef& image)
{
  m_image = image;
  fixupImage();
}

Sprite* Cel::sprite() const
{
  ASSERT(m_layer);
  if (m_layer)
    return m_layer->sprite();
  else
    return NULL;
}

Cel* Cel::link() const
{
  if (m_image.get() == NULL)
    return NULL;

  if (!m_image.unique()) {
    for (frame_t fr=0; fr<m_frame; ++fr) {
      Cel* possible = m_layer->cel(fr);
      if (possible && possible->imageRef().get() == m_image.get())
        return possible;
    }
  }

  return NULL;
}

gfx::Rect Cel::bounds() const
{
  Image* image = this->image();
  ASSERT(image);
  if (image)
    return gfx::Rect(
      m_position.x, m_position.y,
      image->width(), image->height());
  else
    return gfx::Rect();
}

void Cel::setParentLayer(LayerImage* layer)
{
  m_layer = layer;
  fixupImage();
}

void Cel::fixupImage()
{
  // Change the mask color to the sprite mask color
  if (m_layer && m_image.get())
    m_image->setMaskColor(m_layer->sprite()->transparentColor());
}

} // namespace doc
