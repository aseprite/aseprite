/* ASEPRITE
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

#ifndef APP_UTIL_CLIPBOARD_H_INCLUDED
#define APP_UTIL_CLIPBOARD_H_INCLUDED

#include "gfx/point.h"
#include "gfx/size.h"
#include "ui/base.h"

namespace raster {
  class Image;
  class Palette;
}

namespace app {
  class ContextReader;
  class ContextWriter;

  namespace clipboard {
    using namespace raster;

    // TODO Horrible API: refactor it.

    bool can_paste();

    void cut(ContextWriter& context);
    void copy(const ContextReader& context);
    void copy_image(Image* image, Palette* palette, const gfx::Point& point);
    void paste();

    // Returns true and fills the specified "size"" with the image's
    // size in the clipboard, or return false in case that the clipboard
    // doesn't contain an image at all.
    bool get_image_size(gfx::Size& size);

  } // namespace clipboard
} // namespace app

#endif
