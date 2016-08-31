// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SPRITE_POSITION_H_INCLUDED
#define DOC_SPRITE_POSITION_H_INCLUDED
#pragma once

#include "doc/frame.h"
#include "doc/layer.h"
#include "doc/object.h"
#include "doc/object_id.h"

namespace doc {

  class SpritePosition {
  public:
    SpritePosition()
      : m_layerId(NullId)
      , m_frame(0) {
    }

    SpritePosition(const Layer* layer, frame_t frame)
      : m_layerId(layer ? layer->id(): NullId)
      , m_frame(frame) {
    }

    Layer* layer() const { return get<Layer>(m_layerId); }
    ObjectId layerId() const { return m_layerId; }
    frame_t frame() const { return m_frame; }

    void layer(Layer* layer) {
      m_layerId = (layer ? layer->id(): NullId);
    }

    void layerId(ObjectId layerId) {
      m_layerId = layerId;
    }

    void frame(frame_t frame) {
      m_frame = frame;
    }

    bool operator==(const SpritePosition& o) const { return m_layerId == o.m_layerId && m_frame == o.m_frame; }
    bool operator!=(const SpritePosition& o) const { return m_layerId != o.m_layerId || m_frame != o.m_frame; }

  private:
    ObjectId m_layerId;
    frame_t m_frame;
  };

} // namespace doc

#endif
