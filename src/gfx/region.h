// Aseprite Gfx Library
// Copyright (C) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef GFX_REGION_H_INCLUDED
#define GFX_REGION_H_INCLUDED
#pragma once

#include "gfx/rect.h"
#include <vector>
#include <iterator>

namespace gfx {

  template<typename T> class PointT;

  class Region;

  namespace details {

#ifdef PIXMAN_VERSION_MAJOR
    typedef struct pixman_box32 Box;
    typedef struct pixman_region32 Region;
#else
    struct Box {
      int32_t x1, y1, x2, y2;
    };
    struct Region {
      Box extents;
      void* data;
    };
#endif

    template<typename T>
    class RegionIterator : public std::iterator<std::forward_iterator_tag, T> {
    public:
      typedef typename std::iterator<std::forward_iterator_tag, T>::reference reference;

      RegionIterator() : m_ptr(NULL) { }
      RegionIterator(const RegionIterator& o) : m_ptr(o.m_ptr) { }
      template<typename T2>
      RegionIterator(const RegionIterator<T2>& o) : m_ptr(o.m_ptr) { }
      RegionIterator& operator=(const RegionIterator& o) { m_ptr = o.m_ptr; return *this; }
      RegionIterator& operator++() { ++m_ptr; return *this; }
      RegionIterator operator++(int) { RegionIterator o(*this); ++m_ptr; return o; }
      bool operator==(const RegionIterator& o) const { return m_ptr == o.m_ptr; }
      bool operator!=(const RegionIterator& o) const { return m_ptr != o.m_ptr; }
      reference operator*() {
        m_rect.x = m_ptr->x1;
        m_rect.y = m_ptr->y1;
        m_rect.w = m_ptr->x2 - m_ptr->x1;
        m_rect.h = m_ptr->y2 - m_ptr->y1;
        return m_rect;
      }
    private:
      Box* m_ptr;
      mutable Rect m_rect;
      template<typename> friend class RegionIterator;
      friend class ::gfx::Region;
    };

  } // namespace details

  class Region {
  public:
    enum Overlap { Out, In, Part };

    typedef details::RegionIterator<Rect> iterator;
    typedef details::RegionIterator<const Rect> const_iterator;

    Region();
    Region(const Region& copy);
    explicit Region(const Rect& rect);
    Region& operator=(const Rect& rect);
    Region& operator=(const Region& copy);
    ~Region();

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

    bool isEmpty() const;
    Rect bounds() const;
    std::size_t size() const;

    void clear();

    void offset(int dx, int dy);
    void offset(const PointT<int>& delta);

    Region& createIntersection(const Region& a, const Region& b);
    Region& createUnion(const Region& a, const Region& b);
    Region& createSubtraction(const Region& a, const Region& b);

    bool contains(const PointT<int>& pt) const;
    Overlap contains(const Rect& rect) const;

    Rect operator[](int i);
    const Rect operator[](int i) const;

    Region& operator+=(const Region& b) { return createUnion(*this, b); }
    Region& operator|=(const Region& b) { return createUnion(*this, b); }
    Region& operator&=(const Region& b) { return createIntersection(*this, b); }
    Region& operator-=(const Region& b) { return createSubtraction(*this, b); }

  private:
    mutable details::Region m_region;
  };

} // namespace gfx

#endif
