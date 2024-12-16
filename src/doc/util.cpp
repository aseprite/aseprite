// Aseprite Document Library
// Copyright (c) 2020-2024 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "doc/util.h"

#include "doc/image.h"
#include "doc/image_impl.h"
#include "doc/mask.h"
#include "doc/tileset.h"

namespace doc {

void fix_old_tileset(Tileset* tileset)
{
  // Check if the first tile is already the empty tile, in this
  // case we can use this tileset as a new tileset without any
  // conversion.
  if (tileset->size() > 0 && is_empty_image(tileset->get(0).get())) {
    tileset->setBaseIndex(1);
  }
  else {
    // Add the empty tile in the index = 0
    tileset->insert(0, tileset->makeEmptyTile());

    // The tile 1 will be displayed as tile 0 in the editor
    tileset->setBaseIndex(0);
  }
}

void fix_old_tilemap(Image* image,
                     const Tileset* tileset,
                     const tile_t tileIDMask,
                     const tile_t tileFlagsMask)
{
  int delta = (tileset->baseIndex() == 0 ? 1 : 0);

  // Convert old empty tile (0xffffffff) to new empty tile (index 0 = notile)
  transform_image<TilemapTraits>(image, [tileIDMask, tileFlagsMask, delta](color_t c) -> color_t {
    color_t res = c;
    if (c == 0xffffffff)
      res = notile;
    else
      res = (c & tileFlagsMask) | ((c & tileIDMask) + delta);
    return res;
  });
}

Mask make_aligned_mask(const Grid* grid, const Mask* mask)
{
  // Fact: the newBounds will be always larger or equal than oldBounds
  Mask maskOutput;
  if (mask->isFrozen()) {
    ASSERT(false);
    return maskOutput;
  }
  const gfx::Rect& oldBounds(mask->bounds());
  const gfx::Rect newBounds(grid->alignBounds(mask->bounds()));
  ASSERT(newBounds.w > 0 && newBounds.h > 0);
  ImageRef newBitmap;
  if (!mask->bitmap()) {
    maskOutput.replace(newBounds);
    return maskOutput;
  }

  newBitmap.reset(Image::create(IMAGE_BITMAP, newBounds.w, newBounds.h));
  maskOutput.freeze();
  maskOutput.reserve(newBounds);

  const LockImageBits<BitmapTraits> bits(mask->bitmap());
  typename LockImageBits<BitmapTraits>::const_iterator it = bits.begin();
  // We must travel thought the old bitmap and masking the new bitmap
  gfx::Point previousPoint(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
  for (int y = 0; y < oldBounds.h; ++y) {
    for (int x = 0; x < oldBounds.w; ++x, ++it) {
      ASSERT(it != bits.end());
      if (*it) {
        const gfx::Rect newBoundsTile = grid->alignBounds(
          gfx::Rect(oldBounds.x + x, oldBounds.y + y, 1, 1));
        if (previousPoint != newBoundsTile.origin()) {
          // Fill a tile region in the newBitmap
          fill_rect(maskOutput.bitmap(),
                    gfx::Rect(newBoundsTile.x - newBounds.x,
                              newBoundsTile.y - newBounds.y,
                              grid->tileSize().w,
                              grid->tileSize().h),
                    1);
          previousPoint = newBoundsTile.origin();
        }
      }
    }
  }
  maskOutput.unfreeze();
  return maskOutput;
}

} // namespace doc
