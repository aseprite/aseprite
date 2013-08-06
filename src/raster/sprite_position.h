/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#ifndef RASTER_SPRITE_POSITION_H_INCLUDED
#define RASTER_SPRITE_POSITION_H_INCLUDED

#include "raster/frame_number.h"
#include "raster/layer_index.h"

namespace raster {

  class Sprite;

  class SpritePosition {
  public:
    SpritePosition() { }
    SpritePosition(LayerIndex layerIndex, FrameNumber frameNumber)
      : m_layerIndex(layerIndex)
      , m_frameNumber(frameNumber) {
    }

    const LayerIndex& layerIndex() const { return m_layerIndex; }
    const FrameNumber& frameNumber() const { return m_frameNumber; }

    void layerIndex(LayerIndex layerIndex) { m_layerIndex = layerIndex; }
    void frameNumber(FrameNumber frameNumber) { m_frameNumber = frameNumber; }

    bool operator==(const SpritePosition& o) const { return m_layerIndex == o.m_layerIndex && m_frameNumber == o.m_frameNumber; }
    bool operator!=(const SpritePosition& o) const { return m_layerIndex != o.m_layerIndex || m_frameNumber != o.m_frameNumber; }

  private:
    LayerIndex m_layerIndex;
    FrameNumber m_frameNumber;
  };

} // namespace raster

#endif
