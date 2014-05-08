/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <allegro.h>
#include <string.h>

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
#include "raster/raster.h"
#include "ui/manager.h"
#include "ui/system.h"
#include "ui/widget.h"

namespace app {

using namespace raster;

Image* NewImageFromMask(const DocumentLocation& location)
{
  const Sprite* srcSprite = location.sprite();
  const Mask* srcMask = location.document()->getMask();
  const Image* srcMaskBitmap = srcMask->getBitmap();
  const gfx::Rect& srcBounds = srcMask->getBounds();
  int x, y, u, v, getx, gety;
  Image *dst;
  const Image *src = location.image(&x, &y);

  ASSERT(srcSprite);
  ASSERT(srcMask);
  ASSERT(srcMaskBitmap);
  ASSERT(src);

  dst = Image::create(srcSprite->getPixelFormat(), srcBounds.w, srcBounds.h);
  if (!dst)
    return NULL;

  // Clear the new image
  dst->setMaskColor(src->getMaskColor());
  clear_image(dst, dst->getMaskColor());

  // Copy the masked zones
  const LockImageBits<BitmapTraits> maskBits(srcMaskBitmap, gfx::Rect(0, 0, srcBounds.w, srcBounds.h));
  LockImageBits<BitmapTraits>::const_iterator mask_it = maskBits.begin();

  for (v=0; v<srcBounds.h; ++v) {
    for (u=0; u<srcBounds.w; ++u, ++mask_it) {
      ASSERT(mask_it != maskBits.end());

      if (*mask_it) {
        getx = u+srcBounds.x-x;
        gety = v+srcBounds.y-y;

        if ((getx >= 0) && (getx < src->getWidth()) &&
            (gety >= 0) && (gety < src->getHeight()))
          dst->putPixel(u, v, src->getPixel(getx, gety));
      }
    }
  }

  return dst;
}

} // namespace app
