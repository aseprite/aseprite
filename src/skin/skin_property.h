/* ASE - Allegro Sprite Editor
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

#ifndef SKIN_PROPERTY_H_INCLUDED
#define SKIN_PROPERTY_H_INCLUDED

#include "gui/property.h"

enum LookType {
  NormalLook,
  MiniLook,
  WithoutBordersLook,
  LeftButtonLook,
  RightButtonLook,
};

// Property to show widgets with a special look (e.g.: buttons or sliders with mini-borders)
class SkinProperty : public Property
{
public:
  static const char* SkinPropertyName;

  SkinProperty();
  ~SkinProperty();

  LookType getLook() const { return m_look; }
  void setLook(LookType look) { m_look = look; }

  int getUpperLeft() const { return m_upperLeft; }
  int getUpperRight() const { return m_upperRight; }
  int getLowerLeft() const { return m_lowerLeft; }
  int getLowerRight() const { return m_lowerRight; }

  void setUpperLeft(int value) { m_upperLeft = value; }
  void setUpperRight(int value) { m_upperRight = value; }
  void setLowerLeft(int value) { m_lowerLeft = value; }
  void setLowerRight(int value) { m_lowerRight = value; }

private:
  LookType m_look;
  int m_upperLeft;
  int m_upperRight;
  int m_lowerLeft;
  int m_lowerRight;
};

#endif
