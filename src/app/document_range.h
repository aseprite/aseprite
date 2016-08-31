// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DOCUMENT_RANGE_H_INCLUDED
#define APP_DOCUMENT_RANGE_H_INCLUDED
#pragma once

#include "doc/frame.h"
#include "doc/selected_frames.h"
#include "doc/selected_layers.h"

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
    layer_t layers() const { return int(m_selectedLayers.size()); }
    frame_t frames() const { return int(m_selectedFrames.size()); }
    const SelectedLayers& selectedLayers() const  { return m_selectedLayers; }
    const SelectedFrames& selectedFrames() const  { return m_selectedFrames; }

    void displace(layer_t layerDelta, frame_t frameDelta);

    bool contains(Layer* layer) const;

    bool contains(frame_t frame) const {
      return m_selectedFrames.contains(frame);
    }

    bool contains(Layer* layer, frame_t frame) const {
      return contains(layer) && contains(frame);
    }

    void clearRange();
    void startRange(Layer* fromLayer, frame_t fromFrame, Type type);
    void endRange(Layer* toLayer, frame_t toFrame);

    frame_t firstFrame() const { return m_selectedFrames.firstFrame(); }
    frame_t lastFrame() const { return m_selectedFrames.lastFrame(); }

    bool operator==(const DocumentRange& o) const {
      return (m_type == o.m_type &&
              m_selectedLayers == o.m_selectedLayers &&
              m_selectedFrames == o.m_selectedFrames);
    }

    bool convertToCels(const Sprite* sprite);

  private:
    void selectLayerRange(Layer* fromLayer, Layer* toLayer);
    void selectFrameRange(frame_t fromFrame, frame_t toFrame);

    Type m_type;
    SelectedLayers m_selectedLayers;
    SelectedFrames m_selectedFrames;
    Layer* m_selectingFromLayer;
    frame_t m_selectingFromFrame;
  };

} // namespace app

#endif
