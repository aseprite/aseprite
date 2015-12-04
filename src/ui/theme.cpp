// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/point.h"
#include "gfx/size.h"
#include "she/font.h"
#include "she/system.h"
#include "ui/intern.h"
#include "ui/manager.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/view.h"
#include "ui/widget.h"

#include <cstring>

namespace ui {

static Theme* current_theme = NULL;

Theme::Theme()
  : m_guiscale(1)
{
}

Theme::~Theme()
{
  if (current_theme == this)
    CurrentTheme::set(NULL);
}

void Theme::regenerate()
{
  CursorType type = get_mouse_cursor();
  set_mouse_cursor(kNoCursor);

  onRegenerate();

  details::resetFontAllWidgets();

  // TODO We cannot reinitialize all widgets because this mess all
  // child spacing, border, etc. But it could be good to change the
  // uiscale() and get the new look without the need to restart the
  // whole app.
  //details::reinitThemeForAllWidgets();

  set_mouse_cursor(type);
}

//////////////////////////////////////////////////////////////////////

void CurrentTheme::set(Theme* theme)
{
  current_theme = theme;

  if (current_theme) {
    current_theme->regenerate();

    Manager* manager = Manager::getDefault();
    if (manager && !manager->theme())
      manager->setTheme(theme);
  }
}

Theme* CurrentTheme::get()
{
  return current_theme;
}

void drawTextBox(Graphics* g, Widget* widget,
  int* w, int* h, gfx::Color bg, gfx::Color fg)
{
  View* view = View::getView(widget);
  char* text = const_cast<char*>(widget->text().c_str());
  char* beg, *end;
  int x1, y1, x2, y2;
  int x, y, chr, len;
  gfx::Point scroll;
  int viewport_w, viewport_h;
  int textheight = widget->textHeight();
  she::Font* font = widget->font();
  char *beg_end, *old_end;
  int width;

  if (view) {
    gfx::Rect vp = view->viewportBounds()
      .offset(-view->viewport()->bounds().origin());

    x1 = vp.x;
    y1 = vp.y;
    viewport_w = vp.w;
    viewport_h = vp.h;
    scroll = view->viewScroll();
  }
  else {
    x1 = widget->clientBounds().x + widget->border().left();
    y1 = widget->clientBounds().y + widget->border().top();
    viewport_w = widget->clientBounds().w - widget->border().width();
    viewport_h = widget->clientBounds().h - widget->border().height();
    scroll.x = scroll.y = 0;
  }
  x2 = x1 + viewport_w;
  y2 = y1 + viewport_h;

  chr = 0;

  // Without word-wrap
  if (!(widget->align() & WORDWRAP)) {
    width = widget->clientBounds().w;
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
    if (!(widget->align() & WORDWRAP)) {
      end = std::strchr(beg, '\n');
      if (end) {
        chr = *end;
        *end = 0;
      }
    }
    // With word-wrap
    else {
      old_end = NULL;
      for (beg_end=beg;;) {
        end = std::strpbrk(beg_end, " \n");
        if (end) {
          chr = *end;
          *end = 0;
        }

        // To here we can print
        if ((old_end) && (x+font->textLength(beg) > x1-scroll.x+width)) {
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

    len = font->textLength(beg);

    // Render the text
    if (g) {
      int xout;

      if (widget->align() & CENTER)
        xout = x + width/2 - len/2;
      else if (widget->align() & RIGHT)
        xout = x + width - len;
      else                      // Left align
        xout = x;

      g->drawUIString(beg, fg, bg, gfx::Point(xout, y));
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

  if (w) *w += widget->border().width();
  if (h) *h += widget->border().height();

  // Fill bottom area
  if (g && y < y2)
    g->fillRect(bg, gfx::Rect(x1, y, x2 - x1, y2 - y));
}

} // namespace ui
