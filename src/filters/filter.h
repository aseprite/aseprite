/* Aseprite
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

#ifndef FILTERS_FILTER_H_INCLUDED
#define FILTERS_FILTER_H_INCLUDED
#pragma once

namespace filters {

  class FilterManager;

  // Interface which applies a filter to a sprite given a FilterManager
  // which indicates where we have to apply the filter.
  class Filter {
  public:
    virtual ~Filter() { }

    // Returns a proper name for the filter. It is used to show a label
    // with the Undo action.
    virtual const char* getName() = 0;

    // Applies the filter to one RGBA row. You must use
    // FilterManager::getSourceAddress() and advance 32 bits to modify
    // each pixel.
    virtual void applyToRgba(FilterManager* filterMgr) = 0;

    // Applies the filter to one grayscale row. You must use
    // FilterManager::getSourceAddress() and advance 16 bits to modify
    // each pixel.
    virtual void applyToGrayscale(FilterManager* filterMgr) = 0;

    // Applies the filter to one indexed row. You must use
    // FilterManager::getSourceAddress() and advance 8 bits to modify
    // each pixel.
    virtual void applyToIndexed(FilterManager* filterMgr) = 0;

  };

} // namespace filters

#endif
