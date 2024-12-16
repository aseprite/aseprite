// Aseprite Document Library
// Copyright (c) 2020 Igara Studio S.A.
// Copyright (c) 2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/algorithm/fill_selection.h"

#include "doc/grid.h"
#include "doc/image_impl.h"
#include "doc/mask.h"
#include "doc/primitives.h"

namespace doc { namespace algorithm {

void fill_selection(Image* image,
                    const gfx::Rect& imageBounds,
                    const Mask* mask,
                    const color_t color,
                    const Grid* grid)
{
  ASSERT(mask);
  ASSERT(mask->bitmap());
  if (!mask || !mask->bitmap())
    return;

  const auto rc = (imageBounds & mask->bounds());
  if (rc.isEmpty())
    return; // <- There is no intersection between image bounds and mask bounds

  const LockImageBits<BitmapTraits> maskBits(mask->bitmap(), gfx::Rect(rc).offset(-mask->origin()));
  auto it = maskBits.begin();

  for (int v = 0; v < rc.h; ++v) {
    for (int u = 0; u < rc.w; ++u, ++it) {
      ASSERT(it != maskBits.end());
      if (*it) {
        gfx::Point pt(u + rc.x, v + rc.y);

        if (grid) {
          pt = grid->canvasToTile(pt);
        }
        else {
          pt -= imageBounds.origin();
        }

        // TODO use iterator
        put_pixel(image, pt.x, pt.y, color);
      }
    }
  }

  ASSERT(it == maskBits.end());
}

}} // namespace doc::algorithm
