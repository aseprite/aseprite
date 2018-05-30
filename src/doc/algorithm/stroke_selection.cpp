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

namespace doc {
namespace algorithm {

void stroke_selection(Image* image,
                      const gfx::Point& offset,
                      const Mask* origMask,
                      const color_t color)
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
  modify_selection(
    SelectionModifier::Border,
    origMask, &mask, 1,
    BrushType::kCircleBrushType);
  mask.unfreeze();

  // Both mask must have the same bounds.
  ASSERT(mask.bounds() == origMask->bounds());

  if (mask.bitmap())
    fill_selection(image, offset, &mask, color);
}

} // namespace algorithm
} // namespace app
