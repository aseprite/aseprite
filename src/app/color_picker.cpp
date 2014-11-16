/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/color_picker.h"

#include "app/document_location.h"
#include "gfx/point.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/primitives.h"
#include "raster/sprite.h"

namespace app {

ColorPicker::ColorPicker()
  : m_alpha(0)
  , m_layer(NULL)
{
}

void ColorPicker::pickColor(const DocumentLocation& location, int x, int y, Mode mode)
{
  m_alpha = 255;
  m_color = app::Color::fromMask();

  // Get the color from the image
  if (mode == FromComposition) { // Pick from the composed image
    m_color = app::Color::fromImage(
      location.sprite()->pixelFormat(),
      location.sprite()->getPixel(x, y, location.frame()));

    raster::CelList cels;
    location.sprite()->pickCels(x, y, location.frame(), 128, cels);
    if (!cels.empty())
      m_layer = cels.front()->layer();
  }
  else {                        // Pick from the current layer
    int u, v;
    raster::Image* image = location.image(&u, &v, NULL);
    gfx::Point pt(x-u, y-v);

    if (image && image->bounds().contains(pt)) {
      raster::color_t imageColor = get_pixel(image, pt.x, pt.y);

      switch (image->pixelFormat()) {
        case IMAGE_RGB:
          m_alpha = raster::rgba_geta(imageColor);
          break;
        case IMAGE_GRAYSCALE:
          m_alpha = raster::graya_geta(imageColor);
          break;
      }

      m_color = app::Color::fromImage(image->pixelFormat(), imageColor);
      m_layer = const_cast<Layer*>(location.layer());
    }
  }
}

} // namespace app
