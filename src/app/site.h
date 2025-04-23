// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SITE_H_INCLUDED
#define APP_SITE_H_INCLUDED
#pragma once

#include "app/tilemap_mode.h"
#include "app/tileset_mode.h"
#include "doc/cel_list.h"
#include "doc/frame.h"
#include "doc/palette_picks.h"
#include "doc/selected_objects.h"
#include "gfx/fwd.h"
#include "view/range.h"

namespace doc {
class Grid;
class Image;
class Layer;
class Palette;
class RgbMap;
class Sprite;
class Tileset;
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
  enum Focus { None, InEditor, InLayers, InFrames, InCels, InColorBar };

  Site()
    : m_focus(None)
    , m_document(nullptr)
    , m_sprite(nullptr)
    , m_layer(nullptr)
    , m_frame(0)
    , m_tilemapMode(TilemapMode::Pixels)
    , m_tilesetMode(TilesetMode::Manual)
  {
  }

  const Focus focus() const { return m_focus; }
  bool inEditor() const { return m_focus == InEditor; }
  bool inLayers() const { return m_focus == InLayers; }
  bool inFrames() const { return m_focus == InFrames; }
  bool inCels() const { return m_focus == InCels; }
  bool inColorBar() const { return m_focus == InColorBar; }
  bool inTimeline() const { return (inLayers() || inFrames() || inCels()); }

  Doc* document() const { return m_document; }
  doc::Sprite* sprite() const { return m_sprite; }
  doc::Layer* layer() const { return m_layer; }
  doc::frame_t frame() const { return m_frame; }
  doc::Cel* cel() const;
  const view::RealRange& range() const { return m_range; }

  void focus(Focus focus) { m_focus = focus; }
  void document(Doc* document) { m_document = document; }
  void sprite(doc::Sprite* sprite) { m_sprite = sprite; }
  void layer(doc::Layer* layer) { m_layer = layer; }
  void frame(doc::frame_t frame) { m_frame = frame; }
  void range(const view::RealRange& range);

  const doc::SelectedLayers& selectedLayers() const { return m_range.selectedLayers(); }
  const doc::SelectedFrames& selectedFrames() const { return m_range.selectedFrames(); }

  // Selected colors selected in the ColorBar
  const doc::PalettePicks& selectedColors() const { return m_selectedColors; }
  doc::PalettePicks& selectedColors() { return m_selectedColors; }
  void selectedColors(const doc::PalettePicks& colors) { m_selectedColors = colors; }

  // Selected tiles selected in the ColorBar
  const doc::PalettePicks& selectedTiles() const { return m_selectedTiles; }
  doc::PalettePicks& selectedTiles() { return m_selectedTiles; }
  void selectedTiles(const doc::PalettePicks& tiles) { m_selectedTiles = tiles; }

  const doc::SelectedObjects& selectedSlices() const { return m_selectedSlices; }
  doc::SelectedObjects& selectedSlices() { return m_selectedSlices; }
  void selectedSlices(const doc::SelectedObjects& set) { m_selectedSlices = set; }

  doc::Palette* palette();
  doc::Image* image(int* x = nullptr, int* y = nullptr, int* opacity = nullptr) const;
  doc::Palette* palette() const;
  doc::RgbMap* rgbMap() const;

  doc::Tileset* tileset() const;
  doc::Grid grid() const;
  gfx::Rect gridBounds() const;

  void tilemapMode(const TilemapMode mode) { m_tilemapMode = mode; }
  void tilesetMode(const TilesetMode mode) { m_tilesetMode = mode; }
  TilemapMode tilemapMode() const { return m_tilemapMode; }
  TilesetMode tilesetMode() const { return m_tilesetMode; }

  bool shouldTrimCel(doc::Cel* cel) const;

  // Returns the list of selected "unique" cels (to avoid iterating
  // two or more times through linked cels), or just a list of the
  // active cel if there is no range in the given site.
  doc::CelList selectedUniqueCels() const;

  // Same as uniqueCels() with cels from non-locked/editable layers.
  doc::CelList selectedUniqueCelsToEditPixels() const;

  inline bool operator==(const Site& other) const
  {
    return (document() == other.document() && sprite() == other.sprite() &&
            layer() == other.layer() && frame() == other.frame() && cel() == other.cel());
  }

private:
  Focus m_focus;
  Doc* m_document;
  doc::Sprite* m_sprite;
  doc::Layer* m_layer;
  doc::frame_t m_frame;
  view::RealRange m_range;
  doc::PalettePicks m_selectedColors;
  doc::PalettePicks m_selectedTiles;
  doc::SelectedObjects m_selectedSlices;
  TilemapMode m_tilemapMode;
  TilesetMode m_tilesetMode;
};

} // namespace app

#endif
