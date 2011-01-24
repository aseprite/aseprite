/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

// Property to show widgets with a special look (e.g.: buttons or sliders with mini-borders)
class SkinProperty : public Property
{
public:
  static const char* SkinPropertyName;

  SkinProperty();
  ~SkinProperty();

  bool isMiniLook() const;
  void setMiniLook(bool state);

  int getUpperLeft() const;
  int getUpperRight() const;
  int getLowerLeft() const;
  int getLowerRight() const;

  void setUpperLeft(int value);
  void setUpperRight(int value);
  void setLowerLeft(int value);
  void setLowerRight(int value);

private:
  bool m_isMiniLook;
  int m_upperLeft;
  int m_upperRight;
  int m_lowerLeft;
  int m_lowerRight;
};

#endif
