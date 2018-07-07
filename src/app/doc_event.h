// Aseprite
// Copyright (c) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DOC_EVENT_H_INCLUDED
#define APP_DOC_EVENT_H_INCLUDED
#pragma once

#include "gfx/region.h"
#include "doc/frame.h"

namespace doc {
  class Cel;
  class FrameTag;
  class Image;
  class Layer;
  class LayerImage;
  class Sprite;
}

namespace app {
  class Doc;

  using namespace doc;

  class DocEvent {
  public:
    DocEvent(Doc* document)
      : m_document(document)
      , m_sprite(NULL)
      , m_layer(NULL)
      , m_cel(NULL)
      , m_image(NULL)
      , m_imageIndex(-1)
      , m_frame(0)
      , m_targetLayer(NULL)
      , m_targetFrame(0) {
    }

    // Source of the event.
    Doc* document() const { return m_document; }
    Sprite* sprite() const { return m_sprite; }
    Layer* layer() const { return m_layer; }
    Cel* cel() const { return m_cel; }
    Image* image() const { return m_image; }
    int imageIndex() const { return m_imageIndex; }
    frame_t frame() const { return m_frame; }
    FrameTag* frameTag() const { return m_frameTag; }
    const gfx::Region& region() const { return m_region; }

    void sprite(Sprite* sprite) { m_sprite = sprite; }
    void layer(Layer* layer) { m_layer = layer; }
    void cel(Cel* cel) { m_cel = cel; }
    void image(Image* image) { m_image = image; }
    void imageIndex(int imageIndex) { m_imageIndex = imageIndex; }
    void frame(frame_t frame) { m_frame = frame; }
    void frameTag(FrameTag* frameTag) { m_frameTag = frameTag; }
    void region(const gfx::Region& rgn) { m_region = rgn; }

    // Destination of the operation.
    Layer* targetLayer() const { return m_targetLayer; }
    frame_t targetFrame() const { return m_targetFrame; }

    void targetLayer(Layer* layer) { m_targetLayer = layer; }
    void targetFrame(frame_t frame) { m_targetFrame = frame; }

  private:
    Doc* m_document;
    Sprite* m_sprite;
    Layer* m_layer;
    Cel* m_cel;
    Image* m_image;
    int m_imageIndex;
    frame_t m_frame;
    FrameTag* m_frameTag;
    gfx::Region m_region;

    // For copy/move commands, the m_layer/m_frame are source of the
    // operation, and these are the destination of the operation.
    Layer* m_targetLayer;
    frame_t m_targetFrame;
  };

} // namespace app

#endif
