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

#include "app/color.h"
#include "app/color_utils.h"
#include "app/modules/palettes.h"
#include "gfx/hsv.h"
#include "gfx/rgb.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/sprite.h"

namespace app {

gfx::Color color_utils::blackandwhite(gfx::Color color)
{
  if ((gfx::getr(color)*30+gfx::getg(color)*59+gfx::getb(color)*11)/100 < 128)
    return gfx::rgba(0, 0, 0);
  else
    return gfx::rgba(255, 255, 255);
}

gfx::Color color_utils::blackandwhite_neg(gfx::Color color)
{
  if ((gfx::getr(color)*30+gfx::getg(color)*59+gfx::getb(color)*11)/100 < 128)
    return gfx::rgba(255, 255, 255);
  else
    return gfx::rgba(0, 0, 0);
}

gfx::Color color_utils::color_for_ui(const app::Color& color)
{
  gfx::Color c = gfx::ColorNone;

  switch (color.getType()) {

    case app::Color::MaskType:
      c = gfx::ColorNone;
      break;

    case app::Color::RgbType:
    case app::Color::HsvType:
      c = gfx::rgba(
        color.getRed(),
        color.getGreen(),
        color.getBlue(), 255);
      break;

    case app::Color::GrayType:
      c = gfx::rgba(
        color.getGray(),
        color.getGray(),
        color.getGray(), 255);
      break;

    case app::Color::IndexType: {
      int i = color.getIndex();
      ASSERT(i >= 0 && i < (int)get_current_palette()->size());

      uint32_t _c = get_current_palette()->getEntry(i);
      c = gfx::rgba(
        rgba_getr(_c),
        rgba_getg(_c),
        rgba_getb(_c), 255);
      break;
    }

  }

  return c;
}

doc::color_t color_utils::color_for_image(const app::Color& color, PixelFormat format)
{
  if (color.getType() == app::Color::MaskType)
    return 0;

  doc::color_t c = -1;

  switch (format) {
    case IMAGE_RGB:
      c = doc::rgba(color.getRed(), color.getGreen(), color.getBlue(), 255);
      break;
    case IMAGE_GRAYSCALE:
      c = doc::graya(color.getGray(), 255);
      break;
    case IMAGE_INDEXED:
      c = color.getIndex();
      break;
  }

  return c;
}

doc::color_t color_utils::color_for_layer(const app::Color& color, Layer* layer)
{
  return color_for_target(color, ColorTarget(layer));
}

doc::color_t color_utils::color_for_target(const app::Color& color, const ColorTarget& colorTarget)
{
  if (color.getType() == app::Color::MaskType)
    return colorTarget.maskColor();

  doc::color_t c = -1;

  switch (colorTarget.pixelFormat()) {
    case IMAGE_RGB:
      c = doc::rgba(color.getRed(), color.getGreen(), color.getBlue(), 255);
      break;
    case IMAGE_GRAYSCALE:
      c = doc::graya(color.getGray(), 255);
      break;
    case IMAGE_INDEXED:
      if (color.getType() == app::Color::IndexType) {
        c = color.getIndex();
      }
      else {
        c = get_current_palette()->findBestfit(
          color.getRed(),
          color.getGreen(),
          color.getBlue(),
          colorTarget.isTransparent() ?
            colorTarget.maskColor(): // Don't return the mask color
            -1);                     // Return any color, we are in a background layer.
      }
      break;
  }

  return c;
}

} // namespace app
