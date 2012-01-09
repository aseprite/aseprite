/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef RASTER_MASK_H_INCLUDED
#define RASTER_MASK_H_INCLUDED

#include "gfx/rect.h"
#include "raster/gfxobj.h"
#include "raster/image.h"

#include <string>

// Represents the selection (selected pixels, 0/1, 0=non-selected, 1=selected)
class Mask : public GfxObj
{
public:
  Mask();
  Mask(const Mask& mask);
  Mask(int x, int y, Image* bitmap);
  virtual ~Mask();

  int getMemSize() const;

  void setName(const char *name);
  const std::string& getName() const { return m_name; }

  const Image* getBitmap() const { return m_bitmap; }
  Image* getBitmap() { return m_bitmap; }

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
            image_getpixel(m_bitmap, u-m_bounds.x, v-m_bounds.y));
  }

  const gfx::Rect& getBounds() const {
    return m_bounds;
  }

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
  void intersect(int x, int y, int w, int h);
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

#endif
