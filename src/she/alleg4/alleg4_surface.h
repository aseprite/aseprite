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
#include "she/locked_surface.h"
#include "she/surface.h"

namespace she {

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

    void drawRgbaSurface(const LockedSurface* src, int dstx, int dsty) OVERRIDE {
      set_alpha_blender();
      draw_trans_sprite(m_bmp, static_cast<const Alleg4Surface*>(src)->m_bmp, dstx, dsty);
    }

  private:
    BITMAP* m_bmp;
    DestroyFlag m_destroy;
  };

} // namespace she

#endif
