// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef UI_GRAPHICS_H_INCLUDED
#define UI_GRAPHICS_H_INCLUDED
#pragma once

#include "base/shared_ptr.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/size.h"
#include "ui/color.h"

#include <string>

struct BITMAP;
struct FONT;

namespace gfx {
  class Region;
}

namespace ui {

  // Class to render a widget in the screen.
  class Graphics
  {
  public:
    Graphics(BITMAP* bmp, int dx, int dy);
    ~Graphics();

    gfx::Rect getClipBounds() const;
    void setClipBounds(const gfx::Rect& rc);
    bool intersectClipRect(const gfx::Rect& rc);

    void drawHLine(ui::Color color, int x, int y, int w);
    void drawVLine(ui::Color color, int x, int y, int h);
    void drawLine(ui::Color color, const gfx::Point& a, const gfx::Point& b);

    void drawRect(ui::Color color, const gfx::Rect& rc);
    void fillRect(ui::Color color, const gfx::Rect& rc);
    void fillRegion(ui::Color color, const gfx::Region& rgn);

    void drawBitmap(BITMAP* sprite, int x, int y);
    void drawAlphaBitmap(BITMAP* sprite, int x, int y);

    void blit(BITMAP* src, int srcx, int srcy, int dstx, int dsty, int w, int h);

    // ======================================================================
    // FONT & TEXT
    // ======================================================================

    void setFont(FONT* font);
    FONT* getFont();

    void drawString(const std::string& str, Color fg, Color bg, bool fillbg, const gfx::Point& pt);
    void drawString(const std::string& str, Color fg, Color bg, const gfx::Rect& rc, int align);

    gfx::Size measureString(const std::string& str);
    gfx::Size fitString(const std::string& str, int maxWidth, int align);

  private:
    gfx::Size drawStringAlgorithm(const std::string& str, Color fg, Color bg, const gfx::Rect& rc, int align, bool draw);

    BITMAP* m_bmp;
    int m_dx;
    int m_dy;
    gfx::Rect m_clipBounds;
    FONT* m_currentFont;
  };

  // Class to draw directly in the screen.
  class ScreenGraphics : public Graphics
  {
  public:
    ScreenGraphics();
    virtual ~ScreenGraphics();
  };

  // Class to temporary set the Graphics' clip region to a sub-rectangle
  // (in the life-time of the IntersectClip instance).
  class IntersectClip
  {
  public:
    IntersectClip(Graphics* g, const gfx::Rect& rc)
      : m_graphics(g)
      , m_oldClip(g->getClipBounds())
    {
      m_notEmpty = m_graphics->intersectClipRect(rc);
    }

    ~IntersectClip()
    {
      m_graphics->setClipBounds(m_oldClip);
    }

    operator bool() const { return m_notEmpty; }

  private:
    Graphics* m_graphics;
    gfx::Rect m_oldClip;
    bool m_notEmpty;
  };

  typedef SharedPtr<Graphics> GraphicsPtr;

} // namespace ui

#endif
