// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
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
  class Image;
  class Layer;
  class LayerImage;
  class Slice;
  class Sprite;
  class Tag;
}

namespace app {
  class Doc;

  class DocEvent {
  public:
    DocEvent(Doc* doc)
      : m_doc(doc)
      , m_sprite(nullptr)
      , m_layer(nullptr)
      , m_cel(nullptr)
      , m_image(nullptr)
      , m_imageIndex(-1)
      , m_frame(0)
      , m_tag(nullptr)
      , m_slice(nullptr)
      , m_targetLayer(nullptr)
      , m_targetFrame(0) {
    }

    // Source of the event.
    Doc* document() const { return m_doc; }
    doc::Sprite* sprite() const { return m_sprite; }
    doc::Layer* layer() const { return m_layer; }
    doc::Cel* cel() const { return m_cel; }
    doc::Image* image() const { return m_image; }
    int imageIndex() const { return m_imageIndex; }
    doc::frame_t frame() const { return m_frame; }
    doc::Tag* tag() const { return m_tag; }
    doc::Slice* slice() const { return m_slice; }
    const gfx::Region& region() const { return m_region; }

    void sprite(doc::Sprite* sprite) { m_sprite = sprite; }
    void layer(doc::Layer* layer) { m_layer = layer; }
    void cel(doc::Cel* cel) { m_cel = cel; }
    void image(doc::Image* image) { m_image = image; }
    void imageIndex(int imageIndex) { m_imageIndex = imageIndex; }
    void frame(doc::frame_t frame) { m_frame = frame; }
    void tag(doc::Tag* tag) { m_tag = tag; }
    void slice(doc::Slice* slice) { m_slice = slice; }
    void region(const gfx::Region& rgn) { m_region = rgn; }

    // Destination of the operation.
    doc::Layer* targetLayer() const { return m_targetLayer; }
    doc::frame_t targetFrame() const { return m_targetFrame; }

    void targetLayer(doc::Layer* layer) { m_targetLayer = layer; }
    void targetFrame(doc::frame_t frame) { m_targetFrame = frame; }

  private:
    Doc* m_doc;
    doc::Sprite* m_sprite;
    doc::Layer* m_layer;
    doc::Cel* m_cel;
    doc::Image* m_image;
    int m_imageIndex;
    doc::frame_t m_frame;
    doc::Tag* m_tag;
    doc::Slice* m_slice;
    gfx::Region m_region;

    // For copy/move commands, the m_layer/m_frame are source of the
    // operation, and these are the destination of the operation.
    doc::Layer* m_targetLayer;
    doc::frame_t m_targetFrame;
  };

} // namespace app

#endif
