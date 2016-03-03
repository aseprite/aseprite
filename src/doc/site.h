// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SITE_H_INCLUDED
#define DOC_SITE_H_INCLUDED
#pragma once

#include "doc/frame.h"
#include "doc/layer_index.h"

namespace doc {

  class Cel;
  class Document;
  class Image;
  class Layer;
  class Palette;
  class Sprite;

  // Specifies the current location in a context. E.g. the location in
  // the current Editor (current document, sprite, layer, frame,
  // etc.).
  class Site {
  public:
    Site()
      : m_document(nullptr)
      , m_sprite(nullptr)
      , m_layer(nullptr)
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
    Image* image(int* x = nullptr, int* y = nullptr, int* opacity = nullptr) const;
    Palette* palette() const;

  private:
    Document* m_document;
    Sprite* m_sprite;
    Layer* m_layer;
    frame_t m_frame;
  };

} // namespace app

#endif
