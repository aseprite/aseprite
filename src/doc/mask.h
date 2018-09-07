// Aseprite Document Library
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_MASK_H_INCLUDED
#define DOC_MASK_H_INCLUDED
#pragma once

#include "doc/image.h"
#include "doc/image_buffer.h"
#include "doc/image_ref.h"
#include "doc/object.h"
#include "doc/primitives.h"
#include "gfx/rect.h"

#include <string>

namespace doc {

  // Represents the selection (selected pixels, 0/1, 0=non-selected, 1=selected)
  //
  // TODO rename Mask -> Selection
  class Mask : public Object {
  public:
    Mask();
    Mask(const Mask& mask);
    virtual ~Mask();

    virtual int getMemSize() const override;

    void setName(const char *name);
    const std::string& name() const { return m_name; }

    const Image* bitmap() const { return m_bitmap.get(); }
    Image* bitmap() { return m_bitmap.get(); }

    // Returns true if the mask is completely empty (i.e. nothing
    // selected)
    bool isEmpty() const {
      return (!m_bitmap ? true: false);
    }

    // Returns true if the point is inside the mask
    bool containsPoint(int u, int v) const {
      return (m_bitmap.get() &&
              u >= m_bounds.x && u < m_bounds.x+m_bounds.w &&
              v >= m_bounds.y && v < m_bounds.y+m_bounds.h &&
              get_pixel(m_bitmap.get(), u-m_bounds.x, v-m_bounds.y));
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
    void replace(const gfx::Rect& bounds);
    void replace(const doc::Mask& sourceMask) { copyFrom(&sourceMask); }

    // Inverts the mask.
    void invert();

    void add(const doc::Mask& mask);
    void subtract(const doc::Mask& mask);
    void intersect(const doc::Mask& mask);

    void add(const gfx::Rect& bounds);
    void subtract(const gfx::Rect& bounds);
    void intersect(const gfx::Rect& bounds);

    void byColor(const Image* image, int color, int fuzziness);
    void crop(const Image* image);

    // Reserves a rectangle to draw onto the bitmap (you should call
    // shrink after you draw in the bitmap)
    void reserve(const gfx::Rect& bounds);

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
    ImageRef m_bitmap;            // Bitmapped image mask
    ImageBufferPtr m_buffer;      // Buffer used in m_bitmap

    Mask& operator=(const Mask& mask);
  };

} // namespace doc

#endif
