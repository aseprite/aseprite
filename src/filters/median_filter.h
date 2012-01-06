/* ASEPRITE
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

#ifndef FILTERS_MEDIAN_FILTER_PROCESS_H_INCLUDED
#define FILTERS_MEDIAN_FILTER_PROCESS_H_INCLUDED

#include <vector>

#include "filters/filter.h"
#include "filters/tiled_mode.h"

class MedianFilter : public Filter
{
public:
  MedianFilter();

  void setTiledMode(TiledMode tiled);
  void setSize(int width, int height);

  TiledMode getTiledMode() const { return m_tiledMode; }
  int getWidth() const { return m_width; }
  int getHeight() const { return m_height; }

  // Filter implementation
  const char* getName();
  void applyToRgba(FilterManager* filterMgr);
  void applyToGrayscale(FilterManager* filterMgr);
  void applyToIndexed(FilterManager* filterMgr);

private:
  TiledMode m_tiledMode;
  int m_width;
  int m_height;
  int m_ncolors;
  std::vector<std::vector<uint8_t> > m_channel;
};

#endif
