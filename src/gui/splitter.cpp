// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "gui/splitter.h"

#include "gui/list.h"
#include "gui/message.h"
#include "gui/preferred_size_event.h"
#include "gui/system.h"
#include "gui/theme.h"

using namespace gfx;

Splitter::Splitter(int align)
  : Widget(JI_SPLITTER)
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
  m_pos = MID(0, pos, 100);
  invalidate();
}

bool Splitter::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_SETPOS:
      layoutMembers(&msg->setpos.rect);
      return true;

    case JM_DRAW:
      getTheme()->draw_panel(this, &msg->draw.rect);
      return true;

    case JM_BUTTONPRESSED:
      if (isEnabled()) {
        JWidget c1, c2;
        int x1, y1, x2, y2;
        int bar, click_bar;
        JLink link;

        bar = click_bar = 0;

        JI_LIST_FOR_EACH(this->children, link) {
          if (link->next != this->children->end) {
            c1 = reinterpret_cast<JWidget>(link->data);
            c2 = reinterpret_cast<JWidget>(link->next->data);

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
          m_pos = 100.0 * (msg->mouse.x-this->rc->x1) / jrect_w(this->rc);
        }
        else {
          m_pos = 100.0 * (msg->mouse.y-this->rc->y1) / jrect_h(this->rc);
        }

        m_pos = MID(0, m_pos, 100);

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
        JWidget c1, c2;
        JLink link;
        int x1, y1, x2, y2;
        bool change_cursor = false;

        JI_LIST_FOR_EACH(this->children, link) {
          if (link->next != this->children->end) {
            c1 = reinterpret_cast<JWidget>(link->data);
            c2 = reinterpret_cast<JWidget>(link->next->data);

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
            jmouse_set_cursor(JI_CURSOR_SIZE_L);
          else
            jmouse_set_cursor(JI_CURSOR_SIZE_T);
          return true;
        }
        else
          return false;
      }
      break;

  }

  return Widget::onProcessMessage(msg);
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
  JWidget child;
  JLink link;

  visibleChildren = 0;
  JI_LIST_FOR_EACH(this->children, link) {
    child = (JWidget)link->data;
    if (!(child->flags & JI_HIDDEN))
      visibleChildren++;
  }

  int w, h;
  w = h = 0;

  JI_LIST_FOR_EACH(this->children, link) {
    child = (JWidget)link->data;

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

void Splitter::layoutMembers(JRect rect)
{
#define FIXUP(x, y, w, h, l, t, r, b)                                   \
  do {                                                                  \
    avail = jrect_##w(this->rc) - this->child_spacing;                  \
                                                                        \
    pos->x##1 = this->rc->x##1;                                         \
    pos->y##1 = this->rc->y##1;                                         \
    pos->x##2 = pos->x##1 + avail*m_pos/100;                            \
    /* TODO uncomment this to make a restricted panel */                \
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

  if (jlist_length(this->children) == 2) {
    JWidget child1 = reinterpret_cast<JWidget>(jlist_first(this->children)->data);
    JWidget child2 = reinterpret_cast<JWidget>(jlist_first(this->children)->next->data);
    //Size reqSize1 = child1->getPreferredSize();
    //Size reqSize2 = child2->getPreferredSize();

    if (this->getAlign() & JI_HORIZONTAL)
      FIXUP(x, y, w, h, l, t, r, b);
    else
      FIXUP(y, x, h, w, t, l, b, r);
  }

  jrect_free(pos);
}
