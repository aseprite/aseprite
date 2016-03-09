// Aseprite UI Library
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/graphics.h"

#include "base/string.h"
#include "gfx/clip.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/region.h"
#include "gfx/size.h"
#include "she/display.h"
#include "she/font.h"
#include "she/surface.h"
#include "she/system.h"
#include "ui/manager.h"
#include "ui/theme.h"

namespace ui {

Graphics::Graphics(she::Surface* surface, int dx, int dy)
  : m_surface(surface)
  , m_dx(dx)
  , m_dy(dy)
{
}

Graphics::~Graphics()
{
  // If we were drawing in the screen surface, we mark these regions
  // as dirty for the final flip.
  if (m_surface == she::instance()->defaultDisplay()->getSurface())
    Manager::getDefault()->dirtyRect(m_dirtyBounds);
}

int Graphics::width() const
{
  return m_surface->width();
}

int Graphics::height() const
{
  return m_surface->height();
}

gfx::Rect Graphics::getClipBounds() const
{
  return m_surface->getClipBounds().offset(-m_dx, -m_dy);
}

void Graphics::setClipBounds(const gfx::Rect& rc)
{
  m_surface->setClipBounds(gfx::Rect(rc).offset(m_dx, m_dy));
}

bool Graphics::intersectClipRect(const gfx::Rect& rc)
{
  return m_surface->intersectClipRect(gfx::Rect(rc).offset(m_dx, m_dy));
}

void Graphics::setDrawMode(DrawMode mode, int param)
{
  switch (mode) {
    case DrawMode::Solid:
      m_surface->setDrawMode(she::DrawMode::Solid);
      break;
    case DrawMode::Xor:
      m_surface->setDrawMode(she::DrawMode::Xor);
      break;
    case DrawMode::Checked:
      m_surface->setDrawMode(she::DrawMode::Checked, param);
      break;
  }
}

gfx::Color Graphics::getPixel(int x, int y)
{
  she::SurfaceLock lock(m_surface);
  return m_surface->getPixel(m_dx+x, m_dy+y);
}

void Graphics::putPixel(gfx::Color color, int x, int y)
{
  dirty(gfx::Rect(m_dx+x, m_dy+y, 1, 1));

  she::SurfaceLock lock(m_surface);
  m_surface->putPixel(color, m_dx+x, m_dy+y);
}

void Graphics::drawHLine(gfx::Color color, int x, int y, int w)
{
  dirty(gfx::Rect(m_dx+x, m_dy+y, w, 1));

  she::SurfaceLock lock(m_surface);
  m_surface->drawHLine(color, m_dx+x, m_dy+y, w);
}

void Graphics::drawVLine(gfx::Color color, int x, int y, int h)
{
  dirty(gfx::Rect(m_dx+x, m_dy+y, 1, h));

  she::SurfaceLock lock(m_surface);
  m_surface->drawVLine(color, m_dx+x, m_dy+y, h);
}

void Graphics::drawLine(gfx::Color color, const gfx::Point& _a, const gfx::Point& _b)
{
  gfx::Point a(m_dx+_a.x, m_dy+_a.y);
  gfx::Point b(m_dx+_b.x, m_dy+_b.y);
  dirty(gfx::Rect(a, b));

  she::SurfaceLock lock(m_surface);
  m_surface->drawLine(color, a, b);
}

void Graphics::drawRect(gfx::Color color, const gfx::Rect& rcOrig)
{
  gfx::Rect rc(rcOrig);
  rc.offset(m_dx, m_dy);
  dirty(rc);

  she::SurfaceLock lock(m_surface);
  m_surface->drawRect(color, rc);
}

void Graphics::fillRect(gfx::Color color, const gfx::Rect& rcOrig)
{
  gfx::Rect rc(rcOrig);
  rc.offset(m_dx, m_dy);
  dirty(rc);

  she::SurfaceLock lock(m_surface);
  m_surface->fillRect(color, rc);
}

void Graphics::fillRegion(gfx::Color color, const gfx::Region& rgn)
{
  for (gfx::Region::iterator it=rgn.begin(), end=rgn.end(); it!=end; ++it)
    fillRect(color, *it);
}

void Graphics::fillAreaBetweenRects(gfx::Color color,
  const gfx::Rect& outer, const gfx::Rect& inner)
{
  if (!outer.intersects(inner))
    fillRect(color, outer);
  else {
    gfx::Region rgn(outer);
    rgn.createSubtraction(rgn, gfx::Region(inner));
    fillRegion(color, rgn);
  }
}

void Graphics::drawSurface(she::Surface* surface, int x, int y)
{
  dirty(gfx::Rect(m_dx+x, m_dy+y, surface->width(), surface->height()));

  she::SurfaceLock lockSrc(surface);
  she::SurfaceLock lockDst(m_surface);
  m_surface->drawSurface(surface, m_dx+x, m_dy+y);
}

void Graphics::drawRgbaSurface(she::Surface* surface, int x, int y)
{
  dirty(gfx::Rect(m_dx+x, m_dy+y, surface->width(), surface->height()));

  she::SurfaceLock lockSrc(surface);
  she::SurfaceLock lockDst(m_surface);
  m_surface->drawRgbaSurface(surface, m_dx+x, m_dy+y);
}

void Graphics::drawColoredRgbaSurface(she::Surface* surface, gfx::Color color, int x, int y)
{
  dirty(gfx::Rect(m_dx+x, m_dy+y, surface->width(), surface->height()));

  she::SurfaceLock lockSrc(surface);
  she::SurfaceLock lockDst(m_surface);
  m_surface->drawColoredRgbaSurface(surface, color, gfx::ColorNone,
    gfx::Clip(m_dx+x, m_dy+y, 0, 0, surface->width(), surface->height()));
}

void Graphics::blit(she::Surface* srcSurface, int srcx, int srcy, int dstx, int dsty, int w, int h)
{
  dirty(gfx::Rect(m_dx+dstx, m_dy+dsty, w, h));

  she::SurfaceLock lockSrc(srcSurface);
  she::SurfaceLock lockDst(m_surface);
  srcSurface->blitTo(m_surface, srcx, srcy, m_dx+dstx, m_dy+dsty, w, h);
}

void Graphics::setFont(she::Font* font)
{
  m_font = font;
}

void Graphics::drawChar(int chr, gfx::Color fg, gfx::Color bg, int x, int y)
{
  dirty(gfx::Rect(gfx::Point(m_dx+x, m_dy+y), measureChar(chr)));

  she::SurfaceLock lock(m_surface);
  m_surface->drawChar(m_font, fg, bg, m_dx+x, m_dy+y, chr);
}

void Graphics::drawString(const std::string& str, gfx::Color fg, gfx::Color bg, const gfx::Point& ptOrig)
{
  gfx::Point pt(m_dx+ptOrig.x, m_dy+ptOrig.y);
  dirty(gfx::Rect(pt.x, pt.y, m_font->textLength(str), m_font->height()));

  she::SurfaceLock lock(m_surface);
  m_surface->drawString(m_font, fg, bg, pt.x, pt.y, str);
}

void Graphics::drawUIString(const std::string& str, gfx::Color fg, gfx::Color bg, const gfx::Point& pt,
                            bool drawUnderscore)
{
  she::SurfaceLock lock(m_surface);
  base::utf8_const_iterator it(str.begin()), end(str.end());
  int x = m_dx+pt.x;
  int y = m_dy+pt.y;
  int underscored_x = 0;
  int underscored_w = -1;

  while (it != end) {
    if (*it == '&') {
      ++it;
      if (it != end && *it != '&') {
        underscored_x = x;
        underscored_w = m_font->charWidth(*it);
      }
    }
    m_surface->drawChar(m_font, fg, bg, x, y, *it);
    x += m_font->charWidth(*it);
    ++it;
  }

  y += m_font->height();
  if (drawUnderscore && underscored_w > 0) {
    m_surface->fillRect(fg,
      gfx::Rect(underscored_x, y, underscored_w, guiscale()));
    y += guiscale();
  }

  dirty(gfx::Rect(pt, gfx::Point(x, y)));
}

void Graphics::drawAlignedUIString(const std::string& str, gfx::Color fg, gfx::Color bg, const gfx::Rect& rc, int align)
{
  doUIStringAlgorithm(str, fg, bg, rc, align, true);
}

gfx::Size Graphics::measureChar(int chr)
{
  return gfx::Size(
    m_font->charWidth(chr),
    m_font->height());
}

gfx::Size Graphics::measureUIString(const std::string& str)
{
  return gfx::Size(
    Graphics::measureUIStringLength(str, m_font),
    m_font->height());
}

// static
int Graphics::measureUIStringLength(const std::string& str, she::Font* font)
{
  base::utf8_const_iterator it(str.begin()), end(str.end());
  int length = 0;

  while (it != end) {
    if (*it == '&')
      ++it;

    length += font->charWidth(*it);
    ++it;
  }

  return length;
}

gfx::Size Graphics::fitString(const std::string& str, int maxWidth, int align)
{
  return doUIStringAlgorithm(str, gfx::ColorNone, gfx::ColorNone, gfx::Rect(0, 0, maxWidth, 0), align, false);
}

gfx::Size Graphics::doUIStringAlgorithm(const std::string& str, gfx::Color fg, gfx::Color bg, const gfx::Rect& rc, int align, bool draw)
{
  gfx::Point pt(0, rc.y);

  if ((align & (MIDDLE | BOTTOM)) != 0) {
    gfx::Size preSize = doUIStringAlgorithm(str, gfx::ColorNone, gfx::ColorNone, rc, 0, false);
    if (align & MIDDLE)
      pt.y = rc.y + rc.h/2 - preSize.h/2;
    else if (align & BOTTOM)
      pt.y = rc.y + rc.h - preSize.h;
  }

  gfx::Size calculatedSize(0, 0);
  std::size_t beg, end, new_word_beg, old_end;
  std::string line;
  int lineSeparation = 2*guiscale();

  // Draw line-by-line
  for (beg=end=0; end != std::string::npos; ) {
    pt.x = rc.x;

    // Without word-wrap
    if ((align & WORDWRAP) == 0) {
      end = str.find('\n', beg);
    }
    // With word-wrap
    else {
      old_end = std::string::npos;
      for (new_word_beg=beg;;) {
        end = str.find_first_of(" \n", new_word_beg);

        // If we have already a word to print (old_end != npos), and
        // we are out of the available width (rc.w) using the new "end",
        if ((old_end != std::string::npos) &&
            (rc.w > 0) &&
            (pt.x+m_font->textLength(str.substr(beg, end-beg).c_str()) > rc.w)) {
          // We go back to the "old_end" and paint from "beg" to "end"
          end = old_end;
          break;
        }
        // If we have more words to print...
        else if (end != std::string::npos) {
          // Force line break, now we have to paint from "beg" to "end"
          if (str[end] == '\n')
            break;

          // White-space, this is a beginning of a new word.
          new_word_beg = end+1;
        }
        // We are in the end of text
        else
          break;

        old_end = end;
      }
    }

    // Get the entire line to be painted
    line = str.substr(beg, end-beg);

    gfx::Size lineSize(
      m_font->textLength(line.c_str()),
      m_font->height()+lineSeparation);
    calculatedSize.w = MAX(calculatedSize.w, lineSize.w);

    // Render the text
    if (draw) {
      int xout;
      if ((align & CENTER) == CENTER)
        xout = pt.x + rc.w/2 - lineSize.w/2;
      else if ((align & RIGHT) == RIGHT)
        xout = pt.x + rc.w - lineSize.w;
      else
        xout = pt.x;

      drawString(line, fg, bg, gfx::Point(xout, pt.y));

      if (!gfx::is_transparent(bg))
        fillAreaBetweenRects(bg,
          gfx::Rect(rc.x, pt.y, rc.w, lineSize.h),
          gfx::Rect(xout, pt.y, lineSize.w, lineSize.h));
    }

    pt.y += lineSize.h;
    calculatedSize.h += lineSize.h;
    beg = end+1;
  }

  if (calculatedSize.h > 0)
    calculatedSize.h -= lineSeparation;

  // Fill bottom area
  if (draw && !gfx::is_transparent(bg)) {
    if (pt.y < rc.y+rc.h)
      fillRect(bg, gfx::Rect(rc.x, pt.y, rc.w, rc.y+rc.h-pt.y));
  }

  return calculatedSize;
}

//////////////////////////////////////////////////////////////////////
// ScreenGraphics

ScreenGraphics::ScreenGraphics()
  : Graphics(she::instance()->defaultDisplay()->getSurface(), 0, 0)
{
  setFont(CurrentTheme::get()->getDefaultFont());
}

ScreenGraphics::~ScreenGraphics()
{
}

} // namespace ui
