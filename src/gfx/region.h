// Aseprite Gfx Library
// Copyright (C) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef GFX_REGION_H_INCLUDED
#define GFX_REGION_H_INCLUDED

#include "gfx/rect.h"
#include <vector>
#include <iterator>

namespace gfx {

  template<typename T> class PointT;

  class Region;

  namespace details {

    struct Box {
      int x1, y1, x2, y2;

      operator Rect() const {
        return Rect(x1, y1, x2-x1, y2-y1);
      }
    };

    struct RegionData {
      long size;
      long numRects;
      // From here this struct has an array of rectangles (Box[size])
    };

    struct Region {
      Box extents;
      RegionData* data;
    };

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
      reference operator*() { m_rect = *m_ptr; return m_rect; }
    private:
      Box* m_ptr;
      Rect m_rect;
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
    Rect getBounds() const;
    size_t size() const;

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

  private:
    mutable details::Region m_region;
  };

} // namespace gfx

#endif
