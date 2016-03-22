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

#include <stdexcept>

#include "freetype/ftglyph.h"
#include "ft2build.h"
#include FT_FREETYPE_H

namespace app {

template<typename Iterator, typename Func>
static void for_each_glyph(FT_Face face, bool antialias,
                           Iterator first, Iterator end,
                           Func callback)
{
  bool use_kerning = (FT_HAS_KERNING(face) ? true: false);

  // Calculate size
  FT_UInt prev_glyph = 0;
  int x = 0;
  for (; first != end; ++first) {
    FT_UInt glyph_index = FT_Get_Char_Index(face, *first);

    if (use_kerning && prev_glyph && glyph_index) {
      FT_Vector kerning;
      FT_Get_Kerning(face, prev_glyph, glyph_index,
                     FT_KERNING_DEFAULT, &kerning);
      x += kerning.x >> 6;
    }

    FT_Error err = FT_Load_Glyph(
      face, glyph_index,
      FT_LOAD_RENDER |
      FT_LOAD_NO_BITMAP |
      (antialias ? FT_LOAD_TARGET_NORMAL:
                   FT_LOAD_TARGET_MONO));

    if (!err) {
      callback(x, face->glyph);
      x += face->glyph->advance.x >> 6;
    }

    prev_glyph = glyph_index;
  }
}

class ScopedFTLib {
public:
  ScopedFTLib(FT_Library& ft) : m_ft(ft) {
    FT_Init_FreeType(&m_ft);
  }
  ~ScopedFTLib() {
    FT_Done_FreeType(m_ft);
  }
private:
  FT_Library& m_ft;
};

class ScopedFTFace {
public:
  ScopedFTFace(FT_Face& face) : m_face(face) {
  }
  ~ScopedFTFace() {
    FT_Done_Face(m_face);
   }
private:
  FT_Face& m_face;
};

doc::Image* render_text(const std::string& fontfile, int fontsize,
                        const std::string& text,
                        doc::color_t color,
                        bool antialias)
{
  base::UniquePtr<doc::Image> image(nullptr);
  FT_Library ft;
  ScopedFTLib init_and_done_ft(ft);

  FT_Open_Args args;
  memset(&args, 0, sizeof(args));
  args.flags = FT_OPEN_PATHNAME;
  args.pathname = (FT_String*)fontfile.c_str();

  FT_Face face;
  FT_Error err = FT_Open_Face(ft, &args, 0, &face);
  if (!err) {
    ScopedFTFace done_face(face);

    // Set font size
    FT_Set_Pixel_Sizes(face, fontsize, fontsize);

    // Calculate text size
    base::utf8_const_iterator begin(text.begin()), end(text.end());
    gfx::Rect bounds(0, 0, 0, 0);
    for_each_glyph(
      face, antialias, begin, end,
      [&bounds](int x, FT_GlyphSlot glyph) {
        bounds |= gfx::Rect(x + glyph->bitmap_left,
                            -glyph->bitmap_top,
                            (int)glyph->bitmap.width,
                            (int)glyph->bitmap.rows);
      });

    // Render the image and copy it to the clipboard
    if (!bounds.isEmpty()) {
      image.reset(doc::Image::create(doc::IMAGE_RGB, bounds.w, bounds.h));
      doc::clear_image(image, 0);

      for_each_glyph(
        face, antialias, begin, end,
        [&bounds, &image, color, antialias](int x, FT_GlyphSlot glyph) {
          int t, yimg = - bounds.y - glyph->bitmap_top;

          for (int v=0; v<(int)glyph->bitmap.rows; ++v, ++yimg) {
            const uint8_t* p = glyph->bitmap.buffer + v*glyph->bitmap.pitch;
            int ximg = x - bounds.x + glyph->bitmap_left;
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
                            MUL_UN8(doc::rgba_geta(color), alpha, t))));
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
