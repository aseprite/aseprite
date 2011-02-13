// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <allegro.h>

#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/size.h"
#include "gui/draw.h"
#include "gui/font.h"
#include "gui/graphics.h"
#include "gui/theme.h"

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

gfx::Size Graphics::measureString(const std::string& str)
{
  return gfx::Size(ji_font_text_len(m_currentFont, str.c_str()),
		   text_height(m_currentFont));
}

//////////////////////////////////////////////////////////////////////
// ScreenGraphics

ScreenGraphics::ScreenGraphics()
  : Graphics(screen, 0, 0)
{
  setFont(font);		// Allegro default font
}

ScreenGraphics::~ScreenGraphics()
{
}
