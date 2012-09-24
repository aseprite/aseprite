// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <allegro.h>

#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/size.h"
#include "ui/draw.h"
#include "ui/font.h"
#include "ui/graphics.h"
#include "ui/theme.h"

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

int Graphics::getBitsPerPixel() const
{
  return bitmap_color_depth(m_bmp);
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

void Graphics::drawVLine(int color, int x, int y, int h)
{
  vline(m_bmp,
        m_dx+x,
        m_dy+y,
        m_dy+y+h-1, color);
}

void Graphics::drawRect(int color, const gfx::Rect& rc)
{
  rect(m_bmp,
       m_dx+rc.x,
       m_dy+rc.y,
       m_dx+rc.x+rc.w-1,
       m_dy+rc.y+rc.h-1, color);
}

void Graphics::fillRect(int color, const gfx::Rect& rc)
{
  rectfill(m_bmp,
           m_dx+rc.x,
           m_dy+rc.y,
           m_dx+rc.x+rc.w-1,
           m_dy+rc.y+rc.h-1, color);
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

FONT* Graphics::getFont()
{
  return m_currentFont;
}

void Graphics::drawString(const std::string& str, int fg_color, int bg_color, bool fill_bg, const gfx::Point& pt)
{
  jdraw_text(m_bmp, m_currentFont, str.c_str(), m_dx+pt.x, m_dy+pt.y,
             fg_color, bg_color, fill_bg, 1 * jguiscale());
}

void Graphics::drawString(const std::string& str, int fg_color, int bg_color, const gfx::Rect& rc, int align)
{
  drawStringAlgorithm(str, fg_color, bg_color, rc, align, true);
}

gfx::Size Graphics::measureString(const std::string& str)
{
  return gfx::Size(ji_font_text_len(m_currentFont, str.c_str()),
                   text_height(m_currentFont));
}

gfx::Size Graphics::fitString(const std::string& str, int maxWidth, int align)
{
  return drawStringAlgorithm(str, 0, 0, gfx::Rect(0, 0, maxWidth, 0), align, false);
}

gfx::Size Graphics::drawStringAlgorithm(const std::string& str, int fg_color, int bg_color, const gfx::Rect& rc, int align, bool draw)
{
  gfx::Size calculatedSize(0, 0);
  size_t beg, end, new_word_beg, old_end;
  std::string line;
  gfx::Point pt;

  // Draw line-by-line
  pt.y = rc.y;
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

      ji_font_set_aa_mode(m_currentFont, bg_color);
      textout_ex(m_bmp, m_currentFont, line.c_str(), m_dx+xout, m_dy+pt.y, fg_color, bg_color);

      jrectexclude(m_bmp,
                   m_dx+rc.x, m_dy+pt.y, m_dx+rc.x+rc.w-1, m_dy+pt.y+lineSize.h-1,
                   m_dx+xout, m_dy+pt.y, m_dx+xout+lineSize.w-1, m_dy+pt.y+lineSize.h-1, bg_color);
    }

    pt.y += lineSize.h;
    beg = end+1;
  }

  calculatedSize.h += pt.y;

  // Fill bottom area
  if (draw) {
    if (pt.y < rc.y+rc.h)
      fillRect(bg_color, gfx::Rect(rc.x, pt.y, rc.w, rc.y+rc.h-pt.y));
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
