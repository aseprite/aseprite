// Aseprite Document Library
// Copyright (c) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_CELS_RANGE_H_INCLUDED
#define DOC_CELS_RANGE_H_INCLUDED
#pragma once

#include "doc/frame.h"
#include "doc/object_id.h"
#include "doc/selected_frames.h"

#include <set>

namespace doc {

  class Cel;
  class Layer;
  class SelectedFrames;
  class Sprite;

  class CelsRange {
  public:
    enum Flags {
      ALL,
      UNIQUE,
    };

    CelsRange(const Sprite* sprite,
              const SelectedFrames& selFrames,
              const Flags flags = ALL);

    class iterator {
    public:
      typedef Cel* value_type;
      typedef std::ptrdiff_t difference_type;
      typedef Cel** pointer;
      typedef Cel*& reference;
      typedef std::forward_iterator_tag iterator_category;

      iterator(const SelectedFrames& selFrames);
      iterator(const Sprite* sprite,
               const SelectedFrames& selFrames,
               const Flags flags);

      bool operator==(const iterator& other) const {
        return m_cel == other.m_cel;
      }

      bool operator!=(const iterator& other) const {
        return !operator==(other);
      }

      Cel* operator*() const {
        return m_cel;
      }

      iterator& operator++();

    private:
      Cel* m_cel;
      const SelectedFrames& m_selFrames;
      SelectedFrames::const_iterator m_frameIterator;
      Flags m_flags;
      std::set<ObjectId> m_visited;
    };

    iterator begin() { return m_begin; }
    iterator end() { return m_end; }

    int size() {
      int count = 0;
      for (auto it=begin(), e=end(); it!=e; ++it)
        ++count;
      return count;
    }

  private:
    SelectedFrames m_selFrames;
    iterator m_begin, m_end;
  };

} // namespace doc

#endif
