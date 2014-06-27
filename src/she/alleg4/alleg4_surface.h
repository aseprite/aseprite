// SHE library
// Copyright (C) 2012-2014  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_ALLEG4_SURFACE_H_INCLUDED
#define SHE_ALLEG4_SURFACE_H_INCLUDED
#pragma once

#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "base/compiler_specific.h"
#include "base/string.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "she/locked_surface.h"
#include "she/surface.h"

namespace she {

  inline int to_allegro(Color color) {
    if (is_transparent(color))
      return -1;
    else
      return makecol(getr(color), getg(color), getb(color));
  }

  class Alleg4Surface : public Surface
                      , public LockedSurface {
  public:
    enum DestroyFlag {
      None = 0,
      DeleteThis = 1,
      DestroyHandle = 2,
      DeleteAndDestroy = DeleteThis | DestroyHandle,
    };

    Alleg4Surface(BITMAP* bmp, DestroyFlag destroy)
      : m_bmp(bmp)
      , m_destroy(destroy) {
    }

    Alleg4Surface(int width, int height, DestroyFlag destroy)
      : m_bmp(create_bitmap(width, height))
      , m_destroy(destroy) {
    }

    Alleg4Surface(int width, int height, int bpp, DestroyFlag destroy)
      : m_bmp(create_bitmap_ex(bpp, width, height))
      , m_destroy(destroy) {
    }

    ~Alleg4Surface() {
      if (m_destroy & DestroyHandle)
        destroy_bitmap(m_bmp);
    }

    // Surface implementation

    void dispose() OVERRIDE {
      if (m_destroy & DeleteThis)
        delete this;
    }

    int width() const OVERRIDE {
      return m_bmp->w;
    }

    int height() const OVERRIDE {
      return m_bmp->h;
    }

    bool isDirectToScreen() const OVERRIDE {
      return m_bmp == screen;
    }

    gfx::Rect getClipBounds() OVERRIDE {
      return gfx::Rect(
        m_bmp->cl,
        m_bmp->ct,
        m_bmp->cr - m_bmp->cl,
        m_bmp->cb - m_bmp->ct);
    }

    void setClipBounds(const gfx::Rect& rc) OVERRIDE {
      set_clip_rect(m_bmp,
        rc.x,
        rc.y,
        rc.x+rc.w-1,
        rc.y+rc.h-1);
    }

    bool intersectClipRect(const gfx::Rect& rc) OVERRIDE {
      add_clip_rect(m_bmp,
        rc.x,
        rc.y,
        rc.x+rc.w-1,
        rc.y+rc.h-1);

      return
        (m_bmp->cl < m_bmp->cr &&
         m_bmp->ct < m_bmp->cb);
    }

    LockedSurface* lock() OVERRIDE {
      acquire_bitmap(m_bmp);
      return this;
    }

    void applyScale(int scale) OVERRIDE {
      if (scale < 2)
        return;

      BITMAP* scaled =
        create_bitmap_ex(bitmap_color_depth(m_bmp),
          m_bmp->w*scale,
          m_bmp->h*scale);

      for (int y=0; y<scaled->h; ++y)
        for (int x=0; x<scaled->w; ++x)
          putpixel(scaled, x, y, getpixel(m_bmp, x/scale, y/scale));

      if (m_destroy & DestroyHandle)
        destroy_bitmap(m_bmp);

      m_bmp = scaled;
      m_destroy = DestroyHandle;
    }

    void* nativeHandle() OVERRIDE {
      return reinterpret_cast<void*>(m_bmp);
    }

    // LockedSurface implementation

    void unlock() OVERRIDE {
      release_bitmap(m_bmp);
    }

    void clear() OVERRIDE {
      clear_to_color(m_bmp, 0);
    }

    uint8_t* getData(int x, int y) OVERRIDE {
      switch (bitmap_color_depth(m_bmp)) {
        case 8: return (uint8_t*)(((uint8_t*)bmp_write_line(m_bmp, y)) + x);
        case 15:
        case 16: return (uint8_t*)(((uint16_t*)bmp_write_line(m_bmp, y)) + x);
        case 24: return (uint8_t*)(((uint8_t*)bmp_write_line(m_bmp, y)) + x*3);
        case 32: return (uint8_t*)(((uint32_t*)bmp_write_line(m_bmp, y)) + x);
      }
      return NULL;
    }

    void getFormat(SurfaceFormatData* formatData) OVERRIDE {
      formatData->format = kRgbaSurfaceFormat;
      formatData->bitsPerPixel = bitmap_color_depth(m_bmp);

      switch (formatData->bitsPerPixel) {
        case 8:
          formatData->redShift   = 0;
          formatData->greenShift = 0;
          formatData->blueShift  = 0;
          formatData->alphaShift = 0;
          formatData->redMask    = 0;
          formatData->greenMask  = 0;
          formatData->blueMask   = 0;
          formatData->alphaMask  = 0;
          break;
        case 15:
          formatData->redShift   = _rgb_r_shift_15;
          formatData->greenShift = _rgb_g_shift_15;
          formatData->blueShift  = _rgb_b_shift_15;
          formatData->alphaShift = 0;
          formatData->redMask    = 31 << _rgb_r_shift_15;
          formatData->greenMask  = 31 << _rgb_g_shift_15;
          formatData->blueMask   = 31 << _rgb_b_shift_15;
          formatData->alphaMask  = 0;
          break;
        case 16:
          formatData->redShift   = _rgb_r_shift_16;
          formatData->greenShift = _rgb_g_shift_16;
          formatData->blueShift  = _rgb_b_shift_16;
          formatData->alphaShift = 0;
          formatData->redMask    = 31 << _rgb_r_shift_16;
          formatData->greenMask  = 63 << _rgb_g_shift_16;
          formatData->blueMask   = 31 << _rgb_b_shift_16;
          formatData->alphaMask  = 0;
          break;
        case 24:
          formatData->redShift   = _rgb_r_shift_24;
          formatData->greenShift = _rgb_g_shift_24;
          formatData->blueShift  = _rgb_b_shift_24;
          formatData->alphaShift = 0;
          formatData->redMask    = 255 << _rgb_r_shift_24;
          formatData->greenMask  = 255 << _rgb_g_shift_24;
          formatData->blueMask   = 255 << _rgb_b_shift_24;
          formatData->alphaMask  = 0;
          break;
        case 32:
          formatData->redShift   = _rgb_r_shift_32;
          formatData->greenShift = _rgb_g_shift_32;
          formatData->blueShift  = _rgb_b_shift_32;
          formatData->alphaShift = _rgb_a_shift_32;
          formatData->redMask    = 255 << _rgb_r_shift_32;
          formatData->greenMask  = 255 << _rgb_g_shift_32;
          formatData->blueMask   = 255 << _rgb_b_shift_32;
          formatData->alphaMask  = 255 << _rgb_a_shift_32;
          break;
      }
    }

    void drawHLine(she::Color color, int x, int y, int w) OVERRIDE {
      hline(m_bmp, x, y, x+w-1, to_allegro(color));
    }

    void drawVLine(she::Color color, int x, int y, int h) OVERRIDE {
      vline(m_bmp, x, y, y+h-1, to_allegro(color));
    }

    void drawLine(she::Color color, const gfx::Point& a, const gfx::Point& b) OVERRIDE {
      line(m_bmp, a.x, a.y, b.x, b.y, to_allegro(color));
    }

    void drawRect(she::Color color, const gfx::Rect& rc) OVERRIDE {
      rect(m_bmp, rc.x, rc.y, rc.x+rc.w-1, rc.y+rc.h-1, to_allegro(color));
    }

    void fillRect(she::Color color, const gfx::Rect& rc) OVERRIDE {
      rectfill(m_bmp, rc.x, rc.y, rc.x+rc.w-1, rc.y+rc.h-1, to_allegro(color));
    }

    void blitTo(LockedSurface* dest, int srcx, int srcy, int dstx, int dsty, int width, int height) const OVERRIDE {
      ASSERT(m_bmp);
      ASSERT(dest);
      ASSERT(static_cast<Alleg4Surface*>(dest)->m_bmp);

      blit(m_bmp,
        static_cast<Alleg4Surface*>(dest)->m_bmp,
        srcx, srcy,
        dstx, dsty,
        width, height);
    }

    void drawSurface(const LockedSurface* src, int dstx, int dsty) OVERRIDE {
      draw_sprite(m_bmp, static_cast<const Alleg4Surface*>(src)->m_bmp, dstx, dsty);
    }

    void drawRgbaSurface(const LockedSurface* src, int dstx, int dsty) OVERRIDE {
      set_alpha_blender();
      draw_trans_sprite(m_bmp, static_cast<const Alleg4Surface*>(src)->m_bmp, dstx, dsty);
    }

    void drawChar(Font* sheFont, she::Color fg, she::Color bg, int x, int y, int chr) OVERRIDE {
      FONT* allegFont = reinterpret_cast<FONT*>(sheFont->nativeHandle());

      allegFont->vtable->render_char(allegFont, chr,
        to_allegro(fg),
        to_allegro(bg),
        m_bmp, x, y);
    }

    void drawString(Font* sheFont, she::Color fg, she::Color bg, int x, int y, const std::string& str) OVERRIDE {
      FONT* allegFont = reinterpret_cast<FONT*>(sheFont->nativeHandle());
      base::utf8_const_iterator it(str.begin()), end(str.end());
      int length = 0;
      int sysfg = to_allegro(fg);
      int sysbg = to_allegro(bg);

      while (it != end) {
        allegFont->vtable->render_char(allegFont, *it, sysfg, sysbg, m_bmp, x, y);
        x += sheFont->charWidth(*it);
        ++it;
      }
    }

  private:
    BITMAP* m_bmp;
    DestroyFlag m_destroy;
  };

} // namespace she

#endif
