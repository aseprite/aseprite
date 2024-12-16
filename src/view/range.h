// Aseprite View Library
// Copyright (C) 2020-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef VIEW_RANGE_H_INCLUDED
#define VIEW_RANGE_H_INCLUDED
#pragma once

#include "doc/cel_list.h"
#include "doc/frame.h"
#include "doc/selected_frames.h"
#include "doc/selected_layers.h"

#include <iosfwd>

namespace doc {
class Cel;
class Sprite;
} // namespace doc

namespace view {

class Range {
public:
  enum Type { kNone = 0, kCels = 1, kFrames = 2, kLayers = 4 };

  Range();
  Range(doc::Cel* cel);
  Range(const Range&) = default;
  Range& operator=(const Range&) = default;

  Type type() const { return m_type; }
  bool enabled() const { return m_type != kNone; }
  doc::layer_t layers() const { return int(m_selectedLayers.size()); }
  doc::frame_t frames() const { return int(m_selectedFrames.size()); }
  const doc::SelectedLayers& selectedLayers() const { return m_selectedLayers; }
  const doc::SelectedFrames& selectedFrames() const { return m_selectedFrames; }

  void setType(const Type type);
  void setSelectedLayers(const doc::SelectedLayers& layers, const bool touchFlags = true);
  void setSelectedFrames(const doc::SelectedFrames& frames, const bool touchFlags = true);

  void displace(const doc::layer_t layerDelta, const doc::frame_t frameDelta);

  bool contains(const doc::Layer* layer) const;
  bool contains(const doc::frame_t frame) const { return m_selectedFrames.contains(frame); }
  bool contains(const doc::Layer* layer, const doc::frame_t frame) const;

  // If the range includes the given layer, it will be erased from
  // the selection and other candidates might be selected (e.g. a
  // sibling, parent, etc.) using the
  // candidate_if_layer_is_deleted() function.
  void eraseAndAdjust(const doc::Layer* layer);

  void clearRange();
  void startRange(doc::Layer* fromLayer, doc::frame_t fromFrame, Type type);
  void endRange(doc::Layer* toLayer, doc::frame_t toFrame);

  void selectLayer(doc::Layer* layer);
  void selectLayers(const doc::SelectedLayers& selLayers);

  doc::frame_t firstFrame() const { return m_selectedFrames.firstFrame(); }
  doc::frame_t lastFrame() const { return m_selectedFrames.lastFrame(); }

  bool operator==(const Range& o) const
  {
    return (m_type == o.m_type && m_selectedLayers == o.m_selectedLayers &&
            m_selectedFrames == o.m_selectedFrames);
  }

  bool convertToCels(const doc::Sprite* sprite);

  bool write(std::ostream& os) const;
  bool read(std::istream& is);

private:
  void selectLayerRange(doc::Layer* fromLayer, doc::Layer* toLayer);
  void selectFrameRange(doc::frame_t fromFrame, doc::frame_t toFrame);

  Type m_type; // Last used type of the range
  int m_flags; // All used types in startRange()
  doc::SelectedLayers m_selectedLayers;
  doc::SelectedFrames m_selectedFrames;
  doc::Layer* m_selectingFromLayer;
  doc::frame_t m_selectingFromFrame;
};

// TODO We should make these types strongly-typed and not just aliases.
// E.g.
//   using VirtualRange = RangeT<col_t>;
//   using RealRange = RangeT<fr_t>;
using VirtualRange = Range;
using RealRange = Range;

} // namespace view

#endif // VIEW_RANGE_H_INCLUDED
