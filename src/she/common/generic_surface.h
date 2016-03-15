// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_COMMON_GENERIC_SURFACE_H
#define SHE_COMMON_GENERIC_SURFACE_H
#pragma once

#include "gfx/clip.h"
#include "she/common/freetype_font.h"
#include "she/common/sprite_sheet_font.h"

namespace she {

namespace {

#define MUL_UN8(a, b, t)                               \
  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))

gfx::Color blend(const gfx::Color backdrop, gfx::Color src)
{
  if (gfx::geta(backdrop) == 0)
    return src;
  else if (gfx::geta(src) == 0)
    return backdrop;

  int Br, Bg, Bb, Ba;
  int Sr, Sg, Sb, Sa;
  int Rr, Rg, Rb, Ra;

  Br = gfx::getr(backdrop);
  Bg = gfx::getg(backdrop);
  Bb = gfx::getb(backdrop);
  Ba = gfx::geta(backdrop);

  Sr = gfx::getr(src);
  Sg = gfx::getg(src);
  Sb = gfx::getb(src);
  Sa = gfx::geta(src);

  int t;
  Ra = Ba + Sa - MUL_UN8(Ba, Sa, t);
  Rr = Br + (Sr-Br) * Sa / Ra;
  Rg = Bg + (Sg-Bg) * Sa / Ra;
  Rb = Bb + (Sb-Bb) * Sa / Ra;

  return gfx::rgba(Rr, Rg, Rb, Ra);
}

} // anoynmous namespace

template<typename Base>
class GenericDrawColoredRgbaSurface : public Base {
public:

  void drawColoredRgbaSurface(const Surface* src, gfx::Color fg, gfx::Color bg, const gfx::Clip& clipbase) override {
    gfx::Clip clip(clipbase);
    if (!clip.clip(this->width(),
                   this->height(),
                   src->width(), src->height()))
      return;

    SurfaceFormatData format;
    src->getFormat(&format);

    ASSERT(format.format == kRgbaSurfaceFormat);
    ASSERT(format.bitsPerPixel == 32);

    for (int v=0; v<clip.size.h; ++v) {
      const uint32_t* ptr = (const uint32_t*)src->getData(
        clip.src.x, clip.src.y+v);

      for (int u=0; u<clip.size.w; ++u) {
        gfx::Color dstColor = this->getPixel(clip.dst.x+u, clip.dst.y+v);
        if (gfx::geta(bg) > 0)
          dstColor = blend(dstColor, bg);

        uint32_t src = (((*ptr) & format.alphaMask) >> format.alphaShift);
        if (src > 0) {
          src = gfx::rgba(gfx::getr(fg),
                          gfx::getg(fg),
                          gfx::getb(fg), src);
          dstColor = blend(dstColor, src);
        }

        this->putPixel(dstColor, clip.dst.x+u, clip.dst.y+v);
        ++ptr;
      }
    }
  }
};

template<typename Base>
class GenericDrawTextSurface : public Base {
public:

  void drawChar(Font* font, gfx::Color fg, gfx::Color bg, int x, int y, int chr) override {
    switch (font->type()) {

      case FontType::kSpriteSheet: {
        SpriteSheetFont* ssFont = static_cast<SpriteSheetFont*>(font);

        gfx::Rect charBounds = ssFont->getCharBounds(chr);
        if (!charBounds.isEmpty()) {
          Surface* sheet = ssFont->getSurfaceSheet();
          SurfaceLock lock(sheet);
          this->drawColoredRgbaSurface(sheet, fg, bg, gfx::Clip(x, y, charBounds));
        }
        break;
      }

      case FontType::kTrueType: {
        // TODO avoid a temporary string
        std::wstring str;
        str.push_back(chr);
        drawString(font, fg, bg, x, y, base::to_utf8(str).c_str());
        break;
      }

    }
  }

  void drawString(Font* font, gfx::Color fg, gfx::Color bg, int x, int y, const std::string& str) override {
    switch (font->type()) {

      case FontType::kSpriteSheet: {
        base::utf8_const_iterator it(str.begin()), end(str.end());
        while (it != end) {
          drawChar(font, fg, bg, x, y, *it);
          x += font->charWidth(*it);
          ++it;
        }
        break;
      }

      case FontType::kTrueType: {
        FreeTypeFont* ttFont = static_cast<FreeTypeFont*>(font);
        bool antialias = ttFont->face().antialias();
        int fg_alpha = gfx::geta(fg);

        gfx::Rect bounds = ttFont->face().calcTextBounds(str);
        gfx::Rect clipBounds = this->getClipBounds();

        she::SurfaceFormatData fd;
        this->getFormat(&fd);

        ttFont->face().forEachGlyph(
          str,
          [this, x, y, fg, fg_alpha, bg, antialias, &clipBounds, &bounds, &fd](const ft::Face::Glyph& glyph) {
            gfx::Rect origDstBounds(x - bounds.x + int(glyph.x),
                                    y - bounds.y + int(glyph.y),
                                    int(glyph.bitmap->width),
                                    int(glyph.bitmap->rows));
            gfx::Rect dstBounds = origDstBounds;
            dstBounds &= clipBounds;
            if (dstBounds.isEmpty())
              return;

            int clippedRows = dstBounds.y - origDstBounds.y;
            int dst_y = dstBounds.y;
            int t;
            for (int v=0; v<dstBounds.h; ++v, ++dst_y) {
              int bit = 0;
              const uint8_t* p = glyph.bitmap->buffer
                + (v+clippedRows)*glyph.bitmap->pitch;
              int dst_x = dstBounds.x;
              uint32_t* dst_address =
                (uint32_t*)this->getData(dst_x, dst_y);

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
          });
        break;
      }

    }
  }

};

} // namespace she

#endif
