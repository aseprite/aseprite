// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/graphics.h"

#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/size.h"
#include "ui/draw.h"
#include "ui/font.h"
#include "ui/theme.h"

#include <allegro.h>
#include <allegro/internal/aintern.h>

namespace ui {

Graphics::Graphics(BITMAP* bmp, int dx, int dy)
  : m_bmp(bmp)
  , m_dx(dx)
  , m_dy(dy)
{
}

Graphics::~Graphics()
{
}

gfx::Rect Graphics::getClipBounds() const
{
  return gfx::Rect(m_bmp->cl-m_dx,
                   m_bmp->ct-m_dy,
                   m_bmp->cr-m_bmp->cl,
                   m_bmp->cb-m_bmp->ct);
}

void Graphics::setClipBounds(const gfx::Rect& rc)
{
  set_clip_rect(m_bmp,
                m_dx+rc.x,
                m_dy+rc.y,
                m_dx+rc.x+rc.w-1,
                m_dy+rc.y+rc.h-1);
}

bool Graphics::intersectClipRect(const gfx::Rect& rc)
{
  add_clip_rect(m_bmp,
                m_dx+rc.x,
                m_dy+rc.y,
                m_dx+rc.x+rc.w-1,
                m_dy+rc.y+rc.h-1);

  return (m_bmp->cl < m_bmp->cr &&
          m_bmp->ct < m_bmp->cb);
}

void Graphics::drawHLine(ui::Color color, int x, int y, int w)
{
  hline(m_bmp,
        m_dx+x,
        m_dy+y,
        m_dx+x+w-1, to_system(color));
}

void Graphics::drawVLine(ui::Color color, int x, int y, int h)
{
  vline(m_bmp,
        m_dx+x,
        m_dy+y,
        m_dy+y+h-1, to_system(color));
}

void Graphics::drawLine(ui::Color color, const gfx::Point& a, const gfx::Point& b)
{
  line(m_bmp,
    m_dx+a.x,
    m_dy+a.y,
    m_dx+b.x,
    m_dy+b.y, to_system(color));
}

void Graphics::drawRect(ui::Color color, const gfx::Rect& rc)
{
  rect(m_bmp,
       m_dx+rc.x,
       m_dy+rc.y,
       m_dx+rc.x+rc.w-1,
       m_dy+rc.y+rc.h-1, to_system(color));
}

void Graphics::fillRect(ui::Color color, const gfx::Rect& rc)
{
  rectfill(m_bmp,
           m_dx+rc.x,
           m_dy+rc.y,
           m_dx+rc.x+rc.w-1,
           m_dy+rc.y+rc.h-1, to_system(color));
}

void Graphics::fillRegion(ui::Color color, const gfx::Region& rgn)
{
  for (gfx::Region::iterator it=rgn.begin(), end=rgn.end(); it!=end; ++it)
    fillRect(color, *it);
}

void Graphics::fillAreaBetweenRects(ui::Color color,
  const gfx::Rect& outer, const gfx::Rect& inner)
{
  if (!outer.intersects(inner))
    fillRect(color, inner);
  else {
    gfx::Region rgn(outer);
    rgn.createSubtraction(rgn, gfx::Region(inner));
    fillRegion(color, rgn);
  }
}

void Graphics::drawBitmap(BITMAP* sprite, int x, int y)
{
  draw_sprite(m_bmp, sprite, m_dx+x, m_dy+y);
}

void Graphics::drawAlphaBitmap(BITMAP* sprite, int x, int y)
{
  set_alpha_blender();
  draw_trans_sprite(m_bmp, sprite, m_dx+x, m_dy+y);
}

void Graphics::blit(BITMAP* src, int srcx, int srcy, int dstx, int dsty, int w, int h)
{
  ::blit(src, m_bmp, srcx, srcy, m_dx+dstx, m_dy+dsty, w, h);
}

void Graphics::setFont(FONT* font)
{
  m_currentFont = font;
}

void Graphics::drawChar(int chr, Color fg, Color bg, int x, int y)
{
    // to_system(is_transparent(bg) ? getColor(ThemeColor::Background): bg));
  ji_font_set_aa_mode(getFont(), bg);
  getFont()->vtable->render_char(getFont(), chr,
    to_system(fg),
    to_system(bg),
    m_bmp, m_dx+x, m_dy+y);
}

void Graphics::drawString(const std::string& str, Color fg, Color bg, bool fillbg, const gfx::Point& pt)
{
  _draw_text(m_bmp, m_currentFont, str.c_str(), m_dx+pt.x, m_dy+pt.y,
    fg, bg, fillbg, 1 * jguiscale());
}

void Graphics::drawString(const std::string& str, Color fg, Color bg, const gfx::Rect& rc, int align)
{
  drawStringAlgorithm(str, fg, bg, rc, align, true);
}

gfx::Size Graphics::measureChar(int chr)
{
  return gfx::Size(
    getFont()->vtable->char_length(getFont(), chr),
    text_height(getFont()));
}

gfx::Size Graphics::measureString(const std::string& str)
{
  return gfx::Size(
    ji_font_text_len(getFont(), str.c_str()),
    text_height(getFont()));
}

gfx::Size Graphics::fitString(const std::string& str, int maxWidth, int align)
{
  return drawStringAlgorithm(str, ColorNone, ColorNone, gfx::Rect(0, 0, maxWidth, 0), align, false);
}

gfx::Size Graphics::drawStringAlgorithm(const std::string& str, Color fg, Color bg, const gfx::Rect& rc, int align, bool draw)
{
  gfx::Point pt(0, rc.y);

  if ((align & (JI_MIDDLE | JI_BOTTOM)) != 0) {
    gfx::Size preSize = drawStringAlgorithm(str, ColorNone, ColorNone, rc, 0, false);
    if (align & JI_MIDDLE)
      pt.y = rc.y + rc.h/2 - preSize.h/2;
    else if (align & JI_BOTTOM)
      pt.y = rc.y + rc.h - preSize.h;
  }

  gfx::Size calculatedSize(0, 0);
  size_t beg, end, new_word_beg, old_end;
  std::string line;

  // Draw line-by-line
  for (beg=end=0; end != std::string::npos; ) {
    pt.x = rc.x;

    // Without word-wrap
    if ((align & JI_WORDWRAP) == 0) {
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
            (pt.x+measureString(str.substr(beg, end-beg)).w > rc.w)) {
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

    gfx::Size lineSize = measureString(line);
    calculatedSize.w = MAX(calculatedSize.w, lineSize.w);

    // Render the text
    if (draw) {
      int xout;
      if ((align & JI_CENTER) == JI_CENTER)
        xout = pt.x + rc.w/2 - lineSize.w/2;
      else if ((align & JI_RIGHT) == JI_RIGHT)
        xout = pt.x + rc.w - lineSize.w;
      else
        xout = pt.x;

      ji_font_set_aa_mode(m_currentFont, to_system(bg));
      textout_ex(m_bmp, m_currentFont, line.c_str(), m_dx+xout, m_dy+pt.y, to_system(fg), to_system(bg));

      if (!is_transparent(bg))
        fillAreaBetweenRects(bg,
          gfx::Rect(rc.x, pt.y, rc.w, lineSize.h),
          gfx::Rect(xout, pt.y, lineSize.w, lineSize.h));
    }

    pt.y += lineSize.h;
    calculatedSize.h += lineSize.h;
    beg = end+1;
  }

  // Fill bottom area
  if (draw && !is_transparent(bg)) {
    if (pt.y < rc.y+rc.h)
      fillRect(bg, gfx::Rect(rc.x, pt.y, rc.w, rc.y+rc.h-pt.y));
  }

  return calculatedSize;
}

//////////////////////////////////////////////////////////////////////
// ScreenGraphics

ScreenGraphics::ScreenGraphics()
  : Graphics(screen, 0, 0)
{
  setFont(font);                // Allegro default font
}

ScreenGraphics::~ScreenGraphics()
{
}

} // namespace ui
