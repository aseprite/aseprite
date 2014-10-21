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
#include "doc/stock.h"

namespace doc {

Cel::Cel(FrameNumber frame, int image)
  : Object(ObjectType::Cel)
  , m_layer(NULL)
  , m_frame(frame)
  , m_image(image)
{
  m_x = 0;
  m_y = 0;
  m_opacity = 255;
}

Cel::Cel(const Cel& cel)
  : Object(cel)
  , m_layer(NULL)
  , m_frame(cel.m_frame)
  , m_image(cel.m_image)
{
  m_x = cel.m_x;
  m_y = cel.m_y;
  m_opacity = cel.m_opacity;
}

Cel::~Cel()
{
}

Image* Cel::image() const
{
  ASSERT(m_layer != NULL);
  ASSERT(m_image >= 0);
  ASSERT(m_image < m_layer->sprite()->stock()->size());

  if (m_layer) {
    Stock* stock = m_layer->sprite()->stock();

    ASSERT(stock);
    ASSERT(m_image >= 0 && m_image < stock->size());

    if (m_image >= 0 && m_image < stock->size()) {
      Image* image = stock->getImage(m_image);
      ASSERT((m_image == 0 && !image) || (m_image != 0 && image));
      return image;
    }
  }

  return NULL;
}

Sprite* Cel::sprite() const
{
  ASSERT(m_layer);
  if (m_layer)
    return m_layer->sprite();
  else
    return NULL;
}

gfx::Rect Cel::bounds() const
{
  Image* image = this->image();
  ASSERT(image);
  if (image)
    return gfx::Rect(m_x, m_y, image->width(), image->height());
  else
    return gfx::Rect();
}

} // namespace doc
