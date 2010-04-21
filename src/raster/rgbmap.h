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

#ifndef RASTER_RGBMAP_H_INCLUDED
#define RASTER_RGBMAP_H_INCLUDED

#include "Vaca/NonCopyable.h"
#include "raster/gfxobj.h"

class Palette;

class RgbMap : public GfxObj
	     , Vaca::NonCopyable
{
public:
  RgbMap();
  virtual ~RgbMap();

  bool match(const Palette* palette) const;
  void regenerate(const Palette* palette);

  int mapColor(int r, int g, int b) const;

private:
  class RgbMapImpl* m_impl;
};

#endif
