// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DOC_RANGE_H_INCLUDED
#define APP_DOC_RANGE_H_INCLUDED
#pragma once

#include "doc/frame.h"
#include "doc/selected_frames.h"
#include "doc/selected_layers.h"

#include <iosfwd>

namespace doc {
  class Cel;
  class Sprite;
}

namespace app {
  using namespace doc;

  class DocRange {
  public:
    enum Type { kNone = 0,
                kCels = 1,
                kFrames = 2,
                kLayers = 4 };

    DocRange();
    DocRange(Cel* cel);

    Type type() const { return m_type; }
    bool enabled() const { return m_type != kNone; }
    layer_t layers() const { return int(m_selectedLayers.size()); }
    frame_t frames() const { return int(m_selectedFrames.size()); }
    const SelectedLayers& selectedLayers() const  { return m_selectedLayers; }
    const SelectedFrames& selectedFrames() const  { return m_selectedFrames; }

    void displace(layer_t layerDelta, frame_t frameDelta);

    bool contains(const Layer* layer) const;
    bool contains(const frame_t frame) const {
      return m_selectedFrames.contains(frame);
    }
    bool contains(const Layer* layer,
                  const frame_t frame) const;

    void clearRange();
    void startRange(Layer* fromLayer, frame_t fromFrame, Type type);
    void endRange(Layer* toLayer, frame_t toFrame);

    void selectLayer(Layer* layer);
    void selectLayers(const SelectedLayers& selLayers);

    frame_t firstFrame() const { return m_selectedFrames.firstFrame(); }
    frame_t lastFrame() const { return m_selectedFrames.lastFrame(); }

    bool operator==(const DocRange& o) const {
      return (m_type == o.m_type &&
              m_selectedLayers == o.m_selectedLayers &&
              m_selectedFrames == o.m_selectedFrames);
    }

    bool convertToCels(const Sprite* sprite);

    bool write(std::ostream& os) const;
    bool read(std::istream& is);

  private:
    void selectLayerRange(Layer* fromLayer, Layer* toLayer);
    void selectFrameRange(frame_t fromFrame, frame_t toFrame);

    Type m_type;                // Last used type of the range
    int m_flags;                // All used types in startRange()
    SelectedLayers m_selectedLayers;
    SelectedFrames m_selectedFrames;
    Layer* m_selectingFromLayer;
    frame_t m_selectingFromFrame;
  };

} // namespace app

#endif
