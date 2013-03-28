// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "gfx/point.h"
#include "gfx/size.h"
#include "ui/draw.h"
#include "ui/font.h"
#include "ui/intern.h"
#include "ui/manager.h"
#include "ui/rect.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/view.h"
#include "ui/widget.h"

namespace ui {

static Theme* current_theme = NULL;

static void draw_text(BITMAP *bmp, FONT *f, const char* text, int x, int y,
                      ui::Color fg_color, ui::Color bg_color, bool fill_bg);

Theme::Theme()
{
  this->name = "Theme";
  this->default_font = font;    // Default Allegro font
  this->scrollbar_size = 0;
  this->guiscale = 1;
}

Theme::~Theme()
{
  if (default_font && default_font != font)
    destroy_font(default_font);

  if (current_theme == this)
    CurrentTheme::set(NULL);
}

void Theme::regenerate()
{
  CursorType type = jmouse_get_cursor();
  jmouse_set_cursor(kNoCursor);

  onRegenerate();

  jmouse_set_cursor(type);
}

//////////////////////////////////////////////////////////////////////

void CurrentTheme::set(Theme* theme)
{
  current_theme = theme;

  if (current_theme) {
    current_theme->regenerate();

    Manager* manager = Manager::getDefault();
    if (manager && !manager->getTheme())
      manager->setTheme(theme);
  }
}

Theme* CurrentTheme::get()
{
  return current_theme;
}

BITMAP* ji_apply_guiscale(BITMAP* original)
{
  int scale = jguiscale();
  if (scale > 1) {
    BITMAP* scaled = create_bitmap_ex(bitmap_color_depth(original),
                                      original->w*scale,
                                      original->h*scale);

    for (int y=0; y<scaled->h; ++y)
      for (int x=0; x<scaled->w; ++x)
        putpixel(scaled, x, y, getpixel(original, x/scale, y/scale));

    destroy_bitmap(original);
    return scaled;
  }
  else
    return original;
}

void _ji_theme_draw_sprite_color(BITMAP *bmp, BITMAP *sprite,
                                 int x, int y, int color)
{
  int u, v, mask = bitmap_mask_color(sprite);

  for (v=0; v<sprite->h; v++)
    for (u=0; u<sprite->w; u++)
      if (getpixel(sprite, u, v) != mask)
        putpixel(bmp, x+u, y+v, color);
}

void _ji_theme_textbox_draw(BITMAP *bmp, Widget* widget,
                            int *w, int *h, ui::Color bg, ui::Color fg)
{
  View* view = View::getView(widget);
  char *text = (char*)widget->getText(); // TODO warning: removing const modifier
  char *beg, *end;
  int x1, y1, x2, y2;
  int x, y, chr, len;
  gfx::Point scroll;
  int viewport_w, viewport_h;
  int textheight = jwidget_get_text_height(widget);
  FONT *font = widget->getFont();
  char *beg_end, *old_end;
  int width;

  if (view) {
    gfx::Rect vp = view->getViewportBounds();
    x1 = vp.x;
    y1 = vp.y;
    viewport_w = vp.w;
    viewport_h = vp.h;
    scroll = view->getViewScroll();
  }
  else {
    x1 = widget->rc->x1 + widget->border_width.l;
    y1 = widget->rc->y1 + widget->border_width.t;
    viewport_w = jrect_w(widget->rc) - widget->border_width.l - widget->border_width.r;
    viewport_h = jrect_h(widget->rc) - widget->border_width.t - widget->border_width.b;
    scroll.x = scroll.y = 0;
  }
  x2 = x1+viewport_w-1;
  y2 = y1+viewport_h-1;

  chr = 0;

  /* without word-wrap */
  if (!(widget->getAlign() & JI_WORDWRAP)) {
    width = jrect_w(widget->rc);
  }
  /* with word-wrap */
  else {
    if (w) {
      width = *w;
      *w = 0;
    }
    else {
      /* TODO modificable option? I don't think so, this is very internal stuff */
#if 0
      /* shows more information in x-scroll 0 */
      width = viewport_w;
#else
      /* make good use of the complete text-box */
      if (view) {
        gfx::Size maxSize = view->getScrollableSize();
        width = MAX(viewport_w, maxSize.w);
      }
      else {
        width = viewport_w;
      }
#endif
    }
  }

  /* draw line-by-line */
  y = y1 - scroll.y;
  for (beg=end=text; end; ) {
    x = x1 - scroll.x;

    /* without word-wrap */
    if (!(widget->getAlign() & JI_WORDWRAP)) {
      end = ustrchr(beg, '\n');
      if (end) {
        chr = *end;
        *end = 0;
      }
    }
    /* with word-wrap */
    else {
      old_end = NULL;
      for (beg_end=beg;;) {
        end = ustrpbrk(beg_end, " \n");
        if (end) {
          chr = *end;
          *end = 0;
        }

        /* to here we can print */
        if ((old_end) && (x+text_length(font, beg) > x1-scroll.x+width)) {
          if (end)
            *end = chr;

          end = old_end;
          chr = *end;
          *end = 0;
          break;
        }
        /* we can print one word more */
        else if (end) {
          /* force break */
          if (chr == '\n')
            break;

          *end = chr;
          beg_end = end+1;
        }
        /* we are in the end of text */
        else
          break;

        old_end = end;
      }
    }

    len = text_length(font, beg);

    /* render the text in the "bmp" */
    if (bmp) {
      int xout;

      if (widget->getAlign() & JI_CENTER)
        xout = x + width/2 - len/2;
      else if (widget->getAlign() & JI_RIGHT)
        xout = x + width - len;
      else                      /* left */
        xout = x;

      draw_text(bmp, font, beg, xout, y, fg, bg, true);

      jrectexclude(bmp,
                   x1, y, x2, y+textheight-1,
                   xout, y, xout+len-1, y+textheight-1, bg);
    }

    /* width */
    if (w)
      *w = MAX(*w, len);

    y += textheight;

    if (end) {
      *end = chr;
      beg = end+1;
    }
  }

  /* height */
  if (h)
    *h = (y-y1+scroll.y);

  if (w) *w += widget->border_width.l + widget->border_width.r;
  if (h) *h += widget->border_width.t + widget->border_width.b;

  // Fill bottom area
  if (bmp) {
    if (y <= y2)
      rectfill(bmp, x1, y, x2, y2, to_system(bg));
  }
}

static void draw_text(BITMAP *bmp, FONT *f, const char *text, int x, int y,
                      ui::Color fg_color, ui::Color bg_color, bool fill_bg)
{
  // TODO Optional anti-aliased textout
  ji_font_set_aa_mode(f, to_system(bg_color));
  textout_ex(bmp, f, text, x, y, to_system(fg_color), (fill_bg ? to_system(bg_color): -1));
}

} // namespace ui
