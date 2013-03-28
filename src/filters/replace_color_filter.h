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

#ifndef FILTERS_REPLACE_COLOR_FILTER_H_INCLUDED
#define FILTERS_REPLACE_COLOR_FILTER_H_INCLUDED

#include "filters/filter.h"

class ReplaceColorFilter : public Filter
{
public:
  ReplaceColorFilter();

  void setFrom(int from);
  void setTo(int to);
  void setTolerance(int tolerance);

  int getFrom() const { return m_from; }
  int getTo() const { return m_to; }
  int getTolerance() const { return m_tolerance; }

  // Filter implementation
  const char* getName();
  void applyToRgba(FilterManager* filterMgr);
  void applyToGrayscale(FilterManager* filterMgr);
  void applyToIndexed(FilterManager* filterMgr);

private:
  int m_from;
  int m_to;
  int m_tolerance;
};

#endif
