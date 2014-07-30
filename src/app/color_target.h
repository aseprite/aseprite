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

#ifndef APP_COLOR_TARGET_H_INCLUDED
#define APP_COLOR_TARGET_H_INCLUDED
#pragma once

#include "raster/color.h"
#include "raster/layer.h"
#include "raster/pixel_format.h"
#include "raster/sprite.h"

namespace app {

  // Represents the kind of surface where we'll use a color.
  class ColorTarget {
  public:
    enum LayerType {
      BackgroundLayer,
      TransparentLayer
    };

    ColorTarget(LayerType layerType, raster::PixelFormat pixelFormat, raster::color_t maskColor) :
      m_layerType(layerType),
      m_pixelFormat(pixelFormat),
      m_maskColor(maskColor) {
    }

    ColorTarget(raster::Layer* layer) :
      m_layerType(layer->isBackground() ? BackgroundLayer: TransparentLayer),
      m_pixelFormat(layer->sprite()->pixelFormat()),
      m_maskColor(layer->sprite()->transparentColor()) {
    }

    bool isBackground() const { return m_layerType == BackgroundLayer; }
    bool isTransparent() const { return m_layerType == TransparentLayer; }
    LayerType layerType() const { return m_layerType; }
    raster::PixelFormat pixelFormat() const { return m_pixelFormat; }
    raster::color_t maskColor() const { return m_maskColor; }

  private:
    LayerType m_layerType;
    raster::PixelFormat m_pixelFormat;
    raster::color_t m_maskColor;
  };

} // namespace app

#endif
