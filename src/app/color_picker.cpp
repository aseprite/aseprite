// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/color_picker.h"

#include "app/doc.h"
#include "app/pref/preferences.h"
#include "app/site.h"
#include "app/util/wrap_point.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer_tilemap.h"
#include "doc/primitives.h"
#include "doc/render_plan.h"
#include "doc/sprite.h"
#include "doc/tile.h"
#include "doc/tile_primitives.h"
#include "doc/tileset.h"
#include "gfx/point.h"
#include "render/get_sprite_pixel.h"

#include <algorithm>

namespace app {

namespace {

const int kOpacityThreshold = 1;

bool get_cel_pixel(const Cel* cel,
                   const double x,
                   const double y,
                   const frame_t frame,
                   color_t& output)
{
  gfx::RectF celBounds;
  if (cel->layer()->isReference())
    celBounds = cel->boundsF();
  else
    celBounds = cel->bounds();

  const doc::Image* image = cel->image();
  gfx::PointF pos(x, y);
  if (!celBounds.contains(pos))
    return false;

  // For tilemaps
  if (image->pixelFormat() == IMAGE_TILEMAP) {
    ASSERT(cel->layer()->isTilemap());

    doc::tile_index ti;
    doc::tile_flags tf;
    return get_tile_pixel(cel, pos, ti, tf, output);
  }
  // Regular images
  else {
    pos.x = (pos.x - celBounds.x) * image->width() / celBounds.w;
    pos.y = (pos.y - celBounds.y) * image->height() / celBounds.h;
    const gfx::Point ipos(pos);
    if (!image->bounds().contains(ipos))
      return false;

    output = get_pixel(image, ipos.x, ipos.y);
    return true;
  }
}

} // namespace

ColorPicker::ColorPicker() : m_tile(doc::notile), m_alpha(0), m_layer(nullptr)
{
}

void ColorPicker::pickColor(const Site& site,
                            const gfx::PointF& _pos,
                            const render::Projection& proj,
                            const Mode mode)
{
  const doc::Sprite* sprite = site.sprite();
  gfx::PointF pos = _pos;

  m_alpha = 255;
  m_color = app::Color::fromMask();

  // Check tiled mode
  if (sprite && site.document()) {
    auto doc = static_cast<const Doc*>(site.document());
    DocumentPreferences& docPref = Preferences::instance().document(doc);

    pos = wrap_pointF(docPref.tiled.mode(), site.sprite()->size(), pos);
  }

  // Get the color from the image
  switch (mode) {
    // Pick from the composed image
    case FromComposition: {
      doc::RenderPlan plan;
      plan.addLayer(sprite->root(), site.frame());

      doc::CelList cels;
      sprite->pickCels(pos, kOpacityThreshold, plan, cels);
      if (!cels.empty())
        m_layer = cels.front()->layer();

      if (site.tilemapMode() == TilemapMode::Tiles) {
        if (cels.empty())
          return;

        const Cel* cel = cels.front();
        if (!cel->image()->isTilemap())
          return;

        doc::tile_index ti;
        doc::tile_flags tf;
        doc::color_t pixelColor;
        if (get_tile_pixel(cel, pos, ti, tf, pixelColor)) {
          m_tile = doc::tile(ti, tf);
          m_color = app::Color::fromTile(m_tile);
        }
      }
      else if (site.tilemapMode() == TilemapMode::Pixels) {
        m_color = app::Color::fromImage(
          sprite->pixelFormat(),
          render::get_sprite_pixel(sprite,
                                   pos.x,
                                   pos.y,
                                   site.frame(),
                                   proj,
                                   Preferences::instance().experimental.newBlend()));
      }
      break;
    }

    // Pick from the current layer
    case FromActiveLayer: {
      const Cel* cel = site.cel();
      if (!cel)
        return;

      if (site.tilemapMode() == TilemapMode::Tiles) {
        doc::tile_index ti;
        doc::tile_flags tf;
        doc::color_t pixelColor;
        if (cel->layer()->isTilemap() && get_tile_pixel(cel, pos, ti, tf, pixelColor)) {
          m_tile = doc::tile(ti, tf);
          m_color = app::Color::fromTile(m_tile);
        }
      }
      else if (site.tilemapMode() == TilemapMode::Pixels) {
        doc::color_t imageColor;
        if (!get_cel_pixel(cel, pos.x, pos.y, site.frame(), imageColor))
          return;

        doc::PixelFormat pixelFormat = (cel->layer()->isTilemap() ? sprite->pixelFormat() :
                                                                    cel->image()->pixelFormat());
        switch (pixelFormat) {
          case IMAGE_RGB:       m_alpha = doc::rgba_geta(imageColor); break;
          case IMAGE_GRAYSCALE: m_alpha = doc::graya_geta(imageColor); break;
        }

        m_color = app::Color::fromImage(pixelFormat, imageColor);
        m_layer = const_cast<Layer*>(site.layer());
      }
      break;
    }

    case FromFirstReferenceLayer: {
      doc::RenderPlan plan;
      for (doc::Layer* refLayer : sprite->allVisibleReferenceLayers())
        plan.addLayer(refLayer, site.frame());

      doc::CelList cels;
      sprite->pickCels(pos, kOpacityThreshold, plan, cels);

      for (const Cel* cel : cels) {
        doc::color_t imageColor;
        if (get_cel_pixel(cel, pos.x, pos.y, site.frame(), imageColor)) {
          m_color = app::Color::fromImage(cel->image()->pixelFormat(), imageColor);
          m_layer = cel->layer();
          break;
        }
      }
      break;
    }
  }
}

} // namespace app
