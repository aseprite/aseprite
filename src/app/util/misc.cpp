// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/color.h"
#include "app/document_location.h"
#include "app/ini_file.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "app/util/misc.h"
#include "doc/doc.h"
#include "ui/manager.h"
#include "ui/system.h"
#include "ui/widget.h"

namespace app {

using namespace doc;

Image* NewImageFromMask(const DocumentLocation& location)
{
  const Sprite* srcSprite = location.sprite();
  const Mask* srcMask = location.document()->mask();
  const Image* srcMaskBitmap = srcMask->bitmap();
  const gfx::Rect& srcBounds = srcMask->bounds();
  int x, y, u, v, getx, gety;
  Image *dst;
  const Image *src = location.image(&x, &y);

  ASSERT(srcSprite);
  ASSERT(srcMask);
  ASSERT(srcMaskBitmap);

  dst = Image::create(srcSprite->pixelFormat(), srcBounds.w, srcBounds.h);
  if (!dst)
    return NULL;

  // Clear the new image
  dst->setMaskColor(src ? src->maskColor(): srcSprite->transparentColor());
  clear_image(dst, dst->maskColor());

  // Copy the masked zones
  if (src) {
    const LockImageBits<BitmapTraits> maskBits(srcMaskBitmap, gfx::Rect(0, 0, srcBounds.w, srcBounds.h));
    LockImageBits<BitmapTraits>::const_iterator mask_it = maskBits.begin();

    for (v=0; v<srcBounds.h; ++v) {
      for (u=0; u<srcBounds.w; ++u, ++mask_it) {
        ASSERT(mask_it != maskBits.end());

        if (*mask_it) {
          getx = u+srcBounds.x-x;
          gety = v+srcBounds.y-y;

          if ((getx >= 0) && (getx < src->width()) &&
            (gety >= 0) && (gety < src->height()))
            dst->putPixel(u, v, src->getPixel(getx, gety));
        }
      }
    }
  }

  return dst;
}

} // namespace app
