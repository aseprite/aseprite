// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/splitter.h"

#include "ui/load_layout_event.h"
#include "ui/message.h"
#include "ui/preferred_size_event.h"
#include "ui/resize_event.h"
#include "ui/save_layout_event.h"
#include "ui/system.h"
#include "ui/theme.h"

#include <sstream>

namespace ui {

using namespace gfx;

Splitter::Splitter(Type type, int align)
  : Widget(kSplitterWidget)
  , m_type(type)
  , m_pos(50)
{
  setAlign(align);
  initTheme();
}

void Splitter::setPosition(double pos)
{
  if (m_type == ByPercentage)
    m_pos = MID(0, pos, 100);
  else
    m_pos = pos;

  invalidate();
}

bool Splitter::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kMouseDownMessage:
      if (isEnabled()) {
        Widget* c1, *c2;
        int x1, y1, x2, y2;
        int bar, click_bar;
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();

        bar = click_bar = 0;

        UI_FOREACH_WIDGET_WITH_END(getChildren(), it, end) {
          if (it+1 != end) {
            c1 = *it;
            c2 = *(it+1);

            ++bar;

            if (this->getAlign() & JI_HORIZONTAL) {
              x1 = c1->getBounds().x2();
              y1 = getBounds().y;
              x2 = c2->getBounds().x;
              y2 = getBounds().y2();
            }
            else {
              x1 = getBounds().x;
              y1 = c1->getBounds().y2();
              x2 = getBounds().x2();
              y2 = c2->getBounds().y;
            }

            if ((mousePos.x >= x1) && (mousePos.x < x2) &&
                (mousePos.y >= y1) && (mousePos.y < y2))
              click_bar = bar;
          }
        }

        if (!click_bar)
          break;

        captureMouse();

        // Continue with motion message...
      }
      else
        break;

    case kMouseMoveMessage:
      if (hasCapture()) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();

        if (getAlign() & JI_HORIZONTAL) {
          switch (m_type) {
            case ByPercentage:
              m_pos = 100.0 * (mousePos.x - getBounds().x) / getBounds().w;
              m_pos = MID(0, m_pos, 100);
              break;
            case ByPixel:
              m_pos = mousePos.x - getBounds().x;
              break;
          }
        }
        else {
          switch (m_type) {
            case ByPercentage:
              m_pos = 100.0 * (mousePos.y-getBounds().y) / getBounds().h;
              m_pos = MID(0, m_pos, 100);
              break;
            case ByPixel:
              m_pos = (mousePos.y-getBounds().y);
              break;
          }
        }

        layout();
        return true;
      }
      break;

    case kMouseUpMessage:
      if (hasCapture()) {
        releaseMouse();
        return true;
      }
      break;

    case kSetCursorMessage:
      if (isEnabled()) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        Widget* c1, *c2;
        int x1, y1, x2, y2;
        bool change_cursor = false;

        UI_FOREACH_WIDGET_WITH_END(getChildren(), it, end) {
          if (it+1 != end) {
            c1 = *it;
            c2 = *(it+1);

            if (this->getAlign() & JI_HORIZONTAL) {
              x1 = c1->getBounds().x2();
              y1 = getBounds().y;
              x2 = c2->getBounds().x;
              y2 = getBounds().y2();
            }
            else {
              x1 = getBounds().x;
              y1 = c1->getBounds().y2();
              x2 = getBounds().x2();
              y2 = c2->getBounds().y;
            }

            if ((mousePos.x >= x1) && (mousePos.x < x2) &&
                (mousePos.y >= y1) && (mousePos.y < y2)) {
              change_cursor = true;
              break;
            }
          }
        }

        if (change_cursor) {
          if (getAlign() & JI_HORIZONTAL)
            set_mouse_cursor(kSizeWECursor);
          else
            set_mouse_cursor(kSizeNSCursor);
          return true;
        }
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

