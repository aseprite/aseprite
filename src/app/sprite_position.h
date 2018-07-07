// Aseprite
// Copyright (c) 2001-2018 David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef DOC_SPRITE_POSITION_H_INCLUDED
#define DOC_SPRITE_POSITION_H_INCLUDED
#pragma once

#include "doc/frame.h"
#include "doc/layer.h"
#include "doc/object.h"
#include "doc/object_id.h"

namespace app {

  class SpritePosition {
  public:
    SpritePosition()
      : m_layerId(doc::NullId)
      , m_frame(0) {
    }

    SpritePosition(const doc::Layer* layer, doc::frame_t frame)
      : m_layerId(layer ? layer->id(): doc::NullId)
      , m_frame(frame) {
    }

    doc::Layer* layer() const { return doc::get<doc::Layer>(m_layerId); }
    doc::ObjectId layerId() const { return m_layerId; }
    doc::frame_t frame() const { return m_frame; }

    void layer(doc::Layer* layer) {
      m_layerId = (layer ? layer->id(): doc::NullId);
    }

    void layerId(doc::ObjectId layerId) {
      m_layerId = layerId;
    }

    void frame(doc::frame_t frame) {
      m_frame = frame;
    }

    bool operator==(const SpritePosition& o) const { return m_layerId == o.m_layerId && m_frame == o.m_frame; }
    bool operator!=(const SpritePosition& o) const { return m_layerId != o.m_layerId || m_frame != o.m_frame; }

  private:
    doc::ObjectId m_layerId;
    doc::frame_t m_frame;
  };

} // namespace app

#endif
