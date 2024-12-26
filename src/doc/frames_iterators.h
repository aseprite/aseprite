// Aseprite Document Library
// Copyright (c) 2024 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_FRAMES_ITERATORS_H_INCLUDED
#define DOC_FRAMES_ITERATORS_H_INCLUDED
#pragma once

#include "doc/frame_range.h"

#include <iosfwd>
#include <vector>

namespace doc { namespace frames {
typedef std::vector<FrameRange> Ranges;

class const_iterator {
  static const int kNullFrame = -2;

public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = frame_t;
  using difference_type = std::ptrdiff_t;
  using pointer = frame_t*;
  using reference = frame_t&;

  const_iterator(const frames::Ranges::const_iterator& it) : m_it(it), m_frame(kNullFrame) {}

  const_iterator& operator++()
  {
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

  frame_t operator*() const
  {
    if (m_frame == kNullFrame)
      m_frame = m_it->fromFrame;
    return m_frame;
  }

  bool operator==(const const_iterator& o) const
  {
    return (m_it == o.m_it && m_frame == o.m_frame);
  }

  bool operator!=(const const_iterator& it) const { return !operator==(it); }

private:
  mutable frames::Ranges::const_iterator m_it;
  mutable frame_t m_frame;
};

class const_reverse_iterator {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = frame_t;
  using difference_type = std::ptrdiff_t;
  using pointer = frame_t*;
  using reference = frame_t&;

  const_reverse_iterator(const frames::Ranges::const_reverse_iterator& it) : m_it(it), m_frame(-1)
  {
  }

  const_reverse_iterator& operator++()
  {
    if (m_frame < 0)
      m_frame = m_it->toFrame;

    int step = (m_it->fromFrame > m_it->toFrame) - (m_it->fromFrame < m_it->toFrame);

    if ((step < 0 && m_frame > m_it->fromFrame) || (step > 0 && m_frame < m_it->fromFrame))
      m_frame += step;
    else {
      m_frame = -1;
      ++m_it;
    }

    return *this;
  }

  frame_t operator*() const
  {
    if (m_frame < 0)
      m_frame = m_it->toFrame;
    return m_frame;
  }

  bool operator==(const const_reverse_iterator& o) const
  {
    return (m_it == o.m_it && m_frame == o.m_frame);
  }

  bool operator!=(const const_reverse_iterator& it) const { return !operator==(it); }

private:
  mutable frames::Ranges::const_reverse_iterator m_it;
  mutable frame_t m_frame;
};

template<typename T>
class Reversed {
public:
  typedef const_reverse_iterator const_iterator;

  const_iterator begin() const { return m_frames.rbegin(); }
  const_iterator end() const { return m_frames.rend(); }

  Reversed(const T& frames) : m_frames(frames) {}

private:
  const T& m_frames;
};

}} // namespace doc::frames

#endif // DOC_FRAMES_ITERATORS_H_INCLUDED
