// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/color_picker.h"

#include "app/document.h"
#include "app/pref/preferences.h"
#include "app/util/wrap_value.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/primitives.h"
#include "doc/site.h"
#include "doc/sprite.h"
#include "gfx/point.h"
#include "render/get_sprite_pixel.h"

namespace app {

ColorPicker::ColorPicker()
  : m_alpha(0)
  , m_layer(NULL)
{
}

void ColorPicker::pickColor(const doc::Site& site,
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
    const app::Document* doc = static_cast<const app::Document*>(site.document());
    DocumentPreferences& docPref = Preferences::instance().document(doc);

    if (int(docPref.tiled.mode()) & int(filters::TiledMode::X_AXIS))
      pos.x = wrap_value<double>(pos.x, site.sprite()->width());

    if (int(docPref.tiled.mode()) & int(filters::TiledMode::Y_AXIS))
      pos.y = wrap_value<double>(pos.y, site.sprite()->height());
  }

  // Get the color from the image
  if (mode == FromComposition) { // Pick from the composed image
    m_color = app::Color::fromImage(
      sprite->pixelFormat(),
      render::get_sprite_pixel(sprite, pos.x, pos.y,
                               site.frame(), proj));

    doc::CelList cels;
    sprite->pickCels(pos.x, pos.y, site.frame(), 128, cels);
    if (!cels.empty())
      m_layer = cels.front()->layer();
  }
  else {                        // Pick from the current layer
    const Cel* cel = site.cel();
    if (cel) {
      gfx::RectF celBounds;
      if (cel->layer()->isReference())
        celBounds = cel->boundsF();
      else
        celBounds = cel->bounds();

      const doc::Image* image = cel->image();

      if (!celBounds.contains(pos))
        return;

      pos.x = (pos.x-celBounds.x)*image->width()/celBounds.w;
      pos.y = (pos.y-celBounds.y)*image->height()/celBounds.h;
      const gfx::Point ipos(pos);
      if (!image->bounds().contains(ipos))
        return;

      const doc::color_t imageColor =
        get_pixel(image, ipos.x, ipos.y);

      switch (image->pixelFormat()) {
        case IMAGE_RGB:
          m_alpha = doc::rgba_geta(imageColor);
          break;
        case IMAGE_GRAYSCALE:
          m_alpha = doc::graya_geta(imageColor);
          break;
      }

      m_color = app::Color::fromImage(image->pixelFormat(), imageColor);
      m_layer = const_cast<Layer*>(site.layer());
    }
  }
}

} // namespace app
