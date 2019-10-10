// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SITE_H_INCLUDED
#define APP_SITE_H_INCLUDED
#pragma once

#include "app/doc_range.h"
#include "doc/frame.h"
#include "doc/palette_picks.h"
#include "doc/selected_objects.h"
#include "gfx/fwd.h"

namespace doc {
  class Cel;
  class Image;
  class Layer;
  class Palette;
  class RgbMap;
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
    const DocRange& range() const { return m_range; }

    void focus(Focus focus) { m_focus = focus; }
    void document(Doc* document) { m_document = document; }
    void sprite(doc::Sprite* sprite) { m_sprite = sprite; }
    void layer(doc::Layer* layer) { m_layer = layer; }
    void frame(doc::frame_t frame) { m_frame = frame; }
    void range(const DocRange& range);

    const doc::SelectedLayers& selectedLayers() const { return m_range.selectedLayers(); }
    const doc::SelectedFrames& selectedFrames() const { return m_range.selectedFrames(); }

    // Selected colors selected in the ColorBar
    const doc::PalettePicks& selectedColors() const { return m_selectedColors; }
    doc::PalettePicks& selectedColors() { return m_selectedColors; }
    void selectedColors(const doc::PalettePicks& colors) {
      m_selectedColors = colors;
    }

    const doc::SelectedObjects& selectedSlices() const { return m_selectedSlices; }
    doc::SelectedObjects& selectedSlices() { return m_selectedSlices; }
    void selectedSlices(const doc::SelectedObjects& set) {
      m_selectedSlices = set;
    }

    doc::Palette* palette();
    doc::Image* image(int* x = nullptr, int* y = nullptr, int* opacity = nullptr) const;
    doc::Palette* palette() const;
    doc::RgbMap* rgbMap() const;

    gfx::Rect gridBounds() const;

  private:
    Focus m_focus;
    Doc* m_document;
    doc::Sprite* m_sprite;
    doc::Layer* m_layer;
    doc::frame_t m_frame;
    DocRange m_range;
    doc::PalettePicks m_selectedColors;
    doc::SelectedObjects m_selectedSlices;
  };

} // namespace app

#endif
