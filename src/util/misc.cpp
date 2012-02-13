/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "config.h"

#include <allegro.h>
#include <string.h>

#include "app.h"
#include "app/color.h"
#include "gui/list.h"
#include "gui/manager.h"
#include "gui/system.h"
#include "gui/widget.h"
#include "ini_file.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/raster.h"
#include "util/misc.h"
#include "widgets/editor/editor.h"
#include "widgets/statebar.h"

Image* NewImageFromMask(const Document* srcDocument)
{
  const Sprite* srcSprite = srcDocument->getSprite();
  const Mask* srcMask = srcDocument->getMask();
  const Image* srcBitmap = srcMask->getBitmap();
  const gfx::Rect& srcBounds = srcMask->getBounds();
  const uint8_t* address;
  int x, y, u, v, getx, gety;
  Image *dst;
  const Image *src = srcSprite->getCurrentImage(&x, &y);
  div_t d;

  ASSERT(srcSprite);
  ASSERT(srcMask);
  ASSERT(srcBitmap);
  ASSERT(src);

  dst = Image::create(srcSprite->getPixelFormat(), srcBounds.w, srcBounds.h);
  if (!dst)
    return NULL;

  // Clear the new image
  image_clear(dst, 0);

  // Copy the masked zones
  for (v=0; v<srcBounds.h; v++) {
    d = div(0, 8);
    address = ((const uint8_t**)srcBitmap->line)[v]+d.quot;

    for (u=0; u<srcBounds.w; u++) {
      if ((*address & (1<<d.rem))) {
        getx = u+srcBounds.x-x;
        gety = v+srcBounds.y-y;

        if ((getx >= 0) && (getx < src->w) &&
            (gety >= 0) && (gety < src->h))
          dst->putpixel(u, v, src->getpixel(getx, gety));
      }

      _image_bitmap_next_bit(d, address);
    }
  }

  return dst;
}
