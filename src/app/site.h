// Aseprite
// Copyright (c) 2001-2018 David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SITE_H_INCLUDED
#define APP_SITE_H_INCLUDED
#pragma once

#include "doc/frame.h"
#include "doc/selected_frames.h"
#include "doc/selected_layers.h"

namespace doc {
  class Cel;
  class Image;
  class Layer;
  class Palette;
  class Sprite;
} // namespace doc

namespace app {
  class Doc;

  // Specifies the current location in a context. E.g. the location in
  // the current Editor (current document, sprite, layer, frame,
  // etc.).
  class Site {
  public:
    // Were is the user focus. E.g. If this focus is in the timeline,
    // it means that commands should applied in the context of the
    // timeline (layers, or frames, or cels).
    enum Focus {
      None,
      InEditor,
      InLayers,
      InFrames,
      InCels,
      InColorBar
    };

    Site()
      : m_focus(None)
      , m_document(nullptr)
      , m_sprite(nullptr)
      , m_layer(nullptr)
      , m_frame(0) { }

    const Focus focus() const { return m_focus; }
    bool inEditor() const { return m_focus == InEditor; }
    bool inLayers() const { return m_focus == InLayers; }
    bool inFrames() const { return m_focus == InFrames; }
    bool inCels() const { return m_focus == InCels; }
    bool inColorBar() const { return m_focus == InColorBar; }
    bool inTimeline() const { return (inLayers() || inFrames() || inCels()); }

    const Doc* document() const { return m_document; }
    const doc::Sprite* sprite() const { return m_sprite; }
    const doc::Layer* layer() const { return m_layer; }
    doc::frame_t frame() const { return m_frame; }
    const doc::Cel* cel() const;

    Doc* document() { return m_document; }
    doc::Sprite* sprite() { return m_sprite; }
    doc::Layer* layer() { return m_layer; }
    doc::Cel* cel();

    void focus(Focus focus) { m_focus = focus; }
    void document(Doc* document) { m_document = document; }
    void sprite(doc::Sprite* sprite) { m_sprite = sprite; }
    void layer(doc::Layer* layer) { m_layer = layer; }
    void frame(doc::frame_t frame) { m_frame = frame; }

    const doc::SelectedLayers& selectedLayers() const { return m_selectedLayers; }
    doc::SelectedLayers& selectedLayers() { return m_selectedLayers; }
    void selectedLayers(const doc::SelectedLayers& selectedLayers) {
      m_selectedLayers = selectedLayers;
    }

    const doc::SelectedFrames& selectedFrames() const { return m_selectedFrames; }
    doc::SelectedFrames& selectedFrames() { return m_selectedFrames; }
    void selectedFrames(const doc::SelectedFrames& selectedFrames) {
      m_selectedFrames = selectedFrames;
    }

    doc::Palette* palette();
    doc::Image* image(int* x = nullptr, int* y = nullptr, int* opacity = nullptr) const;
    doc::Palette* palette() const;

  private:
    Focus m_focus;
    Doc* m_document;
    doc::Sprite* m_sprite;
    doc::Layer* m_layer;
    doc::frame_t m_frame;
    doc::SelectedLayers m_selectedLayers;
    doc::SelectedFrames m_selectedFrames;
  };

} // namespace app

#endif
