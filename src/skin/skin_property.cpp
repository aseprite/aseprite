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

#include "config.h"

#include "skin/skin_property.h"

const char* SkinProperty::SkinPropertyName = "SkinProperty";

SkinProperty::SkinProperty()
  : Property(SkinPropertyName)
{
  m_isMiniLook = false;
  m_upperLeft = 0;
  m_upperRight = 0;
  m_lowerLeft = 0;
  m_lowerRight = 0;
}

SkinProperty::~SkinProperty()
{
}

bool SkinProperty::isMiniLook() const
{
  return m_isMiniLook;
}

void SkinProperty::setMiniLook(bool state)
{
  m_isMiniLook = state;
}

int SkinProperty::getUpperLeft() const
{
  return m_upperLeft;
}

int SkinProperty::getUpperRight() const
{
  return m_upperRight;
}

int SkinProperty::getLowerLeft() const
{
  return m_lowerLeft;
}

int SkinProperty::getLowerRight() const
{
  return m_lowerRight;
}

void SkinProperty::setUpperLeft(int value)
{
  m_upperLeft = value;
}

void SkinProperty::setUpperRight(int value)
{
  m_upperRight = value;
}

void SkinProperty::setLowerLeft(int value)
{
  m_lowerLeft = value;
}

void SkinProperty::setLowerRight(int value)
{
  m_lowerRight = value;
}
