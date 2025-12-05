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
  mask.reserve(bounds);
  mask.freeze();
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
    int imgW = imageBounds.w;
    int imgH = imageBounds.h;
    int rectW = mask.bounds().w + (2 * width);
    int rectH = mask.bounds().h + (2 * width);
    int startX = imageBounds.x + ((imgW - rectW) / 2);
    int startY = imageBounds.y + ((imgH - rectH) / 2);

    mask.reserve(imageBounds);
    mask.freeze();
    auto* bmp = mask.bitmap();
    for (int y = startY; y < startY + rectH; ++y) {
      for (int x = startX; x < startX + rectW; ++x) {
        bmp->putPixel(x, y, 1);
      }
    }
    mask.unfreeze();

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
