// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef FILTERS_FILTER_MANAGER_H_INCLUDED
#define FILTERS_FILTER_MANAGER_H_INCLUDED
#pragma once

#include "base/task.h"
#include "doc/pixel_format.h"
#include "filters/target.h"

// Creates src_address, dst_address, x, x2, and y variables to iterate
// through a row of the target. Skips non-selected areas.
// Requires the "filterMgr" variable.
#define FILTER_LOOP_THROUGH_ROW_BEGIN(Type)                             \
  const Target target = filterMgr->getTarget();                         \
  auto src_address = (const Type*)filterMgr->getSourceAddress();        \
  auto dst_address = (Type*)filterMgr->getDestinationAddress();         \
  int x = filterMgr->x();                                               \
  const int x2 = x + filterMgr->getWidth();                             \
  [[maybe_unused]] const int y = filterMgr->y();                        \
  auto& token = filterMgr->taskToken();                                 \
  for (; x < x2 && !token.canceled();                                   \
       ++x, ++src_address, ++dst_address) {                             \
    if (filterMgr->skipPixel())                                         \
      continue;

#define FILTER_LOOP_THROUGH_ROW_END()                                   \
  }

namespace doc {
  class Image;
}

namespace filters {

  class FilterIndexedData;

  // Information given to a filter (Filter interface) to apply it to a
  // single row. Basically an Filter implementation has to obtain
  // colors from getSourceAddress(), applies some kind of transformation
  // to that color, and save the result in getDestinationAddress().
  // This process must be repeated getWidth() times.
  class FilterManager {
  public:
    virtual ~FilterManager() { }

    virtual doc::PixelFormat pixelFormat() const = 0;

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
    virtual const doc::Image* getSourceImage() = 0;

    // Returns the first X coordinate of the row to apply the filter.
    virtual int x() const = 0;

    // Returns the Y coordinate of the row.
    virtual int y() const = 0;

    // Returns true if this is the first row. Useful to apply a filter
    // to the palette in the first row.
    virtual bool isFirstRow() const = 0;

    // Returns true if the mask is actived (so we are applying the
    // filter just to a part of the sprite).
    virtual bool isMaskActive() const = 0;

    // Task token used to cancel the filter operation (the filter can
    // check this each X pixels to know if the user canceled the
    // operation)
    virtual base::task_token& taskToken() const = 0;
  };

} // namespace filters

#endif
