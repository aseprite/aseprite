// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_MASK_H_INCLUDED
#define DOC_MASK_H_INCLUDED
#pragma once

#include "gfx/rect.h"
#include "doc/image.h"
#include "doc/object.h"
#include "doc/primitives.h"

#include <string>

namespace doc {

  // Represents the selection (selected pixels, 0/1, 0=non-selected, 1=selected)
  class Mask : public Object {
  public:
    Mask();
    Mask(const Mask& mask);
    Mask(int x, int y, Image* bitmap);
    virtual ~Mask();

    int getMemSize() const;

    void setName(const char *name);
    const std::string& name() const { return m_name; }

    const Image* bitmap() const { return m_bitmap; }
    Image* bitmap() { return m_bitmap; }

    // Returns true if the mask is completely empty (i.e. nothing
    // selected)
    bool isEmpty() const {
      return (!m_bitmap ? true: false);
    }

    // Returns true if the point is inside the mask
    bool containsPoint(int u, int v) const {
      return (m_bitmap &&
              u >= m_bounds.x && u < m_bounds.x+m_bounds.w &&
              v >= m_bounds.y && v < m_bounds.y+m_bounds.h &&
              get_pixel(m_bitmap, u-m_bounds.x, v-m_bounds.y));
    }

    const gfx::Rect& bounds() const { return m_bounds; }

    void setOrigin(int x, int y) {
      m_bounds.x = x;
      m_bounds.y = y;
    }

    // These functions can be used to disable the automatic call to
    // "shrink" method (so you can do a lot of modifications without
    // lossing time shrinking the mask in each little operation).
    void freeze();
    void unfreeze();

    // Returns true if the mask is frozen (See freeze/unfreeze functions).
    bool isFrozen() const { return m_freeze_count > 0; }

    // Returns true if the mask is a rectangular region.
    bool isRectangular() const;

    // Clears the mask.
    void clear();

    // Copies the data from the given mask.
    void copyFrom(const Mask* sourceMask);

    // Replace the whole mask with the given region.
    void replace(int x, int y, int w, int h);
    void replace(const gfx::Rect& bounds);

    // Inverts the mask.
    void invert();

    // Adds the specified rectangle in the mask/selection
    void add(int x, int y, int w, int h);
    void add(const gfx::Rect& bounds);

    void subtract(int x, int y, int w, int h);
    void subtract(const gfx::Rect& bounds);
    void intersect(int x, int y, int w, int h);
    void intersect(const gfx::Rect& bounds);
    void byColor(const Image* image, int color, int fuzziness);
    void crop(const Image* image);

    // Reserves a rectangle to draw onto the bitmap (you should call
    // shrink after you draw in the bitmap)
    void reserve(int x, int y, int w, int h);

    // Shrinks all sides of the mask to the minimum possible looking at
    // empty pixels in the bitmap
    void shrink();

    // Displaces the mask bounds origin point.
    void offsetOrigin(int dx, int dy);

  private:
    void initialize();

    int m_freeze_count;
    std::string m_name;           // Mask name
    gfx::Rect m_bounds;           // Region bounds
    Image* m_bitmap;              // Bitmapped image mask
  };

} // namespace doc

#endif
