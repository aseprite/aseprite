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

#ifndef RASTER_FRAME_NUMBER_H_INCLUDED
#define RASTER_FRAME_NUMBER_H_INCLUDED
#pragma once

namespace raster {

  class FrameNumber {
  public:
    FrameNumber() : m_value(0) { }
    explicit FrameNumber(int value) : m_value(value) { }

    FrameNumber next(int i = 1) const { return FrameNumber(m_value+i); };
    FrameNumber previous(int i = 1) const { return FrameNumber(m_value-i); };

    operator int() { return m_value; }
    operator const int() const { return m_value; }

    FrameNumber& operator=(const FrameNumber& o) { m_value = o.m_value; return *this; }
    FrameNumber& operator++() { ++m_value; return *this; }
    FrameNumber& operator--() { --m_value; return *this; }
    FrameNumber operator++(int) { FrameNumber old(*this); ++m_value; return old; }
    FrameNumber operator--(int) { FrameNumber old(*this); --m_value; return old; }
    bool operator<(const FrameNumber& o) const { return m_value < o.m_value; }
    bool operator>(const FrameNumber& o) const { return m_value > o.m_value; }
    bool operator<=(const FrameNumber& o) const { return m_value <= o.m_value; }
    bool operator>=(const FrameNumber& o) const { return m_value >= o.m_value; }
    bool operator==(const FrameNumber& o) const { return m_value == o.m_value; }
    bool operator!=(const FrameNumber& o) const { return m_value != o.m_value; }

  private:
    int m_value;
  };

  inline FrameNumber operator+(const FrameNumber& x, const FrameNumber& y) {
    return FrameNumber((int)x + (int)y);
  }

  inline FrameNumber operator-(const FrameNumber& x, const FrameNumber& y) {
    return FrameNumber((int)x - (int)y);
  }

} // namespace raster

#endif
