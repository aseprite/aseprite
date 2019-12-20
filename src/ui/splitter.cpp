// Aseprite UI Library
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/splitter.h"

#include "base/clamp.h"
#include "ui/load_layout_event.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/resize_event.h"
#include "ui/save_layout_event.h"
#include "ui/scale.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"

#include <algorithm>
#include <sstream>

namespace ui {

using namespace gfx;

Splitter::Splitter(Type type, int align)
  : Widget(kSplitterWidget)
  , m_type(type)
  , m_pos(50)
  , m_guiscale(guiscale())
{
  setAlign(align);
  initTheme();
}

void Splitter::setPosition(double pos)
{
  m_pos = pos;
  limitPos();
  onPositionChange();

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

        for (auto it=children().begin(),
               end=children().end();
               it != end; ) {
          auto next = it;
          ++next;

          if (next != end) {
            c1 = *it;
            c2 = *next;

            ++bar;

            if (this->align() & HORIZONTAL) {
              x1 = c1->bounds().x2();
              y1 = bounds().y;
              x2 = c2->bounds().x;
              y2 = bounds().y2();
            }
            else {
              x1 = bounds().x;
              y1 = c1->bounds().y2();
              x2 = bounds().x2();
              y2 = c2->bounds().y;
            }

            if ((mousePos.x >= x1) && (mousePos.x < x2) &&
                (mousePos.y >= y1) && (mousePos.y < y2))
              click_bar = bar;
          }

          it = next;
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

        if (align() & HORIZONTAL) {
          switch (m_type) {
            case ByPercentage:
              m_pos = 100.0 * (mousePos.x - bounds().x) / bounds().w;
              break;
            case ByPixel:
              m_pos = mousePos.x - bounds().x;
              break;
          }
        }
        else {
          switch (m_type) {
            case ByPercentage:
              m_pos = 100.0 * (mousePos.y - bounds().y) / bounds().h;
              break;
            case ByPixel:
              m_pos = mousePos.y - bounds().y;
              break;
          }
        }

        limitPos();
        onPositionChange();
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
      if (isEnabled() && (!manager()->getCapture() || hasCapture())) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        Widget* c1, *c2;
        int x1, y1, x2, y2;
        bool change_cursor = false;

        for (auto it=children().begin(),
               end=children().end();
               it != end; ) {
          auto next = it;
          ++next;

          if (next != end) {
            c1 = *it;
            c2 = *(it+1);

            if (this->align() & HORIZONTAL) {
              x1 = c1->bounds().x2();
              y1 = bounds().y;
              x2 = c2->bounds().x;
              y2 = bounds().y2();
            }
            else {
              x1 = bounds().x;
              y1 = c1->bounds().y2();
              x2 = bounds().x2();
              y2 = c2->bounds().y;
            }

            if ((mousePos.x >= x1) && (mousePos.x < x2) &&
                (mousePos.y >= y1) && (mousePos.y < y2)) {
              change_cursor = true;
              break;
            }
          }

          it = next;
        }

        if (change_cursor) {
          if (align() & HORIZONTAL)
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

void Splitter::onInitTheme(InitThemeEvent& ev)
{
  if (m_type == ByPixel) m_pos /= m_guiscale;
  m_guiscale = ui::guiscale();
  if (m_type == ByPixel) m_pos *= m_guiscale;

  Widget::onInitTheme(ev);
}

void Splitter::onResize(ResizeEvent& ev)
{
#define LAYOUT_TWO_CHILDREN(x, y, w, h, l, t, r, b)                     \
  {                                                                     \
    avail = rc.w - childSpacing();                                      \
                                                                        \
    pos.x = rc.x;                                                       \
    pos.y = rc.y;                                                       \
    switch (m_type) {                                                   \
      case ByPercentage:                                                \
        pos.w = int(avail*m_pos/100);                                   \
        break;                                                          \
      case ByPixel:                                                     \
        pos.w = int(m_pos);                                             \
        break;                                                          \
    }                                                                   \
                                                                        \
    /* TODO uncomment this to make a restricted splitter */             \
    /* pos.w = base::clamp(pos.w, reqSize1.w, avail-reqSize2.w); */     \
    pos.h = rc.h;                                                       \
                                                                        \
    child1->setBounds(pos);                                             \
    gfx::Rect child1Pos = child1->bounds();                             \
                                                                        \
    pos.x = child1Pos.x + child1Pos.w + childSpacing();                 \
    pos.y = rc.y;                                                       \
    pos.w = avail - child1Pos.w;                                        \
    pos.h = rc.h;                                                       \
                                                                        \
    child2->setBounds(pos);                                             \
  }

  gfx::Rect rc(ev.bounds());
  gfx::Rect pos(0, 0, 0, 0);
  int avail;

  setBoundsQuietly(rc);
  limitPos();

  Widget* child1 = panel1();
  Widget* child2 = panel2();

  if (child1 && child2) {
    if (align() & HORIZONTAL) {
      LAYOUT_TWO_CHILDREN(x, y, w, h, l, t, r, b);
    }
    else {
      LAYOUT_TWO_CHILDREN(y, x, h, w, t, l, b, r);
    }
  }
  else if (child1)
    child1->setBounds(rc);
  else if (child2)
    child2->setBounds(rc);
}

void Splitter::onSizeHint(SizeHintEvent& ev)
{
#define GET_CHILD_SIZE(w, h)                         \
  do {                                               \
    w = std::max(w, reqSize.w);                      \
    h = std::max(h, reqSize.h);                      \
  } while(0)

#define FINAL_SIZE(w)                                     \
  do {                                                    \
    w *= visibleChildren;                                 \
    w += childSpacing() * (visibleChildren-1);            \
  } while(0)

  int visibleChildren;
  Size reqSize;

  visibleChildren = 0;
  for (auto child : children()) {
    if (child->isVisible())
      visibleChildren++;
  }

  int w, h;
  w = h = 0;

  for (auto child : children()) {
    if (!child->isVisible())
      continue;

    reqSize = child->sizeHint();

    if (this->align() & HORIZONTAL)
      GET_CHILD_SIZE(w, h);
    else
      GET_CHILD_SIZE(h, w);
  }

  if (visibleChildren > 0) {
    if (this->align() & HORIZONTAL)
      FINAL_SIZE(w);
    else
      FINAL_SIZE(h);
  }

  w += border().width();
  h += border().height();

  ev.setSizeHint(Size(w, h));
}

void Splitter::onLoadLayout(LoadLayoutEvent& ev)
{
  ev.stream() >> m_pos;
  if (m_pos < 0) m_pos = 0;
  if (m_type == ByPixel)
    m_pos *= m_guiscale;
}

void Splitter::onSaveLayout(SaveLayoutEvent& ev)
{
  double pos = (m_type == ByPixel ? m_pos / m_guiscale: m_pos);
  ev.stream() << pos;
}

void Splitter::onPositionChange()
{
  layout();
}

Widget* Splitter::panel1() const
{
  const WidgetsList& list = children();
  if (list.size() >= 1 && list[0]->isVisible())
    return list[0];
  else
    return nullptr;
}

Widget* Splitter::panel2() const
{
  const WidgetsList& list = children();
  if (list.size() >= 2 && list[1]->isVisible())
    return list[1];
  else
    return nullptr;
}

void Splitter::limitPos()
{
  if (align() & HORIZONTAL) {
    switch (m_type) {
      case ByPercentage:
        m_pos = base::clamp<double>(m_pos, 0, 100);
        break;
      case ByPixel:
        if (isVisible())
          m_pos = base::clamp<double>(m_pos, 0, bounds().w);
        break;
    }
  }
  else {
    switch (m_type) {
      case ByPercentage:
        m_pos = base::clamp<double>(m_pos, 0, 100);
        break;
      case ByPixel:
        if (isVisible())
          m_pos = base::clamp<double>(m_pos, 0, bounds().h);
        break;
    }
  }
}

} // namespace ui
