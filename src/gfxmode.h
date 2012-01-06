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

#ifndef GFXMODE_H_INCLUDED
#define GFXMODE_H_INCLUDED

// Holds settings about a graphics mode.
class GfxMode
{
public:
  GfxMode();

  void updateWithCurrentMode();
  bool setGfxMode() const;

  int getCard() const { return m_card; }
  int getWidth() const { return m_width; }
  int getHeight() const { return m_height; }
  int getDepth() const { return m_depth; }
  int getScaling() const { return m_scaling; }

  void setCard(int card) { m_card = card; }
  void setWidth(int width) { m_width = width; }
  void setHeight(int height) { m_height = height; }
  void setDepth(int depth) { m_depth = depth; }
  void setScaling(int scaling) { m_scaling = scaling; }

private:
  int m_card;
  int m_width, m_height;
  int m_depth;
  int m_scaling;
};

// Tries a new graphics mode, and restores it back in the
// destructor. If the keep() method is used, the mode is not
// restored.
class CurrentGfxModeGuard
{
public:
  // Saves the current graphics mode as "the original" one.
  CurrentGfxModeGuard();

  // Restores the graphics mode to its original in case we have used
  // tryGfxMode() and we did not call keep(). After calling keep(),
  // this destructor will do just nothing.
  ~CurrentGfxModeGuard();

  // Changes the current graphics mode. Returns true if the mode was
  // changed successfully. If the function cannot change the new mode
  // and cannot restore the original one, it finishes the program
  // (with exit()).
  bool tryGfxMode(const GfxMode& newMode);

  // Keeps the last graphics mode set with tryGfxMode(). Indicates
  // that the destructor should not restore the graphics mode to the
  // original one.
  void keep();

  // It is the original graphics mode configured when the constructor
  // of this class was called.
  const GfxMode& getOriginal() const;

private:
  // The original gfx mode.
  GfxMode m_oldMode;

  // True if we should keep any mode set with tryGfxMode() method.
  bool m_keep;
};

#endif
