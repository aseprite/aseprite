// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/util/freetype_utils.h"

#include "doc/blend_funcs.h"
#include "doc/blend_internals.h"
#include "doc/color.h"
#include "doc/image.h"
#include "doc/primitives.h"
#include "ft/algorithm.h"
#include "ft/face.h"
#include "ft/hb_shaper.h"
#include "ft/lib.h"

#include <memory>
#include <stdexcept>

namespace app {

doc::Image* render_text(const std::string& fontfile, int fontsize,
                        const std::string& text,
                        doc::color_t color,
                        bool antialias)
{
  std::unique_ptr<doc::Image> image(nullptr);
  ft::Lib ft;

  ft::Face face(ft.open(fontfile));
  if (face.isValid()) {
    // Set font size
    face.setSize(fontsize);
    face.setAntialias(antialias);

    // Calculate text size
    gfx::Rect bounds = ft::calc_text_bounds(face, text);

    // Render the image and copy it to the clipboard
    if (!bounds.isEmpty()) {
      image.reset(doc::Image::create(doc::IMAGE_RGB, bounds.w, bounds.h));
      doc::clear_image(image.get(), 0);

      ft::ForEachGlyph<ft::Face> feg(face);
      if (feg.initialize(base::utf8_const_iterator(text.begin()),
                         base::utf8_const_iterator(text.end()))) {
        do {
          auto glyph = feg.glyph();
          if (!glyph)
            continue;

          int t, yimg = - bounds.y + int(glyph->y);

          for (int v=0; v<int(glyph->bitmap->rows); ++v, ++yimg) {
            const uint8_t* p = glyph->bitmap->buffer + v*glyph->bitmap->pitch;
            int ximg = - bounds.x + int(glyph->x);
            int bit = 0;

            for (int u=0; u<int(glyph->bitmap->width); ++u, ++ximg) {
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
                  image.get(), ximg, yimg,
                  doc::rgba_blender_normal(
                    doc::get_pixel(image.get(), ximg, yimg),
                    output_color));
              }
            }
          }
        } while (feg.nextChar());
      }
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
