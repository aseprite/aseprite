// Aseprite Document Library
// Copyright (c) 2023-2024 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_FRAMES_SEQUENCE_H_INCLUDED
#define DOC_FRAMES_SEQUENCE_H_INCLUDED
#pragma once

#include "doc/selected_frames.h"

namespace doc {

// This class is based in several code of the SelectedFrames class.
// TODO: At some point we should remove the duplicated code between these
// classes.
class FramesSequence {
public:
  FramesSequence() {}
  FramesSequence(const SelectedFrames& selectedFrames);

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

  FramesSequence filter(frame_t fromFrame, frame_t toFrame) const;

  bool contains(frame_t frame) const;

  frame_t firstFrame() const { return (!m_ranges.empty() ? m_ranges.front().fromFrame : -1); }
  frame_t lastFrame() const { return (!m_ranges.empty() ? m_ranges.back().toFrame : -1); }
  frame_t lowestFrame() const;

  void displace(frame_t frameDelta);
  frames::Reversed<FramesSequence> reversed() const { return frames::Reversed(*this); }

  FramesSequence makeReverse() const;
  FramesSequence makePingPong() const;

  bool operator==(const FramesSequence& o) const { return m_ranges == o.m_ranges; }

  bool operator!=(const FramesSequence& o) const { return !operator==(o); }

  bool write(std::ostream& os) const;
  bool read(std::istream& is);

private:
  frames::Ranges m_ranges;
};

} // namespace doc

#endif // DOC_FRAMES_SEQUENCE_H_INCLUDED
