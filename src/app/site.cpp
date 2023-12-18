// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/site.h"

#include "app/pref/preferences.h"
#include "doc/cel.h"
#include "doc/grid.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/sprite.h"
#include "doc/tileset.h"
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

Cel* Site::cel() const
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
      if (opacity) *opacity = std::clamp(cel->opacity(), 0, 255);
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

doc::Tileset* Site::tileset() const
{
  if (m_layer && m_layer->isTilemap())
    return static_cast<LayerTilemap*>(m_layer)->tileset();
  else
    return nullptr;
}

Grid Site::grid() const
{
  if (m_layer && m_layer->isTilemap()) {
    doc::Grid grid = static_cast<LayerTilemap*>(m_layer)->tileset()->grid();
    if (const Cel* cel = m_layer->cel(m_frame))
      grid.origin(grid.origin() + cel->position());
    return grid;
  }

  gfx::Rect rc = gridBounds();
  doc::Grid grid = Grid(rc.size());
  grid.origin(gfx::Point(rc.x % rc.w, rc.y % rc.h));
  return grid;
}

gfx::Rect Site::gridBounds() const
{
  gfx::Rect bounds;
  if (m_layer && m_layer->isTilemap()) {
    const Grid& grid = static_cast<LayerTilemap*>(m_layer)->tileset()->grid();
    gfx::Point offset = grid.tileOffset();
    if (const Cel* cel = m_layer->cel(m_frame))
      offset += cel->bounds().origin();
    bounds = gfx::Rect(offset, grid.tileSize());
    if (!bounds.isEmpty())
      return bounds;
  }
  else {
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
  }

  return doc::Sprite::DefaultGridBounds();
}

bool Site::shouldTrimCel(Cel* cel) const
{
  return (cel &&
          cel->layer() &&
          cel->layer()->isTransparent() &&
          // Don't trim tiles in manual mode
          !(m_tilemapMode == TilemapMode::Pixels &&
            m_tilesetMode == TilesetMode::Manual &&
            cel->layer()->isTilemap()));
}

} // namespace app
