// SHE library
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/draw_text.h"

#include "ft/algorithm.h"
#include "ft/hb_shaper.h"
#include "gfx/clip.h"
#include "she/common/freetype_font.h"
#include "she/common/generic_surface.h"
#include "she/common/sprite_sheet_font.h"

namespace she {

gfx::Rect draw_text(Surface* surface, Font* font,
                    base::utf8_const_iterator it,
                    const base::utf8_const_iterator& end,
                    gfx::Color fg, gfx::Color bg,
                    int x, int y,
                    DrawTextDelegate* delegate)
{
  gfx::Rect textBounds;

  switch (font->type()) {

    case FontType::kSpriteSheet: {
      SpriteSheetFont* ssFont = static_cast<SpriteSheetFont*>(font);
      while (it != end) {
        int chr = *it;
        if (delegate)
          delegate->preProcessChar(chr, fg, bg);

        gfx::Rect charBounds = ssFont->getCharBounds(chr);
        gfx::Rect outCharBounds(x, y, charBounds.w, charBounds.h);
        if (delegate && !delegate->preDrawChar(outCharBounds))
          break;

        if (!charBounds.isEmpty()) {
          if (surface) {
            Surface* sheet = ssFont->getSurfaceSheet();
            SurfaceLock lock(sheet);
            surface->drawColoredRgbaSurface(sheet, fg, bg, gfx::Clip(x, y, charBounds));
          }
        }

        textBounds |= outCharBounds;
        if (delegate)
          delegate->postDrawChar(outCharBounds);

        x += charBounds.w;
        ++it;
      }
      break;
    }

    case FontType::kTrueType: {
      FreeTypeFont* ttFont = static_cast<FreeTypeFont*>(font);
      bool antialias = ttFont->face().antialias();
      int fg_alpha = gfx::geta(fg);

      gfx::Rect clipBounds;
      she::SurfaceFormatData fd;
      if (surface) {
        clipBounds = surface->getClipBounds();
        surface->getFormat(&fd);
      }

      ft::ForEachGlyph<FreeTypeFont::Face> feg(ttFont->face());
      if (feg.initialize(it, end)) {
        do {
          if (delegate)
            delegate->preProcessChar(feg.unicodeChar(), fg, bg);

          auto glyph = feg.glyph();
          if (!glyph)
            continue;

          gfx::Rect origDstBounds(
            x + int(glyph->startX),
            y + int(glyph->y),
            int(glyph->endX) - int(glyph->startX),
            int(glyph->bitmap->rows) ? int(glyph->bitmap->rows): 1);

          if (delegate && !delegate->preDrawChar(origDstBounds))
            break;

          origDstBounds.x = x + int(glyph->x);
          origDstBounds.w = int(glyph->bitmap->width);
          origDstBounds.h = int(glyph->bitmap->rows);

          gfx::Rect dstBounds = origDstBounds;
          if (surface)
            dstBounds &= clipBounds;

          if (surface && !dstBounds.isEmpty()) {
            int clippedRows = dstBounds.y - origDstBounds.y;
            int dst_y = dstBounds.y;
            int t;
            for (int v=0; v<dstBounds.h; ++v, ++dst_y) {
              int bit = 0;
              const uint8_t* p = glyph->bitmap->buffer
                + (v+clippedRows)*glyph->bitmap->pitch;
              int dst_x = dstBounds.x;
              uint32_t* dst_address =
                (uint32_t*)surface->getData(dst_x, dst_y);

              // Skip first clipped pixels
              for (int u=0; u<dstBounds.x-origDstBounds.x; ++u) {
                if (antialias) {
                  ++p;
                }
                else {
                  if (bit == 8) {
                    bit = 0;
                    ++p;
                  }
                }
              }

              for (int u=0; u<dstBounds.w; ++u, ++dst_x) {
                ASSERT(clipBounds.contains(gfx::Point(dst_x, dst_y)));

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

                uint32_t backdrop = *dst_address;
                gfx::Color backdropColor =
                  gfx::rgba(
                    ((backdrop & fd.redMask) >> fd.redShift),
                    ((backdrop & fd.greenMask) >> fd.greenShift),
                    ((backdrop & fd.blueMask) >> fd.blueShift),
                    ((backdrop & fd.alphaMask) >> fd.alphaShift));

                gfx::Color output = gfx::rgba(gfx::getr(fg),
                                              gfx::getg(fg),
                                              gfx::getb(fg),
                                              MUL_UN8(fg_alpha, alpha, t));
                if (gfx::geta(bg) > 0)
                  output = blend(blend(backdropColor, bg), output);
                else
                  output = blend(backdropColor, output);

                *dst_address =
                  ((gfx::getr(output) << fd.redShift  ) & fd.redMask  ) |
                  ((gfx::getg(output) << fd.greenShift) & fd.greenMask) |
                  ((gfx::getb(output) << fd.blueShift ) & fd.blueMask ) |
                  ((gfx::geta(output) << fd.alphaShift) & fd.alphaMask);

                ++dst_address;
              }
            }
          }

          if (!origDstBounds.w) origDstBounds.w = 1;
          if (!origDstBounds.h) origDstBounds.h = 1;
          textBounds |= origDstBounds;
          if (delegate)
            delegate->postDrawChar(origDstBounds);
        } while (feg.nextChar());
      }
      break;
    }

  }

  return textBounds;
}

} // namespace she
