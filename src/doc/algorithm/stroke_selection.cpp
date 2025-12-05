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
#include "doc/brush_type.h"
#include "doc/mask.h"

namespace doc { namespace algorithm {

void stroke_selection(Image* image,
                      const gfx::Rect& imageBounds,
                      const Mask* origMask,
                      const color_t color,
                      const Grid* grid)
{
  stroke_selection(image,
                   imageBounds,
                   origMask,
                   color,
                   1,
                   "inside",
                   doc::BrushType::kCircleBrushType,
                   grid);
}

void stroke_selection(Image* image,
                      const gfx::Rect& imageBounds,
                      const Mask* origMask,
                      const color_t color,
                      int width,
                      const std::string& location,
                      doc::BrushType brushType,
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
  // width and location handling
  std::string effectiveLocation = location;
  if (width == 1 && effectiveLocation == "center") {
    // For 1px, directly draw outside
    effectiveLocation = "outside";
  }
  if (effectiveLocation == "center") {
    // center: draw outside once, then inside once, merge masks
    // Prioritize allocating more pixels to outside
    int halfOut = (width + 1) / 2; // outside多1
    int halfIn = width - halfOut;
    Mask outsideMask, insideMask;
    // outside
    if (halfOut > 0) {
      gfx::Rect extBounds = bounds;
      extBounds.inflate(halfOut * 2, halfOut * 2);
      extBounds.offset(-halfOut, -halfOut);
      if (extBounds.w > 0 && extBounds.h > 0) {
        Mask expandedMask;
        expandedMask.reserve(extBounds);
        expandedMask.freeze();
        modify_selection(SelectionModifier::Expand, origMask, &expandedMask, halfOut, brushType);
        expandedMask.unfreeze();
        if (expandedMask.bitmap()) {
          Mask borderMask;
          borderMask.reserve(extBounds);
          borderMask.freeze();
          modify_selection(SelectionModifier::Border, &expandedMask, &borderMask, width, brushType);
          borderMask.unfreeze();
          if (borderMask.bitmap()) {
            borderMask.subtract(*origMask);
            outsideMask.reserve(extBounds);
            outsideMask.copyFrom(&borderMask);
          }
        }
      }
    }
    // inside
    if (halfIn > 0 && bounds.w > 1 && bounds.h > 1) {
      // Only generate insideMask when the selection is large enough to prevent crashes
      Mask borderMask;
      borderMask.reserve(bounds);
      borderMask.freeze();
      modify_selection(SelectionModifier::Border, origMask, &borderMask, halfIn, brushType);
      borderMask.unfreeze();
      if (borderMask.bitmap()) {
        insideMask.reserve(bounds);
        insideMask.copyFrom(&borderMask);
      }
    }
    // Merge masks, unify regions first
    if (outsideMask.bitmap() && insideMask.bitmap()) {
      gfx::Rect unionBounds = outsideMask.bounds().createUnion(insideMask.bounds());
      Mask out2, in2;
      out2.reserve(unionBounds);
      in2.reserve(unionBounds);
      out2.copyFrom(&outsideMask);
      in2.copyFrom(&insideMask);
      out2.unfreeze();
      out2.add(in2);
      mask.reserve(unionBounds);
      mask.copyFrom(&out2);
    }
    else if (outsideMask.bitmap()) {
      mask.reserve(outsideMask.bounds());
      mask.copyFrom(&outsideMask);
    }
    else if (insideMask.bitmap()) {
      mask.reserve(insideMask.bounds());
      mask.copyFrom(&insideMask);
    }
    else {
      return;
    }
  }
  else if (effectiveLocation == "outside") {
    // Based on the selection, expand the selection, get the border, and subtract the selection
    // itself Expand width in all four directions to ensure the border algorithm is not clipped,
    // preserving corner pixels
    gfx::Rect extBounds = bounds;
    extBounds.inflate(width * 2, width * 2);
    extBounds.offset(-width, -width);
    if (extBounds.w <= 0 || extBounds.h <= 0)
      return;

    Mask expandedMask;
    expandedMask.reserve(extBounds);
    expandedMask.freeze();
    modify_selection(SelectionModifier::Expand, origMask, &expandedMask, width, brushType);
    expandedMask.unfreeze();

    Mask borderMask;
    borderMask.reserve(extBounds);
    borderMask.freeze();
    modify_selection(SelectionModifier::Border, &expandedMask, &borderMask, width, brushType);
    borderMask.unfreeze();

    borderMask.subtract(*origMask);
    mask.reserve(extBounds);
    mask.copyFrom(&borderMask);
  }
  else {
    // inside
    mask.reserve(bounds);
    mask.freeze();
    modify_selection(SelectionModifier::Border, origMask, &mask, width, brushType);
    mask.unfreeze();

    // Both mask must have the same bounds.
    ASSERT(mask.bounds() == origMask->bounds());
  }
  mask.unfreeze();

  if (mask.bitmap()) {
    fill_selection(image, imageBounds, &mask, color, grid);
  }
}

}} // namespace doc::algorithm
