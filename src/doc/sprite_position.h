// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SPRITE_POSITION_H_INCLUDED
#define DOC_SPRITE_POSITION_H_INCLUDED
#pragma once

#include "doc/frame_number.h"
#include "doc/layer_index.h"

namespace doc {

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

} // namespace doc

#endif
