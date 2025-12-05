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
  std::string effectiveLocation = location;
  if (width == 1) {
    // 1px时直接画outside
    effectiveLocation = "outside";
  }
  if (effectiveLocation == "center") {
    // center: 先画一次outside，再画一次inside，合并mask
    // 优先分配更多像素给outside
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
        modify_selection(SelectionModifier::Expand,
                         origMask,
                         &expandedMask,
                         halfOut,
                         BrushType::kCircleBrushType);
        expandedMask.unfreeze();
        if (expandedMask.bitmap()) {
          Mask borderMask;
          borderMask.reserve(extBounds);
          borderMask.freeze();
          modify_selection(SelectionModifier::Border,
                           &expandedMask,
                           &borderMask,
                           width,
                           BrushType::kCircleBrushType);
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
      // 只有选区足够大时才生成 insideMask，防止崩溃
      Mask borderMask;
      borderMask.reserve(bounds);
      borderMask.freeze();
      modify_selection(SelectionModifier::Border,
                       origMask,
                       &borderMask,
                       halfIn,
                       BrushType::kCircleBrushType);
      borderMask.unfreeze();
      if (borderMask.bitmap()) {
        insideMask.reserve(bounds);
        insideMask.copyFrom(&borderMask);
      }
    }
    // 合并mask，先统一区域
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
    } else if (outsideMask.bitmap()) {
      mask.reserve(outsideMask.bounds());
      mask.copyFrom(&outsideMask);
    } else if (insideMask.bitmap()) {
      mask.reserve(insideMask.bounds());
      mask.copyFrom(&insideMask);
    } else {
      return;
    }
    // 调试输出
    std::string msg3 = "rect mask bounds: " + std::to_string(mask.bounds().x) + "," +
                       std::to_string(mask.bounds().y) + "," + std::to_string(mask.bounds().w) +
                       "," + std::to_string(mask.bounds().h) + "\n" +
                       "rect mask empty: " + (mask.bounds().isEmpty() ? "true" : "false");
    ui::AlertPtr alert3 = ui::Alert::create("title");
    alert3->addLabel(msg3, ui::LEFT);
  }
  else if (effectiveLocation == "outside") {
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
