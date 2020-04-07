// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/site.h"

#include "app/pref/preferences.h"
#include "base/clamp.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "ui/system.h"

namespace app {

using namespace doc;

Palette* Site::palette()
{
  return (m_sprite ? m_sprite->palette(m_frame): nullptr);
}

RgbMap* Site::rgbMap() const
{
  return (m_sprite ? m_sprite->rgbMap(m_frame): nullptr);
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
      if (opacity) *opacity = base::clamp(cel->opacity(), 0, 255);
    }
  }

  return image;
}

Palette* Site::palette() const
{
  return (m_sprite ? m_sprite->palette(m_frame): nullptr);
}

void Site::range(const DocRange& range)
{
  m_range = range;
  switch (range.type()) {
    case DocRange::kCels:   m_focus = Site::InCels; break;
    case DocRange::kFrames: m_focus = Site::InFrames; break;
    case DocRange::kLayers: m_focus = Site::InLayers; break;
  }
}

gfx::Rect Site::gridBounds() const
{
  gfx::Rect bounds;
  if (m_sprite) {
    bounds = m_sprite->gridBounds();
    if (!bounds.isEmpty())
      return bounds;
  }
  if (ui::is_ui_thread()) {
    bounds = Preferences::instance().document(m_document).grid.bounds();
    if (!bounds.isEmpty())
      return bounds;
  }
  return doc::Sprite::DefaultGridBounds();
}

} // namespace app
