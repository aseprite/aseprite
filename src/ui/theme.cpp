// Aseprite UI Library
// Copyright (C) 2001-2017  David Capello
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
    set_theme(nullptr);
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

void set_theme(Theme* theme)
{
  if (theme) {
    // As the regeneration may fail, first we regenerate the theme and
    // then we set is as "the current theme." E.g. In case that we'd
    // like to show some kind of error message with the UI controls,
    // we should be able to use the previous theme to do so (instead
    // of this new unsuccessfully regenerated theme).
    theme->regenerate();

    current_theme = theme;

    Manager* manager = Manager::getDefault();
    if (manager && !manager->theme())
      manager->setTheme(theme);
  }
}

Theme* get_theme()
{
  return current_theme;
}

void drawTextBox(Graphics* g, Widget* widget,
                 int* w, int* h, gfx::Color bg, gfx::Color fg)
{
  View* view = View::getView(widget);
  char* text = const_cast<char*>(widget->text().c_str());
  char* beg, *end;
  int x1, y1;
  int x, y, chr, len;
  gfx::Point scroll;
  int textheight = widget->textHeight();
  she::Font* font = widget->font();
  char *beg_end, *old_end;
  int width;
  gfx::Rect vp;

  if (view) {
    vp = view->viewportBounds().offset(-widget->bounds().origin());
    scroll = view->viewScroll();
  }
  else {
    vp = widget->clientBounds();
    scroll.x = scroll.y = 0;
  }
  x1 = widget->clientBounds().x + widget->border().left();
  y1 = widget->clientBounds().y + widget->border().top();

  // Fill background
  if (g)
    g->fillRect(bg, vp);

  chr = 0;

  // Without word-wrap
  if (!(widget->align() & WORDWRAP)) {
    width = widget->clientChildrenBounds().w;
  }
  // With word-wrap
  else {
    if (w) {
      width = *w;
      *w = 0;
    }
    else {
      // TODO modificable option? I don't think so, this is very internal stuff
#if 0
      // Shows more information in x-scroll 0
      width = vp.w;
#else
      // Make good use of the complete text-box
      if (view) {
        gfx::Size maxSize = view->getScrollableSize();
        width = MAX(vp.w, maxSize.w);
      }
      else {
        width = vp.w;
      }
      width -= widget->border().width();
#endif
    }
  }

  // Draw line-by-line
  y = y1;
  for (beg=end=text; end; ) {
    x = x1;

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
        if ((old_end) && (x+font->textLength(beg) > x1+width-scroll.x)) {
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

      g->drawText(beg, fg, gfx::ColorNone, gfx::Point(xout, y));
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
}

} // namespace ui
