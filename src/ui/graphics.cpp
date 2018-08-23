// Aseprite UI Library
// Copyright (C) 2001-2018  David Capello
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
#include "os/display.h"
#include "os/draw_text.h"
#include "os/font.h"
#include "os/surface.h"
#include "os/system.h"
#include "ui/manager.h"
#include "ui/scale.h"
#include "ui/theme.h"

#include <cctype>

namespace ui {

Graphics::Graphics(os::Surface* surface, int dx, int dy)
  : m_surface(surface)
  , m_dx(dx)
  , m_dy(dy)
{
}

Graphics::~Graphics()
{
  // If we were drawing in the screen surface, we mark these regions
  // as dirty for the final flip.
  if (m_surface == os::instance()->defaultDisplay()->getSurface())
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

int Graphics::getSaveCount() const
{
  return m_surface->getSaveCount();
}

gfx::Rect Graphics::getClipBounds() const
{
  return m_surface->getClipBounds().offset(-m_dx, -m_dy);
}

void Graphics::saveClip()
{
  m_surface->saveClip();
}

void Graphics::restoreClip()
{
  m_surface->restoreClip();
}

bool Graphics::clipRect(const gfx::Rect& rc)
{
  return m_surface->clipRect(gfx::Rect(rc).offset(m_dx, m_dy));
}

void Graphics::setDrawMode(DrawMode mode, int param,
                           const gfx::Color a,
                           const gfx::Color b)
{
  switch (mode) {
    case DrawMode::Solid:
      m_surface->setDrawMode(os::DrawMode::Solid);
      break;
    case DrawMode::Xor:
      m_surface->setDrawMode(os::DrawMode::Xor);
      break;
    case DrawMode::Checked:
      m_surface->setDrawMode(os::DrawMode::Checked, param, a, b);
      break;
  }
}

gfx::Color Graphics::getPixel(int x, int y)
{
  os::SurfaceLock lock(m_surface);
  return m_surface->getPixel(m_dx+x, m_dy+y);
}

void Graphics::putPixel(gfx::Color color, int x, int y)
{
  dirty(gfx::Rect(m_dx+x, m_dy+y, 1, 1));

  os::SurfaceLock lock(m_surface);
  m_surface->putPixel(color, m_dx+x, m_dy+y);
}

void Graphics::drawHLine(gfx::Color color, int x, int y, int w)
{
  dirty(gfx::Rect(m_dx+x, m_dy+y, w, 1));

  os::SurfaceLock lock(m_surface);
  m_surface->drawHLine(color, m_dx+x, m_dy+y, w);
}

void Graphics::drawVLine(gfx::Color color, int x, int y, int h)
{
  dirty(gfx::Rect(m_dx+x, m_dy+y, 1, h));

  os::SurfaceLock lock(m_surface);
  m_surface->drawVLine(color, m_dx+x, m_dy+y, h);
}

void Graphics::drawLine(gfx::Color color, const gfx::Point& _a, const gfx::Point& _b)
{
  gfx::Point a(m_dx+_a.x, m_dy+_a.y);
  gfx::Point b(m_dx+_b.x, m_dy+_b.y);
  dirty(gfx::Rect(a, b));

  os::SurfaceLock lock(m_surface);
  m_surface->drawLine(color, a, b);
}

void Graphics::drawRect(gfx::Color color, const gfx::Rect& rcOrig)
{
  gfx::Rect rc(rcOrig);
  rc.offset(m_dx, m_dy);
  dirty(rc);

  os::SurfaceLock lock(m_surface);
  m_surface->drawRect(color, rc);
}

void Graphics::fillRect(gfx::Color color, const gfx::Rect& rcOrig)
{
  gfx::Rect rc(rcOrig);
  rc.offset(m_dx, m_dy);
  dirty(rc);

  os::SurfaceLock lock(m_surface);
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

void Graphics::drawSurface(os::Surface* surface, int x, int y)
{
  dirty(gfx::Rect(m_dx+x, m_dy+y, surface->width(), surface->height()));

  os::SurfaceLock lockSrc(surface);
  os::SurfaceLock lockDst(m_surface);
  m_surface->drawSurface(surface, m_dx+x, m_dy+y);
}

void Graphics::drawSurface(os::Surface* surface,
                           const gfx::Rect& srcRect,
                           const gfx::Rect& dstRect)
{
  dirty(gfx::Rect(m_dx+dstRect.x, m_dy+dstRect.y,
                  dstRect.w, dstRect.h));

  os::SurfaceLock lockSrc(surface);
  os::SurfaceLock lockDst(m_surface);
  m_surface->drawSurface(
    surface,
    srcRect,
    gfx::Rect(dstRect).offset(m_dx, m_dy));
}

void Graphics::drawRgbaSurface(os::Surface* surface, int x, int y)
{
  dirty(gfx::Rect(m_dx+x, m_dy+y, surface->width(), surface->height()));

  os::SurfaceLock lockSrc(surface);
  os::SurfaceLock lockDst(m_surface);
  m_surface->drawRgbaSurface(surface, m_dx+x, m_dy+y);
}

void Graphics::drawRgbaSurface(os::Surface* surface, int srcx, int srcy, int dstx, int dsty, int w, int h)
{
  dirty(gfx::Rect(m_dx+dstx, m_dy+dsty, w, h));

  os::SurfaceLock lockSrc(surface);
  os::SurfaceLock lockDst(m_surface);
  m_surface->drawRgbaSurface(surface, srcx, srcy, m_dx+dstx, m_dy+dsty, w, h);
}

void Graphics::drawRgbaSurface(os::Surface* surface,
                               const gfx::Rect& srcRect,
                               const gfx::Rect& dstRect)
{
  dirty(gfx::Rect(m_dx+dstRect.x, m_dy+dstRect.y,
                  dstRect.w, dstRect.h));

  os::SurfaceLock lockSrc(surface);
  os::SurfaceLock lockDst(m_surface);
  m_surface->drawRgbaSurface(
    surface,
    srcRect,
    gfx::Rect(dstRect).offset(m_dx, m_dy));
}

void Graphics::drawColoredRgbaSurface(os::Surface* surface, gfx::Color color, int x, int y)
{
  dirty(gfx::Rect(m_dx+x, m_dy+y, surface->width(), surface->height()));

  os::SurfaceLock lockSrc(surface);
  os::SurfaceLock lockDst(m_surface);
  m_surface->drawColoredRgbaSurface(surface, color, gfx::ColorNone,
    gfx::Clip(m_dx+x, m_dy+y, 0, 0, surface->width(), surface->height()));
}

void Graphics::drawColoredRgbaSurface(os::Surface* surface, gfx::Color color,
                                      int srcx, int srcy, int dstx, int dsty, int w, int h)
{
  dirty(gfx::Rect(m_dx+dstx, m_dy+dsty, w, h));

  os::SurfaceLock lockSrc(surface);
  os::SurfaceLock lockDst(m_surface);
  m_surface->drawColoredRgbaSurface(surface, color, gfx::ColorNone,
    gfx::Clip(m_dx+dstx, m_dy+dsty, srcx, srcy, w, h));
}

void Graphics::blit(os::Surface* srcSurface, int srcx, int srcy, int dstx, int dsty, int w, int h)
{
  dirty(gfx::Rect(m_dx+dstx, m_dy+dsty, w, h));

  os::SurfaceLock lockSrc(srcSurface);
  os::SurfaceLock lockDst(m_surface);
  srcSurface->blitTo(m_surface, srcx, srcy, m_dx+dstx, m_dy+dsty, w, h);
}

void Graphics::setFont(os::Font* font)
{
  m_font = font;
}

void Graphics::drawText(base::utf8_const_iterator it,
                        const base::utf8_const_iterator& end,
                        gfx::Color fg, gfx::Color bg,
                        const gfx::Point& origPt,
                        os::DrawTextDelegate* delegate)
{
  gfx::Point pt(m_dx+origPt.x, m_dy+origPt.y);

  os::SurfaceLock lock(m_surface);
  gfx::Rect textBounds =
    os::draw_text(m_surface, m_font, it, end, fg, bg, pt.x, pt.y, delegate);

  dirty(gfx::Rect(pt.x, pt.y, textBounds.w, textBounds.h));
}

void Graphics::drawText(const std::string& str, gfx::Color fg, gfx::Color bg, const gfx::Point& pt)
{
  drawText(base::utf8_const_iterator(str.begin()),
           base::utf8_const_iterator(str.end()),
           fg, bg, pt, nullptr);
}

namespace {

class DrawUITextDelegate : public os::DrawTextDelegate {
public:
  DrawUITextDelegate(os::Surface* surface,
                     os::Font* font, const int mnemonic)
    : m_surface(surface)
    , m_font(font)
    , m_mnemonic(std::tolower(mnemonic))
    , m_underscoreColor(gfx::ColorNone) {
  }

  gfx::Rect bounds() const { return m_bounds; }

  void preProcessChar(const int index,
                      const int codepoint,
                      gfx::Color& fg,
                      gfx::Color& bg) override {
    if (m_surface) {
      if (m_mnemonic &&
          // TODO use ICU library to lower unicode chars
          std::tolower(codepoint) == m_mnemonic) {
        m_underscoreColor = fg;
        m_mnemonic = 0;         // Just one time
      }
      else {
        m_underscoreColor = gfx::ColorNone;
      }
    }
  }

  bool preDrawChar(const gfx::Rect& charBounds) override {
    m_bounds |= charBounds;
    return true;
  }

  void postDrawChar(const gfx::Rect& charBounds) override {
    if (!gfx::is_transparent(m_underscoreColor)) {
      // TODO underscore height = guiscale() should be configurable from ui::Theme
      int dy = 0;
      if (m_font->type() == os::FontType::kTrueType) // TODO use other method to locate the underline
        dy += guiscale();
      gfx::Rect underscoreBounds(charBounds.x, charBounds.y+charBounds.h+dy,
                                 charBounds.w, guiscale());
      m_surface->fillRect(m_underscoreColor, underscoreBounds);
      m_bounds |= underscoreBounds;
    }
  }

private:
  os::Surface* m_surface;
  os::Font* m_font;
  int m_mnemonic;
  gfx::Color m_underscoreColor;
  gfx::Rect m_bounds;
};

}

void Graphics::drawUIText(const std::string& str, gfx::Color fg, gfx::Color bg,
                          const gfx::Point& pt, const int mnemonic)
{
  os::SurfaceLock lock(m_surface);
  int x = m_dx+pt.x;
  int y = m_dy+pt.y;

  DrawUITextDelegate delegate(m_surface, m_font, mnemonic);
  os::draw_text(m_surface, m_font,
                 base::utf8_const_iterator(str.begin()),
                 base::utf8_const_iterator(str.end()),
                 fg, bg, x, y, &delegate);

  dirty(delegate.bounds());
}

void Graphics::drawAlignedUIText(const std::string& str, gfx::Color fg, gfx::Color bg,
                                 const gfx::Rect& rc, const int align)
{
  doUIStringAlgorithm(str, fg, bg, rc, align, true);
}

gfx::Size Graphics::measureUIText(const std::string& str)
{
  return gfx::Size(
    Graphics::measureUITextLength(str, m_font),
    m_font->height());
}

// static
int Graphics::measureUITextLength(const std::string& str, os::Font* font)
{
  DrawUITextDelegate delegate(nullptr, font, 0);
  os::draw_text(nullptr, font,
                 base::utf8_const_iterator(str.begin()),
                 base::utf8_const_iterator(str.end()),
                 gfx::ColorNone, gfx::ColorNone, 0, 0,
                 &delegate);
  return delegate.bounds().w;
}

gfx::Size Graphics::fitString(const std::string& str, int maxWidth, int align)
{
  return doUIStringAlgorithm(str, gfx::ColorNone, gfx::ColorNone,
                             gfx::Rect(0, 0, maxWidth, 0), align, false);
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

      drawText(line, fg, bg, gfx::Point(xout, pt.y));

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

void Graphics::dirty(const gfx::Rect& bounds)
{
  gfx::Rect rc = m_surface->getClipBounds();
  rc = rc.createIntersection(bounds);
  if (!rc.isEmpty())
    m_dirtyBounds |= rc;
}

//////////////////////////////////////////////////////////////////////
// ScreenGraphics

ScreenGraphics::ScreenGraphics()
  : Graphics(os::instance()->defaultDisplay()->getSurface(), 0, 0)
{
  setFont(get_theme()->getDefaultFont());
}

ScreenGraphics::~ScreenGraphics()
{
}

} // namespace ui
