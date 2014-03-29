/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifndef RASTER_LAYER_INDEX_H_INCLUDED
#define RASTER_LAYER_INDEX_H_INCLUDED
#pragma once

namespace raster {

  class LayerIndex {
  public:
    LayerIndex() : m_value(0) { }
    explicit LayerIndex(int value) : m_value(value) { }

    LayerIndex next(int i = 1) const { return LayerIndex(m_value+i); };
    LayerIndex previous(int i = 1) const { return LayerIndex(m_value-i); };

    operator int() { return m_value; }
    operator const int() const { return m_value; }

    LayerIndex& operator=(const LayerIndex& o) { m_value = o.m_value; return *this; }
    LayerIndex& operator++() { ++m_value; return *this; }
    LayerIndex& operator--() { --m_value; return *this; }
    LayerIndex operator++(int) { LayerIndex old(*this); ++m_value; return old; }
    LayerIndex operator--(int) { LayerIndex old(*this); --m_value; return old; }
    bool operator<(const LayerIndex& o) const { return m_value < o.m_value; }
    bool operator>(const LayerIndex& o) const { return m_value > o.m_value; }
    bool operator<=(const LayerIndex& o) const { return m_value <= o.m_value; }
    bool operator>=(const LayerIndex& o) const { return m_value >= o.m_value; }
    bool operator==(const LayerIndex& o) const { return m_value == o.m_value; }
    bool operator!=(const LayerIndex& o) const { return m_value != o.m_value; }

  private:
    int m_value;
  };

  inline LayerIndex operator+(const LayerIndex& x, const LayerIndex& y) {
    return LayerIndex((int)x + (int)y);
  }

  inline LayerIndex operator-(const LayerIndex& x, const LayerIndex& y) {
    return LayerIndex((int)x - (int)y);
  }

} // namespace raster

#endif
