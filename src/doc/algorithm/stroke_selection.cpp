// Aseprite Document Library
// Copyright (c) 2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/algorithm/stroke_selection.h"

#include "doc/algorithm/fill_selection.h"
#include "doc/algorithm/modify_selection.h"
#include "doc/mask.h"

namespace doc { namespace algorithm {


void stroke_selection(Image* image,
                      const gfx::Rect& imageBounds,
                      const Mask* origMask,
                      const color_t color,
                      const Grid* grid)
{
  stroke_selection(image, imageBounds, origMask, color, 1, "inside", grid);
}

void stroke_selection(Image* image,
                      const gfx::Rect& imageBounds,
                      const Mask* origMask,
                      const color_t color,
                      int width,
                      const std::string& location,
                      const Grid* grid)
{
  ASSERT(origMask);
  ASSERT(origMask->bitmap());
  if (!origMask || !origMask->bitmap())
    return;

  gfx::Rect bounds = origMask->bounds();
  if (bounds.isEmpty())
    return;

  Mask mask;
  mask.reserve(bounds);
  mask.freeze();
  // width 参数用于描边宽度
  modify_selection(SelectionModifier::Border, origMask, &mask, width, BrushType::kCircleBrushType);
  mask.unfreeze();

  // TODO: location 参数可用于后续算法扩展（目前未实现偏移逻辑）

  ASSERT(mask.bounds() == origMask->bounds());

  if (mask.bitmap())
    fill_selection(image, imageBounds, &mask, color, grid);
}

}} // namespace doc::algorithm
