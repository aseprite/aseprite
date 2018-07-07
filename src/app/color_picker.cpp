// Aseprite
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
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "gfx/point.h"
#include "render/get_sprite_pixel.h"

namespace app {

namespace {

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

  pos.x = (pos.x-celBounds.x)*image->width()/celBounds.w;
  pos.y = (pos.y-celBounds.y)*image->height()/celBounds.h;
  const gfx::Point ipos(pos);
  if (!image->bounds().contains(ipos))
    return false;

  output = get_pixel(image, ipos.x, ipos.y);
  return true;
}

}

ColorPicker::ColorPicker()
  : m_alpha(0)
  , m_layer(NULL)
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

    pos = wrap_pointF(docPref.tiled.mode(),
                      site.sprite()->size(), pos);
  }

  // Get the color from the image
  switch (mode) {

    // Pick from the composed image
    case FromComposition: {
      m_color = app::Color::fromImage(
        sprite->pixelFormat(),
        render::get_sprite_pixel(sprite, pos.x, pos.y,
                                 site.frame(), proj));

      doc::CelList cels;
      sprite->pickCels(pos.x, pos.y, site.frame(), 128,
                       sprite->allVisibleLayers(), cels);
      if (!cels.empty())
        m_layer = cels.front()->layer();
      break;
    }

    // Pick from the current layer
    case FromActiveLayer: {
      const Cel* cel = site.cel();
      if (cel) {
        doc::color_t imageColor;
        if (!get_cel_pixel(cel, pos.x, pos.y,
                           site.frame(), imageColor))
          return;

        const doc::Image* image = cel->image();
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
      break;
    }

    case FromFirstReferenceLayer: {
      doc::CelList cels;
      sprite->pickCels(pos.x, pos.y, site.frame(), 128,
                       sprite->allVisibleReferenceLayers(), cels);

      for (const Cel* cel : cels) {
        doc::color_t imageColor;
        if (get_cel_pixel(cel, pos.x, pos.y,
                          site.frame(), imageColor)) {
          m_color = app::Color::fromImage(
            cel->image()->pixelFormat(), imageColor);
          m_layer = cel->layer();
          break;
        }
      }
      break;
    }
  }
}

} // namespace app
