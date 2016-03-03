// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/site.h"

#include "base/base.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace doc {

LayerIndex Site::layerIndex() const
{
  return (m_sprite && m_layer ?
          m_sprite->layerToIndex(m_layer): LayerIndex());
}

void Site::layerIndex(LayerIndex layerIndex)
{
  ASSERT(m_sprite != NULL);
  m_layer = m_sprite->indexToLayer(layerIndex);
}

Palette* Site::palette()
{
  return (m_sprite ? m_sprite->palette(m_frame): NULL);
}

const Cel* Site::cel() const
{
  if (m_layer)
    return m_layer->cel(m_frame);
  else
    return NULL;
}

Cel* Site::cel()
{
  if (m_layer)
    return m_layer->cel(m_frame);
  else
    return NULL;
}

Image* Site::image(int* x, int* y, int* opacity) const
{
  Image* image = NULL;

  if (m_sprite) {
    if (const Cel* cel = this->cel()) {
      image = cel->image();
      if (x) *x = cel->x();
      if (y) *y = cel->y();
      if (opacity) *opacity = MID(0, cel->opacity(), 255);
    }
  }

  return image;
}

Palette* Site::palette() const
{
  return (m_sprite ? m_sprite->palette(m_frame): NULL);
}

} // namespace doc
