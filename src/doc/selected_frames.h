// Aseprite Document Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SELECTED_FRAMES_H_INCLUDED
#define DOC_SELECTED_FRAMES_H_INCLUDED
#pragma once

#include "doc/frame_range.h"

#include <iterator>
#include <vector>

namespace doc {

  class SelectedFrames {
    typedef std::vector<FrameRange> Ranges;

  public:
    class const_iterator : public std::iterator<std::forward_iterator_tag, frame_t> {
    public:
      const_iterator(const Ranges::const_iterator& it)
        : m_it(it), m_frame(-1) {
      }

      const_iterator& operator++() {
        if (m_frame < 0)
          m_frame = m_it->fromFrame;

        if (m_frame < m_it->toFrame)
          ++m_frame;
        else {
          m_frame = -1;
          ++m_it;
        }

        return *this;
      }

      frame_t operator*() const {
        if (m_frame < 0)
          m_frame = m_it->fromFrame;
        return m_frame;
      }

      bool operator==(const const_iterator& o) const {
        return (m_it == o.m_it && m_frame == o.m_frame);
      }

      bool operator!=(const const_iterator& it) const {
        return !operator==(it);
      }

    private:
      mutable Ranges::const_iterator m_it;
      mutable frame_t m_frame;
    };

    const_iterator begin() const { return const_iterator(m_ranges.begin()); }
    const_iterator end() const { return const_iterator(m_ranges.end()); }

    void insert(frame_t frame);
    void insert(frame_t fromFrame, frame_t toFrame);

    bool contains(frame_t frame) const;

  private:
    Ranges m_ranges;
  };

} // namespace doc

#endif  // DOC_LAYER_LIST_H_INCLUDED
