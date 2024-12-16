// Aseprite
// Copyright (C) 2019-2021 Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/util/new_image_from_mask.h"

#include "app/doc.h"
#include "app/site.h"
#include "doc/image_impl.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/primitives.h"
#include "render/render.h"

namespace app {

using namespace doc;

Image* new_image_from_mask(const Site& site, const bool newBlend)
{
  const Mask* srcMask = site.document()->mask();
  return new_image_from_mask(site, srcMask, newBlend);
}

doc::Image* new_image_from_mask(const Site& site,
                                const doc::Mask* srcMask,
                                const bool newBlend,
                                bool merged)
{
  const Sprite* srcSprite = site.sprite();
  ASSERT(srcSprite);
  ASSERT(srcMask);

  const Image* srcMaskBitmap = srcMask->bitmap();
  const gfx::Rect& srcBounds = srcMask->bounds();

  ASSERT(srcMaskBitmap);
  ASSERT(!srcBounds.isEmpty());

  std::unique_ptr<Image> dst(Image::create(srcSprite->pixelFormat(), srcBounds.w, srcBounds.h));
  if (!dst)
    return nullptr;

  // Clear the new image
  dst->setMaskColor(srcSprite->transparentColor());
  clear_image(dst.get(), dst->maskColor());

  const Image* src = nullptr;
  int x = 0, y = 0;
  if (merged || site.layer()->isTilemap()) {
    render::Render render;
    render.setNewBlend(newBlend);
    if (merged)
      render.renderSprite(dst.get(), srcSprite, site.frame(), gfx::Clip(0, 0, srcBounds));
    else {
      ASSERT(site.layer()->isTilemap());
      if (auto cel = site.cel()) {
        render.renderCel(dst.get(),
                         cel,
                         srcSprite,
                         cel->image(),
                         cel->layer(),
                         srcSprite->palette(cel->frame()),
                         cel->bounds(),
                         gfx::Clip(0, 0, srcBounds),
                         255,
                         BlendMode::NORMAL);
      }
    }

    src = dst.get();
  }
  else {
    src = site.image(&x, &y);
  }

  // Copy the masked zones
  if (src) {
    if (srcMaskBitmap) {
      // Copy active layer with mask
      const LockImageBits<BitmapTraits> maskBits(srcMaskBitmap,
                                                 gfx::Rect(0, 0, srcBounds.w, srcBounds.h));
      LockImageBits<BitmapTraits>::const_iterator mask_it = maskBits.begin();

      for (int v = 0; v < srcBounds.h; ++v) {
        for (int u = 0; u < srcBounds.w; ++u, ++mask_it) {
          ASSERT(mask_it != maskBits.end());

          if (src != dst.get()) {
            if (*mask_it) {
              int getx = u + srcBounds.x - x;
              int gety = v + srcBounds.y - y;

              if ((getx >= 0) && (getx < src->width()) && (gety >= 0) && (gety < src->height()))
                dst->putPixel(u, v, src->getPixel(getx, gety));
            }
          }
          else {
            if (!*mask_it) {
              dst->putPixel(u, v, dst->maskColor());
            }
          }
        }
      }
    }
    else if (src != dst.get()) {
      copy_image(dst.get(), src, -srcBounds.x, -srcBounds.y);
    }
  }

  return dst.release();
}

doc::Image* new_tilemap_from_mask(const Site& site, const doc::Mask* srcMask)
{
  ASSERT(site.sprite());
  ASSERT(srcMask);

  const Cel* srcCel = site.cel();
  if (!srcCel)
    return nullptr;

  const doc::Grid grid = site.grid();
  const Image* srcMaskBitmap = srcMask->bitmap();
  const gfx::Rect& srcBounds = srcMask->bounds();
  const gfx::Rect srcTilesBounds = grid.canvasToTile(srcBounds);

  ASSERT(srcMaskBitmap);
  ASSERT(!srcTilesBounds.isEmpty());
  ASSERT(site.layer()->isTilemap());

  std::unique_ptr<Image> dst(Image::create(IMAGE_TILEMAP, srcTilesBounds.w, srcTilesBounds.h));
  if (!dst)
    return nullptr;

  // Clear the new tilemap
  clear_image(dst.get(), dst->maskColor());

  // Copy the masked zones
  if (srcMaskBitmap) {
    // Copy active layer with mask
    const LockImageBits<BitmapTraits> maskBits(srcMaskBitmap,
                                               gfx::Rect(0, 0, srcBounds.w, srcBounds.h));
    auto mask_it = maskBits.begin();
    const gfx::Point originPt = grid.canvasToTile(srcBounds.origin());

    for (int v = 0; v < srcBounds.h; ++v) {
      for (int u = 0; u < srcBounds.w; ++u, ++mask_it) {
        ASSERT(mask_it != maskBits.end());
        if (*mask_it) {
          gfx::Point srcPt = grid.canvasToTile(gfx::Point(srcBounds.x + u, srcBounds.y + v));
          gfx::Point dstPt = srcPt - originPt;

          if (dst->bounds().contains(dstPt) && srcCel->image()->bounds().contains(srcPt)) {
            dst->putPixel(dstPt.x, dstPt.y, srcCel->image()->getPixel(srcPt.x, srcPt.y));
          }
        }
      }
    }
  }
  else {
    // Just copy tilemap data
    dst->copy(srcCel->image(), gfx::Clip(0, 0, srcTilesBounds));
  }

  return dst.release();
}

} // namespace app
