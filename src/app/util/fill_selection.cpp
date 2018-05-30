// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/util/fill_selection.h"

#include "doc/image_impl.h"
#include "doc/mask.h"
#include "doc/primitives.h"

namespace app {

using namespace doc;

void fill_selection(Image* image,
                    const gfx::Point& offset,
                    const Mask* mask,
                    const color_t color)
{
  ASSERT(mask->bitmap());
  if (!mask->bitmap())
    return;

  const LockImageBits<BitmapTraits> maskBits(mask->bitmap());
  LockImageBits<BitmapTraits>::const_iterator it = maskBits.begin();

  const gfx::Rect maskBounds = mask->bounds();
  for (int v=0; v<maskBounds.h; ++v) {
    for (int u=0; u<maskBounds.w; ++u, ++it) {
      ASSERT(it != maskBits.end());
      if (*it) {
        put_pixel(image,
                  u + offset.x,
                  v + offset.y, color);
      }
    }
  }

  ASSERT(it == maskBits.end());
}

} // namespace app
