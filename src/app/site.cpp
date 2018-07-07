// Aseprite
// Copyright (c) 2001-2018 David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/site.h"

#include "base/base.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {

using namespace doc;

Palette* Site::palette()
{
  return (m_sprite ? m_sprite->palette(m_frame): nullptr);
}

const Cel* Site::cel() const
{
  if (m_layer)
    return m_layer->cel(m_frame);
  else
    return nullptr;
}

Cel* Site::cel()
{
  if (m_layer)
    return m_layer->cel(m_frame);
  else
    return nullptr;
}

Image* Site::image(int* x, int* y, int* opacity) const
{
  Image* image = nullptr;

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
  return (m_sprite ? m_sprite->palette(m_frame): nullptr);
}

} // namespace app
