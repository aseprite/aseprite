// Aseprite Document Library
// Copyright (c) 2022-2024 Igara Studio S.A.
// Copyright (c) 2016-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SELECTED_FRAMES_H_INCLUDED
#define DOC_SELECTED_FRAMES_H_INCLUDED
#pragma once

#include "doc/frames_iterators.h"

namespace doc {

// The FramesSequence class is based in several code of this class.
// TODO: At some point we should remove the duplicated code between these
// classes.
class SelectedFrames {
public:
  frames::const_iterator begin() const { return frames::const_iterator(m_ranges.begin()); }
  frames::const_iterator end() const { return frames::const_iterator(m_ranges.end()); }
  frames::const_reverse_iterator rbegin() const
  {
    return frames::const_reverse_iterator(m_ranges.rbegin());
  }
  frames::const_reverse_iterator rend() const
  {
    return frames::const_reverse_iterator(m_ranges.rend());
  }

  std::size_t size() const;
  std::size_t ranges() const { return m_ranges.size(); }
  bool empty() const { return m_ranges.empty(); }

  void clear();
  void insert(frame_t frame);
  void insert(frame_t fromFrame, frame_t toFrame);
  SelectedFrames filter(frame_t fromFrame, frame_t toFrame) const;

  bool contains(frame_t frame) const;

  frame_t firstFrame() const { return (!m_ranges.empty() ? m_ranges.front().fromFrame : -1); }
  frame_t lastFrame() const { return (!m_ranges.empty() ? m_ranges.back().toFrame : -1); }

  void displace(frame_t frameDelta);
  frames::Reversed<SelectedFrames> reversed() const { return frames::Reversed(*this); }

  SelectedFrames makeReverse() const;
  SelectedFrames makePingPong() const;

  bool operator==(const SelectedFrames& o) const { return m_ranges == o.m_ranges; }

  bool operator!=(const SelectedFrames& o) const { return !operator==(o); }

  bool write(std::ostream& os) const;
  bool read(std::istream& is);

private:
  frames::Ranges m_ranges;
};

} // namespace doc

#endif // DOC_SELECTED_FRAMES_H_INCLUDED
