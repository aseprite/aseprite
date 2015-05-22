// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_ALLEG4_SURFACE_H_INCLUDED
#define SHE_ALLEG4_SURFACE_H_INCLUDED
#pragma once

#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "base/string.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "she/locked_surface.h"
#include "she/surface.h"
#include "she/common/locked_surface.h"

namespace {

void checked_mode(int offset)
{
  static BITMAP* pattern = NULL;
  int x, y, fg, bg;

  if (offset < 0) {
    if (pattern) {
      destroy_bitmap(pattern);
      pattern = NULL;
    }
    drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);
    return;
  }

  if (!pattern)
    pattern = create_bitmap(8, 8);

  bg = makecol(0, 0, 0);
  fg = makecol(255, 255, 255);
  offset = 7 - (offset & 7);

  clear_bitmap(pattern);

  for (y=0; y<8; y++)
    for (x=0; x<8; x++)
      putpixel(pattern, x, y, ((x+y+offset)&7) < 4 ? fg: bg);

  drawing_mode(DRAW_MODE_COPY_PATTERN, pattern, 0, 0);
}

}

namespace she {

  inline int to_allegro(int color_depth, gfx::Color color) {
    if (color_depth == 32) {
      return makeacol32(
        gfx::getr(color), gfx::getg(color), gfx::getb(color), gfx::geta(color));
    }
    else {
      if (gfx::is_transparent(color))
        return -1;
      else
        return makecol_depth(color_depth, gfx::getr(color), gfx::getg(color), gfx::getb(color));
    }
  }

  inline gfx::Color from_allegro(int color_depth, int color) {
    return gfx::rgba(
      getr_depth(color_depth, color),
      getg_depth(color_depth, color),
      getb_depth(color_depth, color));
  }

  class Alleg4Surface : public Surface
                      , public CommonLockedSurface {
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
      if (m_destroy & DestroyHandle) {
        if (m_bmp)
          destroy_bitmap(m_bmp);
      }
    }

    // Surface implementation

    void dispose() override {
      if (m_destroy & DeleteThis)
        delete this;
    }

    int width() const override {
      return m_bmp->w;
    }

    int height() const override {
      return m_bmp->h;
    }

    bool isDirectToScreen() const override {
      return m_bmp == screen;
    }

    gfx::Rect getClipBounds() override {
      return gfx::Rect(
        m_bmp->cl,
        m_bmp->ct,
        m_bmp->cr - m_bmp->cl,
        m_bmp->cb - m_bmp->ct);
    }

    void setClipBounds(const gfx::Rect& rc) override {
      set_clip_rect(m_bmp,
        rc.x,
        rc.y,
        rc.x+rc.w-1,
        rc.y+rc.h-1);
    }

    bool intersectClipRect(const gfx::Rect& rc) override {
      add_clip_rect(m_bmp,
        rc.x,
        rc.y,
        rc.x+rc.w-1,
        rc.y+rc.h-1);

      return
        (m_bmp->cl < m_bmp->cr &&
         m_bmp->ct < m_bmp->cb);
    }

    LockedSurface* lock() override {
      acquire_bitmap(m_bmp);
      return this;
    }

    void setDrawMode(DrawMode mode, int param) {
      switch (mode) {
        case DrawMode::Solid: checked_mode(-1); break;
        case DrawMode::Xor: xor_mode(TRUE); break;
        case DrawMode::Checked: checked_mode(param); break;
      }
    }

