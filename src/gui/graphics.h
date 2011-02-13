// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_GRAPHICS_H_INCLUDED
#define GUI_GRAPHICS_H_INCLUDED

#include <string>
#include "gfx/rect.h"

struct BITMAP;
struct FONT;

// Class to render a widget in the screen.
class Graphics
{
public:
  Graphics(BITMAP* bmp, int dx, int dy);
  ~Graphics();

  int getBitsPerPixel() const;

  gfx::Rect getClipBounds() const;
  void setClipBounds(const gfx::Rect& rc);
  bool intersectClipRect(const gfx::Rect& rc);

  void drawVLine(int color, int x, int y, int h);

  void drawRect(int color, const gfx::Rect& rc);
  void fillRect(int color, const gfx::Rect& rc);

  void drawBitmap(BITMAP* sprite, int x, int y);
  void drawAlphaBitmap(BITMAP* sprite, int x, int y);

  void blit(BITMAP* src, int srcx, int srcy, int dstx, int dsty, int w, int h);

  // ======================================================================
  // FONT & TEXT
  // ======================================================================

  void setFont(FONT* font);
  FONT* getFont();

  void drawString(const std::string& str, int fg_color, int bg_color, bool fillbg, const gfx::Point& pt);

  gfx::Size measureString(const std::string& str);
  
private:
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

#endif
