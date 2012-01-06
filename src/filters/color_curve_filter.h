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

#ifndef FILTERS_COLOR_CURVE_FILTER_H_INCLUDED
#define FILTERS_COLOR_CURVE_FILTER_H_INCLUDED

#include <vector>

#include "filters/filter.h"

class ColorCurve;

class ColorCurveFilter : public Filter
{
public:
  ColorCurveFilter();

  void setCurve(ColorCurve* curve);
  ColorCurve* getCurve() const { return m_curve; }

  // Filter implementation
  const char* getName();
  void applyToRgba(FilterManager* filterMgr);
  void applyToGrayscale(FilterManager* filterMgr);
  void applyToIndexed(FilterManager* filterMgr);

private:
  ColorCurve* m_curve;
  std::vector<int> m_cmap;
};

#endif
