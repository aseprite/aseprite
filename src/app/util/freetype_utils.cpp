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
  if (face.isValid()) {
    // Set font size
    face.setSize(fontsize);
    face.setAntialias(antialias);

    // Calculate text size
    gfx::Rect bounds = face.calcTextBounds(text);

    // Render the image and copy it to the clipboard
    if (!bounds.isEmpty()) {
      image.reset(doc::Image::create(doc::IMAGE_RGB, bounds.w, bounds.h));
      doc::clear_image(image, 0);

      face.forEachGlyph(
        text,
        [&bounds, &image, color, antialias](const ft::Glyph& glyph) {
          int t, yimg = - bounds.y + int(glyph.y);

          for (int v=0; v<int(glyph.bitmap->rows); ++v, ++yimg) {
            const uint8_t* p = glyph.bitmap->buffer + v*glyph.bitmap->pitch;
            int ximg = - bounds.x + int(glyph.x);
            int bit = 0;

            for (int u=0; u<int(glyph.bitmap->width); ++u, ++ximg) {
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

              int output_alpha = MUL_UN8(doc::rgba_geta(color), alpha, t);
              if (output_alpha) {
                doc::color_t output_color =
                  doc::rgba(doc::rgba_getr(color),
                            doc::rgba_getg(color),
                            doc::rgba_getb(color),
                            output_alpha);

                doc::put_pixel(
                  image, ximg, yimg,
                  doc::rgba_blender_normal(
                    doc::get_pixel(image, ximg, yimg),
                    output_color));
              }
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
