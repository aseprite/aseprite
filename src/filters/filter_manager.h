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

#ifndef FILTERS_FILTER_MANAGER_H_INCLUDED
#define FILTERS_FILTER_MANAGER_H_INCLUDED

#include "filters/target.h"

class FilterIndexedData;
class Image;

// Information given to a filter (Filter interface) to apply it to a
// single row. Basically an Filter implementation has to obtain
// colors from getSourceAddress(), applies some kind of transformation
// to that color, and save the result in getDestinationAddress().
// This process must be repeated getWidth() times.
class FilterManager
{
public:
  virtual ~FilterManager() { }

  // Gets the address of the first pixel which has the original color
  // to apply the filter.
  virtual const void* getSourceAddress() = 0;

  // Gets the address of the first pixel which is destination of the
  // filter.
  virtual void* getDestinationAddress() = 0;

  // Returns the width of the row to apply the filter. You must apply
  // the Filter "getWidth()" times, in each pixel from getSourceAddress().
  virtual int getWidth() = 0;

  // Returns the target of the Filter, i.e. what channels/components
  // (e.g. Red, Green, or Blue) will be modified by the filter.
  virtual Target getTarget() = 0;

  // Returns a interface needed by filters which operate over indexed
  // images. FilterIndexedData interface provides a Palette and a
  // RgbMap to help the filter to make its job.
  virtual FilterIndexedData* getIndexedData() = 0;

  // Returns true if you should skip the current pixel (do not apply
  // the filter). You must increment all your internal source and
  // destination address pointers one pixel without applying the
  // filter.
  //
  // This method is used to skip non-selected pixels (when the
  // selection is actived).
  virtual bool skipPixel() = 0;

  //////////////////////////////////////////////////////////////////////
  // Special members for 2D filters like convolution matrices.

  // Returns the source image.
  virtual const Image* getSourceImage() = 0;

  // Returns the first X coordinate of the row to apply the filter.
  virtual int getX() = 0;

  // Returns the Y coordinate of the row.
  virtual int getY() = 0;

};

#endif