void Splitter::onResize(ResizeEvent& ev)
{
#define FIXUP(x, y, w, h, l, t, r, b)                                   \
  do {                                                                  \
    avail = rect.w - this->child_spacing;                               \
                                                                        \
    pos.x = rect.x;                                                     \
    pos.y = rect.y;                                                     \
    switch (m_type) {                                                   \
      case ByPercentage:                                                \
        pos.w = avail*m_pos/100;                                        \
        break;                                                          \
      case ByPixel:                                                     \
        pos.w = m_pos;                                                  \
        break;                                                          \
    }                                                                   \
                                                                        \
    /* TODO uncomment this to make a restricted splitter */             \
    /* pos.w = MID(reqSize1.w, pos.w, avail-reqSize2.w); */             \
    pos.h = rect.h;                                                     \
                                                                        \
    child1->setBounds(pos);                                             \
    gfx::Rect child1Pos = child1->getBounds();                          \
                                                                        \
    pos.x = child1Pos.x + child1Pos.w + this->child_spacing;            \
    pos.y = rect.y;                                                     \
    pos.w = avail - child1Pos.w;                                        \
    pos.h = rect.h;                                                     \
                                                                        \
    child2->setBounds(pos);                                             \
  } while(0)

  gfx::Rect rect(ev.getBounds());
  gfx::Rect pos(0, 0, 0, 0);
  int avail;

  setBoundsQuietly(rect);

  if (getChildren().size() == 2) {
    Widget* child1 = getChildren()[0];
    Widget* child2 = getChildren()[1];

    if (this->getAlign() & JI_HORIZONTAL)
      FIXUP(x, y, w, h, l, t, r, b);
    else
      FIXUP(y, x, h, w, t, l, b, r);
  }
}

void Splitter::onPaint(PaintEvent& ev)
{
  getTheme()->paintSplitter(ev);
}

void Splitter::onPreferredSize(PreferredSizeEvent& ev)
{
#define GET_CHILD_SIZE(w, h)                    \
  do {                                          \
    w = MAX(w, reqSize.w);                      \
    h = MAX(h, reqSize.h);                      \
  } while(0)

#define FINAL_SIZE(w)                                     \
  do {                                                    \
    w *= visibleChildren;                                 \
    w += this->child_spacing * (visibleChildren-1);       \
  } while(0)

  int visibleChildren;
  Size reqSize;

  visibleChildren = 0;
  UI_FOREACH_WIDGET(getChildren(), it) {
    Widget* child = *it;
    if (!(child->flags & JI_HIDDEN))
      visibleChildren++;
  }

  int w, h;
  w = h = 0;

  UI_FOREACH_WIDGET(getChildren(), it) {
    Widget* child = *it;

    if (child->flags & JI_HIDDEN)
      continue;

    reqSize = child->getPreferredSize();

    if (this->getAlign() & JI_HORIZONTAL)
      GET_CHILD_SIZE(w, h);
    else
      GET_CHILD_SIZE(h, w);
  }

  if (visibleChildren > 0) {
    if (this->getAlign() & JI_HORIZONTAL)
      FINAL_SIZE(w);
    else
      FINAL_SIZE(h);
  }

  w += this->border_width.l + this->border_width.r;
  h += this->border_width.t + this->border_width.b;

  ev.setPreferredSize(Size(w, h));
}

void Splitter::onLoadLayout(LoadLayoutEvent& ev)
{
  ev.stream() >> m_pos;
  if (m_pos < 0) m_pos = 0;
  if (m_type == ByPixel)
    m_pos *= guiscale();

  // Do for all children
  UI_FOREACH_WIDGET(getChildren(), it)
    (*it)->loadLayout();
}

void Splitter::onSaveLayout(SaveLayoutEvent& ev)
{
  double pos = (m_type == ByPixel ? m_pos / guiscale(): m_pos);
  ev.stream() << pos;

  // Do for all children
  UI_FOREACH_WIDGET(getChildren(), it)
    (*it)->saveLayout();
}

} // namespace ui
