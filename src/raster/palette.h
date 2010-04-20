/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#ifndef RASTER_PALETTE_H_INCLUDED
#define RASTER_PALETTE_H_INCLUDED

#include "raster/gfxobj.h"
#include <allegro/color.h>
#include <vector>
#include <cassert>

class Palette : public GfxObj
{
public:
  Palette(int frame, size_t ncolors);
  Palette(const Palette& palette);
  ~Palette();

  static Palette* createGrayscale();

  size_t size() const { return m_colors.size(); }
  void resize(size_t ncolors);

  size_t getModifications() const { return m_modifications; }

  int getFrame() const { return m_frame; }
  void setFrame(int frame);

  ase_uint32 getEntry(size_t i) const {
    assert(i >= 0 && i < size());
    return m_colors[i];
  }

  void setEntry(size_t i, ase_uint32 color);

  void copyColorsTo(Palette* dst) const;

  int countDiff(const Palette* other, int* from, int* to) const;

  void makeBlack();
  void makeHorzRamp(int from, int to);
  void makeVertRamp(int from, int to, int columns);
  void makeRectRamp(int from, int to, int columns);

  void toAllegro(RGB* rgb) const;
  void fromAllegro(const RGB* rgb);

  static Palette* load(const char *filename);
  bool save(const char *filename) const;

  int findBestfit(int r, int g, int b) const;

private:
  int m_frame;
  std::vector<ase_uint32> m_colors;
  size_t m_modifications;
};

#endif
