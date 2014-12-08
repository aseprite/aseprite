/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#ifndef APP_ZOOM_H_INCLUDED
#define APP_ZOOM_H_INCLUDED
#pragma once

#include "gfx/rect.h"

namespace app {

  class Zoom {
  public:
    Zoom(int num, int den)
      : m_num(num), m_den(den) {
    }

    double scale() const { return static_cast<double>(m_num) / static_cast<double>(m_den); }

    int apply(int x) const { return x * m_num / m_den; }
    int remove(int x) const { return x * m_den / m_num; }

    double apply(double x) const { return x * m_num / m_den; }
    double remove(double x) const { return x * m_den / m_num; }

    gfx::Rect apply(const gfx::Rect& r) const {
      return gfx::Rect(
        apply(r.x), apply(r.y),
        apply(r.x+r.w) - apply(r.x),
        apply(r.y+r.h) - apply(r.y));
    }
    gfx::Rect remove(const gfx::Rect& r) const {
      return gfx::Rect(
        remove(r.x), remove(r.y),
        remove(r.x+r.w) - remove(r.x),
        remove(r.y+r.h) - remove(r.y));
    }

    void in();
    void out();

    bool operator==(const Zoom& other) const {
      return m_num == other.m_num && m_den == other.m_den;
    }

    bool operator!=(const Zoom& other) const {
      return !operator==(other);
    }

  private:
    int m_num;
    int m_den;
  };

} // namespace app

#endif // APP_ZOOM_H_INCLUDED
