// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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
    case app::Color::HslType:
      c = gfx::rgba(
        color.getRed(),
        color.getGreen(),
        color.getBlue(),
        color.getAlpha());
      break;

    case app::Color::GrayType:
      c = gfx::rgba(
        color.getGray(),
        color.getGray(),
        color.getGray(),
        color.getAlpha());
      break;

    case app::Color::IndexType: {
      int i = color.getIndex();
      ASSERT(i >= 0 && i < (int)get_current_palette()->size());

      uint32_t _c = get_current_palette()->getEntry(i);
      c = gfx::rgba(
        rgba_getr(_c),
        rgba_getg(_c),
        rgba_getb(_c),
        color.getAlpha());
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
      c = doc::rgba(color.getRed(), color.getGreen(), color.getBlue(), color.getAlpha());
      break;
    case IMAGE_GRAYSCALE:
      c = doc::graya(color.getGray(), color.getAlpha());
      break;
    case IMAGE_INDEXED:
      c = color.getIndex();
      break;
  }

  return c;
}

doc::color_t color_utils::color_for_image_without_alpha(const app::Color& color, PixelFormat format)
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

doc::color_t color_utils::color_for_target_mask(const app::Color& color, const ColorTarget& colorTarget)
{
  int c = -1;

  if (color.getType() == app::Color::MaskType) {
    c = colorTarget.maskColor();
  }
  else {
    switch (colorTarget.pixelFormat()) {
      case IMAGE_RGB:
        c = doc::rgba(color.getRed(), color.getGreen(), color.getBlue(), color.getAlpha());
        break;
      case IMAGE_GRAYSCALE:
        c = doc::graya(color.getGray(), color.getAlpha());
        break;
      case IMAGE_INDEXED:
        if (color.getType() == app::Color::IndexType) {
          c = color.getIndex();
        }
        else {
          int r = color.getRed();
          int g = color.getGreen();
          int b = color.getBlue();
          int a = color.getAlpha();
          int mask = (colorTarget.isTransparent() ?
                      colorTarget.maskColor(): // Don't return the mask color
                      -1);

          c = get_current_palette()->findExactMatch(r, g, b, a, mask);
          if (c < 0)
            c = get_current_palette()->findBestfit(r, g, b, a, mask);
        }
        break;
    }
  }

  return (doc::color_t)c;
}

// TODO remove this function using a special RGB background layer (24bpp or 32bpp ignoring alpha)
doc::color_t color_utils::color_for_target(const app::Color& color, const ColorTarget& colorTarget)
{
  doc::color_t c = color_utils::color_for_target_mask(color, colorTarget);

  if (colorTarget.isBackground()) {
    switch (colorTarget.pixelFormat()) {
      case IMAGE_RGB: c |= doc::rgba_a_mask; break;
      case IMAGE_GRAYSCALE: c |= doc::graya_a_mask; break;
    }
  }

  return c;
}

} // namespace app
