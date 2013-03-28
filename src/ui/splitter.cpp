// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "ui/splitter.h"

#include "ui/load_layout_event.h"
#include "ui/message.h"
#include "ui/preferred_size_event.h"
#include "ui/save_layout_event.h"
#include "ui/system.h"
#include "ui/theme.h"

#include <sstream>

using namespace gfx;

namespace ui {

Splitter::Splitter(Type type, int align)
  : Widget(JI_SPLITTER)
  , m_type(type)
  , m_pos(50)
{
  setAlign(align);
  initTheme();
}

double Splitter::getPosition() const
{
  return m_pos;
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
  switch (msg->type) {

    case JM_SETPOS:
      layoutMembers(&msg->setpos.rect);
      return true;

    case JM_BUTTONPRESSED:
      if (isEnabled()) {
        Widget* c1, *c2;
        int x1, y1, x2, y2;
        int bar, click_bar;

        bar = click_bar = 0;

        UI_FOREACH_WIDGET_WITH_END(getChildren(), it, end) {
          if (it+1 != end) {
            c1 = *it;
            c2 = *(it+1);

            ++bar;

            if (this->getAlign() & JI_HORIZONTAL) {
              x1 = c1->rc->x2;
              y1 = this->rc->y1;
              x2 = c2->rc->x1;
              y2 = this->rc->y2;
            }
            else {
              x1 = this->rc->x1;
              y1 = c1->rc->y2;
              x2 = this->rc->x2;
              y2 = c2->rc->y1;
            }

            if ((msg->mouse.x >= x1) && (msg->mouse.x < x2) &&
                (msg->mouse.y >= y1) && (msg->mouse.y < y2))
              click_bar = bar;
          }
        }

        if (!click_bar)
          break;

        this->captureMouse();
        /* Continue with motion message...  */
      }
      else
        break;

    case JM_MOTION:
      if (this->hasCapture()) {
        if (this->getAlign() & JI_HORIZONTAL) {
          switch (m_type) {
            case ByPercentage:
              m_pos = 100.0 * (msg->mouse.x-this->rc->x1) / jrect_w(this->rc);
              m_pos = MID(0, m_pos, 100);
              break;
            case ByPixel:
              m_pos = msg->mouse.x-this->rc->x1;
              break;
          }
        }
        else {
          switch (m_type) {
            case ByPercentage:
              m_pos = 100.0 * (msg->mouse.y-this->rc->y1) / jrect_h(this->rc);
              m_pos = MID(0, m_pos, 100);
              break;
            case ByPixel:
              m_pos = (msg->mouse.y-this->rc->y1);
              break;
          }
        }

        jwidget_set_rect(this, this->rc);
        invalidate();
        return true;
      }
      break;

    case JM_BUTTONRELEASED:
      if (hasCapture()) {
        releaseMouse();
        return true;
      }
      break;

    case JM_SETCURSOR:
      if (isEnabled()) {
        Widget* c1, *c2;
        int x1, y1, x2, y2;
        bool change_cursor = false;

        UI_FOREACH_WIDGET_WITH_END(getChildren(), it, end) {
          if (it+1 != end) {
            c1 = *it;
            c2 = *(it+1);

            if (this->getAlign() & JI_HORIZONTAL) {
              x1 = c1->rc->x2;
              y1 = this->rc->y1;
              x2 = c2->rc->x1;
              y2 = this->rc->y2;
            }
            else {
              x1 = this->rc->x1;
              y1 = c1->rc->y2;
              x2 = this->rc->x2;
              y2 = c2->rc->y1;
            }

            if ((msg->mouse.x >= x1) && (msg->mouse.x < x2) &&
                (msg->mouse.y >= y1) && (msg->mouse.y < y2)) {
              change_cursor = true;
              break;
            }
          }
        }

        if (change_cursor) {
          if (this->getAlign() & JI_HORIZONTAL)
            jmouse_set_cursor(kSizeLCursor);
          else
            jmouse_set_cursor(kSizeTCursor);
          return true;
        }
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

void Splitter::onPaint(PaintEvent& ev)
{
  getTheme()->drawSplitter(ev);
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

  // Do for all children
  UI_FOREACH_WIDGET(getChildren(), it)
    (*it)->loadLayout();
}

void Splitter::onSaveLayout(SaveLayoutEvent& ev)
{
  ev.stream() << m_pos;

  // Do for all children
  UI_FOREACH_WIDGET(getChildren(), it)
    (*it)->saveLayout();
}

void Splitter::layoutMembers(JRect rect)
{
#define FIXUP(x, y, w, h, l, t, r, b)                                   \
  do {                                                                  \
    avail = jrect_##w(this->rc) - this->child_spacing;                  \
                                                                        \
    pos->x##1 = this->rc->x##1;                                         \
    pos->y##1 = this->rc->y##1;                                         \
    switch (m_type) {                                                   \
      case ByPercentage:                                                \
      pos->x##2 = pos->x##1 + avail*m_pos/100;                          \
      break;                                                            \
    case ByPixel:                                                       \
      pos->x##2 = pos->x##1 + m_pos;                                    \
      break;                                                            \
    }                                                                   \
                                                                        \
    /* TODO uncomment this to make a restricted splitter */             \
    /* pos->w = MID(reqSize1.w, pos->w, avail-reqSize2.w); */           \
    pos->y##2 = pos->y##1 + jrect_##h(this->rc);                        \
                                                                        \
    jwidget_set_rect(child1, pos);                                      \
                                                                        \
    pos->x##1 = child1->rc->x##1 + jrect_##w(child1->rc)                \
      + this->child_spacing;                                            \
    pos->y##1 = this->rc->y##1;                                         \
    pos->x##2 = pos->x##1 + avail - jrect_##w(child1->rc);              \
    pos->y##2 = pos->y##1 + jrect_##h(this->rc);                        \
                                                                        \
    jwidget_set_rect(child2, pos);                                      \
  } while(0)

  JRect pos = jrect_new(0, 0, 0, 0);
  int avail;

  jrect_copy(this->rc, rect);

  if (getChildren().size() == 2) {
    Widget* child1 = getChildren()[0];
    Widget* child2 = getChildren()[1];

    if (this->getAlign() & JI_HORIZONTAL)
      FIXUP(x, y, w, h, l, t, r, b);
    else
      FIXUP(y, x, h, w, t, l, b, r);
  }

  jrect_free(pos);
}

} // namespace ui
