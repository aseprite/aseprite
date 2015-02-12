// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/color_picker.h"

#include "app/document_location.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "gfx/point.h"
#include "render/get_sprite_pixel.h"

namespace app {

ColorPicker::ColorPicker()
  : m_alpha(0)
  , m_layer(NULL)
{
}

void ColorPicker::pickColor(const DocumentLocation& location,
  const gfx::Point& pos, Mode mode)
{
  m_alpha = 255;
  m_color = app::Color::fromMask();

  // Get the color from the image
  if (mode == FromComposition) { // Pick from the composed image
    m_color = app::Color::fromImage(
      location.sprite()->pixelFormat(),
      render::get_sprite_pixel(location.sprite(), pos.x, pos.y, location.frame()));

    doc::CelList cels;
    location.sprite()->pickCels(pos.x, pos.y, location.frame(), 128, cels);
    if (!cels.empty())
      m_layer = cels.front()->layer();
  }
  else {                        // Pick from the current layer
    int u, v;
    doc::Image* image = location.image(&u, &v, NULL);
    gfx::Point pt(pos.x-u, pos.y-v);

    if (image && image->bounds().contains(pt)) {
      doc::color_t imageColor = get_pixel(image, pt.x, pt.y);

      switch (image->pixelFormat()) {
        case IMAGE_RGB:
          m_alpha = doc::rgba_geta(imageColor);
          break;
        case IMAGE_GRAYSCALE:
          m_alpha = doc::graya_geta(imageColor);
          break;
      }

      m_color = app::Color::fromImage(image->pixelFormat(), imageColor);
      m_layer = const_cast<Layer*>(location.layer());
    }
  }
}

} // namespace app
