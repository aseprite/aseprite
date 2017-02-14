// Aseprite UI Library
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/theme.h"

#include "gfx/point.h"
#include "gfx/size.h"
#include "she/font.h"
#include "she/surface.h"
#include "she/system.h"
#include "ui/intern.h"
#include "ui/manager.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/style.h"
#include "ui/system.h"
#include "ui/view.h"
#include "ui/widget.h"

#include <algorithm>
#include <cstring>

namespace ui {

namespace {

Theme* current_theme = nullptr;

int compare_layer_flags(int a, int b)
{
  // TODO improve this
  return a - b;
}

void for_each_layer(Widget* widget,
                    const Style* style,
                    std::function<void(const Style::Layer&)> callback)
{
  int flags =
    (widget->isEnabled() ? 0: Style::Layer::kDisabled) |
    (widget->isSelected() ? Style::Layer::kSelected: 0) |
    (widget->hasMouseOver() ? Style::Layer::kMouse: 0) |
    (widget->hasFocus() ? Style::Layer::kFocus: 0);

  const Style::Layer* bestLayer = nullptr;

  for (const auto& layer : style->layers()) {
    if (bestLayer &&
        bestLayer->type() != layer.type()) {
      callback(*bestLayer);
      bestLayer = nullptr;
    }

    if ((!layer.flags()
         || (layer.flags() & flags) == layer.flags())
        && (!bestLayer
            || (bestLayer && compare_layer_flags(bestLayer->flags(), layer.flags()) <= 0))) {
      bestLayer = &layer;
    }
  }

  if (bestLayer)
    callback(*bestLayer);
}

} // anonymous namespace

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

void Theme::paintWidget(Graphics* g,
                        Widget* widget,
                        const Style* style,
                        gfx::Rect rc)
{
  ASSERT(g);
  ASSERT(widget);
  ASSERT(style);

  // External background
  g->fillRect(widget->bgColor(), rc);

  for_each_layer(
    widget, style,
    [this, g, widget, &rc](const Style::Layer& layer) {
      paintLayer(g, widget, layer, rc);
    });
}

void Theme::paintLayer(Graphics* g, Widget* widget,
                       const Style::Layer& layer,
                       gfx::Rect& rc)
{
  switch (layer.type()) {

    case Style::Layer::Type::kBackground:
      if (layer.color() != gfx::ColorNone) {
        g->fillRect(layer.color(), rc);
      }

      if (layer.spriteSheet() &&
          !layer.spriteBounds().isEmpty() &&
          !layer.slicesBounds().isEmpty()) {
        Theme::drawSlices(g, layer.spriteSheet(), rc,
                          layer.spriteBounds(),
                          layer.slicesBounds(), true);

        rc.x += layer.slicesBounds().x;
        rc.y += layer.slicesBounds().y;
        rc.w -= layer.spriteBounds().w - layer.slicesBounds().w;
        rc.h -= layer.spriteBounds().h - layer.slicesBounds().h;
      }
      break;

    case Style::Layer::Type::kBorder:
      if (layer.color() != gfx::ColorNone) {
        g->drawRect(layer.color(), rc);
      }

      if (layer.spriteSheet() &&
          !layer.spriteBounds().isEmpty() &&
          !layer.slicesBounds().isEmpty()) {
        Theme::drawSlices(g, layer.spriteSheet(), rc,
                          layer.spriteBounds(),
                          layer.slicesBounds(), false);

        rc.x += layer.slicesBounds().x;
        rc.y += layer.slicesBounds().y;
        rc.w -= layer.spriteBounds().w - layer.slicesBounds().w;
        rc.h -= layer.spriteBounds().h - layer.slicesBounds().h;
      }
      break;

    case Style::Layer::Type::kText:
      if (layer.color() != gfx::ColorNone) {
        gfx::Size textSize = g->measureUIText(widget->text());
        gfx::Point pt;

        if (layer.align() & LEFT)
          pt.x = rc.x;
        else if (layer.align() & RIGHT)
          pt.x = rc.x+rc.w-textSize.w;
        else
          pt.x = rc.x+rc.w/2-textSize.w/2;

        if (layer.align() & TOP)
          pt.y = rc.y;
        else if (layer.align() & BOTTOM)
          pt.y = rc.y+rc.h-textSize.h;
        else
          pt.y = rc.y+rc.h/2-textSize.h/2;

        g->drawUIText(widget->text(),
                      layer.color(),
                      gfx::ColorNone,
                      pt, widget->mnemonic());
      }
      break;

    case Style::Layer::Type::kIcon: {
      she::Surface* icon = layer.icon();
      if (icon) {
        gfx::Size iconSize(icon->width(), icon->height());
        gfx::Point pt;

        if (layer.align() & LEFT)
          pt.x = rc.x;
        else if (layer.align() & RIGHT)
          pt.x = rc.x+rc.w-iconSize.w;
        else
          pt.x = rc.x+rc.w/2-iconSize.w/2;

        if (layer.align() & TOP)
          pt.y = rc.y;
        else if (layer.align() & BOTTOM)
          pt.y = rc.y+rc.h-iconSize.h;
        else
          pt.y = rc.y+rc.h/2-iconSize.h/2;

        if (layer.color() != gfx::ColorNone)
          g->drawColoredRgbaSurface(icon, layer.color(), pt.x, pt.y);
        else
          g->drawRgbaSurface(icon, pt.x, pt.y);
      }
      break;
    }

  }
}

gfx::Size Theme::calcSizeHint(Widget* widget, const Style* style)
{
  gfx::Size sizeHint;
  gfx::Border borderHint;
  calcWidgetMetrics(widget, style, sizeHint, borderHint);
  return sizeHint;
}

void Theme::measureLayer(Widget* widget,
                         const Style::Layer& layer,
                         gfx::Border& borderHint,
                         gfx::Size& textHint, int& textAlign,
                         gfx::Size& iconHint, int& iconAlign)
{
  switch (layer.type()) {

    case Style::Layer::Type::kBackground:
    case Style::Layer::Type::kBorder:
      if (layer.spriteSheet() &&
          !layer.spriteBounds().isEmpty() &&
          !layer.slicesBounds().isEmpty()) {
        borderHint.left(std::max(borderHint.left(), layer.slicesBounds().x));
        borderHint.top(std::max(borderHint.top(), layer.slicesBounds().y));
        borderHint.right(std::max(borderHint.right(), layer.spriteBounds().w - layer.slicesBounds().x2()));
        borderHint.bottom(std::max(borderHint.bottom(), layer.spriteBounds().h - layer.slicesBounds().y2()));
      }
      break;

    case Style::Layer::Type::kText:
      if (layer.color() != gfx::ColorNone) {
        gfx::Size textSize(Graphics::measureUITextLength(widget->text(),
                                                         widget->font()),
                           widget->font()->height());
        textHint.w = std::max(textHint.w, textSize.w);
        textHint.h = std::max(textHint.h, textSize.h);
        textAlign = layer.align();
      }
      break;

    case Style::Layer::Type::kIcon: {
      she::Surface* icon = layer.icon();
      if (icon) {
        iconHint.w = std::max(iconHint.w, icon->width());
        iconHint.h = std::max(iconHint.h, icon->height());
        iconAlign = layer.align();
      }
      break;
    }

  }
}

gfx::Border Theme::calcBorder(Widget* widget, const Style* style)
{
  gfx::Size sizeHint;
  gfx::Border borderHint;
  calcWidgetMetrics(widget, style, sizeHint, borderHint);
  return borderHint;
}

void Theme::calcWidgetMetrics(Widget* widget,
                              const Style* style,
                              gfx::Size& sizeHint,
                              gfx::Border& borderHint)
{
  ASSERT(widget);
  ASSERT(style);

  borderHint = gfx::Border(0, 0, 0, 0);
  gfx::Border paddingHint(0, 0, 0, 0);
  gfx::Size textHint(0, 0);
  gfx::Size iconHint(0, 0);
  int textAlign = CENTER | MIDDLE;
  int iconAlign = CENTER | MIDDLE;

  for_each_layer(
    widget, style,
    [this, widget, &borderHint, &textHint, &textAlign, &iconHint, &iconAlign]
    (const Style::Layer& layer) {
      measureLayer(widget, layer,
                   borderHint,
                   textHint, textAlign,
                   iconHint, iconAlign);
    });

  if (style->border().left() >= 0) borderHint.left(style->border().left());
  if (style->border().top() >= 0) borderHint.top(style->border().top());
  if (style->border().right() >= 0) borderHint.right(style->border().right());
  if (style->border().bottom() >= 0) borderHint.bottom(style->border().bottom());

  if (style->padding().left() >= 0) paddingHint.left(style->padding().left());
  if (style->padding().top() >= 0) paddingHint.top(style->padding().top());
  if (style->padding().right() >= 0) paddingHint.right(style->padding().right());
  if (style->padding().bottom() >= 0) paddingHint.bottom(style->padding().bottom());

  sizeHint = gfx::Size(borderHint.width() + paddingHint.width(),
                       borderHint.height() + paddingHint.height());

  if ((textAlign & (LEFT | CENTER | RIGHT)) == (iconAlign & (LEFT | CENTER | RIGHT)))
    sizeHint.w += std::max(textHint.w, iconHint.w);
  else
    sizeHint.w += textHint.w + iconHint.w;

  if ((textAlign & (TOP | MIDDLE | BOTTOM)) == (iconAlign & (TOP | MIDDLE | BOTTOM)))
    sizeHint.h += std::max(textHint.h, iconHint.h);
  else
    sizeHint.h += textHint.h + iconHint.h;
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

// static
void Theme::drawSlices(Graphics* g, she::Surface* sheet,
                       const gfx::Rect& rc,
                       const gfx::Rect& sprite,
                       const gfx::Rect& slices,
                       const bool drawCenter)
{
  const int w1 = slices.x;
  const int h1 = slices.y;
  const int w2 = slices.w;
  const int h2 = slices.h;
  const int w3 = sprite.w-w1-w2;
  const int h3 = sprite.h-h1-h2;
  const int x2 = rc.x2()-w3;
  const int y2 = rc.y2()-h3;

  // Top
  int x = rc.x;
  int y = rc.y;
  g->drawRgbaSurface(sheet, sprite.x, sprite.y,
                     x, y, w1, h1);
  {
    IntersectClip clip(g, gfx::Rect(rc.x+w1, rc.y, rc.w-w1-w3, h1));
    if (clip) {
      for (x+=w1; x<x2; x+=w2) {
        g->drawRgbaSurface(sheet, sprite.x+w1, sprite.y,
                           x, y, w2, h1);
      }
    }
  }
  g->drawRgbaSurface(sheet, sprite.x+w1+w2, sprite.y,
                     x2, y, w3, h1);

  // Bottom
  x = rc.x;
  y = y2;
  g->drawRgbaSurface(sheet, sprite.x, sprite.y+h1+h2,
                     x, y, w1, h3);
  {
    IntersectClip clip(g, gfx::Rect(rc.x+w1, y2, rc.w-w1-w3, h3));
    if (clip) {
      for (x+=w1; x<x2; x+=w2) {
        g->drawRgbaSurface(sheet, sprite.x+w1, sprite.y+h1+h2,
                           x, y2, w2, h3);
      }
    }
  }
  g->drawRgbaSurface(sheet, sprite.x+w1+w2, sprite.y+h1+h2,
                     x2, y2, w3, h3);

  // Left & Right
  IntersectClip clip(g, gfx::Rect(rc.x, rc.y+h1, rc.w, rc.h-h1-h3));
  if (clip) {
    for (y=rc.y+h1; y<y2; y+=h2) {
      // Left
      g->drawRgbaSurface(sheet, sprite.x, sprite.y+h1,
                         rc.x, y, w1, h2);
      // Right
      g->drawRgbaSurface(sheet, sprite.x+w1+w2, sprite.y+h1,
                         x2, y, w3, h2);
    }
  }

  // Center
  if (drawCenter) {
    IntersectClip clip(g, gfx::Rect(rc.x+w1, rc.y+h1, rc.w-w1-w3, rc.h-h1-h3));
    if (clip) {
      for (y=rc.y+h1; y<y2; y+=h2) {
        for (x=rc.x+w1; x<x2; x+=w2)
          g->drawRgbaSurface(sheet, sprite.x+w1, sprite.y+h1, x, y, w2, h2);
      }
    }
  }
}

// static
void Theme::drawTextBox(Graphics* g, Widget* widget,
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