    void applyScale(int scale) override {
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

    void* nativeHandle() override {
      return reinterpret_cast<void*>(m_bmp);
    }

    // LockedSurface implementation

    int lockedWidth() const override {
      return m_bmp->w;
    }

    int lockedHeight() const override {
      return m_bmp->h;
    }

    void unlock() override {
      release_bitmap(m_bmp);
    }

    void clear() override {
      clear_to_color(m_bmp, 0);
    }

    uint8_t* getData(int x, int y) const override {
      switch (bitmap_color_depth(m_bmp)) {
        case 8: return (uint8_t*)(((uint8_t*)bmp_write_line(m_bmp, y)) + x);
        case 15:
        case 16: return (uint8_t*)(((uint16_t*)bmp_write_line(m_bmp, y)) + x);
        case 24: return (uint8_t*)(((uint8_t*)bmp_write_line(m_bmp, y)) + x*3);
        case 32: return (uint8_t*)(((uint32_t*)bmp_write_line(m_bmp, y)) + x);
      }
      return NULL;
    }

    void getFormat(SurfaceFormatData* formatData) const override {
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

    gfx::Color getPixel(int x, int y) const override {
      return from_allegro(
        bitmap_color_depth(m_bmp),
        getpixel(m_bmp, x, y));
    }

    void putPixel(gfx::Color color, int x, int y) override {
      putpixel(m_bmp, x, y, to_allegro(bitmap_color_depth(m_bmp), color));
    }

    void drawHLine(gfx::Color color, int x, int y, int w) override {
      if (gfx::geta(color) < 255) {
        set_trans_blender(0, 0, 0, gfx::geta(color));
        drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
      }

      hline(m_bmp, x, y, x+w-1, to_allegro(bitmap_color_depth(m_bmp), color));
      solid_mode();
    }

    void drawVLine(gfx::Color color, int x, int y, int h) override {
      if (gfx::geta(color) < 255) {
        set_trans_blender(0, 0, 0, gfx::geta(color));
        drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
      }

      vline(m_bmp, x, y, y+h-1, to_allegro(bitmap_color_depth(m_bmp), color));
      solid_mode();
    }

    void drawLine(gfx::Color color, const gfx::Point& a, const gfx::Point& b) override {
      if (gfx::geta(color) < 255) {
        set_trans_blender(0, 0, 0, gfx::geta(color));
        drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
      }

      line(m_bmp, a.x, a.y, b.x, b.y, to_allegro(bitmap_color_depth(m_bmp), color));
      solid_mode();
    }

    void drawRect(gfx::Color color, const gfx::Rect& rc) override {
      if (gfx::geta(color) < 255) {
        set_trans_blender(0, 0, 0, gfx::geta(color));
        drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
      }

      rect(m_bmp, rc.x, rc.y, rc.x+rc.w-1, rc.y+rc.h-1, to_allegro(bitmap_color_depth(m_bmp), color));
      solid_mode();
    }

    void fillRect(gfx::Color color, const gfx::Rect& rc) override {
      if (gfx::geta(color) < 255) {
        set_trans_blender(0, 0, 0, gfx::geta(color));
        drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
      }

      rectfill(m_bmp, rc.x, rc.y, rc.x+rc.w-1, rc.y+rc.h-1, to_allegro(bitmap_color_depth(m_bmp), color));
      solid_mode();
    }

    void blitTo(LockedSurface* dest, int srcx, int srcy, int dstx, int dsty, int width, int height) const override {
      ASSERT(m_bmp);
      ASSERT(dest);
      ASSERT(static_cast<Alleg4Surface*>(dest)->m_bmp);

      blit(m_bmp,
        static_cast<Alleg4Surface*>(dest)->m_bmp,
        srcx, srcy,
        dstx, dsty,
        width, height);
    }

    void scrollTo(const gfx::Rect& rc, int dx, int dy) override {
      blit(m_bmp, m_bmp,
        rc.x, rc.y,
        rc.x+dx, rc.y+dy,
        rc.w, rc.h);
    }

    void drawSurface(const LockedSurface* src, int dstx, int dsty) override {
      draw_sprite(m_bmp, static_cast<const Alleg4Surface*>(src)->m_bmp, dstx, dsty);
    }

    void drawRgbaSurface(const LockedSurface* src, int dstx, int dsty) override {
      set_alpha_blender();
      draw_trans_sprite(m_bmp, static_cast<const Alleg4Surface*>(src)->m_bmp, dstx, dsty);
    }

  private:
    BITMAP* m_bmp;
    DestroyFlag m_destroy;
  };

} // namespace she

#endif
