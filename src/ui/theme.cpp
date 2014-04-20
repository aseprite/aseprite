// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "gfx/point.h"
#include "gfx/size.h"
#include "ui/draw.h"
#include "ui/font.h"
#include "ui/intern.h"
#include "ui/manager.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/view.h"
#include "ui/widget.h"

namespace ui {

static Theme* current_theme = NULL;

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

void drawTextBox(Graphics* g, Widget* widget,
  int* w, int* h, Color bg, Color fg)
{
  View* view = View::getView(widget);
  char* text = const_cast<char*>(widget->getText().c_str());
  char* beg, *end;
  int x1, y1, x2, y2;
  int x, y, chr, len;
  gfx::Point scroll;
  int viewport_w, viewport_h;
  int textheight = widget->getTextHeight();
  FONT *font = widget->getFont();
  char *beg_end, *old_end;
  int width;

  if (view) {
    gfx::Rect vp = view->getViewportBounds()
      .offset(-view->getViewport()->getBounds().getOrigin());

    x1 = vp.x;
    y1 = vp.y;
    viewport_w = vp.w;
    viewport_h = vp.h;
    scroll = view->getViewScroll();
  }
  else {
    x1 = widget->getClientBounds().x + widget->border_width.l;
    y1 = widget->getClientBounds().y + widget->border_width.t;
    viewport_w = widget->getClientBounds().w - widget->border_width.l - widget->border_width.r;
    viewport_h = widget->getClientBounds().h - widget->border_width.t - widget->border_width.b;
    scroll.x = scroll.y = 0;
  }
  x2 = x1 + viewport_w;
  y2 = y1 + viewport_h;

  chr = 0;

  // Without word-wrap
  if (!(widget->getAlign() & JI_WORDWRAP)) {
    width = widget->getClientBounds().w;
  }
  // With word-wrap
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

  // Draw line-by-line
  y = y1 - scroll.y;
  for (beg=end=text; end; ) {
    x = x1 - scroll.x;

    // Without word-wrap
    if (!(widget->getAlign() & JI_WORDWRAP)) {
      end = ustrchr(beg, '\n');
      if (end) {
        chr = *end;
        *end = 0;
      }
    }
    // With word-wrap
    else {
      old_end = NULL;
      for (beg_end=beg;;) {
        end = ustrpbrk(beg_end, " \n");
        if (end) {
          chr = *end;
          *end = 0;
        }

        // To here we can print
        if ((old_end) && (x+text_length(font, beg) > x1-scroll.x+width)) {
          if (end)
            *end = chr;

          end = old_end;
          chr = *end;
          *end = 0;
          break;
        }
        // We can print one word more
        else if (end) {
          // Force break
          if (chr == '\n')
            break;

          *end = chr;
          beg_end = end+1;
        }
        // We are in the end of text
        else
          break;

        old_end = end;
      }
    }

    len = text_length(font, beg);

    // Render the text
    if (g) {
      int xout;

      if (widget->getAlign() & JI_CENTER)
        xout = x + width/2 - len/2;
      else if (widget->getAlign() & JI_RIGHT)
        xout = x + width - len;
      else                      // Left align
        xout = x;

      g->drawString(beg, fg, bg, true, gfx::Point(xout, y));
      g->fillAreaBetweenRects(bg,
        gfx::Rect(x1, y, x2 - x1, textheight),
        gfx::Rect(xout, y, len, textheight));
    }

    if (w)
      *w = MAX(*w, len);

    y += textheight;

    if (end) {
      *end = chr;
      beg = end+1;
    }
  }

  if (h)
    *h = (y - y1 + scroll.y);

  if (w) *w += widget->border_width.l + widget->border_width.r;
  if (h) *h += widget->border_width.t + widget->border_width.b;

  // Fill bottom area
  if (g && y < y2)
    g->fillRect(bg, gfx::Rect(x1, y, x2 - x1, y2 - y));
}

} // namespace ui
