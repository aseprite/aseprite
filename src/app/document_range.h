// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DOCUMENT_RANGE_H_INCLUDED
#define APP_DOCUMENT_RANGE_H_INCLUDED
#pragma once

#include "doc/frame.h"
#include "doc/layer_index.h"

#include <algorithm>

namespace doc {
  class Cel;
  class Sprite;
}

namespace app {
  using namespace doc;

  class DocumentRange {
  public:
    enum Type { kNone, kCels, kFrames, kLayers };

    DocumentRange();
    DocumentRange(Cel* cel);

    Type type() const { return m_type; }
    bool enabled() const { return m_type != kNone; }
    LayerIndex layerBegin() const  { return std::min(m_layerBegin, m_layerEnd); }
    LayerIndex layerEnd() const    { return std::max(m_layerBegin, m_layerEnd); }
    frame_t frameBegin() const { return std::min(m_frameBegin, m_frameEnd); }
    frame_t frameEnd() const   { return std::max(m_frameBegin, m_frameEnd); }

    int layers() const { return layerEnd() - layerBegin() + 1; }
    frame_t frames() const { return frameEnd() - frameBegin() + 1; }
    void setLayers(int layers);
    void setFrames(frame_t frames);
    void displace(int layerDelta, int frameDelta);

    bool inRange(LayerIndex layer) const;
    bool inRange(frame_t frame) const;
    bool inRange(LayerIndex layer, frame_t frame) const;

    void startRange(LayerIndex layer, frame_t frame, Type type);
    void endRange(LayerIndex layer, frame_t frame);
    void disableRange();

    bool operator==(const DocumentRange& o) const {
      return m_type == o.m_type &&
        layerBegin() == o.layerBegin() && layerEnd() == o.layerEnd() &&
        frameBegin() == o.frameBegin() && frameEnd() == o.frameEnd();
    }

    bool convertToCels(Sprite* sprite);

  private:
    Type m_type;
    LayerIndex m_layerBegin;
    LayerIndex m_layerEnd;
    frame_t m_frameBegin;
    frame_t m_frameEnd;
  };

} // namespace app

#endif
