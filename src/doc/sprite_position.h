// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SPRITE_POSITION_H_INCLUDED
#define DOC_SPRITE_POSITION_H_INCLUDED
#pragma once

#include "doc/frame.h"
#include "doc/layer_index.h"

namespace doc {

  class Sprite;

  class SpritePosition {
  public:
    SpritePosition()
      : m_layerIndex(0)
      , m_frame(0) {
    }
    SpritePosition(LayerIndex layerIndex, frame_t frame)
      : m_layerIndex(layerIndex)
      , m_frame(frame) {
    }

    const LayerIndex& layerIndex() const { return m_layerIndex; }
    const frame_t& frame() const { return m_frame; }

    void layerIndex(LayerIndex layerIndex) { m_layerIndex = layerIndex; }
    void frame(frame_t frame) { m_frame = frame; }

    bool operator==(const SpritePosition& o) const { return m_layerIndex == o.m_layerIndex && m_frame == o.m_frame; }
    bool operator!=(const SpritePosition& o) const { return m_layerIndex != o.m_layerIndex || m_frame != o.m_frame; }

  private:
    LayerIndex m_layerIndex;
    frame_t m_frame;
  };

} // namespace doc

#endif
