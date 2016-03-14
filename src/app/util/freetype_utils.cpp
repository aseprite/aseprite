// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/util/freetype_utils.h"

#include "base/string.h"
#include "base/unique_ptr.h"
#include "doc/blend_funcs.h"
#include "doc/blend_internals.h"
#include "doc/color.h"
#include "doc/image.h"
#include "doc/primitives.h"
#include "ft/face.h"
#include "ft/lib.h"

#include <stdexcept>

namespace app {

doc::Image* render_text(const std::string& fontfile, int fontsize,
                        const std::string& text,
                        doc::color_t color,
                        bool antialias)
{
  base::UniquePtr<doc::Image> image(nullptr);
  ft::Lib ft;

  ft::Face face(ft.open(fontfile));
  if (face) {
    // Set font size
    face.setSize(fontsize);
    face.setAntialias(antialias);

    // Calculate text size
    base::utf8_const_iterator begin(text.begin()), end(text.end());
    gfx::Rect bounds = face.calcTextBounds(begin, end);

    // Render the image and copy it to the clipboard
    if (!bounds.isEmpty()) {
      image.reset(doc::Image::create(doc::IMAGE_RGB, bounds.w, bounds.h));
      doc::clear_image(image, 0);

      face.forEachGlyph(
        begin, end,
        [&bounds, &image, color, antialias](FT_BitmapGlyph glyph, int x) {
          int t, yimg = - bounds.y - glyph->top;

          for (int v=0; v<(int)glyph->bitmap.rows; ++v, ++yimg) {
            const uint8_t* p = glyph->bitmap.buffer + v*glyph->bitmap.pitch;
            int ximg = x - bounds.x + glyph->left;
            int bit = 0;

            for (int u=0; u<(int)glyph->bitmap.width; ++u, ++ximg) {
              int alpha;

              if (antialias) {
                alpha = *(p++);
              }
              else {
                alpha = ((*p) & (1 << (7 - (bit++))) ? 255: 0);
                if (bit == 8) {
                  bit = 0;
                  ++p;
                }
              }

              doc::put_pixel(
                image, ximg, yimg,
                doc::rgba_blender_normal(
                  doc::get_pixel(image, ximg, yimg),
                  doc::rgba(doc::rgba_getr(color),
                            doc::rgba_getg(color),
                            doc::rgba_getb(color),
                            MUL_UN8(doc::rgba_geta(color), alpha, t)), 255));
            }
          }
        });
    }
    else {
      throw std::runtime_error("There is no text");
    }
  }
  else {
    throw std::runtime_error("Error loading font face");
  }

  return (image ? image.release(): nullptr);
}

} // namespace app
