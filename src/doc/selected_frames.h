// Aseprite Document Library
// Copyright (c) 2016-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SELECTED_FRAMES_H_INCLUDED
#define DOC_SELECTED_FRAMES_H_INCLUDED
#pragma once

#include "doc/frame_range.h"

#include <iosfwd>
#include <iterator>
#include <vector>

namespace doc {

  class SelectedFrames {
    typedef std::vector<FrameRange> Ranges;

  public:
    class const_iterator : public std::iterator<std::forward_iterator_tag, frame_t> {
      static const int kNullFrame = -2;
    public:
      const_iterator(const Ranges::const_iterator& it)
        : m_it(it), m_frame(kNullFrame) {
      }

      const_iterator& operator++() {
        if (m_frame == kNullFrame)
          m_frame = m_it->fromFrame;

        if (m_it->fromFrame <= m_it->toFrame) {
          if (m_frame < m_it->toFrame)
            ++m_frame;
          else {
            m_frame = kNullFrame;
            ++m_it;
          }
        }
        else {
          if (m_frame > m_it->toFrame)
            --m_frame;
          else {
            m_frame = kNullFrame;
            ++m_it;
          }
        }

        return *this;
      }

      frame_t operator*() const {
        if (m_frame == kNullFrame)
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

    class const_reverse_iterator : public std::iterator<std::forward_iterator_tag, frame_t> {
    public:
      const_reverse_iterator(const Ranges::const_reverse_iterator& it)
        : m_it(it), m_frame(-1) {
      }

      const_reverse_iterator& operator++() {
        if (m_frame < 0)
          m_frame = m_it->toFrame;

        if (m_frame > m_it->fromFrame)
          --m_frame;
        else {
          m_frame = -1;
          ++m_it;
        }

        return *this;
      }

      frame_t operator*() const {
        if (m_frame < 0)
          m_frame = m_it->toFrame;
        return m_frame;
      }

      bool operator==(const const_reverse_iterator& o) const {
        return (m_it == o.m_it && m_frame == o.m_frame);
      }

      bool operator!=(const const_reverse_iterator& it) const {
        return !operator==(it);
      }

    private:
      mutable Ranges::const_reverse_iterator m_it;
      mutable frame_t m_frame;
    };

    class Reversed {
    public:
      typedef const_reverse_iterator const_iterator;

      const_iterator begin() const { return m_selectedFrames.rbegin(); }
      const_iterator end() const { return m_selectedFrames.rend(); }

      Reversed(const SelectedFrames& selectedFrames)
        : m_selectedFrames(selectedFrames) {
      }

    private:
      const SelectedFrames& m_selectedFrames;
    };

    const_iterator begin() const { return const_iterator(m_ranges.begin()); }
    const_iterator end() const { return const_iterator(m_ranges.end()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(m_ranges.rbegin()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(m_ranges.rend()); }

    std::size_t size() const;
    std::size_t ranges() const { return m_ranges.size(); }
    bool empty() const { return m_ranges.empty(); }

    void clear();
    void insert(frame_t frame);
    void insert(frame_t fromFrame, frame_t toFrame);
    SelectedFrames filter(frame_t fromFrame, frame_t toFrame) const;

    bool contains(frame_t frame) const;

    frame_t firstFrame() const { return (!m_ranges.empty() ? m_ranges.front().fromFrame: -1); }
    frame_t lastFrame() const { return (!m_ranges.empty() ? m_ranges.back().toFrame: -1); }

    void displace(frame_t frameDelta);
    Reversed reversed() const { return Reversed(*this); }

    SelectedFrames makeReverse() const;
    SelectedFrames makePingPong() const;

    bool operator==(const SelectedFrames& o) const {
      return m_ranges == o.m_ranges;
    }

    bool operator!=(const SelectedFrames& o) const {
      return !operator==(o);
    }

    bool write(std::ostream& os) const;
    bool read(std::istream& is);

  private:
    Ranges m_ranges;
  };

} // namespace doc

#endif  // DOC_SELECTED_FRAMES_H_INCLUDED
