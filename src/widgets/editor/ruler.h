/* ASEPRITE
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

#ifndef WIDGETS_EDITOR_RULER_H_INCLUDED
#define WIDGETS_EDITOR_RULER_H_INCLUDED

// A ruler inside the editor. It is used by SelectBoxState to show
// rulers that can be dragged by the user.
class Ruler
{
public:
  enum Orientation { Horizontal, Vertical };

  Ruler()
    : m_orientation(Horizontal)
    , m_position(0)
  {
  }

  Ruler(Orientation orientation, int position)
    : m_orientation(orientation)
    , m_position(position)
  {
  }

  Orientation getOrientation() const
  {
    return m_orientation;
  }

  int getPosition() const
  {
    return m_position;
  }

  void setPosition(int position)
  {
    m_position = position;
  }

private:
  Orientation m_orientation;
  int m_position;
};

#endif // WIDGETS_EDITOR_RULER_H_INCLUDED
