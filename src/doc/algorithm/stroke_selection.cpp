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
#include "ui/alert.h"

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
  // width 参数用于描边宽度
  if (location == "center") {
    // 四边各向内外扩展 width/2，最少1px
    int half = std::max(width / 2, 1);
    Mask tempMask;
    tempMask.reserve(bounds);
    tempMask.freeze();

    // 向外扩展
    modify_selection(SelectionModifier::Expand,
                     origMask,
                     &tempMask,
                     half,
                     BrushType::kCircleBrushType);
    modify_selection(SelectionModifier::Border, &mask, &mask, width, BrushType::kCircleBrushType);
  }
  else if (location == "outside") {
    // 以选区为基准，扩展选区，取边缘，扣除选区本身
    // 四向各扩展width，保证border算法不被裁剪，拐角像素保留
    gfx::Rect extBounds = bounds;
    extBounds.inflate(width * 2, width * 2);
    extBounds.offset(-width, -width);
    if (extBounds.w <= 0 || extBounds.h <= 0)
      return;

    Mask expandedMask;
    expandedMask.reserve(extBounds);
    expandedMask.freeze();
    modify_selection(SelectionModifier::Expand,
                     origMask,
                     &expandedMask,
                     width,
                     BrushType::kCircleBrushType);
    expandedMask.unfreeze();

    Mask borderMask;
    borderMask.reserve(extBounds);
    borderMask.freeze();
    modify_selection(SelectionModifier::Border,
                     &expandedMask,
                     &borderMask,
                     width,
                     BrushType::kCircleBrushType);
    borderMask.unfreeze();

    borderMask.subtract(*origMask);
    mask.reserve(extBounds);
    mask.copyFrom(&borderMask);

    // 调试输出
    std::string msg3 = "rect mask bounds: " + std::to_string(mask.bounds().x) + "," +
                       std::to_string(mask.bounds().y) + "," + std::to_string(mask.bounds().w) +
                       "," + std::to_string(mask.bounds().h) + "\n" +
                       "rect mask empty: " + (mask.bounds().isEmpty() ? "true" : "false");
    ui::AlertPtr alert3 = ui::Alert::create("title");
    alert3->addLabel(msg3, ui::LEFT);
    alert3->show();
  }
  else {
    // inside
    modify_selection(SelectionModifier::Border, origMask, &mask, width, BrushType::kCircleBrushType);
  }
  mask.unfreeze();

  if (mask.bitmap()) {
    ui::Alert::show("is bitmap");
    fill_selection(image, imageBounds, &mask, color, grid);
  }
}

}} // namespace doc::algorithm
