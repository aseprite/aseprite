// Aseprite UI Library
// Copyright (C) 2019-2022  Igara Studio S.A.
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
#include "gfx/matrix.h"
#include "gfx/path.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/region.h"
#include "gfx/size.h"
#include "os/draw_text.h"
#include "os/font.h"
#include "os/sampling.h"
#include "os/surface.h"
#include "os/system.h"
#include "os/window.h"
#include "ui/display.h"
#include "ui/scale.h"
#include "ui/theme.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace ui {

Graphics::Graphics(Display* display, const os::SurfaceRef& surface, int dx, int dy)
  : m_display(display)
  , m_surface(surface)
  , m_dx(dx)
  , m_dy(dy)
{
}

Graphics::~Graphics()
{
  // If we were drawing in the screen surface, we mark these regions
  // as dirty for the final flip.
  if (m_display)
    m_display->dirtyRect(m_dirtyBounds);
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

void Graphics::save()
{
  m_surface->save();
}

void Graphics::concat(const gfx::Matrix& matrix)
{
  m_surface->concat(matrix);
}

void Graphics::setMatrix(const gfx::Matrix& matrix)
{
  m_surface->setMatrix(matrix);
}

void Graphics::resetMatrix()
{
  m_surface->resetMatrix();
}

void Graphics::restore()
{
  m_surface->restore();
}

gfx::Matrix Graphics::matrix() const
{
  return m_surface->matrix();
}

gfx::Color Graphics::getPixel(int x, int y)
{
  os::SurfaceLock lock(m_surface.get());
  return m_surface->getPixel(m_dx+x, m_dy+y);
}

void Graphics::putPixel(gfx::Color color, int x, int y)
{
  dirty(gfx::Rect(m_dx+x, m_dy+y, 1, 1));

  os::SurfaceLock lock(m_surface.get());
  m_surface->putPixel(color, m_dx+x, m_dy+y);
}

void Graphics::drawHLine(int x, int y, int w, const Paint& paint)
{
  dirty(gfx::Rect(m_dx+x, m_dy+y, w, 1));

  os::SurfaceLock lock(m_surface.get());
  m_surface->drawRect(gfx::Rect(m_dx+x, m_dy+y, w, 1), paint);
}

void Graphics::drawHLine(gfx::Color color, int x, int y, int w)
{
  dirty(gfx::Rect(m_dx+x, m_dy+y, w, 1));

  os::SurfaceLock lock(m_surface.get());
  os::Paint paint;
  paint.color(color);
  m_surface->drawRect(gfx::Rect(m_dx+x, m_dy+y, w, 1), paint);
}

void Graphics::drawVLine(int x, int y, int h, const Paint& paint)
{
  dirty(gfx::Rect(m_dx+x, m_dy+y, 1, h));

  os::SurfaceLock lock(m_surface.get());
  m_surface->drawRect(gfx::Rect(m_dx+x, m_dy+y, 1, h), paint);
}

void Graphics::drawVLine(gfx::Color color, int x, int y, int h)
{
  dirty(gfx::Rect(m_dx+x, m_dy+y, 1, h));

  os::SurfaceLock lock(m_surface.get());
  os::Paint paint;
  paint.color(color);
  m_surface->drawRect(gfx::Rect(m_dx+x, m_dy+y, 1, h), paint);
}

void Graphics::drawLine(gfx::Color color, const gfx::Point& _a, const gfx::Point& _b)
{
  gfx::Point a(m_dx+_a.x, m_dy+_a.y);
  gfx::Point b(m_dx+_b.x, m_dy+_b.y);
  dirty(gfx::Rect(a, b));

  os::SurfaceLock lock(m_surface.get());
  os::Paint paint;
  paint.color(color);
  m_surface->drawLine(a, b, paint);
}

void Graphics::drawPath(gfx::Path& path, const Paint& paint)
{
  os::SurfaceLock lock(m_surface.get());

  auto m = matrix();
  save();
  setMatrix(gfx::Matrix::MakeTrans(m_dx, m_dy));
  concat(m);

  m_surface->drawPath(path, paint);

  dirty(matrix().mapRect(path.bounds()).inflate(1, 1));
  restore();
}

void Graphics::drawRect(const gfx::Rect& rcOrig, const Paint& paint)
{
  gfx::Rect rc(rcOrig);
  rc.offset(m_dx, m_dy);
  dirty(rc);

  os::SurfaceLock lock(m_surface.get());
  m_surface->drawRect(rc, paint);
}

void Graphics::drawRect(gfx::Color color, const gfx::Rect& rcOrig)
{
  gfx::Rect rc(rcOrig);
  rc.offset(m_dx, m_dy);
  dirty(rc);

  os::SurfaceLock lock(m_surface.get());
  os::Paint paint;
  paint.color(color);
  paint.style(os::Paint::Stroke);
  m_surface->drawRect(rc, paint);
}

void Graphics::fillRect(gfx::Color color, const gfx::Rect& rcOrig)
{
  gfx::Rect rc(rcOrig);
  rc.offset(m_dx, m_dy);
  dirty(rc);

  os::SurfaceLock lock(m_surface.get());
  os::Paint paint;
  paint.color(color);
  paint.style(os::Paint::Fill);
  m_surface->drawRect(rc, paint);
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
  os::SurfaceLock lockDst(m_surface.get());
  m_surface->drawSurface(surface, m_dx+x, m_dy+y);
}

void Graphics::drawSurface(os::Surface* surface,
                           const gfx::Rect& srcRect,
                           const gfx::Rect& dstRect,
                           const os::Sampling& sampling,
                           const ui::Paint* paint)
{
  dirty(gfx::Rect(m_dx+dstRect.x, m_dy+dstRect.y,
                  dstRect.w, dstRect.h));

  os::SurfaceLock lockSrc(surface);
  os::SurfaceLock lockDst(m_surface.get());
  m_surface->drawSurface(
    surface,
    srcRect,
    gfx::Rect(dstRect).offset(m_dx, m_dy),
    sampling,
    paint);
}

void Graphics::drawRgbaSurface(os::Surface* surface, int x, int y)
{
  dirty(gfx::Rect(m_dx+x, m_dy+y, surface->width(), surface->height()));

  os::SurfaceLock lockSrc(surface);
  os::SurfaceLock lockDst(m_surface.get());
  m_surface->drawRgbaSurface(surface, m_dx+x, m_dy+y);
}

void Graphics::drawRgbaSurface(os::Surface* surface, int srcx, int srcy, int dstx, int dsty, int w, int h)
{
  dirty(gfx::Rect(m_dx+dstx, m_dy+dsty, w, h));

  os::SurfaceLock lockSrc(surface);
  os::SurfaceLock lockDst(m_surface.get());
  m_surface->drawRgbaSurface(surface, srcx, srcy, m_dx+dstx, m_dy+dsty, w, h);
}

void Graphics::drawColoredRgbaSurface(os::Surface* surface, gfx::Color color, int x, int y)
{
  dirty(gfx::Rect(m_dx+x, m_dy+y, surface->width(), surface->height()));

  os::SurfaceLock lockSrc(surface);
  os::SurfaceLock lockDst(m_surface.get());
  m_surface->drawColoredRgbaSurface(surface, color, gfx::ColorNone,
    gfx::Clip(m_dx+x, m_dy+y, 0, 0, surface->width(), surface->height()));
}

void Graphics::drawColoredRgbaSurface(os::Surface* surface, gfx::Color color,
                                      int srcx, int srcy, int dstx, int dsty, int w, int h)
{
  dirty(gfx::Rect(m_dx+dstx, m_dy+dsty, w, h));

  os::SurfaceLock lockSrc(surface);
  os::SurfaceLock lockDst(m_surface.get());
  m_surface->drawColoredRgbaSurface(surface, color, gfx::ColorNone,
    gfx::Clip(m_dx+dstx, m_dy+dsty, srcx, srcy, w, h));
}

void Graphics::drawSurfaceNine(os::Surface* surface,
                               const gfx::Rect& src,
                               const gfx::Rect& center,
                               const gfx::Rect& dst,
                               const bool drawCenter,
                               const ui::Paint* paint)
{
  gfx::Rect displacedDst(m_dx+dst.x, m_dy+dst.y, dst.w, dst.h);
  dirty(displacedDst);

  os::SurfaceLock lockSrc(surface);
  os::SurfaceLock lockDst(m_surface.get());
  m_surface->drawSurfaceNine(surface, src, center, displacedDst, drawCenter, paint);
}

void Graphics::blit(os::Surface* srcSurface, int srcx, int srcy, int dstx, int dsty, int w, int h)
{
  dirty(gfx::Rect(m_dx+dstx, m_dy+dsty, w, h));

  os::SurfaceLock lockSrc(srcSurface);
  os::SurfaceLock lockDst(m_surface.get());
  srcSurface->blitTo(m_surface.get(), srcx, srcy, m_dx+dstx, m_dy+dsty, w, h);
}

void Graphics::setFont(const os::FontRef& font)
{
  m_font = font;
}

void Graphics::drawText(const std::string& str,
                        gfx::Color fg, gfx::Color bg,
                        const gfx::Point& origPt,
                        os::DrawTextDelegate* delegate)
{
  gfx::Point pt(m_dx+origPt.x, m_dy+origPt.y);

  os::SurfaceLock lock(m_surface.get());
  gfx::Rect textBounds =
    os::draw_text(m_surface.get(), m_font.get(), str, fg, bg, pt.x, pt.y, delegate);

  dirty(gfx::Rect(pt.x, pt.y, textBounds.w, textBounds.h));
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
                      gfx::Color& bg,
                      const gfx::Rect& charBounds) override {
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
      if (m_font->type() == os::FontType::FreeType) // TODO use other method to locate the underline
        dy += guiscale();
      gfx::Rect underscoreBounds(charBounds.x, charBounds.y+charBounds.h+dy,
                                 charBounds.w, guiscale());

      os::Paint paint;
      paint.color(m_underscoreColor);
      paint.style(os::Paint::Fill);
      m_surface->drawRect(underscoreBounds, paint);

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
  os::SurfaceLock lock(m_surface.get());
  int x = m_dx+pt.x;
  int y = m_dy+pt.y;

  DrawUITextDelegate delegate(m_surface.get(), m_font.get(), mnemonic);
  os::draw_text(m_surface.get(), m_font.get(), str,
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
    Graphics::measureUITextLength(str, m_font.get()),
    m_font->height());
}

// static
int Graphics::measureUITextLength(const std::string& str, os::Font* font)
{
  DrawUITextDelegate delegate(nullptr, font, 0);
  os::draw_text(nullptr, font, str,
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
  std::size_t beg, end, newBeg;
  std::string line;
  int lineSeparation = 2*guiscale();

  // Draw line-by-line
  for (beg=end=0; end != std::string::npos && beg<str.size(); ) {
    pt.x = rc.x;

    // Without word-wrap
    if ((align & (WORDWRAP | CHARWRAP)) == 0) {
      end = str.find('\n', beg);
      if (end != std::string::npos)
        newBeg = end+1;
      else
        newBeg = std::string::npos;
    }
    // With char-wrap
    else if ((align & CHARWRAP) == CHARWRAP) {
      for (end=beg+1; end<str.size(); ++end) {
        // If we are out of the available width (rc.w) using the new "end"
        if ((rc.w > 0) &&
            (m_font->textLength(str.substr(beg, end-beg).c_str()) > rc.w)) {
          if (end > beg+1)
            --end;
          break;
        }
      }
      newBeg = end;
    }
    // With word-wrap
    else {
      std::size_t old_end = std::string::npos;
      for (std::size_t new_word_beg=beg;;) {
        end = str.find_first_of(" \n", new_word_beg);

        // If we have already a word to print (old_end != npos), and
        // we are out of the available width (rc.w) using the new "end"
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
      newBeg = end+1;
    }

    // Get the entire line to be painted
    if (end != std::string::npos)
      line = str.substr(beg, end-beg);
    else
      line = str.substr(beg);

    gfx::Size lineSize(
      m_font->textLength(line.c_str()),
      m_font->height()+lineSeparation);
    calculatedSize.w = std::max(calculatedSize.w, lineSize.w);

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
    beg = newBeg;

    if (pt.y+lineSize.h >= rc.y2())
      break;
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

void Graphics::invalidate(const gfx::Rect& bounds)
{
  dirty(gfx::Rect(bounds).offset(m_dx, m_dy));
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

ScreenGraphics::ScreenGraphics(Display* display)
  : Graphics(display, AddRef(display->surface()), 0, 0)
{
  setFont(AddRef(get_theme()->getDefaultFont()));
}

ScreenGraphics::~ScreenGraphics()
{
}

} // namespace ui
