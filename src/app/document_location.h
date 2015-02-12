// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_DOCUMENT_LOCATION_H_INCLUDED
#define APP_DOCUMENT_LOCATION_H_INCLUDED
#pragma once

#include "doc/frame.h"
#include "doc/layer_index.h"

namespace doc {
  class Cel;
  class Image;
  class Layer;
  class Palette;
  class Sprite;
}

namespace app {
  class Document;

  using namespace doc;

  // Specifies the current location in a context. If we are in the
  // UIContext, it means the location in the current Editor (current
  // document, sprite, layer, frame, etc.).
  class DocumentLocation {
  public:
    DocumentLocation()
      : m_document(NULL)
      , m_sprite(NULL)
      , m_layer(NULL)
      , m_frame(0) { }

    const Document* document() const { return m_document; }
    const Sprite* sprite() const { return m_sprite; }
    const Layer* layer() const { return m_layer; }
    frame_t frame() const { return m_frame; }
    const Cel* cel() const;

    Document* document() { return m_document; }
    Sprite* sprite() { return m_sprite; }
    Layer* layer() { return m_layer; }
    Cel* cel();

    void document(Document* document) { m_document = document; }
    void sprite(Sprite* sprite) { m_sprite = sprite; }
    void layer(Layer* layer) { m_layer = layer; }
    void frame(frame_t frame) { m_frame = frame; }

    LayerIndex layerIndex() const;
    void layerIndex(LayerIndex layerIndex);
    Palette* palette();
    Image* image(int* x = NULL, int* y = NULL, int* opacity = NULL) const;
    Palette* palette() const;

  private:
    Document* m_document;
    Sprite* m_sprite;
    Layer* m_layer;
    frame_t m_frame;
  };

} // namespace app

#endif
